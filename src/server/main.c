#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "core/p_defines.h"
#include "core/p_assert.h"
#include "core/p_heap.h"
#include "core/p_arena.h"
#include "core/p_time.h"
#include "platform/pe_net.h"

#include "pe_config.h"
#include "game/p_protocol.h"
#include "game/p_entity.h"

#include <string.h>

#define SECONDS_TO_TIME_OUT 10 // seconds
#define CONNECTION_REQUEST_RESPONSE_SEND_RATE 1 // per second

typedef struct peClientData {
    uint64_t connect_time;
    uint64_t last_packet_send_time;
    uint64_t last_packet_receive_time;
    uint32_t entity_index;
} peClientData;

struct peServerState {
    pSocket socket;
    int client_count;
    bool client_connected[MAX_CLIENT_COUNT];
    pAddress client_address[MAX_CLIENT_COUNT];
    pInput client_input[MAX_CLIENT_COUNT];
    peClientData client_data[MAX_CLIENT_COUNT];
} server = {0};

void pe_reset_client_state(int client_index) {
    server.client_connected[client_index] = false;
    memset(&server.client_address[client_index], 0, sizeof(pAddress));
    memset(&server.client_data[client_index], 0, sizeof(peClientData));
}

bool pe_find_free_client_index(int *index) {
    if (server.client_count == MAX_CLIENT_COUNT) {
        return false;
    }
    for (int i = 0; i < MAX_CLIENT_COUNT; i += 1) {
        if (!server.client_connected[i]) {
            *index = i;
            return true;
        }
    }
    return false;
}

bool pe_find_existing_client_index(pAddress address, int *index) {
    for (int i = 0; i < MAX_CLIENT_COUNT; i += 1) {
        if (server.client_connected[i] && p_address_compare(server.client_address[i], address)) {
            *index = i;
            return true;
        }
    }
    return false;
}

void p_send_packet_to_connected_client(int client_index, pPacket *packet) {
    P_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
    P_ASSERT(server.client_connected[client_index]);
    p_send_packet(server.socket, server.client_address[client_index], packet);
    server.client_data[client_index].last_packet_send_time = p_time_now();
}

void pe_connect_client(int client_index, pAddress address) {
    P_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
    P_ASSERT(server.client_count < MAX_CLIENT_COUNT-1);
    P_ASSERT(!server.client_connected[client_index]);

    char address_string_buffer[256];
    char *address_string = p_address_to_string(address, address_string_buffer, sizeof(address_string_buffer));
    fprintf(stdout, "client %d connected (address = %s)\n", client_index, address_string);

    server.client_count += 1;

    pEntity *entity = p_make_entity();
    entity->active = true;
    p_entity_property_set(entity, pEntityProperty_CanCollide);
    p_entity_property_set(entity, pEntityProperty_OwnedByPlayer);
    p_entity_property_set(entity, pEntityProperty_ControlledByPlayer);
    entity->client_index = client_index;
    entity->mesh = pEntityMesh_Cube;

    server.client_connected[client_index] = true;
    server.client_address[client_index] = address;
    server.client_data[client_index].entity_index = entity->index;
    uint64_t time_now = p_time_now();
    server.client_data[client_index].connect_time = time_now;
    server.client_data[client_index].last_packet_receive_time = time_now;

    pPacket packet = {0};
    pConnectionAcceptedMessage connection_accepted_message = {
        .client_index = client_index,
        .entity_index = entity->index
    };
    pMessage message = {
        .type = pMessageType_ConnectionAccepted,
        .connection_accepted = &connection_accepted_message
    };
    p_append_message(&packet, message);
    p_send_packet_to_connected_client(client_index, &packet);
}

void pe_disconnect_client(int client_index, pConnectionClosedReason reason) {
    P_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
    P_ASSERT(server.client_count > 0);
    P_ASSERT(server.client_connected[client_index]);

    if (reason != pConnectionClosedReason_ServerShutdown) {
        char address_string_buffer[256];
        char *address_string = p_address_to_string(server.client_address[client_index], address_string_buffer, sizeof(address_string_buffer));
        printf("client %d disconnected (address = %s, reason = %d)\n", client_index, address_string, reason);
    }

    pPacket packet = {0};
    pConnectionClosedMessage connection_closed_message = {
        .reason = reason
    };
    pMessage message = {
        .type = pMessageType_ConnectionClosed,
        .connection_closed = &connection_closed_message
    };
    p_append_message(&packet, message);
    p_send_packet_to_connected_client(client_index, &packet);

    pe_reset_client_state(client_index);
    server.client_count -= 1;

    pEntity *entities = p_get_entities();
    for (int i = 0; i < MAX_ENTITY_COUNT; i += 1) {
        pEntity *entity = &entities[i];
        if (p_entity_property_get(entity, pEntityProperty_OwnedByPlayer) && entity->client_index == client_index) {
            p_destroy_entity(entity);
        }
    }
}

void pe_process_connection_request_message(pConnectionRequestMessage *msg, pAddress address, bool client_exists, int client_index) {
    if (client_exists) {
        P_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
        P_ASSERT(p_address_compare(address, server.client_address[client_index]));
        float connection_request_repsonse_interval = 1.0f / (float)CONNECTION_REQUEST_RESPONSE_SEND_RATE;
        uint64_t ticks_since_last_sent_packet = p_time_since(server.client_data[client_index].last_packet_send_time);
        float seconds_since_last_sent_packet = (float)p_time_sec(ticks_since_last_sent_packet);
        if (seconds_since_last_sent_packet > connection_request_repsonse_interval) {
            pPacket packet = {0};
            pConnectionAcceptedMessage connection_accepted_message = {
                .client_index = client_index,
                .entity_index = server.client_data[client_index].entity_index
            };
            pMessage message = {
                .type = pMessageType_ConnectionAccepted,
                .connection_accepted = &connection_accepted_message,
            };
            p_append_message(&packet, message);
            p_send_packet_to_connected_client(client_index, &packet);
        }
        return;
    }

    char address_string_buffer[256];
    char *address_string = p_address_to_string(address, address_string_buffer, sizeof(address_string_buffer));
    fprintf(stdout, "processing connection request message (address = %s)\n", address_string);

    if (server.client_count == MAX_CLIENT_COUNT) {
        fprintf(stdout, "connection request denied: server is full");
        pPacket packet = {0};
        pConnectionDeniedMessage connection_denied_message = {
            .reason = pConnectionDeniedReason_ServerFull,
        };
        pMessage message = {
            .type = pMessageType_ConnectionDenied,
            .connection_denied = &connection_denied_message,
        };
        p_append_message(&packet, message);
        p_send_packet(server.socket, address, &packet);
        return;
    }

    int free_client_index;
    bool found_free_client_index = pe_find_free_client_index(&free_client_index);
    P_ASSERT(found_free_client_index);
    pe_connect_client(free_client_index, address);
}

void pe_process_connection_closed_message(pConnectionClosedMessage *msg, pAddress address, bool client_exists, int client_index) {
    if (client_exists) {
        P_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
        P_ASSERT(p_address_compare(address, server.client_address[client_index]));
        pe_disconnect_client(client_index, pConnectionClosedReason_ClientDisconnected);
    }
}

void pe_process_input_state_message(pInputStateMessage *msg, pAddress address, bool client_exists, int client_index) {
    if (client_exists) {
        P_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
        P_ASSERT(p_address_compare(address, server.client_address[client_index]));
        server.client_input[client_index] = msg->input;
        server.client_data[client_index].last_packet_receive_time = p_time_now();
    }
}

void p_receive_packets(pArena *temp_arena) {
    pAddress address;
    pPacket packet = {0};
    pArenaTemp receive_packets_arena_temp = p_arena_temp_begin(temp_arena);
    while (true) {
        pArenaTemp loop_arena_temp = p_arena_temp_begin(temp_arena);
        bool packet_received = p_receive_packet(server.socket, temp_arena, &address, &packet);
        if (!packet_received) {
            break;
        }

        int client_index;
        bool client_exists = pe_find_existing_client_index(address, &client_index);
        for (int i = 0; i < packet.message_count; i += 1) {
            pMessage *message = &packet.messages[i];
            switch (message->type) {
                case pMessageType_ConnectionRequest:
                    pe_process_connection_request_message(message->connection_request, address, client_exists, client_index);
                    break;
                case pMessageType_ConnectionClosed:
                    pe_process_connection_closed_message(message->connection_closed, address, client_exists, client_index);
                    break;
                case pMessageType_InputState:
                    pe_process_input_state_message(message->input_state, address, client_exists, client_index);
                    break;
                default:
                    break;
            }
        }
        p_arena_temp_end(loop_arena_temp);
        packet.message_count = 0;
    }
    p_arena_temp_end(receive_packets_arena_temp);
}

void p_send_packets(void) {
    pPacket packet = {0};
    pWorldStateMessage world_state_message;
    pMessage message = {
        .type = pMessageType_WorldState,
        .world_state = &world_state_message
    };
    p_append_message(&packet, message);
    pEntity *entities = p_get_entities();
    memcpy(world_state_message.entities, entities, MAX_ENTITY_COUNT*sizeof(pEntity));

    for (int i = 0; i < MAX_CLIENT_COUNT; i += 1) {
        if (server.client_connected[i]) {
            // TODO: send pending messages
            p_send_packet_to_connected_client(i, &packet);
        }
    }
}

void pe_check_for_time_out(void) {
    for (int i = 0; i < MAX_CLIENT_COUNT; i += 1) {
        if (server.client_connected[i]) {
            uint64_t ticks_since_last_received_packet = p_time_since(server.client_data[i].last_packet_receive_time);
            float seconds_since_last_received_packet = (float)p_time_sec(ticks_since_last_received_packet);
            if (seconds_since_last_received_packet > (float)SECONDS_TO_TIME_OUT) {
                pe_disconnect_client(i, pConnectionClosedReason_TimedOut);
            }
        }
    }
}

int main(int argc, char *argv[]) {
	pArena temp_arena;
    {
        size_t temp_arena_size = P_MEGABYTES(4);
        p_arena_init(&temp_arena, p_heap_alloc(temp_arena_size), temp_arena_size);
    }

    p_net_init();

    p_socket_create(pAddressFamily_IPv4, &server.socket);
    p_socket_set_nonblocking(server.socket);
    {
        pSocketBindError socket_bind_error = p_socket_bind(server.socket, pAddressFamily_IPv4, SERVER_PORT);
        P_ASSERT(socket_bind_error == pSocketBindError_None);
    }

    p_allocate_entities();

    float dt = 1.0f/60.0f;
    while(true) {
        uint64_t work_start_tick = p_time_now();

        p_receive_packets(&temp_arena);

        pe_check_for_time_out();

        p_update_entities(dt, server.client_input);
        p_cleanup_entities();

        p_send_packets();

        p_arena_clear(&temp_arena);

        uint64_t ticks_spent_working = p_time_since(work_start_tick);
        double seconds_spent_working = p_time_sec(ticks_spent_working);
        double seconds_left_for_cycle = (double)dt - seconds_spent_working;
        if (seconds_left_for_cycle > 0.0) {
            unsigned int ms_sleep_duration = (unsigned int)(1000.0 * seconds_left_for_cycle);
            p_time_sleep(ms_sleep_duration);
        }
    }

    p_socket_destroy(server.socket);

    p_net_shutdown();

    return 0;
}
