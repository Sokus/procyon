#ifndef P_PROTOCOL_H
#define P_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#include "game/p_entity.h"

#define MAX_MESSAGES_PER_PACKET 64

typedef struct pConnectionRequestMessage {
    uint8_t zero;
} pConnectionRequestMessage;

typedef enum pConnectionDeniedReason {
    pConnectionDeniedReason_ServerFull,
    pConnectionDeniedReason_Count,
} pConnectionDeniedReason;
typedef struct pConnectionDeniedMessage {
    pConnectionDeniedReason reason;
} pConnectionDeniedMessage;

typedef struct pConnectionAcceptedMessage {
    int client_index;
    uint32_t entity_index;
} pConnectionAcceptedMessage;

typedef enum pConnectionClosedReason {
    pConnectionClosedReason_ClientDisconnected,
    pConnectionClosedReason_ServerShutdown,
    pConnectionClosedReason_TimedOut,
    pConnectionClosedReason_Count,
} pConnectionClosedReason;
typedef struct pConnectionClosedMessage {
    pConnectionClosedReason reason;
} pConnectionClosedMessage;

typedef struct pInputStateMessage {
    pInput input;
} pInputStateMessage;

typedef struct pWorldStateMessage {
    pEntity entities[MAX_ENTITY_COUNT];
} pWorldStateMessage;

typedef enum pMessageType {
    pMessageType_ConnectionRequest,
    pMessageType_ConnectionDenied,
    pMessageType_ConnectionAccepted,
    pMessageType_ConnectionClosed,
    pMessageType_InputState,
    pMessageType_WorldState,
    pMessageType_Count,
} pMessageType;

typedef struct pMessage {
    pMessageType type;
    union {
        void *any;
        pConnectionRequestMessage *connection_request;
        pConnectionDeniedMessage *connection_denied;
        pConnectionAcceptedMessage *connection_accepted;
        pConnectionClosedMessage *connection_closed;
        pInputStateMessage *input_state;
        pWorldStateMessage *world_state;
    };
} pMessage;

typedef struct pPacket {
    pMessage messages[MAX_MESSAGES_PER_PACKET];
    int message_count;
} pPacket;

struct pArena;
struct pBitStream;
enum pSerializationError;
pMessage p_message_create(struct pArena *arena, pMessageType type);
enum pSerializationError p_serialize_message(struct pBitStream *bs, pMessage *msg);
void p_append_message(pPacket *packet, pMessage msg);

struct pSocket;
struct pAddress;
bool p_send_packet(struct pSocket socket, struct pAddress address, pPacket *packet);
bool p_receive_packet(struct pSocket socket, struct pArena *arena, struct pAddress *address, pPacket *packet);

#endif // P_PROTOCOL_H
