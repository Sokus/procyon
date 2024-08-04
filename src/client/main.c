#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "core/pe_core.h"
#include "math/p_math.h"
#include "graphics/pe_math.h"
#include "platform/pe_time.h"
#include "platform/pe_net.h"
#include "platform/pe_platform.h"
#include "platform/pe_window.h"
#include "graphics/pe_graphics.h"
#include "graphics/pe_model.h"
#include "platform/pe_input.h"
#include "utility/pe_trace.h"

#include "pe_config.h"
#include "game/pe_protocol.h"
#include "game/pe_entity.h"

#define CONNECTION_REQUEST_TIME_OUT 20.0f // seconds
#define CONNECTION_REQUEST_SEND_RATE 1.0f // requests per second

#define SECONDS_TO_TIME_OUT 10.0f

typedef enum peClientNetworkState {
	peClientNetworkState_Disconnected,
	peClientNetworkState_Connecting,
	peClientNetworkState_Connected,
	peClientNetworkState_Error,
} peClientNetworkState;

struct peClientState {
    peCamera camera;
    pVec3 camera_offset;
    peModel model;

    peSocket socket;
    peAddress server_address;
    peClientNetworkState network_state;
    int client_index;
    uint32_t entity_index;
    peInput client_input[MAX_CLIENT_COUNT];
    uint64_t last_packet_send_time;
    uint64_t last_packet_receive_time;
} client = {0};

bool pe_client_should_quit(void) {
    bool should_quit = (
        pe_platform_should_quit() ||
        pe_window_should_quit()
    );
    return should_quit;
}

void pe_client_process_message(peMessage message) {
    PE_TRACE_FUNCTION_BEGIN();
    switch (message.type) {
        case peMessageType_ConnectionDenied: {
            fprintf(stdout, "connection request denied (reason = %d)\n", message.connection_denied->reason);
            client.network_state = peClientNetworkState_Error;
        } break;
        case peMessageType_ConnectionAccepted: {
            if (client.network_state == peClientNetworkState_Connecting) {
                char address_string[256];
                fprintf(stdout, "connected to the server (address = %s)\n", pe_address_to_string(client.server_address, address_string, sizeof(address_string)));
                client.network_state = peClientNetworkState_Connected;
                client.client_index = message.connection_accepted->client_index;
                client.entity_index = message.connection_accepted->entity_index;
                client.last_packet_receive_time = pe_time_now();
            } else if (client.network_state == peClientNetworkState_Connected) {
                PE_ASSERT(client.client_index == message.connection_accepted->client_index);
                client.last_packet_receive_time = pe_time_now();
            }
        } break;
        case peMessageType_ConnectionClosed: {
            if (client.network_state == peClientNetworkState_Connected) {
                fprintf(stdout, "connection closed (reason = %d)\n", message.connection_closed->reason);
                client.network_state = peClientNetworkState_Error;
            }
        } break;
        case peMessageType_WorldState:
            if (client.network_state == peClientNetworkState_Connected) {
                memcpy(pe_get_entities(), message.world_state->entities, MAX_ENTITY_COUNT*sizeof(peEntity));
            }
        default:
            break;
    }
    PE_TRACE_FUNCTION_END();
}

void pe_receive_packets(peArena *temp_arena, peSocket socket) {
    PE_TRACE_FUNCTION_BEGIN();
    peAddress address;
    pePacket packet = {0};
    while (true) {
        peArenaTemp loop_arena_temp = pe_arena_temp_begin(temp_arena);
        bool packet_received = pe_receive_packet(socket, temp_arena, &address, &packet);
        if (!packet_received) {
            break;
        }

        if (!pe_address_compare(address, client.server_address)) {
            return;
        }

        for (int i = 0; i < packet.message_count; i += 1) {
            pe_client_process_message(packet.messages[i]);
        }
        pe_arena_temp_end(loop_arena_temp);
        packet.message_count = 0;
    }
    PE_TRACE_FUNCTION_END();
}

void pe_send_packets(peArena *temp_arena, peInput input) {
    peTraceMark send_packets_tm = PE_TRACE_MARK_BEGIN("prepare packet");
    peArenaTemp send_packets_arena_temp = pe_arena_temp_begin(temp_arena);
    pePacket outgoing_packet = {0};
    switch (client.network_state) {
        case peClientNetworkState_Disconnected: {
            fprintf(stdout, "connecting to the server\n");
            client.network_state = peClientNetworkState_Connecting;
            client.last_packet_receive_time = pe_time_now();
        } break;
        case peClientNetworkState_Connecting: {
            uint64_t ticks_since_last_received_packet = pe_time_since(client.last_packet_receive_time);
            float seconds_since_last_received_packet = (float)pe_time_sec(ticks_since_last_received_packet);
            if (seconds_since_last_received_packet > (float)CONNECTION_REQUEST_TIME_OUT) {
                //fprintf(stdout, "connection request timed out");
                client.network_state = peClientNetworkState_Error;
                break;
            }
            float connection_request_send_interval = 1.0f / (float)CONNECTION_REQUEST_SEND_RATE;
            uint64_t ticks_since_last_sent_packet = pe_time_since(client.last_packet_send_time);
            float seconds_since_last_sent_packet = (float)pe_time_sec(ticks_since_last_sent_packet);
            if (seconds_since_last_sent_packet > connection_request_send_interval) {
                peMessage message = pe_message_create(temp_arena, peMessageType_ConnectionRequest);
                pe_append_message(&outgoing_packet, message);
            }
        } break;
        case peClientNetworkState_Connected: {
            peMessage message = pe_message_create(temp_arena, peMessageType_InputState);
            message.input_state->input = input;
            pe_append_message(&outgoing_packet, message);
        } break;
        default: break;
    }
    PE_TRACE_MARK_END(send_packets_tm);

    if (outgoing_packet.message_count > 0) {
        peTraceMark send_packet_tm = PE_TRACE_MARK_BEGIN("pe_send_packet");
        pe_send_packet(client.socket, client.server_address, &outgoing_packet);
        PE_TRACE_MARK_END(send_packet_tm);
        client.last_packet_send_time = pe_time_now();
    }
    outgoing_packet.message_count = 0;
    pe_arena_temp_end(send_packets_arena_temp);
}

peInput pe_get_input(peCamera camera) {
    pVec2 gamepad_input = {
        .x = pe_input_gamepad_axis(peGamepadAxis_LeftX),
        .y = pe_input_gamepad_axis(peGamepadAxis_LeftY)
    };

    bool right_is_down = pe_input_key_is_down(peKeyboardKey_Right);
    bool left_is_down = pe_input_key_is_down(peKeyboardKey_Left);
    bool down_is_down = pe_input_key_is_down(peKeyboardKey_Down);
    bool up_is_down = pe_input_key_is_down(peKeyboardKey_Up);
    pVec2 keyboard_input = {
        .x = (float)right_is_down - (float)left_is_down,
        .y = (float)down_is_down - (float)up_is_down
    };

    static float look_angle = 0.0f;
    {
#if defined(_WIN32) || defined(__linux__)
        float pos_x, pos_y;
        pe_input_mouse_positon(&pos_x, &pos_y);
        peRay ray = pe_get_mouse_ray(p_vec2(pos_x, pos_y), client.camera);

        pVec3 collision_point;
        if (pe_collision_ray_plane(ray, (pVec3){.y = 1.0f}, 0.0f, &collision_point)) {
            pVec3 relative_collision_point = p_vec3_sub(client.camera.target, collision_point);
            look_angle = atan2f(relative_collision_point.x, relative_collision_point.z);
        }
#endif
    }

    peInput result = {
        .movement = p_vec2(
            PE_CLAMP(gamepad_input.x + keyboard_input.x, -1.0f, 1.0f),
            PE_CLAMP(gamepad_input.y + keyboard_input.y, -1.0f, 1.0f)
        ),
        .angle = look_angle
    };
    return result;
}

int main(int argc, char* argv[]) {
	peArena temp_arena;
    {
        size_t temp_arena_size = PE_MEGABYTES(4);
        pe_arena_init(&temp_arena, pe_heap_alloc(temp_arena_size), temp_arena_size);
    }

    pe_platform_init();
    pe_time_init();
    pe_net_init();
    pe_trace_init();

    pe_window_init(960, 540, "Procyon");
    pe_graphics_init(&temp_arena, 960, 540);
    pe_input_init();

    pe_socket_create(peAddressFamily_IPv4, &client.socket);
    pe_socket_set_nonblocking(client.socket);

    pe_allocate_entities();

#if !defined(PSP)
    #define PE_MODEL_EXTENSION ".p3d"
#else
    #define PE_MODEL_EXTENSION ".pp3d"
#endif
    client.model = pe_model_load(&temp_arena, "./res/assets/fox" PE_MODEL_EXTENSION);

    client.camera = (peCamera){
        .target = {0.0f, 0.7f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f,
    };
    client.camera_offset = p_vec3(0.0f, 1.4f, 2.0f);
    client.camera.position = p_vec3_add(client.camera.target, client.camera_offset);

    bool multiplayer = false;
    if (!multiplayer) {
        peEntity *entity = pe_make_entity();
        entity->active = true;
        pe_entity_property_set(entity, peEntityProperty_CanCollide);
        pe_entity_property_set(entity, peEntityProperty_OwnedByPlayer);
        pe_entity_property_set(entity, peEntityProperty_ControlledByPlayer);
        entity->client_index = 0;
        entity->mesh = peEntityMesh_Cube;
    }

    // TODO: Fix Your Timestep!
    float dt = 1.0f/60.0f;

    while(!pe_client_should_quit()) {
        uint64_t loop_start = pe_time_now();

        pe_window_poll_events();
        pe_input_update();

        if (multiplayer) {
            pe_net_update();
            pe_receive_packets(&temp_arena, client.socket);
        } else {
                client.server_address = pe_address4(127, 0, 0, 1, SERVER_PORT);
            if (
                pe_input_key_pressed(peKeyboardKey_O)
                || pe_input_gamepad_pressed(peGamepadButton_ActionUp)
            ) {
                multiplayer = true;
            }
        }

        peTraceMark entity_logic_tm = PE_TRACE_MARK_BEGIN("entity logic");
        peEntity *entities = pe_get_entities();
        for (int i = 0; i < MAX_ENTITY_COUNT; i += 1) {
            peEntity *entity = &entities[i];
            if (!entity->active) continue;

            if (pe_entity_property_get(entity, peEntityProperty_OwnedByPlayer) && entity->client_index == client.client_index) {
                client.camera.target = p_vec3_add(entity->position, p_vec3(0.0f, 0.7f, 0.0f));
                client.camera.position = p_vec3_add(client.camera.target, client.camera_offset);
            }
        };
        PE_TRACE_MARK_END(entity_logic_tm);

        peInput input = pe_get_input(client.camera);
        if (multiplayer) {
            pe_send_packets(&temp_arena, input);
        } else {
            client.client_input[0] = input;
            pe_update_entities(dt, client.client_input);
        }

        pe_graphics_frame_begin();
        pe_clear_background((peColor){ 20, 20, 20, 255 });

   
        // pe_graphics_draw_point_int(3, 2, PE_COLOR_BLUE);
    
        for (int i = 0; i < 160; i += 1) {
            float position_x = 5.0f * (float)i;
            // pe_graphics_draw_line(p_vec2(position_x, 0.0f), p_vec2(position_x+4.0f, 4.0f), PE_COLOR_RED);
        }
        
        // pe_graphics_draw_texture(&client.model.material[0].diffuse_map, 100.0f, 100.0f, PE_COLOR_WHITE);

        pe_graphics_mode_3d_begin(client.camera);
        {
            pe_graphics_draw_line_3D(p_vec3(0.0f, 0.0f, 0.0f), p_vec3(0.25f, 0.0f, 0.0f), PE_COLOR_RED);
            pe_graphics_draw_line_3D(p_vec3(0.0f, 0.0f, 0.0f), p_vec3(0.0f, 0.25f, 0.0f), PE_COLOR_GREEN);
            pe_graphics_draw_line_3D(p_vec3(0.0f, 0.0f, 0.0f), p_vec3(0.0f, 0.0f, 0.25f), PE_COLOR_BLUE);

            pe_graphics_draw_grid_3D(10, 1.0f);

            for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
                peEntity *entity = &entities[e];
                if (!entity->active) continue;

                pe_model_draw(&client.model, &temp_arena, entity->position, p_vec3(0.0f, entity->angle, 0.0f));
            }
        }
        pe_graphics_mode_3d_end();

        pe_graphics_draw_rectangle(5.0f, 5.0f, 70.0f, 70.0f, (peColor){ 255, 0, 0, 64 });
        pe_graphics_draw_rectangle(30.0f, 30.0f, 70.0f, 70.0f, (peColor){ 0, 255, 0, 64 });
        pe_graphics_draw_rectangle(55.0f, 55.0f, 70.0f, 70.0f, (peColor){ 0, 0, 255, 64 });

        bool vsync = true;
        pe_graphics_frame_end(vsync);
        uint64_t loop_time = pe_time_since(loop_start);
        double loop_time_sec = pe_time_sec(loop_time);
        if (vsync && loop_time_sec < dt) {
            unsigned long sleep_time = (unsigned long)((dt - loop_time_sec) * 1000.0f);
            pe_time_sleep(sleep_time);
        }

        pe_arena_clear(&temp_arena);
    }

    pe_graphics_shutdown();
    pe_window_shutdown();

    pe_socket_destroy(client.socket);
    pe_net_shutdown();

    pe_trace_shutdown();
    pe_platform_shutdown();

    return 0;
}