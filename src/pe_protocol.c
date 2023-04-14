#include "pe_protocol.h"

#include "pe_core.h"
#include "pe_bit_stream.h"
#include "pe_net.h"

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


peSerializationError pe_serialize_message(peBitStream *bs, void *msg, peMessageType type) {
    switch (type) {
        case peMessage_ConnectionRequest:
            return pe_serialize_connection_request_message(bs, msg);
        case peMessage_ConnectionDenied:
            return pe_serialize_connection_denied_message(bs, msg);
        case peMessage_ConnectionAccepted:
            return pe_serialize_connection_accepted_message(bs, msg);
        case peMessage_ConnectionClosed:
            return pe_serialize_connection_closed_message(bs, msg);

        default: PE_PANIC(); break;
    }

    return peSerializationError_ValueOutOfRange;
}

void pe_append_message(pePacket *packet, void *msg) {
    PE_ASSERT(packet->message_count < MAX_MESSAGES_PER_PACKET);
    packet->messages[packet->message_count] = msg;
    packet->message_count += 1;
}

bool pe_send_packet(peSocket socket, peAddress address, pePacket *packet) {
    PE_UNIMPLEMENTED();
    return false;
}

pePacket *pe_receive_packet(peSocket socket, peAddress *address) {
    PE_UNIMPLEMENTED();
    return NULL;
}