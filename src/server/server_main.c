#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"
#include "pe_temp_arena.h"

#include "pe_config.h"
#include "pe_protocol.h"
#include "client/pe_client.h"
#include "game/pe_entity.h"

#define SECONDS_TO_TIME_OUT 10 // seconds
#define CONNECTION_REQUEST_RESPONSE_SEND_RATE 1 // per second

typedef struct peClientData {
    uint64_t connect_time;
    uint64_t last_packet_send_time;
    uint64_t last_packet_receive_time;
    uint32_t entity_index;
    peInput input;
} peClientData;

struct peServerState {
    bool is_hosting;

    peSocket socket;
    int client_count;
    bool client_connected[MAX_CLIENT_COUNT];
    peAddress client_address[MAX_CLIENT_COUNT];
    peClientData client_data[MAX_CLIENT_COUNT];
} server = {0};

void pe_reset_client_state(int client_index) {
    server.client_connected[client_index] = false;
    pe_zero_size(&server.client_address[client_index], sizeof(peAddress));
    pe_zero_size(&server.client_data[client_index], sizeof(peClientData));
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

bool pe_find_existing_client_index(peAddress address, int *index) {
    for (int i = 0; i < MAX_CLIENT_COUNT; i += 1) {
        if (server.client_connected[i] && pe_address_compare(server.client_address[i], address)) {
            *index = i;
            return true;
        }
    }
    return false;
}

void pe_send_packet_to_connected_client(int client_index, pePacket *packet) {
    PE_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
    PE_ASSERT(server.client_connected[client_index]);
    pe_send_packet(server.socket, server.client_address[client_index], packet);
    server.client_data[client_index].last_packet_send_time = pe_time_now();
}

void pe_connect_client(int client_index, peAddress address) {
    PE_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
    PE_ASSERT(server.client_count < MAX_CLIENT_COUNT-1);
    PE_ASSERT(!server.client_connected[client_index]);

    char address_string_buffer[256];
    char *address_string = pe_address_to_string(address, address_string_buffer, sizeof(address_string_buffer));
    fprintf(stdout, "client %d connected (address = %s)\n", client_index, address_string);

    server.client_count += 1;

    peEntity *entity = pe_make_entity();
    entity->active = true;
    pe_entity_property_set(entity, peEntityProperty_CanCollide);
    pe_entity_property_set(entity, peEntityProperty_OwnedByPlayer);
    pe_entity_property_set(entity, peEntityProperty_ControlledByPlayer);
    entity->client_index = client_index;
    entity->mesh = peEntityMesh_Cube;

    server.client_connected[client_index] = true;
    server.client_address[client_index] = address;
    server.client_data[client_index].entity_index = entity->index;
    uint64_t time_now = pe_time_now();
    server.client_data[client_index].connect_time = time_now;
    server.client_data[client_index].last_packet_receive_time = time_now;

    pePacket packet = {0};
    peConnectionAcceptedMessage connection_accepted_message = {
        .client_index = client_index,
        .entity_index = entity->index
    };
    peMessage message = {
        .type = peMessageType_ConnectionAccepted,
        .connection_accepted = &connection_accepted_message
    };
    pe_append_message(&packet, message);
    pe_send_packet_to_connected_client(client_index, &packet);
}

void pe_disconnect_client(int client_index, peConnectionClosedReason reason) {
    PE_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
    PE_ASSERT(server.client_count > 0);
    PE_ASSERT(server.client_connected[client_index]);

    if (reason != peConnectionClosedReason_ServerShutdown) {
        char address_string_buffer[256];
        char *address_string = pe_address_to_string(server.client_address[client_index], address_string_buffer, sizeof(address_string_buffer));
        printf("client %d disconnected (address = %s, reason = %d)\n", client_index, address_string, reason);
    }

    pePacket packet = {0};
    peConnectionClosedMessage connection_closed_message = {
        .reason = reason
    };
    peMessage message = {
        .type = peMessageType_ConnectionClosed,
        .connection_closed = &connection_closed_message
    };
    pe_append_message(&packet, message);
    pe_send_packet_to_connected_client(client_index, &packet);

    pe_reset_client_state(client_index);
    server.client_count -= 1;

    peEntity *entities = pe_get_entities();
    for (int i = 0; i < MAX_ENTITY_COUNT; i += 1) {
        peEntity *entity = &entities[i];
        if (pe_entity_property_get(entity, peEntityProperty_OwnedByPlayer) && entity->client_index == client_index) {
            pe_destroy_entity(entity);
        }
    }
}

void pe_process_connection_request_message(peConnectionRequestMessage *msg, peAddress address, bool client_exists, int client_index) {
    if (client_exists) {
        PE_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
        PE_ASSERT(pe_address_compare(address, server.client_address[client_index]));
        float connection_request_repsonse_interval = 1.0f / (float)CONNECTION_REQUEST_RESPONSE_SEND_RATE;
        uint64_t ticks_since_last_sent_packet = pe_time_since(server.client_data[client_index].last_packet_send_time);
        float seconds_since_last_sent_packet = (float)pe_time_sec(ticks_since_last_sent_packet);
        if (seconds_since_last_sent_packet > connection_request_repsonse_interval) {
            pePacket packet = {0};
            peConnectionAcceptedMessage connection_accepted_message = {
                .client_index = client_index,
                .entity_index = server.client_data[client_index].entity_index
            };
            peMessage message = {
                .type = peMessageType_ConnectionAccepted,
                .connection_accepted = &connection_accepted_message,
            };
            pe_append_message(&packet, message);
            pe_send_packet_to_connected_client(client_index, &packet);
        }
        return;
    }

    char address_string_buffer[256];
    char *address_string = pe_address_to_string(address, address_string_buffer, sizeof(address_string_buffer));
    fprintf(stdout, "processing connection request message (address = %s)\n", address_string);

    if (server.client_count == MAX_CLIENT_COUNT) {
        fprintf(stdout, "connection request denied: server is full");
        pePacket packet = {0};
        peConnectionDeniedMessage connection_denied_message = {
            .reason = peConnectionDeniedReason_ServerFull,
        };
        peMessage message = {
            .type = peMessageType_ConnectionDenied,
            .connection_denied = &connection_denied_message,
        };
        pe_append_message(&packet, message);
        pe_send_packet(server.socket, address, &packet);
        return;
    }

    int free_client_index;
    bool found_free_client_index = pe_find_free_client_index(&free_client_index);
    PE_ASSERT(found_free_client_index);
    pe_connect_client(free_client_index, address);
}

void pe_process_connection_closed_message(peConnectionClosedMessage *msg, peAddress address, bool client_exists, int client_index) {
    if (client_exists) {
        PE_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
        PE_ASSERT(pe_address_compare(address, server.client_address[client_index]));
        pe_disconnect_client(client_index, peConnectionClosedReason_ClientDisconnected);
    }
}

void pe_process_input_state_message(peInputStateMessage *msg, peAddress address, bool client_exists, int client_index) {
    if (client_exists) {
        PE_ASSERT(client_index >= 0 && client_index < MAX_CLIENT_COUNT);
        PE_ASSERT(pe_address_compare(address, server.client_address[client_index]));
        server.client_data[client_index].input = msg->input;
        server.client_data[client_index].last_packet_receive_time = pe_time_now();
    }
}

void pe_process_world_state_message(peWorldStateMessage *msg, peAddress address, bool client_exists, int client_index) {
	//if (client.network_state == peClientNetworkState_Connected) {
		memcpy(pe_get_entities(), msg->entities, MAX_ENTITY_COUNT*sizeof(peEntity));
	//}
}

void pe_receive_packets(void) {
    peAddress address;
    pePacket packet = {0};
    peArena *temp_arena = pe_temp_arena();
    peArenaTemp receive_packets_arena_temp = pe_arena_temp_begin(temp_arena);
    while (true) {
        peArenaTemp loop_arena_temp = pe_arena_temp_begin(temp_arena);
        bool packet_received = pe_receive_packet(server.socket, temp_arena, &address, &packet);
        if (!packet_received) {
            break;
        }

        int client_index;
        bool client_exists = pe_find_existing_client_index(address, &client_index);
        for (int i = 0; i < packet.message_count; i += 1) {
            peMessage *message = &packet.messages[i];
            switch (message->type) {
                case peMessageType_ConnectionRequest:
                    pe_process_connection_request_message(message->connection_request, address, client_exists, client_index);
                    break;
                case peMessageType_ConnectionClosed:
                    pe_process_connection_closed_message(message->connection_closed, address, client_exists, client_index);
                    break;
                case peMessageType_InputState:
                    pe_process_input_state_message(message->input_state, address, client_exists, client_index);
                    break;
#if !defined(PE_SERVER_STANDALONE)
                case peMessageType_WorldState:
                    pe_process_world_state_message(message->world_state, address, client_exists, client_index);
                    break;
#endif
                default:
#if !defined(PE_SERVER_STANDALONE)
                    pe_client_process_message(*message, address);
#endif
                    break;
            }
        }
        pe_arena_temp_end(loop_arena_temp);
        packet.message_count = 0;
    }
    pe_arena_temp_end(receive_packets_arena_temp);
}

void pe_send_packets(void) {
    pePacket packet = {0};
    peWorldStateMessage world_state_message;
    peMessage message = {
        .type = peMessageType_WorldState,
        .world_state = &world_state_message
    };
    pe_append_message(&packet, message);
    peEntity *entities = pe_get_entities();
    memcpy(world_state_message.entities, entities, MAX_ENTITY_COUNT*sizeof(peEntity));

    for (int i = 0; i < MAX_CLIENT_COUNT; i += 1) {
        // TODO: skip local client
        if (server.client_connected[i]) {
            // TODO: send pending messages
            pe_send_packet_to_connected_client(i, &packet);
        }
    }
}

void pe_check_for_time_out(void) {
    for (int i = 0; i < MAX_CLIENT_COUNT; i += 1) {
        if (server.client_connected[i]) {
            uint64_t ticks_since_last_received_packet = pe_time_since(server.client_data[i].last_packet_receive_time);
            float seconds_since_last_received_packet = (float)pe_time_sec(ticks_since_last_received_packet);
            if (seconds_since_last_received_packet > (float)SECONDS_TO_TIME_OUT) {
                pe_disconnect_client(i, peConnectionClosedReason_TimedOut);
            }
        }
    }
}

int main(int argc, char *argv[]) {
	pe_temp_arena_init(PE_MEGABYTES(4));
    pe_time_init();
    pe_net_init();

    pe_socket_create(peAddressFamily_IPv4, &server.socket);
    pe_socket_set_nonblocking(server.socket);
    server.is_hosting = true;
#if defined(PE_SERVER_STANDALONE)
    {
        peSocketBindError socket_bind_error = pe_socket_bind(server.socket, peAddressFamily_IPv4, SERVER_PORT_MIN);
        PE_ASSERT_MSG(socket_bind_error == peSocketBindError_None, "Could not bind to %u on standalone server!\n", SERVER_PORT_MIN);
    }
#else
    for (uint16_t port = SERVER_PORT_MIN; port < SERVER_PORT_MAX; port += 1) {
        peSocketBindError socket_bind_error = pe_socket_bind(server.socket, peAddressFamily_IPv4, port);
        if (socket_bind_error == peSocketBindError_None) {
            break;
        } else {
            server.is_hosting = false;
        }
    }
#endif

    pe_allocate_entities();

#if !defined(PE_SERVER_STANDALONE)
    pe_client_init(&server.socket);
#endif

    /*
        float connection_request_repsonse_interval = 1.0f / (float)CONNECTION_REQUEST_RESPONSE_SEND_RATE;
        uint64_t ticks_since_last_sent_packet = pe_time_since(server.client_data[client_index].last_packet_send_time);
        float seconds_since_last_sent_packet = (float)pe_time_sec(ticks_since_last_sent_packet);
        if (seconds_since_last_sent_packet > connection_request_repsonse_interval) {
    */

    float dt = 1.0f/60.0f;
    while(true) {
        uint64_t work_start_tick = pe_time_now();

        pe_receive_packets();

        if (server.is_hosting) {
            pe_check_for_time_out();

            peEntity *entities = pe_get_entities();
            for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
                peEntity *entity = &entities[e];
                if (!entity->active)
                    continue;

                if (pe_entity_property_get(entity, peEntityProperty_ControlledByPlayer)) {
                    PE_ASSERT(entity->client_index >= 0 && entity->client_index < MAX_CLIENT_COUNT);
                    PE_ASSERT(server.client_connected[entity->client_index] || entity->marked_for_destruction);

                    peInput *input = &server.client_data[entity->client_index].input;
                    entity->velocity.X = input->movement.X * 2.0f;
                    entity->velocity.Z = input->movement.Y * 2.0f;
                    entity->angle = input->angle;
                }

                // PHYSICS
                if (entity->velocity.X != 0.0f || entity->velocity.Z != 0.0f) {
                    HMM_Vec3 position_delta = HMM_MulV3F(entity->velocity, dt);
                    entity->position = HMM_AddV3(entity->position, position_delta);
                }
            }
            pe_cleanup_entities();

            pe_send_packets();
        }

#if !defined(PE_SERVER_STANDALONE)
        pe_client_update();
        if (pe_client_should_quit()) break;
#endif
        pe_arena_clear(pe_temp_arena());

        uint64_t ticks_spent_working = pe_time_since(work_start_tick);
        double seconds_spent_working = pe_time_sec(ticks_spent_working);
        double seconds_left_for_cycle = (double)dt - seconds_spent_working;
        if (seconds_left_for_cycle > 0.0) {
            unsigned int ms_sleep_duration = (unsigned int)(1000.0 * seconds_left_for_cycle);
            pe_time_sleep(ms_sleep_duration);
        }
    }

#if !defined(PE_SERVER_STANDALONE)
    pe_client_shutdown();
#endif

    pe_socket_destroy(server.socket);

    pe_net_shutdown();

    return 0;
}