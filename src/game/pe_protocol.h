#ifndef PE_PROTOCOL_H
#define PE_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#include "game/pe_entity.h"

#define MAX_MESSAGES_PER_PACKET 64

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
    int client_index;
    uint32_t entity_index;
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

typedef struct peInputStateMessage {
    pInput input;
} peInputStateMessage;

typedef struct peWorldStateMessage {
    pEntity entities[MAX_ENTITY_COUNT];
} peWorldStateMessage;

typedef enum peMessageType {
    peMessageType_ConnectionRequest,
    peMessageType_ConnectionDenied,
    peMessageType_ConnectionAccepted,
    peMessageType_ConnectionClosed,
    peMessageType_InputState,
    peMessageType_WorldState,
    peMessageType_Count,
} peMessageType;

typedef struct peMessage {
    peMessageType type;
    union {
        void *any;
        peConnectionRequestMessage *connection_request;
        peConnectionDeniedMessage *connection_denied;
        peConnectionAcceptedMessage *connection_accepted;
        peConnectionClosedMessage *connection_closed;
        peInputStateMessage *input_state;
        peWorldStateMessage *world_state;
    };
} peMessage;

typedef struct pePacket {
    peMessage messages[MAX_MESSAGES_PER_PACKET];
    int message_count;
} pePacket;

struct pArena;
struct pBitStream;
enum pSerializationError;
peMessage pe_message_create(struct pArena *arena, peMessageType type);
enum pSerializationError p_serialize_message(struct pBitStream *bs, peMessage *msg);
void pe_append_message(pePacket *packet, peMessage msg);

struct peSocket;
struct peAddress;
bool pe_send_packet(struct peSocket socket, struct peAddress address, pePacket *packet);
bool pe_receive_packet(struct peSocket socket, struct pArena *arena, struct peAddress *address, pePacket *packet);

#endif // PE_PROTOCOL_H
