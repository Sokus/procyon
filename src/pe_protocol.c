#include "pe_protocol.h"

#include "pe_core.h"
#include "pe_bit_stream.h"
#include "pe_net.h"

#include <stdio.h>

static peSerializationError pe_serialize_connection_request_message(peBitStream *bs, peConnectionRequestMessage *msg) {
    return peSerializationError_None;
}

static peSerializationError pe_serialize_connection_denied_message(peBitStream *bs, peConnectionDeniedMessage *msg) {
    return pe_serialize_enum(bs, &msg->reason, peConnectionDeniedReason_Count);
}

static peSerializationError pe_serialize_connection_accepted_message(peBitStream *bs, peConnectionAcceptedMessage *msg) {
    return peSerializationError_None;
}

static peSerializationError pe_serialize_connection_closed_message(peBitStream *bs, peConnectionClosedMessage *msg) {
    return pe_serialize_enum(bs, &msg->reason, peConnectionClosedReason_Count);
}


peMessage pe_message_create(peAllocator a, peMessageType type) {
    PE_ASSERT(type >= 0);
    PE_ASSERT(type < peMessageType_Count);
    peMessage message = {0};
    size_t message_size;
    switch (type) {
        case peMessageType_ConnectionRequest: message_size = sizeof(peConnectionRequestMessage); break;
        case peMessageType_ConnectionDenied: message_size = sizeof(peConnectionDeniedMessage); break;
        case peMessageType_ConnectionAccepted: message_size = sizeof(peConnectionAcceptedMessage); break;
        case peMessageType_ConnectionClosed: message_size = sizeof(peConnectionClosedMessage); break;
        default: PE_PANIC(); return message;
    }
    message.type = type;
    message.any = pe_alloc(a, message_size);
    return message;
}

void pe_message_destroy(peAllocator a, peMessage msg) {
    pe_free(a, msg.any);
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

        default: PE_PANIC(); break;
    }

    return peSerializationError_ValueOutOfRange;
}

void pe_append_message(pePacket *packet, peMessage msg) {
    PE_ASSERT(packet->message_count < MAX_MESSAGES_PER_PACKET);
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

bool pe_receive_packet(peSocket socket, peAddress *address, pePacket *packet) {
    PE_ASSERT(packet->message_count == 0);

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

    peAllocator allocator = pe_heap_allocator(); // FIXME

    packet->message_count = 0;
    peBitStream read_stream = pe_create_read_stream(buffer, sizeof(buffer), bytes_received);
    while (pe_bit_stream_bytes_processed(&read_stream) < bytes_received) {
        peMessageType message_type;
        peSerializationError s_error;
        s_error = pe_serialize_enum(&read_stream, &message_type, peMessageType_Count);
        if (s_error) goto cleanup;
        peMessage msg = pe_message_create(allocator, message_type);
        packet->messages[packet->message_count] = msg;
        packet->message_count += 1;
        s_error = pe_serialize_message(&read_stream, &msg);
        if (s_error) goto cleanup;
    }
    return true;

cleanup:
    pe_free_packet(allocator, packet);
    return false;
}

void pe_free_packet(peAllocator a, pePacket *packet) {
    for (int i = 0; i < packet->message_count; i += 1) {
        pe_message_destroy(a, packet->messages[i]);
    }
    packet->message_count = 0;
}