#include "pe_client.h"

#include "pe_core.h"
#include "pe_net.h"
#include "pe_platform.h"
#include "pe_graphics.h"
#include "pe_model.h"
#include "pe_time.h"
#include "pe_input.h"

#include "pe_config.h"
#include "pe_protocol.h"
#include "game/pe_entity.h"

#include <stdio.h>
#include <stdbool.h>

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
    HMM_Vec3 camera_offset;
    peModel model;

    peSocket *socket;
    peAddress server_address;
    peClientNetworkState network_state;
    int client_index;
    uint32_t entity_index;
    uint64_t last_packet_send_time;
    uint64_t last_packet_receive_time;
} client = {0};

void pe_client_init(peArena *temp_arena, peSocket *socket) {
    client.socket = socket;
    client.server_address = pe_address4(127, 0, 0, 1, SERVER_PORT_MIN);

    pe_platform_init();
    pe_graphics_init(temp_arena, 960, 540, "Procyon");
    pe_input_init();

#if !defined(PSP)
    #define PE_MODEL_EXTENSION ".p3d"
#else
    #define PE_MODEL_EXTENSION ".pp3d"
#endif
    client.model = pe_model_load(temp_arena, "./res/fox" PE_MODEL_EXTENSION);

    client.camera = (peCamera){
        .target = {0.0f, 0.7f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f,
    };
    client.camera_offset = HMM_V3(0.0f, 1.4f, 2.0f);
    client.camera.position = HMM_AddV3(client.camera.target, client.camera_offset);
}

static float look_angle = 0.0f;
#if defined(_WIN32) || defined(__linux__)
    #include "pe_math.h"
#endif

void pe_client_update(peArena *temp_arena) {
    pe_platform_poll_events();
    pe_input_update();

    peEntity *entities = pe_get_entities();
    for (int i = 0; i < MAX_ENTITY_COUNT; i += 1) {
        peEntity *entity = &entities[i];
        if (!entity->active) continue;

        if (pe_entity_property_get(entity, peEntityProperty_OwnedByPlayer) && entity->client_index == client.client_index) {
            client.camera.target = HMM_AddV3(entity->position, HMM_V3(0.0f, 0.7f, 0.0f));
            client.camera.position = HMM_AddV3(client.camera.target, client.camera_offset);
        }
    };

    pe_camera_update(client.camera);

#if defined(_WIN32) || defined(__linux__)
    {
        float pos_x, pos_y;
        pe_input_mouse_positon(&pos_x, &pos_y);
        peRay ray = pe_get_mouse_ray(HMM_V2(pos_x, pos_y), client.camera);

        HMM_Vec3 collision_point;
        if (pe_collision_ray_plane(ray, (HMM_Vec3){.Y = 1.0f}, 0.0f, &collision_point)) {
            HMM_Vec3 relative_collision_point = HMM_SubV3(client.camera.target, collision_point);
            look_angle = atan2f(relative_collision_point.X, relative_collision_point.Z);
        }
    }
#endif

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
            HMM_Vec2 gamepad_input = {
                .X = pe_input_gamepad_axis(peGamepadAxis_LeftX),
                .Y = pe_input_gamepad_axis(peGamepadAxis_LeftY)
            };
            bool right_is_down = pe_input_key_is_down(peKeyboardKey_Right);
            bool left_is_down = pe_input_key_is_down(peKeyboardKey_Left);
            bool down_is_down = pe_input_key_is_down(peKeyboardKey_Down);
            bool up_is_down = pe_input_key_is_down(peKeyboardKey_Up);
            HMM_Vec2 keyboard_input = {
                .X = (float)right_is_down - (float)left_is_down,
                .Y = (float)down_is_down - (float)up_is_down
            };
            message.input_state->input.movement = HMM_V2(
                PE_CLAMP(gamepad_input.X + keyboard_input.X, -1.0f, 1.0f),
                PE_CLAMP(gamepad_input.Y + keyboard_input.Y, -1.0f, 1.0f)
            );
            message.input_state->input.angle = look_angle;
			pe_append_message(&outgoing_packet, message);
		} break;
		default: break;
	}

	if (outgoing_packet.message_count > 0) {
		pe_send_packet(*client.socket, client.server_address, &outgoing_packet);
		client.last_packet_send_time = pe_time_now();
	}
	outgoing_packet.message_count = 0;
    pe_arena_temp_end(send_packets_arena_temp);

    pe_graphics_frame_begin();
    pe_clear_background((peColor){ 20, 20, 20, 255 });

    for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
        peEntity *entity = &entities[e];
        if (!entity->active) continue;

        pe_model_draw(&client.model, temp_arena, entity->position, HMM_V3(0.0f, entity->angle, 0.0f));
    }

    pe_graphics_frame_end(true);
}

bool pe_client_should_quit(void) {
    return pe_platform_should_quit();
}

void pe_client_shutdown(void) {
    pe_graphics_shutdown();
    pe_platform_shutdown();
}

void pe_client_process_message(peMessage message, peAddress sender) {
    if (!pe_address_compare(sender, client.server_address)) {
        return;
    }
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
		default: break;
    }
}