#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "core/p_defines.h"
#include "core/p_assert.h"
#include "core/p_heap.h"
#include "core/p_time.h"
#include "core/p_arena.h"
#include "core/p_scratch.h"
#include "math/p_math.h"
#include "graphics/p_graphics_math.h"
#include "platform/p_net.h"
#include "platform/p_window.h"
#include "graphics/p_graphics.h"
#include "graphics/p_model.h"
#include "platform/p_input.h"
#include "utility/p_trace.h"

#include "p_config.h"
#include "game/p_protocol.h"
#include "game/p_entity.h"

#include "platform/p_file.h"
#include "graphics/p_pbm.h"

#include "stb_image.h"

#define CONNECTION_REQUEST_TIME_OUT 20.0f // seconds
#define CONNECTION_REQUEST_SEND_RATE 1.0f // requests per second

#define SECONDS_TO_TIME_OUT 10.0f

typedef enum pClientNetworkState {
	pClientNetworkState_Disconnected,
	pClientNetworkState_Connecting,
	pClientNetworkState_Connected,
	pClientNetworkState_Error,
} pClientNetworkState;

struct pClientState {
    pCamera camera;
    pVec3 camera_offset;
    pModel model;

    pSocket socket;
    pAddress server_address;
    pClientNetworkState network_state;
    int client_index;
    uint32_t entity_index;
    pInput client_input[MAX_CLIENT_COUNT];
    uint64_t last_packet_send_time;
    uint64_t last_packet_receive_time;
} client = {0};

void p_client_process_message(pMessage message) {
    P_TRACE_FUNCTION_BEGIN();
    switch (message.type) {
        case pMessageType_ConnectionDenied: {
            fprintf(stdout, "connection request denied (reason = %d)\n", message.connection_denied->reason);
            client.network_state = pClientNetworkState_Error;
        } break;
        case pMessageType_ConnectionAccepted: {
            if (client.network_state == pClientNetworkState_Connecting) {
                char address_string[256];
                fprintf(stdout, "connected to the server (address = %s)\n", p_address_to_string(client.server_address, address_string, sizeof(address_string)));
                client.network_state = pClientNetworkState_Connected;
                client.client_index = message.connection_accepted->client_index;
                client.entity_index = message.connection_accepted->entity_index;
                client.last_packet_receive_time = p_time_now();
            } else if (client.network_state == pClientNetworkState_Connected) {
                P_ASSERT(client.client_index == message.connection_accepted->client_index);
                client.last_packet_receive_time = p_time_now();
            }
        } break;
        case pMessageType_ConnectionClosed: {
            if (client.network_state == pClientNetworkState_Connected) {
                fprintf(stdout, "connection closed (reason = %d)\n", message.connection_closed->reason);
                client.network_state = pClientNetworkState_Error;
            }
        } break;
        case pMessageType_WorldState:
            if (client.network_state == pClientNetworkState_Connected) {
                memcpy(p_get_entities(), message.world_state->entities, MAX_ENTITY_COUNT*sizeof(pEntity));
            }
        default:
            break;
    }
    P_TRACE_FUNCTION_END();
}

void p_receive_packets(pSocket socket) {
    P_TRACE_FUNCTION_BEGIN();
    pAddress address;
    pPacket packet = {0};
    pArenaTemp scratch = p_scratch_begin(NULL, 0);
    while (true) {
        pArenaTemp loop_arena_temp = p_arena_temp_begin(scratch.arena);
        bool packet_received = p_receive_packet(socket, scratch.arena, &address, &packet);
        if (!packet_received) {
            break;
        }

        if (!p_address_compare(address, client.server_address)) {
            return;
        }

        for (int i = 0; i < packet.message_count; i += 1) {
            p_client_process_message(packet.messages[i]);
        }
        p_arena_temp_end(loop_arena_temp);
        packet.message_count = 0;
    }
    p_scratch_end(scratch);
    P_TRACE_FUNCTION_END();
}

void p_send_packets(pInput input) {
    pTraceMark send_packets_tm = P_TRACE_MARK_BEGIN("prepare packet");
    pArenaTemp scratch = p_scratch_begin(NULL, 0);
    pPacket outgoing_packet = {0};
    switch (client.network_state) {
        case pClientNetworkState_Disconnected: {
            fprintf(stdout, "connecting to the server\n");
            client.network_state = pClientNetworkState_Connecting;
            client.last_packet_receive_time = p_time_now();
        } break;
        case pClientNetworkState_Connecting: {
            uint64_t ticks_since_last_received_packet = p_time_since(client.last_packet_receive_time);
            float seconds_since_last_received_packet = (float)p_time_sec(ticks_since_last_received_packet);
            if (seconds_since_last_received_packet > (float)CONNECTION_REQUEST_TIME_OUT) {
                //fprintf(stdout, "connection request timed out");
                client.network_state = pClientNetworkState_Error;
                break;
            }
            float connection_request_send_interval = 1.0f / (float)CONNECTION_REQUEST_SEND_RATE;
            uint64_t ticks_since_last_sent_packet = p_time_since(client.last_packet_send_time);
            float seconds_since_last_sent_packet = (float)p_time_sec(ticks_since_last_sent_packet);
            if (seconds_since_last_sent_packet > connection_request_send_interval) {
                pMessage message = p_message_create(scratch.arena, pMessageType_ConnectionRequest);
                p_append_message(&outgoing_packet, message);
            }
        } break;
        case pClientNetworkState_Connected: {
            pMessage message = p_message_create(scratch.arena, pMessageType_InputState);
            message.input_state->input = input;
            p_append_message(&outgoing_packet, message);
        } break;
        default: break;
    }
    P_TRACE_MARK_END(send_packets_tm);

    if (outgoing_packet.message_count > 0) {
        pTraceMark send_packet_tm = P_TRACE_MARK_BEGIN("p_send_packet");
        p_send_packet(client.socket, client.server_address, &outgoing_packet);
        P_TRACE_MARK_END(send_packet_tm);
        client.last_packet_send_time = p_time_now();
    }
    outgoing_packet.message_count = 0;
    p_scratch_end(scratch);
}

static pVec2 last_nonzero_gamepad_input = { 0.0f, 1.0f };

pInput p_get_input(pCamera camera) {
    pVec2 gamepad_input = {
        .x = p_input_gamepad_axis(pGamepadAxis_LeftX),
        .y = p_input_gamepad_axis(pGamepadAxis_LeftY)
    };

    bool right_is_down = p_input_key_is_down(pKeyboardKey_Right);
    bool left_is_down = p_input_key_is_down(pKeyboardKey_Left);
    bool down_is_down = p_input_key_is_down(pKeyboardKey_Down);
    bool up_is_down = p_input_key_is_down(pKeyboardKey_Up);
    pVec2 keyboard_input = {
        .x = (float)right_is_down - (float)left_is_down,
        .y = (float)down_is_down - (float)up_is_down
    };

    static float look_angle = 0.0f;
    {
#if defined(_WIN32) || defined(__linux__)
        float pos_x, pos_y;
        p_input_mouse_positon(&pos_x, &pos_y);
        pRay ray = p_get_mouse_ray(p_vec2(pos_x, pos_y), client.camera);

        pVec3 collision_point;
        if (p_collision_ray_plane(ray, (pVec3){.y = 1.0f}, 0.0f, &collision_point)) {
            pVec3 relative_collision_point = p_vec3_sub(client.camera.target, collision_point);
            look_angle = atan2f(relative_collision_point.x, relative_collision_point.z);
        }
#endif
#if defined(__PSP__)
        if (gamepad_input.x != 0.0f || gamepad_input.y != 0.0f) {
            last_nonzero_gamepad_input = gamepad_input;
        }
        look_angle = atan2f(-last_nonzero_gamepad_input.x, -last_nonzero_gamepad_input.y);
#endif
    }

    pInput result = {
        .movement = p_vec2(
            P_CLAMP(gamepad_input.x + keyboard_input.x, -1.0f, 1.0f),
            P_CLAMP(gamepad_input.y + keyboard_input.y, -1.0f, 1.0f)
        ),
        .angle = look_angle
    };
    return result;
}

pTexture codepage = {0};

void p_graphics_draw_string(pString str, int pos_x, int pos_y, pColor color) {
    int src_char_w = 9;
    int src_char_h = 16;
    float dst_char_w = 9.0f;
    float dst_char_h = 16.0f;
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    for (size_t c_idx = 0; c_idx < str.size; c_idx += 1) {
        char c = P_MIN(str.data[c_idx], 255);
        switch (c) {
            case ' ':
            case '\t':
                offset_x += dst_char_w;
                break;
            case '\n':
                offset_x = 0.0f;
                offset_y += dst_char_h;
                break;
            default: {
                int column = (int)(c % 28);
                int row = (int)(c / 28);
                pRect src = {
                    .x = (float)(column * src_char_w),
                    .y = (float)(row * src_char_h),
                    .width = (float)src_char_w,
                    .height = (float)src_char_h,
                };
                pRect dst = {
                    (float)pos_x + offset_x,
                    (float)pos_y + offset_y,
                    dst_char_w,
                    dst_char_h
                };
                p_graphics_draw_texture_ex(&codepage, src, dst, color);
                offset_x += dst_char_w;
            } break;
        }
    }
}

int main(int argc, char* argv[]) {
    p_net_init();
    p_trace_init();

    p_window_set_target_fps(60);
    p_window_init(960, 540, "Procyon");

    p_socket_create(pAddressFamily_IPv4, &client.socket);
    p_socket_set_nonblocking(client.socket);

    p_allocate_entities();

    pArenaTemp scratch = p_scratch_begin(NULL, 0);

    client.model = p_model_load("./res/models/fox.p3d");

    {
        // pTraceMark tm_pbm_load = P_TRACE_MARK_BEGIN("pbm_load");
        // pFileContents pbm_file_contents = p_file_read_contents(scratch.arena, "./res/cp437.pbm", false);
        // pbmFile pbm_file;
        // bool pbm_parse_result = pbm_parse(pbm_file_contents.data, pbm_file_contents.size, &pbm_file);
        // P_TRACE_MARK_END(tm_pbm_load);
        // codepage = p_texture_create_pbm(&pbm_file);
        // p_scratch_end(scratch);

        int w,h,n;
        pTraceMark tm_stbi_load = P_TRACE_MARK_BEGIN("stbi_load");
        unsigned char *cp437_data = stbi_load("./res/CP437.png", &w, &h, &n, 4);
        P_TRACE_MARK_END(tm_stbi_load);
        codepage = p_texture_create(cp437_data, w, h);
        stbi_image_free(cp437_data);
    }

    client.camera = (pCamera){
        .target = {0.0f, 0.7f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f,
    };
    client.camera_offset = p_vec3(0.0f, 1.4f, 2.0f);
    client.camera.position = p_vec3_add(client.camera.target, client.camera_offset);

    bool multiplayer = false;
    if (!multiplayer) {
        pEntity *entity = p_make_entity();
        entity->active = true;
        p_entity_property_set(entity, pEntityProperty_CanCollide);
        p_entity_property_set(entity, pEntityProperty_OwnedByPlayer);
        p_entity_property_set(entity, pEntityProperty_ControlledByPlayer);
        entity->client_index = 0;
        entity->mesh = pEntityMesh_Cube;
    }

    float dt = p_window_delta_time();

    while(!p_window_should_quit()) {
        if (multiplayer) {
            p_net_update();
            p_receive_packets(client.socket);
        } else {
                client.server_address = p_address4(127, 0, 0, 1, SERVER_PORT);
            if (
                p_input_key_pressed(pKeyboardKey_O)
                || p_input_gamepad_pressed(pGamepadButton_ActionUp)
            ) {
                multiplayer = true;
            }
        }

        pTraceMark entity_logic_tm = P_TRACE_MARK_BEGIN("entity logic");
        pEntity *entities = p_get_entities();
        for (int i = 0; i < MAX_ENTITY_COUNT; i += 1) {
            pEntity *entity = &entities[i];
            if (!entity->active) continue;

            if (p_entity_property_get(entity, pEntityProperty_OwnedByPlayer) && entity->client_index == client.client_index) {
                client.camera.target = p_vec3_add(entity->position, p_vec3(0.0f, 0.7f, 0.0f));
                client.camera.position = p_vec3_add(client.camera.target, client.camera_offset);
            }
        };
        P_TRACE_MARK_END(entity_logic_tm);

        pInput input = p_get_input(client.camera);
        if (multiplayer) {
            p_send_packets(input);
        } else {
            client.client_input[0] = input;
            p_update_entities(dt, client.client_input);
        }

        p_window_frame_begin();
        p_clear_background((pColor){ 20, 20, 20, 255 });

        // p_graphics_draw_point_int(3, 2, P_COLOR_BLUE);

        for (int i = 0; i < 160; i += 1) {
            float position_x = 5.0f * (float)i;
            // p_graphics_draw_line(p_vec2(position_x, 0.0f), p_vec2(position_x+4.0f, 4.0f), P_COLOR_RED);
        }

        // p_graphics_draw_texture(&client.model.material[0].diffuse_map, 100.0f, 100.0f, P_COLOR_WHITE);

        p_graphics_mode_3d_begin(client.camera);
        {
            // p_graphics_draw_line_3D(p_vec3(0.0f, 0.0f, 0.0f), p_vec3(0.25f, 0.0f, 0.0f), P_COLOR_RED);
            // p_graphics_draw_line_3D(p_vec3(0.0f, 0.0f, 0.0f), p_vec3(0.0f, 0.25f, 0.0f), P_COLOR_GREEN);
            // p_graphics_draw_line_3D(p_vec3(0.0f, 0.0f, 0.0f), p_vec3(0.0f, 0.0f, 0.25f), P_COLOR_BLUE);

            p_graphics_draw_grid_3D(10, 1.0f);

            for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
                pEntity *entity = &entities[e];
                if (!entity->active) continue;

                p_model_draw(&client.model, entity->position, p_vec3(0.0f, entity->angle, 0.0f));
            }
        }
        p_graphics_mode_3d_end();

        // p_graphics_draw_texture(&client.model.material[0].diffuse_map, 10.0f, 10.0f, P_COLOR_WHITE);
        float w = 252.0f*0.25f;
        float h = 160.0f*0.25f;
        pColor colors[] = {
            { 255, 0, 0, 64 },
            { 255, 255, 0, 64 },
            { 0, 255, 0, 64 },
            { 0, 255, 255, 64 },
            { 0, 0, 255, 64 },
            { 255, 255, 255, 64 },
        };
        pTraceMark tm_issue_rectangles = P_TRACE_MARK_BEGIN("issue rectangles");
        // p_graphics_draw_texture(&codepage, 0.0f, 0.0f, 1.0f, P_COLOR_WHITE);

        {
            pDynamicDrawStats stats = p_graphics_dynamic_draw_stats();
            char *stat_format = "dynamic draw:\n"
                "vertex: %4d/%4d (peak: %d)\n"
                "batch:  %4d/%4d (peak: %d)\n";
            pString stat_str = p_string_format(
                scratch.arena,
                stat_format,
                stats.last_vertex_used,
                P_MAX_DYNAMIC_DRAW_VERTEX_COUNT,
                stats.peak_vertex_used,
                stats.last_batch_count,
                P_MAX_DYNAMIC_DRAW_BATCH_COUNT,
                stats.peak_batch_count
            );
            p_graphics_draw_string(stat_str, 5, 5, P_COLOR_WHITE);
        }

        P_TRACE_MARK_END(tm_issue_rectangles);

        bool vsync = true;
        p_window_frame_end(vsync);
        p_scratch_clear();
    }

    p_socket_destroy(client.socket);
    p_net_shutdown();
    p_trace_shutdown();
    p_window_shutdown();
    return 0;
}
