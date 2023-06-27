#include "pe_time.h"
#include "pe_core.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>

#pragma comment (lib, "gdi32")
#pragma comment (lib, "user32")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

#include "HandmadeMath.h"

void *pe_m3d_malloc(size_t size);
void  pe_m3d_free(void *ptr);
void *pe_m3d_realloc(void *ptr, size_t new_size);

#include <stdbool.h>
#include <wchar.h>
#include <stdio.h>
#include <math.h>

#include "win32/win32_shader.h"

#include "p3d.h"

struct peDirectXState {
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain1 *swapchain;
    ID3D11RenderTargetView *render_target_view;
    ID3D11DepthStencilView *depth_stencil_view;

    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3DBlob *vs_blob, *ps_blob;
    ID3D11InputLayout *input_layout;

    ID3D11RasterizerState *rasterizer_state;
    ID3D11SamplerState *sampler_state;
    ID3D11DepthStencilState *depth_stencil_state;

    ID3D11ShaderResourceView *default_texture_view;
} directx_state = {0};

int window_width = 960;
int window_height = 540;

ID3D11Buffer *pe_shader_constant_projection_buffer;
ID3D11Buffer *pe_shader_constant_view_buffer;
ID3D11Buffer *pe_shader_constant_model_buffer;
ID3D11Buffer *pe_shader_constant_light_buffer;
ID3D11Buffer *pe_shader_constant_material_buffer;

static peArena temp_arena;

//
// TEXTURE
//

ID3D11ShaderResourceView *pe_texture_upload(void *data, unsigned width, unsigned height, int format) {
    DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;
    int bytes_per_pixel = 0;
    switch (format) {
        case 1:
            dxgi_format = DXGI_FORMAT_R8_UNORM;
            bytes_per_pixel = 1;
            break;
        case 4:
            dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            bytes_per_pixel = 4;
            break;
        default: PE_PANIC_MSG("Unsupported message format: %d\n", format); break;
    }
    D3D11_TEXTURE2D_DESC texture_desc = {
        .Width = width,
        .Height = height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = dxgi_format,
        .SampleDesc = {
            .Count = 1,
        },
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
    };
    D3D11_SUBRESOURCE_DATA subresource_data = {
        .pSysMem = data,
        .SysMemPitch = bytes_per_pixel * width,
    };
    HRESULT hr;
    ID3D11Texture2D *texture;
    hr = ID3D11Device_CreateTexture2D(directx_state.device, &texture_desc, &subresource_data, &texture);
    ID3D11ShaderResourceView *texture_view;
    hr = ID3D11Device_CreateShaderResourceView(directx_state.device, (ID3D11Resource*)texture, NULL, &texture_view);
    ID3D11Texture2D_Release(texture);
    return texture_view;
}

ID3D11ShaderResourceView *pe_create_default_texture(void) {
    uint32_t texture_data[1] = { 0xFFFFFFFF };
    return pe_texture_upload(texture_data, 1, 1, 4);
};

ID3D11ShaderResourceView *pe_create_grid_texture(void) {
    uint32_t texture_data[2*2] = {
        0xFFFFFFFF, 0xFF7F7F7F,
        0xFF7F7F7F, 0xFFFFFFFF,
    };
    return pe_texture_upload(texture_data, 2, 2, 4);
}

void pe_bind_texture(ID3D11ShaderResourceView *texture) {
    ID3D11DeviceContext_PSSetShaderResources(directx_state.context, 0, 1, &texture);
}

//
// MESH
//

typedef union peColor {
    struct { uint8_t r, g, b, a; };
    struct { uint32_t rgba; };
} peColor;

#define PE_COLOR_WHITE (peColor){ 255, 255, 255, 255 }

HMM_Vec4 pe_color_to_vec4(peColor color) {
    HMM_Vec4 vec4 = {
        .R = (float)color.r / 255.0f,
        .G = (float)color.g / 255.0f,
        .B = (float)color.b / 255.0f,
        .A = (float)color.a / 255.0f,
    };
    return vec4;
}

peColor pe_color_uint32(uint32_t color_uint32) {
    peColor color = {
        .r = (uint8_t)((color_uint32 >>  0) & 0xFF),
        .g = (uint8_t)((color_uint32 >>  8) & 0xFF),
        .b = (uint8_t)((color_uint32 >> 16) & 0xFF),
        .a = (uint8_t)((color_uint32 >> 24) & 0xFF),
    };
    return color;
}


typedef struct peMaterial {
    peColor diffuse_color;
    ID3D11ShaderResourceView *diffuse_map;
} peMaterial;

peMaterial pe_default_material(void) {
    peMaterial material = {
        .diffuse_color = PE_COLOR_WHITE,
        .diffuse_map = directx_state.default_texture_view,
    };
    return material;
}

typedef struct peMesh {
    int num_vertex;
    int num_index;

    ID3D11Buffer *pos_buffer;
    ID3D11Buffer *norm_buffer;
    ID3D11Buffer *tex_buffer;
    ID3D11Buffer *color_buffer;
    ID3D11Buffer *index_buffer;
} peMesh;

ID3D11Buffer *pe_d3d11_create_buffer(void *data, UINT byte_width, D3D11_USAGE usage, UINT bind_flags) {
    D3D11_BUFFER_DESC buffer_desc = {
        .ByteWidth = byte_width,
        .Usage = usage,
        .BindFlags = bind_flags,
    };
    D3D11_SUBRESOURCE_DATA subresource_data = {
        .pSysMem = data,
        .SysMemPitch = byte_width,
    };
    ID3D11Buffer *buffer = NULL;
    HRESULT hr = ID3D11Device_CreateBuffer(directx_state.device, &buffer_desc, &subresource_data, &buffer);
    return buffer;
}

peMesh pe_gen_mesh_cube(float width, float height, float length) {
    float pos[] = {
		-width/2.0f, -height/2.0f,  length/2.0f,
		 width/2.0f, -height/2.0f,  length/2.0f,
		 width/2.0f,  height/2.0f,  length/2.0f,
		-width/2.0f,  height/2.0f,  length/2.0f,
		-width/2.0f, -height/2.0f, -length/2.0f,
		-width/2.0f,  height/2.0f, -length/2.0f,
		 width/2.0f,  height/2.0f, -length/2.0f,
		 width/2.0f, -height/2.0f, -length/2.0f,
		-width/2.0f,  height/2.0f, -length/2.0f,
		-width/2.0f,  height/2.0f,  length/2.0f,
		 width/2.0f,  height/2.0f,  length/2.0f,
		 width/2.0f,  height/2.0f, -length/2.0f,
		-width/2.0f, -height/2.0f, -length/2.0f,
		 width/2.0f, -height/2.0f, -length/2.0f,
		 width/2.0f, -height/2.0f,  length/2.0f,
		-width/2.0f, -height/2.0f,  length/2.0f,
		 width/2.0f, -height/2.0f, -length/2.0f,
		 width/2.0f,  height/2.0f, -length/2.0f,
		 width/2.0f,  height/2.0f,  length/2.0f,
		 width/2.0f, -height/2.0f,  length/2.0f,
		-width/2.0f, -height/2.0f, -length/2.0f,
		-width/2.0f, -height/2.0f,  length/2.0f,
		-width/2.0f,  height/2.0f,  length/2.0f,
		-width/2.0f,  height/2.0f, -length/2.0f,
    };

    float norm[] = {
		 0.0, 0.0, 1.0,  0.0, 0.0, 1.0,  0.0, 0.0, 1.0,  0.0, 0.0, 1.0,
		 0.0, 0.0,-1.0,  0.0, 0.0,-1.0,  0.0, 0.0,-1.0,  0.0, 0.0,-1.0,
		 0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,
		 0.0,-1.0, 0.0,  0.0,-1.0, 0.0,  0.0,-1.0, 0.0,  0.0,-1.0, 0.0,
		 1.0, 0.0, 0.0,  1.0, 0.0, 0.0,  1.0, 0.0, 0.0,  1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0,
    };

    float tex[] = {
		0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0,
		1.0, 0.0,  1.0, 1.0,  0.0, 1.0,  0.0, 0.0,
		0.0, 1.0,  0.0, 0.0,  1.0, 0.0,  1.0, 1.0,
		1.0, 1.0,  0.0, 1.0,  0.0, 0.0,  1.0, 0.0,
		1.0, 0.0,  1.0, 1.0,  0.0, 1.0,  0.0, 0.0,
		0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0,
    };

    uint32_t color[24];
    for (int i = 0; i < PE_COUNT_OF(color); i += 1) {
        color[i] = 0xffffffff;
    }

    uint32_t index[] = {
		0,  1,  2,  0,  2,  3,
		4,  5,  6,  4,  6,  7,
		8,  9, 10,  8, 10, 11,
	   12, 13, 14, 12, 14, 15,
	   16, 17, 18, 16, 18, 19,
	   20, 21, 22, 20, 22, 23,
    };

    peMesh mesh = {0};
    int num_vertex = PE_COUNT_OF(pos)/3;
    int num_index = PE_COUNT_OF(index);
    mesh.pos_buffer = pe_d3d11_create_buffer(pos, 3*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.norm_buffer = pe_d3d11_create_buffer(norm, 3*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.tex_buffer = pe_d3d11_create_buffer(tex, 2*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.color_buffer = pe_d3d11_create_buffer(color, num_vertex*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.index_buffer = pe_d3d11_create_buffer(index, num_index*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER);
    mesh.num_vertex = num_vertex;
    mesh.num_index = num_index;
    return mesh;
}

peMesh pe_gen_mesh_quad(float width, float length) {
	float pos[] = {
		-width/2.0f, 0.0f, -length/2.0f,
		-width/2.0f, 0.0f,  length/2.0f,
		 width/2.0f, 0.0f, -length/2.0f,
		 width/2.0f, 0.0f,  length/2.0f,
	};


    float norm[] = {
		0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,
    };

	float tex[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};

    uint32_t color[6];
    for (int i = 0; i < PE_COUNT_OF(color); i += 1) {
        color[i] = 0xffffffff;
    }

	uint32_t index[] = {
		0, 1, 2,
		1, 3, 2
	};

    peMesh mesh = {0};
    int num_vertex = PE_COUNT_OF(pos)/3;
    int num_index = PE_COUNT_OF(index);
    mesh.pos_buffer = pe_d3d11_create_buffer(pos, 3*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.norm_buffer = pe_d3d11_create_buffer(norm, 3*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.tex_buffer = pe_d3d11_create_buffer(tex, 2*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.color_buffer = pe_d3d11_create_buffer(color, num_vertex*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.index_buffer = pe_d3d11_create_buffer(index, num_index*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER);
    mesh.num_vertex = num_vertex;
    mesh.num_index = num_index;
    return mesh;
}

void pe_draw_mesh(peMesh *mesh, peMaterial material, HMM_Vec3 position, HMM_Vec3 rotation) {
    HMM_Mat4 rotate_x = HMM_Rotate_RH(rotation.X, (HMM_Vec3){1.0f, 0.0f, 0.0f});
    HMM_Mat4 rotate_y = HMM_Rotate_RH(rotation.Y, (HMM_Vec3){0.0f, 1.0f, 0.0f});
    HMM_Mat4 rotate_z = HMM_Rotate_RH(rotation.Z, (HMM_Vec3){0.0f, 0.0f, 1.0f});
    HMM_Mat4 translate = HMM_Translate(position);

    HMM_Mat4 model_matrix = HMM_MulM4(HMM_MulM4(HMM_MulM4(translate, rotate_z), rotate_y), rotate_x);
    peShaderConstant_Model *constant_model = pe_shader_constant_begin_map(directx_state.context, pe_shader_constant_model_buffer);
    constant_model->matrix = model_matrix;
    pe_shader_constant_end_map(directx_state.context, pe_shader_constant_model_buffer);

    peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(directx_state.context, pe_shader_constant_material_buffer);
    //constant_material->has_diffuse = material.has_diffuse;
    constant_material->diffuse_color = pe_color_to_vec4(material.diffuse_color);
    pe_shader_constant_end_map(directx_state.context, pe_shader_constant_material_buffer);

    pe_bind_texture(material.diffuse_map);

    ID3D11Buffer *vertex_buffers[] = {
        mesh->pos_buffer,
        mesh->norm_buffer,
        mesh->tex_buffer,
        mesh->color_buffer,
    };
    uint32_t vertex_buffer_strides[] = {
        3 * sizeof(float), // positions
        3 * sizeof(float), // normals
        2 * sizeof(float), // texcoords
        sizeof(uint32_t),
    };
    uint32_t vertex_buffer_offsets[] = { 0, 0, 0, 0 };

    ID3D11DeviceContext_IASetPrimitiveTopology(directx_state.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetInputLayout(directx_state.context, directx_state.input_layout);
    ID3D11DeviceContext_IASetVertexBuffers(directx_state.context, 0, PE_COUNT_OF(vertex_buffers),
        vertex_buffers, vertex_buffer_strides, vertex_buffer_offsets);
    ID3D11DeviceContext_IASetIndexBuffer(directx_state.context, mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

    ID3D11DeviceContext_DrawIndexed(directx_state.context, mesh->num_index, 0, 0);
}

//
//
//

void pe_create_swapchain_resources(void) {
    HRESULT hr;

    ID3D11Texture2D *render_target;
    hr = IDXGISwapChain1_GetBuffer(directx_state.swapchain, 0, &IID_ID3D11Texture2D, &render_target);
    hr = ID3D11Device_CreateRenderTargetView(directx_state.device, (ID3D11Resource*)render_target, NULL, &directx_state.render_target_view);

    D3D11_TEXTURE2D_DESC depth_buffer_desc;
    ID3D11Texture2D_GetDesc(render_target, &depth_buffer_desc);
    depth_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D *depth_buffer;
    hr = ID3D11Device_CreateTexture2D(directx_state.device, &depth_buffer_desc, NULL, &depth_buffer);
    hr = ID3D11Device_CreateDepthStencilView(directx_state.device, (ID3D11Resource*)depth_buffer, NULL, &directx_state.depth_stencil_view);

    ID3D11Texture2D_Release(depth_buffer);
    ID3D11Texture2D_Release(render_target);
}

void pe_destroy_swapchain_resources(void) {
    ID3D11DeviceContext_OMSetRenderTargets(directx_state.context, 0, NULL, NULL);

    PE_ASSERT(directx_state.render_target_view != NULL);
    ID3D11RenderTargetView_Release(directx_state.render_target_view);
    directx_state.render_target_view = NULL;

    PE_ASSERT(directx_state.depth_stencil_view != NULL);
    ID3D11DepthStencilView_Release(directx_state.depth_stencil_view);
    directx_state.render_target_view = NULL;
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

static void d3d11_set_viewport(int width, int height) {
    D3D11_VIEWPORT viewport = {
        .TopLeftX = 0.0f, .TopLeftY = 0.0f,
        .Width = (float)width, .Height = (float)height,
        .MinDepth = 0.0f, .MaxDepth = 1.0f
    };
    ID3D11DeviceContext_RSSetViewports(directx_state.context, 1, &viewport);
}

void glfw_framebuffer_size_proc(GLFWwindow *window, int width, int height) {
    ID3D11DeviceContext_Flush(directx_state.context);
    pe_destroy_swapchain_resources();
    HRESULT hr = IDXGISwapChain1_ResizeBuffers(directx_state.swapchain, 0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);
    pe_create_swapchain_resources();

    d3d11_set_viewport(width, height);

    window_width = width;
    window_height = height;
}

void pe_clear_background(peColor color) {
    float r = (float)color.r / 255.0f;
    float g = (float)color.g / 255.0f;
    float b = (float)color.b / 255.0f;

    // FIXME: render a sprite to get exact background color
    FLOAT clear_color[4] = { powf(r, 2.0f), powf(g, 2.0f), powf(b, 2.0f), 1.0f };
    ID3D11DeviceContext_ClearRenderTargetView(directx_state.context, directx_state.render_target_view, clear_color);
    ID3D11DeviceContext_ClearDepthStencilView(directx_state.context, directx_state.depth_stencil_view, D3D11_CLEAR_DEPTH, 1, 0);
}

// I/O

typedef struct peFileContents {
    peAllocator allocator;
    void *data;
    size_t size;
} peFileContents;

peFileContents pe_file_read_contents(peAllocator allocator, char *file_path, bool zero_terminate) {
    peFileContents result = {0};
    result.allocator = allocator;

    HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        return result;
    }

    DWORD file_size;
    if ((file_size = GetFileSize(file_handle, &file_size)) == INVALID_FILE_SIZE) {
        CloseHandle(file_handle);
        return result;
    }

    size_t total_size = zero_terminate ? file_size + 1 : file_size;
    void *data = pe_alloc(allocator, total_size);
    if (data == NULL) {
        CloseHandle(file_handle);
        return result;
    }

    DWORD bytes_read = 0;
    if (!ReadFile(file_handle, data, file_size, &bytes_read, NULL)) {
        CloseHandle(file_handle);
        pe_free(allocator, data);
        return result;
    }
    PE_ASSERT(bytes_read == file_size);
    CloseHandle(file_handle);

    result.data = data;
    result.size = total_size;
    if (zero_terminate) {
        ((uint8_t*)data)[file_size] = 0;
    }

    return result;
}

void pe_file_free_contents(peFileContents contents) {
    pe_free(contents.allocator, contents.data);
}

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

typedef struct peCamera {
    HMM_Vec3 position;
    HMM_Vec3 target;
    HMM_Vec3 up;
    float fovy;
} peCamera;

static void pe_camera_update(peCamera camera) {
    peShaderConstant_Projection *projection = pe_shader_constant_begin_map(directx_state.context, pe_shader_constant_projection_buffer);
    projection->matrix = pe_mat4_perspective((float)window_width/(float)window_height, 1.0f, 1.0f, 9.0f);
    pe_shader_constant_end_map(directx_state.context, pe_shader_constant_projection_buffer);

    peShaderConstant_View *constant_view = pe_shader_constant_begin_map(directx_state.context, pe_shader_constant_view_buffer);
    constant_view->matrix = HMM_LookAt_RH(camera.position, camera.target, camera.up);
    pe_shader_constant_end_map(directx_state.context, pe_shader_constant_view_buffer);
}

typedef struct peRay {
    HMM_Vec3 position;
    HMM_Vec3 direction;
} peRay;

static peRay pe_get_mouse_ray(HMM_Vec2 mouse, peCamera camera) {
    float window_width_float = (float)window_width;
    float window_height_float = (float)window_height;
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

//
// MODELS
//

typedef struct peModel {
    peArena arena;

    int num_vertex;
    int num_mesh;
    int *num_index;
    int *index_offset;
    int *vertex_offset;

    peMaterial *material;

    ID3D11Buffer *pos_buffer;
    ID3D11Buffer *norm_buffer;
    ID3D11Buffer *tex_buffer;
    ID3D11Buffer *color_buffer;
    ID3D11Buffer *index_buffer;
} peModel;

peModel pe_model_load(char *file_path) {
    peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);
    peAllocator temp_allocator = pe_arena_allocator(&temp_arena);

    peFileContents p3d_file_contents = pe_file_read_contents(temp_allocator, file_path, false);
    uint8_t *p3d_file_pointer = p3d_file_contents.data;

    p3dStaticInfo *p3d_static_info = (void*)p3d_file_pointer;
    p3d_file_pointer += sizeof(p3dStaticInfo);

    int16_t *p3d_position = (int16_t*)p3d_file_pointer;
    p3d_file_pointer += 3 * p3d_static_info->num_vertex * sizeof(int16_t);

    int16_t *p3d_normal = (int16_t*)p3d_file_pointer;
    p3d_file_pointer += 3 * p3d_static_info->num_vertex * sizeof(int16_t);

    uint16_t *p3d_texcoord = (uint16_t*)p3d_file_pointer;
    p3d_file_pointer += 2 * p3d_static_info->num_vertex * sizeof(uint16_t);

    uint32_t *p3d_index = (uint32_t*)p3d_file_pointer;
    p3d_file_pointer += p3d_static_info->num_index * sizeof(uint32_t);

    p3dMesh *p3d_mesh = (p3dMesh*)p3d_file_pointer;
    p3d_file_pointer += p3d_static_info->num_meshes * sizeof(p3dMesh);

    peModel model = {0};
    model.num_mesh = p3d_static_info->num_meshes;
    model.num_vertex = p3d_static_info->num_vertex;
    int num_index = p3d_static_info->num_index;

    size_t num_index_size = model.num_mesh*sizeof(int);
    size_t index_offset_size = model.num_mesh*sizeof(int);
    size_t vertex_offset_size = model.num_mesh*sizeof(int);
    size_t material_size = model.num_mesh*sizeof(peMaterial);

    size_t estimated_memory_size = num_index_size + index_offset_size + vertex_offset_size + material_size + 3*(PE_DEFAULT_MEMORY_ALIGNMENT-1);

    pe_arena_init_from_allocator(&model.arena, pe_heap_allocator(), estimated_memory_size);
    peAllocator model_allocator = pe_arena_allocator(&model.arena);

    model.num_index = pe_alloc(model_allocator, num_index_size);
    pe_zero_size(model.num_index, num_index_size);
    model.index_offset = pe_alloc(model_allocator, index_offset_size);
    model.vertex_offset = pe_alloc(model_allocator, vertex_offset_size);

    model.material = pe_alloc(model_allocator, material_size);
    for (int i = 0; i < model.num_mesh; i += 1) {
        model.material[i] = pe_default_material();
    }

    float *pos = pe_alloc(temp_allocator, 3*model.num_vertex*sizeof(float));
    float *norm = pe_alloc(temp_allocator, 3*model.num_vertex*sizeof(float));
    float *tex = pe_alloc(temp_allocator, 2*model.num_vertex*sizeof(float));
    uint32_t *color = pe_alloc(temp_allocator, model.num_vertex*sizeof(uint32_t));
    //uint32_t *index = pe_alloc(temp_allocator, p3d_static_info->num_index*sizeof(uint32_t));

    for (int p = 0; p < 3*model.num_vertex; p += 1) {
        pos[p] = (float)p3d_position[p] / (float)INT16_MAX * p3d_static_info->scale;
    }

    for (int n = 0; n < 3*model.num_vertex; n += 1) {
        norm[n] = (float)p3d_normal[n] / (float)INT16_MAX;
    }

    for (int t = 0; t < 2*model.num_vertex; t += 1) {
        tex[t] = (float)p3d_texcoord[t] / (float)UINT16_MAX;
    }

    for (int c = 0; c < model.num_vertex; c += 1) {
        color[c] = PE_COLOR_WHITE.rgba;
    }

    for (int m = 0; m < model.num_mesh; m += 1) {
        model.num_index[m] = p3d_mesh[m].num_index;
        model.index_offset[m] = p3d_mesh[m].index_offset;
        model.vertex_offset[m] = p3d_mesh[m].vertex_offset;
        model.material[m].diffuse_color.rgba = p3d_mesh[m].diffuse_color;

        /* TODO: diffuse textures
        if (p3d_mesh[m].has_diffuse_texture) {
            int diffuse_texture_x, diffuse_texture_y = 0;
            stbi_uc *diffuse_texture;
            {
                stbi_uc *buffer = p3d_binary_data + p3d_mesh[m].diffuse_map_data_offset;
                int buffer_size = p3d_mesh[m].diffuse_map_data_size;
                diffuse_texture = stbi_load_from_memory(buffer, buffer_size, &diffuse_texture_x, &diffuse_texture_y, NULL, 4);
                model.material[m].diffuse_map = pe_texture_upload(diffuse_texture, diffuse_texture_x, diffuse_texture_y, 4);
            }
        }
        */

    }

    model.pos_buffer = pe_d3d11_create_buffer(pos, 3*model.num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.norm_buffer = pe_d3d11_create_buffer(norm, 3*model.num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.tex_buffer = pe_d3d11_create_buffer(tex, 2*model.num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.color_buffer = pe_d3d11_create_buffer(color, model.num_vertex*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.index_buffer = pe_d3d11_create_buffer(p3d_index, num_index*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER);

    pe_temp_arena_memory_end(temp_arena_memory);

    return model;
}

static char *pe_m3d_current_path = NULL;
static peAllocator m3d_allocator = {0};

void *pe_m3d_malloc(size_t size) {
    return pe_alloc(m3d_allocator, size);
};

void pe_m3d_free(void *ptr) {
    pe_free(m3d_allocator, ptr);
}

unsigned char *pe_m3d_read_callback(char *filename, unsigned int *size) {
    char file_path[512] = "\0";
    int file_path_length = 0;
    if (pe_m3d_current_path != NULL) {
        int one_past_last_slash_offset = 0;
        for (int i = 0; i < PE_COUNT_OF(file_path)-1; i += 1) {
            if (pe_m3d_current_path[i] == '\0') break;
            if (pe_m3d_current_path[i] == '/') {
                one_past_last_slash_offset = i + 1;
            }
        }
        memcpy(file_path, pe_m3d_current_path, one_past_last_slash_offset);
        file_path_length += one_past_last_slash_offset;
    }
    strcat_s(file_path+file_path_length, sizeof(file_path)-file_path_length-1, filename);

    printf("M3D read callback: %s\n", file_path);
    peFileContents file_contents = pe_file_read_contents(m3d_allocator, file_path, false);
    *size = (unsigned int)file_contents.size;
    return file_contents.data;
}

void pe_draw_model(peModel *model, HMM_Vec3 position, HMM_Vec3 rotation) {
    HMM_Mat4 rotate_x = HMM_Rotate_RH(rotation.X, (HMM_Vec3){1.0f, 0.0f, 0.0f});
    HMM_Mat4 rotate_y = HMM_Rotate_RH(rotation.Y, (HMM_Vec3){0.0f, 1.0f, 0.0f});
    HMM_Mat4 rotate_z = HMM_Rotate_RH(rotation.Z, (HMM_Vec3){0.0f, 0.0f, 1.0f});
    HMM_Mat4 translate = HMM_Translate(position);

    HMM_Mat4 model_matrix = HMM_MulM4(HMM_MulM4(HMM_MulM4(translate, rotate_z), rotate_y), rotate_x);
    peShaderConstant_Model *constant_model = pe_shader_constant_begin_map(directx_state.context, pe_shader_constant_model_buffer);
    constant_model->matrix = model_matrix;
    pe_shader_constant_end_map(directx_state.context, pe_shader_constant_model_buffer);

    ID3D11Buffer *buffs[] = {
        model->pos_buffer,
        model->norm_buffer,
        model->tex_buffer,
        model->color_buffer,
    };
    uint32_t strides[] = {
        3*sizeof(float),
        3*sizeof(float),
        2*sizeof(float),
        sizeof(uint32_t),
    };
    uint32_t offsets[4] = {0, 0, 0, 0};

    ID3D11DeviceContext_IASetPrimitiveTopology(directx_state.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetInputLayout(directx_state.context, directx_state.input_layout);
    ID3D11DeviceContext_IASetVertexBuffers(directx_state.context, 0, 4, buffs, strides, offsets);
    ID3D11DeviceContext_IASetIndexBuffer(directx_state.context, model->index_buffer, DXGI_FORMAT_R32_UINT, 0);

    for (int m = 0; m < model->num_mesh; m += 1) {
        peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(directx_state.context, pe_shader_constant_material_buffer);
        //constant_material->has_diffuse = model->material[m].has_diffuse;
        constant_material->diffuse_color = pe_color_to_vec4(model->material[m].diffuse_color);
        pe_bind_texture(model->material[m].diffuse_map);
        pe_shader_constant_end_map(directx_state.context, pe_shader_constant_material_buffer);

        ID3D11DeviceContext_DrawIndexed(directx_state.context, model->num_index[m], model->index_offset[m], model->vertex_offset[m]);
    }
}


int main(int argc, char *argv[]) {
    pe_time_init();
    pe_net_init();

    pe_arena_init_from_allocator(&temp_arena, pe_heap_allocator(), PE_MEGABYTES(128));

    pe_allocate_entities();

    glfwInit();

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(window_width, window_height, "Procyon", NULL, NULL);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_proc);
    HWND hwnd = glfwGetWin32Window(window);

    HRESULT hr;

    // create D3D11 device & context
    {
        UINT flags = 0;
#ifndef NDEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, ARRAYSIZE(levels),
            D3D11_SDK_VERSION, &directx_state.device, NULL, &directx_state.context);
        // make sure device creation succeeeds before continuing
        // for simple applciation you could retry device creation with
        // D3D_DRIVER_TYPE_WARP driver type which enables software rendering
        // (could be useful on broken drivers or remote desktop situations)
        PE_ASSERT(SUCCEEDED(hr));
    }

#ifndef NDEBUG
    {
        ID3D11InfoQueue* info;
        hr = ID3D11Device_QueryInterface(directx_state.device, &IID_ID3D11InfoQueue, (void**)&info);
        hr = ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        hr = ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        ID3D11InfoQueue_Release(info);
    }
#endif

    // create DXGI swap chain
    {
        IDXGIDevice* dxgi_device;
        hr = ID3D11Device_QueryInterface(directx_state.device, &IID_IDXGIDevice, (void**)&dxgi_device);

        IDXGIAdapter* dxgi_adapter;
        hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);

        IDXGIFactory2* factory;
        hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, (void**)&factory);

        DXGI_SWAP_CHAIN_DESC1 desc =
        {
            .Width = 0,
            .Height = 0,
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            .Stereo = FALSE,
            .SampleDesc = {
                .Count = 1,
                .Quality = 0,
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .Scaling = DXGI_SCALING_STRETCH,
            .SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
            .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
            .Flags = 0,
        };

        hr = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)directx_state.device, hwnd, &desc, NULL, NULL, &directx_state.swapchain);

        pe_create_swapchain_resources();

        IDXGIFactory2_Release(factory);
        IDXGIAdapter_Release(dxgi_adapter);
        IDXGIDevice_Release(dxgi_device);
    }

    // Compile shaders
    {
        PE_ASSERT(pe_vertex_shader_create(directx_state.device, L"res/shader.hlsl", &directx_state.vertex_shader, &directx_state.vs_blob));

        D3D11_INPUT_ELEMENT_DESC input_element_descs[] = {
            { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,    2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COL", 0, DXGI_FORMAT_R8G8B8A8_UNORM , 3, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        HRESULT hr = ID3D11Device_CreateInputLayout(
            directx_state.device,
            input_element_descs,
            PE_COUNT_OF(input_element_descs),
            ID3D10Blob_GetBufferPointer(directx_state.vs_blob),
            ID3D10Blob_GetBufferSize(directx_state.vs_blob),
            &directx_state.input_layout
        );

        PE_ASSERT(pe_pixel_shader_create(directx_state.device, L"res/shader.hlsl", &directx_state.pixel_shader, &directx_state.ps_blob));
    }

    {
        HRESULT hr;
        D3D11_RASTERIZER_DESC rasterizer_desc = {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_BACK,
            .FrontCounterClockwise = TRUE,
        };
        hr = ID3D11Device_CreateRasterizerState(directx_state.device, &rasterizer_desc, &directx_state.rasterizer_state);

        D3D11_SAMPLER_DESC sampler_desc = {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
            .ComparisonFunc = D3D11_COMPARISON_NEVER
        };
        hr = ID3D11Device_CreateSamplerState(directx_state.device, &sampler_desc, &directx_state.sampler_state);

        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS
        };
        hr = ID3D11Device_CreateDepthStencilState(directx_state.device, &depth_stencil_desc, &directx_state.depth_stencil_state);
    }

    pe_shader_constant_buffer_init(directx_state.device, sizeof(peShaderConstant_Projection), &pe_shader_constant_projection_buffer);
    pe_shader_constant_buffer_init(directx_state.device, sizeof(peShaderConstant_View), &pe_shader_constant_view_buffer);
    pe_shader_constant_buffer_init(directx_state.device, sizeof(peShaderConstant_Model), &pe_shader_constant_model_buffer);
    pe_shader_constant_buffer_init(directx_state.device, sizeof(peShaderConstant_Light), &pe_shader_constant_light_buffer);
    pe_shader_constant_buffer_init(directx_state.device, sizeof(peShaderConstant_Material), &pe_shader_constant_material_buffer);

    d3d11_set_viewport(window_width, window_height);

    directx_state.default_texture_view = pe_create_default_texture();

    ///////////////////

	peSocketCreateError socket_create_error = pe_socket_create(peSocket_IPv4, 0, &client_socket);
	fprintf(stdout, "socket create result: %d\n", socket_create_error);

	server_address = pe_address4(127, 0, 0, 1, SERVER_PORT);

    ///////////////////

    peModel model = pe_model_load("./res/ybot.p3d");
    //peModel model = pe_m3d_model_load("./res/amy/amy.m3d");

    peMesh mesh = pe_gen_mesh_cube(1.0f, 1.0f, 1.0f);
    peMesh quad = pe_gen_mesh_quad(1.0f, 1.0f);

    peShaderConstant_Light *constant_light = pe_shader_constant_begin_map(directx_state.context, pe_shader_constant_light_buffer);
    constant_light->vector = (HMM_Vec3){ 1.0f, -1.0f, 1.0f };
    pe_shader_constant_end_map(directx_state.context, pe_shader_constant_light_buffer);

    peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(directx_state.context, pe_shader_constant_material_buffer);
    constant_material->has_diffuse = true;
    constant_material->diffuse_color = HMM_V4(0.0f, 0.0f, 255.0f, 255.0f);
    pe_shader_constant_end_map(directx_state.context, pe_shader_constant_material_buffer);

    HMM_Vec3 camera_offset = { 0.0f, 1.4f, 2.0f };
    peCamera camera = {
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 55.0f,
    };
    camera.position = HMM_AddV3(camera.target, camera_offset);

    float look_angle = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

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
            glfwGetCursorPos(window, &xpos, &ypos);
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
                bool key_d = glfwGetKey(window, GLFW_KEY_D);
                bool key_a = glfwGetKey(window, GLFW_KEY_A);
                bool key_w = glfwGetKey(window, GLFW_KEY_W);
                bool key_s = glfwGetKey(window, GLFW_KEY_S);
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
        pe_clear_background((peColor){ 20, 20, 20, 255 });

        ID3D11DeviceContext_VSSetShader(directx_state.context, directx_state.vertex_shader, NULL, 0);
        ID3D11DeviceContext_PSSetShader(directx_state.context, directx_state.pixel_shader, NULL, 0);

        ID3D11Buffer *constant_buffers[] = {
            pe_shader_constant_projection_buffer,
            pe_shader_constant_view_buffer,
            pe_shader_constant_model_buffer,
            pe_shader_constant_light_buffer,
            pe_shader_constant_material_buffer,
        };
        ID3D11DeviceContext_VSSetConstantBuffers(directx_state.context, 0, PE_COUNT_OF(constant_buffers), constant_buffers);

        ID3D11DeviceContext_RSSetState(directx_state.context, directx_state.rasterizer_state);

        pe_bind_texture(directx_state.default_texture_view);
        ID3D11DeviceContext_PSSetSamplers(directx_state.context, 0, 1, &directx_state.sampler_state);

        ID3D11DeviceContext_OMSetRenderTargets(directx_state.context, 1, &directx_state.render_target_view, directx_state.depth_stencil_view);
        ID3D11DeviceContext_OMSetDepthStencilState(directx_state.context, directx_state.depth_stencil_state, 0);
        ID3D11DeviceContext_OMSetBlendState(directx_state.context, NULL, NULL, ~(uint32_t)(0));

		for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
			peEntity *entity = &entities[e];
			if (!entity->active) continue;

            if (entity->mesh == peEntityMesh_Cube) {
                pe_draw_model(&model, entity->position, (HMM_Vec3){ .Y = entity->angle });
            } else if (entity->mesh == peEntityMesh_Quad) {
                //pe_draw_mesh(&quad, entity->position, (HMM_Vec3){ .Y = entity->angle });
            }
		}

        IDXGISwapChain1_Present(directx_state.swapchain, 1, 0);

        pe_free_all(pe_arena_allocator(&temp_arena));
    }


    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}