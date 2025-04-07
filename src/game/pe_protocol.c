#include "pe_protocol.h"

#include "core/p_assert.h"
#include "core/p_arena.h"
#include "pe_bit_stream.h"
#include "platform/pe_net.h"
#include "game/pe_entity.h"
#include "pe_config.h"

#include <stdio.h>
#include <string.h>

static peSerializationError pe_serialize_connection_request_message(peBitStream *bs, peConnectionRequestMessage *msg) {
    return peSerializationError_None;
}

static peSerializationError pe_serialize_connection_denied_message(peBitStream *bs, peConnectionDeniedMessage *msg) {
    return pe_serialize_enum(bs, &msg->reason, peConnectionDeniedReason_Count);
}

static peSerializationError pe_serialize_connection_accepted_message(peBitStream *bs, peConnectionAcceptedMessage *msg) {
    peSerializationError err = peSerializationError_None;
    err = pe_serialize_range_int(bs, &msg->client_index, 0, MAX_CLIENT_COUNT); if (err) return err;
    err = pe_serialize_u32(bs, &msg->entity_index); if (err) return err;
    return err;
}

static peSerializationError pe_serialize_connection_closed_message(peBitStream *bs, peConnectionClosedMessage *msg) {
    return pe_serialize_enum(bs, &msg->reason, peConnectionClosedReason_Count);
}

static peSerializationError pe_serialize_input_state_message(peBitStream *bs, peInputStateMessage *msg) {
    return pe_serialize_input(bs, &msg->input);
}

static peSerializationError pe_serialize_world_state_message(peBitStream *bs, peWorldStateMessage *msg) {
    peSerializationError err = peSerializationError_None;
    for (int i = 0; i < P_COUNT_OF(msg->entities); i += 1) {
        err = pe_serialize_entity(bs, &msg->entities[i]);
        if (err) return err;
    }
    return err;
}

peMessage pe_message_create(pArena *arena, peMessageType type) {
    P_ASSERT(type >= 0);
    P_ASSERT(type < peMessageType_Count);
    peMessage message = {0};
    size_t message_size;
    switch (type) {
        case peMessageType_ConnectionRequest: message_size = sizeof(peConnectionRequestMessage); break;
        case peMessageType_ConnectionDenied: message_size = sizeof(peConnectionDeniedMessage); break;
        case peMessageType_ConnectionAccepted: message_size = sizeof(peConnectionAcceptedMessage); break;
        case peMessageType_ConnectionClosed: message_size = sizeof(peConnectionClosedMessage); break;
        case peMessageType_InputState: message_size = sizeof(peInputStateMessage); break;
        case peMessageType_WorldState: message_size = sizeof(peWorldStateMessage); break;
        default: P_PANIC(); return message;
    }
    message.type = type;
    message.any = p_arena_alloc(arena, message_size);
    memset(message.any, 0, message_size);
    return message;
}

peSerializationError pe_serialize_message(peBitStream *bs, peMessage *msg) {
    switch (msg->type) {
        case peMessageType_ConnectionRequest:
            return pe_serialize_connection_request_message(bs, msg->connection_request);
        case peMessageType_ConnectionDenied:
            return pe_serialize_connection_denied_message(bs, msg->connection_denied);
        case peMessageType_ConnectionAccepted:
            return pe_serialize_connection_accepted_message(bs, msg->connection_accepted);
        case peMessageType_ConnectionClosed:
            return pe_serialize_connection_closed_message(bs, msg->connection_closed);
        case peMessageType_InputState:
            return pe_serialize_input_state_message(bs, msg->input_state);
        case peMessageType_WorldState:
            return pe_serialize_world_state_message(bs, msg->world_state);
        default: P_PANIC(); break;
    }

    return peSerializationError_ValueOutOfRange;
}

void pe_append_message(pePacket *packet, peMessage msg) {
    P_ASSERT(packet->message_count < MAX_MESSAGES_PER_PACKET);
    packet->messages[packet->message_count].type = msg.type;
    packet->messages[packet->message_count].any = msg.any;
    packet->message_count += 1;
}

bool pe_send_packet(peSocket socket, peAddress address, pePacket *packet) {
    uint8_t buffer[1400];

    peBitStream write_stream = pe_create_write_stream(buffer, sizeof(buffer));
    peSerializationError err = peSerializationError_None;
    for (int i = 0; i < packet->message_count; i += 1) {
        err = pe_serialize_enum(&write_stream, &packet->messages[i].type, peMessageType_Count);
        if (err) return false;
        err = pe_serialize_message(&write_stream, &packet->messages[i]);
        if (err) return false;
    }
    pe_bit_stream_flush_bits(&write_stream);
    int bytes_written = pe_bit_stream_bytes_processed(&write_stream);

    peSocketSendError send_error = pe_socket_send(socket, address, buffer, bytes_written);
    if (send_error != peSocketSendError_None) {
        fprintf(stdout, "peSocketSendError: %u\n", send_error);
        return false;
    }

    return true;
}

bool pe_receive_packet(peSocket socket, pArena *arena, peAddress *address, pePacket *packet) {
    P_ASSERT(packet->message_count == 0);

    uint8_t buffer[1400];
    int bytes_received;
    peSocketReceiveError srcv_error = pe_socket_receive(socket, buffer, sizeof(buffer), &bytes_received, address);
    if (srcv_error != peSocketReceiveError_None) {
        switch (srcv_error) {
        #if defined(_WIN32)
            case peSocketReceiveError_WouldBlock:
            case peSocketReceiveError_RemoteNotListening:
                return false;
        #else
            case peSocketReceiveError_Timeout:
                return false;
        #endif
            default: {
                fprintf(stdout, "peSocketReceiveError: %u\n", srcv_error);
                return false;
            } break;
        }
    }

    packet->message_count = 0;
    peBitStream read_stream = pe_create_read_stream(buffer, sizeof(buffer), bytes_received);
    while (pe_bit_stream_bytes_processed(&read_stream) < bytes_received) {
        peMessageType message_type;
        peSerializationError s_error;
        s_error = pe_serialize_enum(&read_stream, &message_type, peMessageType_Count);
        if (s_error) return false;
        peMessage msg = pe_message_create(arena, message_type);
        packet->messages[packet->message_count] = msg;
        packet->message_count += 1;
        s_error = pe_serialize_message(&read_stream, &msg);
        if (s_error) return false;
    }
    return true;
}
