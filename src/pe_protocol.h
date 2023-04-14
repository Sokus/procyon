#ifndef PE_PROTOCOL_H
#define PE_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_MESSAGES_PER_PACKET 64

typedef struct pePacket {
    void *messages[MAX_MESSAGES_PER_PACKET];
    int message_count;
} pePacket;

typedef enum peMessageType {
    peMessage_ConnectionRequest,
    peMessage_ConnectionDenied,
    peMessage_ConnectionAccepted,
    peMessage_ConnectionClosed,
    peMessageType_Count,
} peMessageType;

typedef struct peConnectionRequestMessage {
    uint8_t zero;
} peConnectionRequestMessage;

typedef enum peConnectionDeniedReason {
    peConnectionDeniedReason_ServerFull,
    peConnectionDeniedReason_Count,
} peConnectionDeniedReason;
typedef struct peConnectionDeniedMessage {
    peConnectionDeniedReason reason;
} peConnectionDeniedMessage;

typedef struct peConnectionAcceptedMessage {
    uint8_t zero;
} peConnectionAcceptedMessage;

typedef enum peConnectionClosedReason {
    peConnectionClosedReason_ClientDisconnected,
    peConnectionClosedReason_ServerShutdown,
    peConnectionClosedReason_TimedOut,
    peConnectionClosedReason_Count,
} peConnectionClosedReason;
typedef struct peConnectionClosedMessage {
    peConnectionClosedReason reason;
} peConnectionClosedMessage;

typedef enum peSerializationError peSerializationError;
typedef struct peBitStream peBitStream;
peSerializationError pe_serialize_message(peBitStream *bs, void *msg, peMessageType type);
void pe_append_message(pePacket *packet, void *msg);

typedef struct peSocket peSocket;
typedef struct peAddress peAddress;
bool pe_send_packet(peSocket socket, peAddress address, pePacket *packet);
pePacket *pe_receive_packet(peSocket socket, peAddress *address);

#endif // PE_PROTOCOL_H