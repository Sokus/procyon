#include "p_protocol.h"

#include "core/p_assert.h"
#include "core/p_arena.h"
#include "p_bit_stream.h"
#include "platform/p_net.h"
#include "game/p_entity.h"
#include "pe_config.h"

#include <stdio.h>
#include <string.h>

static pSerializationError p_serialize_connection_request_message(pBitStream *bs, pConnectionRequestMessage *msg) {
    return pSerializationError_None;
}

static pSerializationError p_serialize_connection_denied_message(pBitStream *bs, pConnectionDeniedMessage *msg) {
    return p_serialize_enum(bs, &msg->reason, pConnectionDeniedReason_Count);
}

static pSerializationError p_serialize_connection_accepted_message(pBitStream *bs, pConnectionAcceptedMessage *msg) {
    pSerializationError err = pSerializationError_None;
    err = p_serialize_range_int(bs, &msg->client_index, 0, MAX_CLIENT_COUNT); if (err) return err;
    err = p_serialize_u32(bs, &msg->entity_index); if (err) return err;
    return err;
}

static pSerializationError p_serialize_connection_closed_message(pBitStream *bs, pConnectionClosedMessage *msg) {
    return p_serialize_enum(bs, &msg->reason, pConnectionClosedReason_Count);
}

static pSerializationError p_serialize_input_state_message(pBitStream *bs, pInputStateMessage *msg) {
    return p_serialize_input(bs, &msg->input);
}

static pSerializationError p_serialize_world_state_message(pBitStream *bs, pWorldStateMessage *msg) {
    pSerializationError err = pSerializationError_None;
    for (int i = 0; i < P_COUNT_OF(msg->entities); i += 1) {
        err = p_serialize_entity(bs, &msg->entities[i]);
        if (err) return err;
    }
    return err;
}

pMessage p_message_create(pArena *arena, pMessageType type) {
    P_ASSERT(type >= 0);
    P_ASSERT(type < pMessageType_Count);
    pMessage message = {0};
    size_t message_size;
    switch (type) {
        case pMessageType_ConnectionRequest: message_size = sizeof(pConnectionRequestMessage); break;
        case pMessageType_ConnectionDenied: message_size = sizeof(pConnectionDeniedMessage); break;
        case pMessageType_ConnectionAccepted: message_size = sizeof(pConnectionAcceptedMessage); break;
        case pMessageType_ConnectionClosed: message_size = sizeof(pConnectionClosedMessage); break;
        case pMessageType_InputState: message_size = sizeof(pInputStateMessage); break;
        case pMessageType_WorldState: message_size = sizeof(pWorldStateMessage); break;
        default: P_PANIC(); return message;
    }
    message.type = type;
    message.any = p_arena_alloc(arena, message_size);
    memset(message.any, 0, message_size);
    return message;
}

pSerializationError p_serialize_message(pBitStream *bs, pMessage *msg) {
    switch (msg->type) {
        case pMessageType_ConnectionRequest:
            return p_serialize_connection_request_message(bs, msg->connection_request);
        case pMessageType_ConnectionDenied:
            return p_serialize_connection_denied_message(bs, msg->connection_denied);
        case pMessageType_ConnectionAccepted:
            return p_serialize_connection_accepted_message(bs, msg->connection_accepted);
        case pMessageType_ConnectionClosed:
            return p_serialize_connection_closed_message(bs, msg->connection_closed);
        case pMessageType_InputState:
            return p_serialize_input_state_message(bs, msg->input_state);
        case pMessageType_WorldState:
            return p_serialize_world_state_message(bs, msg->world_state);
        default: P_PANIC(); break;
    }

    return pSerializationError_ValueOutOfRange;
}

void p_append_message(pPacket *packet, pMessage msg) {
    P_ASSERT(packet->message_count < MAX_MESSAGES_PER_PACKET);
    packet->messages[packet->message_count].type = msg.type;
    packet->messages[packet->message_count].any = msg.any;
    packet->message_count += 1;
}

bool p_send_packet(pSocket socket, pAddress address, pPacket *packet) {
    uint8_t buffer[1400];

    pBitStream write_stream = p_create_write_stream(buffer, sizeof(buffer));
    pSerializationError err = pSerializationError_None;
    for (int i = 0; i < packet->message_count; i += 1) {
        err = p_serialize_enum(&write_stream, &packet->messages[i].type, pMessageType_Count);
        if (err) return false;
        err = p_serialize_message(&write_stream, &packet->messages[i]);
        if (err) return false;
    }
    p_bit_stream_flush_bits(&write_stream);
    int bytes_written = p_bit_stream_bytes_processed(&write_stream);

    pSocketSendError send_error = p_socket_send(socket, address, buffer, bytes_written);
    if (send_error != pSocketSendError_None) {
        fprintf(stdout, "pSocketSendError: %u\n", send_error);
        return false;
    }

    return true;
}

bool p_receive_packet(pSocket socket, pArena *arena, pAddress *address, pPacket *packet) {
    P_ASSERT(packet->message_count == 0);

    uint8_t buffer[1400];
    int bytes_received;
    pSocketReceiveError srcv_error = p_socket_receive(socket, buffer, sizeof(buffer), &bytes_received, address);
    if (srcv_error != pSocketReceiveError_None) {
        switch (srcv_error) {
        #if defined(_WIN32)
            case pSocketReceiveError_WouldBlock:
            case pSocketReceiveError_RemoteNotListening:
                return false;
        #else
            case pSocketReceiveError_Timeout:
                return false;
        #endif
            default: {
                fprintf(stdout, "pSocketReceiveError: %u\n", srcv_error);
                return false;
            } break;
        }
    }

    packet->message_count = 0;
    pBitStream read_stream = p_create_read_stream(buffer, sizeof(buffer), bytes_received);
    while (p_bit_stream_bytes_processed(&read_stream) < bytes_received) {
        pMessageType message_type;
        pSerializationError s_error;
        s_error = p_serialize_enum(&read_stream, &message_type, pMessageType_Count);
        if (s_error) return false;
        pMessage msg = p_message_create(arena, message_type);
        packet->messages[packet->message_count] = msg;
        packet->message_count += 1;
        s_error = p_serialize_message(&read_stream, &msg);
        if (s_error) return false;
    }
    return true;
}
