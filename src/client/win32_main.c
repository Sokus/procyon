

#include "pe_core.h"
#include "pe_graphics.h"
#include "pe_time.h"
#include "pe_file_io.h"
#include "pe_platform.h"
#include "win32/win32_shader.h"
#include "win32/win32_d3d.h"
#include "pe_window_glfw.h"
#include "pe_model.h"
#include "p3d.h"
#include "pe_temp_arena.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "HandmadeMath.h"

#include <stdbool.h>
#include <wchar.h>
#include <stdio.h>
#include <math.h>

//
// NET CLIENT STUFF
//

#include <float.h>

#include "pe_net.h"
#include "pe_protocol.h"
#include "pe_config.h"
#include "game/pe_entity.h"

#define CONNECTION_REQUEST_TIME_OUT 20.0f // seconds
#define CONNECTION_REQUEST_SEND_RATE 1.0f // # per second

#define SECONDS_TO_TIME_OUT 10.0f

typedef enum peClientNetworkState {
	peClientNetworkState_Disconnected,
	peClientNetworkState_Connecting,
	peClientNetworkState_Connected,
	peClientNetworkState_Error,
} peClientNetworkState;

static peSocket client_socket;
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
	while (pe_receive_packet(client_socket, allocator, &address, &packet)) {
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
						network_state = peClientNetworkState_Error;
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

//
// MATH
//

static HMM_Vec3 pe_vec3_unproject(
    HMM_Vec3 v,
    float viewport_x, float viewport_y,
    float viewport_width, float viewport_height,
    float viewport_min_z, float viewport_max_z,
    HMM_Mat4 projection,
    HMM_Mat4 view
) {
    // https://github.com/microsoft/DirectXMath/blob/bec07458c994bd7553638e4d499e17cfedd07831/Extensions/DirectXMathFMA4.h#L229
    HMM_Vec4 d = HMM_V4(-1.0f, 1.0f, 0.0f, 0.0f);

    HMM_Vec4 scale = HMM_V4(viewport_width * 0.5f, -viewport_height * 0.5f, viewport_max_z - viewport_min_z, 1.0f);
    scale = HMM_V4(1.0f/scale.X, 1.0f/scale.Y, 1.0f/scale.Z, 1.0f/scale.W);

    HMM_Vec4 offset = HMM_V4(-viewport_x, -viewport_y, -viewport_min_z, 0.0f);
    offset = HMM_AddV4(HMM_MulV4(scale, offset), d);

    HMM_Mat4 transform = HMM_MulM4(projection, view);
    transform = HMM_InvGeneralM4(transform);

    HMM_Vec4 result = HMM_AddV4(HMM_MulV4(HMM_V4(v.X, v.Y, v.Z, 1.0f), scale), offset);
    result = HMM_MulM4V4(transform, result);

    if (result.W < FLT_EPSILON) {
        result.W = FLT_EPSILON;
    }

    result = HMM_DivV4F(result, result.W);

    return result.XYZ;
}

HMM_Mat4 pe_mat4_perspective(float w, float h, float n, float f) {
    // https://github.com/microsoft/DirectXMath/blob/bec07458c994bd7553638e4d499e17cfedd07831/Inc/DirectXMathMatrix.inl#L2350
    HMM_Mat4 result = {
		2 * n / w,  0,         0,           0,
		0,          2 * n / h, 0,           0,
		0,          0,          f / (n - f), n * f / (n - f),
		0,          0,         -1,           0,
    };
    return result;
}

typedef struct peCamera {
    HMM_Vec3 position;
    HMM_Vec3 target;
    HMM_Vec3 up;
    float fovy;
} peCamera;

static void pe_camera_update(peCamera camera) {
    peShaderConstant_Projection *projection = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_projection_buffer);
    projection->matrix = pe_mat4_perspective((float)pe_screen_width()/(float)pe_screen_height(), 1.0f, 1.0f, 9.0f);
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_projection_buffer);

    peShaderConstant_View *constant_view = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_view_buffer);
    constant_view->matrix = HMM_LookAt_RH(camera.position, camera.target, camera.up);
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_view_buffer);
}

typedef struct peRay {
    HMM_Vec3 position;
    HMM_Vec3 direction;
} peRay;

static peRay pe_get_mouse_ray(HMM_Vec2 mouse, peCamera camera) {
    float window_width_float = (float)pe_screen_width();
    float window_height_float = (float)pe_screen_height();
    HMM_Mat4 projection = pe_mat4_perspective(window_width_float/window_height_float, 1.0f, 1.0f, 9.0f);
    HMM_Mat4 view = HMM_LookAt_RH(camera.position, camera.target, camera.up);

    HMM_Vec3 near_point = pe_vec3_unproject(HMM_V3(mouse.X, mouse.Y, 0.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    HMM_Vec3 far_point = pe_vec3_unproject(HMM_V3(mouse.X, mouse.Y, 1.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    HMM_Vec3 direction = HMM_NormV3(HMM_SubV3(far_point, near_point));
    peRay ray = {
        .position = camera.position,
        .direction = direction,
    };
    return ray;
}

static bool pe_collision_ray_plane(peRay ray, HMM_Vec3 plane_normal, float plane_d, HMM_Vec3 *collision_point) {
    // https://www.cs.princeton.edu/courses/archive/fall00/cs426/lectures/raycast/sld017.htm

    float dot_product = HMM_DotV3(ray.direction, plane_normal);
    if (dot_product == 0.0f) {
        return false;
    }

    float t = -(HMM_DotV3(ray.position, plane_normal) + plane_d) / dot_product;
    if (t < 0.0f) {
        return false;
    }

    *collision_point = HMM_AddV3(ray.position, HMM_MulV3F(ray.direction, t));
    return true;
}

int frame = 0;

ID3D11ShaderResourceView *pe_create_grid_texture(void) {
    uint32_t texture_data[2*2] = {
        0xFFFFFFFF, 0xFF7F7F7F,
        0xFF7F7F7F, 0xFFFFFFFF,
    };
    return pe_texture_upload(texture_data, 2, 2, 4);
}

int main(int argc, char *argv[]) {
    pe_temp_allocator_init(PE_MEGABYTES(4));
    pe_time_init();
    pe_net_init();
    pe_platform_init();

    pe_graphics_init(960, 540, "Procyon");

    pe_allocate_entities();

    ///////////////////

	peSocketCreateError socket_create_error = pe_socket_create(peSocket_IPv4, 0, &client_socket);
	fprintf(stdout, "socket create result: %d\n", socket_create_error);

	server_address = pe_address4(127, 0, 0, 1, SERVER_PORT);

    ///////////////////

    peModel model = pe_model_load("./res/fox.p3d");

    printf("model loaded\n");

    //peMesh mesh = pe_gen_mesh_cube(1.0f, 1.0f, 1.0f);
    //peMesh quad = pe_gen_mesh_quad(1.0f, 1.0f);

    peShaderConstant_Light *constant_light = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_light_buffer);
    constant_light->vector = (HMM_Vec3){ 1.0f, -1.0f, 1.0f };
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_light_buffer);

    peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_material_buffer);
    constant_material->has_diffuse = true;
    constant_material->diffuse_color = HMM_V4(0.0f, 0.0f, 255.0f, 255.0f);
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_material_buffer);

    HMM_Vec3 camera_offset = { 0.0f, 1.4f, 2.0f };
    peCamera camera = {
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f,
    };
    camera.position = HMM_AddV3(camera.target, camera_offset);

    float look_angle = 0.0f;

    float frame_time = 1.0f/30.0f;
    float current_frame_time = 0.0f;

    while (!pe_platform_should_quit()) {
        pe_platform_poll_events();

		if (network_state != peClientNetworkState_Disconnected) {
			pe_receive_packets();
		}

        for (int i = 0; i < MAX_ENTITY_COUNT; i += 1) {
            peEntity *entity = &entities[i];
            if (!entity->active) continue;

            if (pe_entity_property_get(entity, peEntityProperty_OwnedByPlayer) && entity->client_index == client_index) {
                camera.target = HMM_AddV3(entity->position, HMM_V3(0.0f, 0.7f, 0.0f));
                camera.position = HMM_AddV3(camera.target, camera_offset);
            }
        };

        pe_camera_update(camera);

        {
            double xpos, ypos;
            glfwGetCursorPos(pe_glfw.window, &xpos, &ypos);
            peRay ray = pe_get_mouse_ray((HMM_Vec2){(float)xpos, (float)ypos}, camera);

            HMM_Vec3 collision_point;
            if (pe_collision_ray_plane(ray, (HMM_Vec3){.Y = 1.0f}, 0.0f, &collision_point)) {
                HMM_Vec3 relative_collision_point = HMM_SubV3(camera.target, collision_point);
                look_angle = atan2f(relative_collision_point.X, relative_collision_point.Z);
            }
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
					//fprintf(stdout, "connection request timed out");
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
				//message.input_state->input.movement.X = pe_input_axis(peGamepadAxis_LeftX);
				//message.input_state->input.movement.Y = pe_input_axis(peGamepadAxis_LeftY);
                bool key_d = glfwGetKey(pe_glfw.window, GLFW_KEY_D);
                bool key_a = glfwGetKey(pe_glfw.window, GLFW_KEY_A);
                bool key_w = glfwGetKey(pe_glfw.window, GLFW_KEY_W);
                bool key_s = glfwGetKey(pe_glfw.window, GLFW_KEY_S);
                message.input_state->input.movement.X = (float)key_d - (float)key_a;
                message.input_state->input.movement.Y = (float)key_s - (float)key_w;
                message.input_state->input.angle = look_angle;
				pe_append_message(&outgoing_packet, message);
			} break;
			default: break;
		}

		if (outgoing_packet.message_count > 0) {
			pe_send_packet(client_socket, server_address, &outgoing_packet);
			last_packet_send_time = pe_time_now();
			for (int m = 0; m < outgoing_packet.message_count; m += 1) {
				pe_message_destroy(pe_heap_allocator(), outgoing_packet.messages[m]);
			}
			outgoing_packet.message_count = 0;
		}

        pe_graphics_frame_begin();
        pe_clear_background((peColor){ 20, 20, 20, 255 });

        current_frame_time += 1.0f/60.0f;

		for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
			peEntity *entity = &entities[e];
			if (!entity->active) continue;

            if (entity->mesh == peEntityMesh_Cube) {
                pe_model_draw(&model, entity->position, (HMM_Vec3){ .Y = entity->angle });
            } else if (entity->mesh == peEntityMesh_Quad) {
                //pe_draw_mesh(&quad, entity->position, (HMM_Vec3){ .Y = entity->angle });
            }
		}

        if (current_frame_time > frame_time) {
            current_frame_time = 0.0f;
            frame = (frame + 1) % model.animation[0].num_frames;
        }

        pe_graphics_frame_end(true);

        pe_free_all(pe_temp_arena());
    }

    pe_glfw_shutdown();

    return 0;
}