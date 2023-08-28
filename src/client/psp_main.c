#include "pe_core.h"
#include "pe_time.h"
#include "pe_input.h"
#include "pe_protocol.h"
#include "pe_net.h"
#include "game/pe_entity.h"
#include "pe_config.h"
#include "pe_file_io.h"
#include "pe_graphics.h"
#include "pe_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>

#include "HandmadeMath.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <pspgum.h>

#include "pe_platform.h"

static peTexture default_texture;

void pe_default_texture_init(void) {
	int texture_width = 8;
	int texture_height = 8;
	size_t texture_data_size = texture_width * texture_height * sizeof(uint16_t);

	peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(pe_temp_arena());
	{
		uint16_t *texture_data = pe_alloc(pe_temp_allocator(), texture_data_size);
		for (int y = 0; y < texture_height; y++) {
			for (int x = 0; x < texture_width; x++) {
				if (y < texture_height/2 && x < texture_width/2 || y >= texture_height/2 && x >= texture_width/2) {
					texture_data[y*texture_width + x] = pe_color_to_5650(PE_COLOR_WHITE);
				} else {
					texture_data[y*texture_width + x] = pe_color_to_5650(PE_COLOR_GRAY);
				}
			}
		}
		default_texture = pe_texture_create(texture_data, texture_width, texture_height, GU_PSM_5650, true);
	}
	pe_temp_arena_memory_end(temp_arena_memory);
}

//
// MODELS
//

int frame = 0;

void pe_perspective(float fovy, float aspect, float near, float far) {
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(fovy, aspect, near, far);
}

void pe_view_lookat(HMM_Vec3 eye, HMM_Vec3 target, HMM_Vec3 up) {
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	sceGumLookAt((ScePspFVector3*)&eye, (ScePspFVector3*)&target, (ScePspFVector3*)&up);
}

//
// NET CLIENT STUFF
//

#define CONNECTION_REQUEST_TIME_OUT 20.0f // seconds
#define CONNECTION_REQUEST_SEND_RATE 1.0f // # per second

#define SECONDS_TO_TIME_OUT 10.0f

typedef enum peClientNetworkState {
	peClientNetworkState_Disconnected,
	peClientNetworkState_Connecting,
	peClientNetworkState_Connected,
	peClientNetworkState_Error,
} peClientNetworkState;

static peSocket socket;
peAddress server_address;
peClientNetworkState network_state = peClientNetworkState_Disconnected;
int client_index;
uint32_t entity_index;
uint64_t last_packet_send_time = 0;
uint64_t last_packet_receive_time = 0;

pePacket outgoing_packet = {0};

extern peEntity *entities;

void pe_receive_packets(void) {
	peAddress address;
	pePacket packet = {0};
	peAllocator allocator = pe_heap_allocator();
	while (pe_receive_packet(socket, allocator, &address, &packet)) {
		if (!pe_address_compare(address, server_address)) {
			goto message_cleanup;
		}

		for (int m = 0; m < packet.message_count; m += 1) {
			peMessage message = packet.messages[m];
			switch (message.type) {
				case peMessageType_ConnectionDenied: {
					fprintf(stdout, "connection request denied (reason = %d)\n", message.connection_denied->reason);
					network_state = peClientNetworkState_Error;
				} break;
				case peMessageType_ConnectionAccepted: {
					if (network_state == peClientNetworkState_Connecting) {
						char address_string[256];
						fprintf(stdout, "connected to the server (address = %s)\n", pe_address_to_string(server_address, address_string, sizeof(address_string)));
						network_state = peClientNetworkState_Connected;
						client_index = message.connection_accepted->client_index;
						entity_index = message.connection_accepted->entity_index;
						last_packet_receive_time = pe_time_now();
					} else if (network_state == peClientNetworkState_Connected) {
						PE_ASSERT(client_index == message.connection_accepted->client_index);
						last_packet_receive_time = pe_time_now();
					}
				} break;
				case peMessageType_ConnectionClosed: {
					if (network_state == peClientNetworkState_Connected) {
						fprintf(stdout, "connection closed (reason = %d)\n", message.connection_closed->reason);
						network_state = peClientNetworkState_Error	;
					}
				} break;
				case peMessageType_WorldState: {
					if (network_state == peClientNetworkState_Connected) {
						memcpy(entities, message.world_state->entities, MAX_ENTITY_COUNT*sizeof(peEntity));
					}
				} break;
				default: break;
			}
		}

message_cleanup:
		for (int m = 0; m < packet.message_count; m += 1) {
			pe_message_destroy(allocator, packet.messages[m]);
		}
		packet.message_count = 0;
	}
}

int main(int argc, char* argv[])
{
	pe_platform_init();

	pe_graphics_init(0, 0, NULL);
	pe_default_texture_init();

	pe_net_init();
	pe_time_init();
	pe_input_init();

	pe_allocate_entities();

	peSocketCreateError socket_create_error = pe_socket_create(peSocket_IPv4, 0, &socket);
	fprintf(stdout, "socket create result: %d\n", socket_create_error);

	server_address = pe_address4(192, 168, 128, 81, SERVER_PORT);

    stbi_set_flip_vertically_on_load(GU_FALSE);

	peModel model = pe_model_load("./res/fox.pp3d");

	char *fox_diffuse_texture_paths[] = {
		"./res/fox_body_diffuse.png",
		"./res/fox_sword_diffuse.png"
	};
	for (int t = 0; t < 2; t += 1) {
		int w, h, channels;
		stbi_uc *stbi_data = stbi_load(fox_diffuse_texture_paths[t], &w, &h, &channels, STBI_rgb_alpha);
		model.material[t].has_diffuse_map = true;
		model.material[t].diffuse_map = pe_texture_create(stbi_data, w, h, GU_PSM_8888, true);
	}

	float aspect = (float)pe_screen_width()/(float)pe_screen_height();
	pe_perspective(55.0f, aspect, 0.5f, 1000.0f);

	HMM_Vec3 eye_offset = { 0.0f, 1.7f, 3.0f };
	HMM_Vec3 target = { 0.0f, 0.0f, -1.0f };
	HMM_Vec3 eye = HMM_AddV3(target, eye_offset);
	HMM_Vec3 up = { 0.0f, 1.0f, 0.0f };
	pe_view_lookat(eye, target, up);

	float frame_time = 1.0f/30.0f;
	float current_frame_time = 0.0f;

	float dt = 1.0f/60.0f;
	while(!pe_platform_should_quit())
	{
		pe_net_update();
		pe_input_update();

		if (network_state != peClientNetworkState_Disconnected) {
			//pe_receive_packets();
		}

		switch (network_state) {
			case peClientNetworkState_Disconnected: {
				fprintf(stdout, "connecting to the server\n");
				network_state = peClientNetworkState_Connecting;
				last_packet_receive_time = pe_time_now();
			} break;
			case peClientNetworkState_Connecting: {
				uint64_t ticks_since_last_received_packet = pe_time_since(last_packet_receive_time);
				float seconds_since_last_received_packet = (float)pe_time_sec(ticks_since_last_received_packet);
				if (seconds_since_last_received_packet > (float)CONNECTION_REQUEST_TIME_OUT) {
					fprintf(stdout, "connection request timed out\n");
					network_state = peClientNetworkState_Error;
					break;
				}
				float connection_request_send_interval = 1.0f / (float)CONNECTION_REQUEST_SEND_RATE;
				uint64_t ticks_since_last_sent_packet = pe_time_since(last_packet_send_time);
				float seconds_since_last_sent_packet = (float)pe_time_sec(ticks_since_last_sent_packet);
				if (seconds_since_last_sent_packet > connection_request_send_interval) {
					peMessage message = pe_message_create(pe_heap_allocator(), peMessageType_ConnectionRequest);
					pe_append_message(&outgoing_packet, message);
				}
			} break;
			case peClientNetworkState_Connected: {
				peMessage message = pe_message_create(pe_heap_allocator(), peMessageType_InputState);
				message.input_state->input.movement.X = pe_input_axis(peGamepadAxis_LeftX);
				message.input_state->input.movement.Y = pe_input_axis(peGamepadAxis_LeftY);
				pe_append_message(&outgoing_packet, message);
			} break;
			default: break;
		}

		if (outgoing_packet.message_count > 0) {
			//pe_send_packet(socket, server_address, &outgoing_packet);
			last_packet_send_time = pe_time_now();
			for (int m = 0; m < outgoing_packet.message_count; m += 1) {
				pe_message_destroy(pe_heap_allocator(), outgoing_packet.messages[m]);
			}
			outgoing_packet.message_count = 0;
		}

		pe_graphics_frame_begin();

		pe_clear_background((peColor){20, 20, 20, 255});

		pe_texture_bind(default_texture);

		current_frame_time += dt;

		static float rotate_y = 3.14f;
		rotate_y += 3.14 * dt * 0.256f;

		pe_model_draw(&model, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, rotate_y, 0.0f));

		if (current_frame_time > frame_time) {
			current_frame_time = 0.0f;
			frame = (frame + 1) % model.animation[0].num_frames;
		}

		for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
			peEntity *entity = &entities[e];
			if (!entity->active) continue;
		}

		pe_graphics_frame_end(true);

		pe_free_all(pe_temp_allocator());
	}

	sceGuTerm();

	sceKernelExitGame();
	return 0;
}