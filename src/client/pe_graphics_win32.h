#ifndef PE_GRAPHICS_WIN32_H_HEADER_GUARD
#define PE_GRAPHICS_WIN32_H_HEADER_GUARD

#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_2.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "GLFW/glfw3.h"
#include "HandmadeMath.h"

#include <stddef.h>
#include <wchar.h>
#include <stdbool.h>

typedef struct peDirect3D {
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
} peDirect3D;
extern peDirect3D pe_d3d;

extern ID3D11Buffer *pe_shader_constant_projection_buffer;
extern ID3D11Buffer *pe_shader_constant_view_buffer;
extern ID3D11Buffer *pe_shader_constant_model_buffer;
extern ID3D11Buffer *pe_shader_constant_light_buffer;
extern ID3D11Buffer *pe_shader_constant_material_buffer;
extern ID3D11Buffer *pe_shader_constant_skeleton_buffer;

void pe_d3d11_init(void);

ID3D11Buffer *pe_d3d11_create_buffer(void *data, UINT byte_width, D3D11_USAGE usage, UINT bind_flags);

ID3D11ShaderResourceView *pe_texture_upload(void *data, unsigned width, unsigned height, int format);
void pe_bind_texture(ID3D11ShaderResourceView *texture);

//
// SHADERS
//

typedef __declspec(align(16)) struct peShaderConstant_Projection {
    HMM_Mat4 matrix;
} peShaderConstant_Projection;

typedef __declspec(align(16)) struct peShaderConstant_View {
    HMM_Mat4 matrix;
} peShaderConstant_View;

typedef __declspec(align(16)) struct peShaderConstant_Model {
    HMM_Mat4 matrix;
} peShaderConstant_Model;

typedef __declspec(align(16)) struct peShaderConstant_Light {
    HMM_Vec3 vector;
} peShaderConstant_Light;

typedef __declspec(align(16)) struct peShaderConstant_Material {
    bool has_diffuse;
    HMM_Vec4 diffuse_color;
} peShaderConstant_Material;

typedef __declspec(align(16)) struct peShaderConstant_Skeleton {
    bool has_skeleton;
    HMM_Mat4 matrix_bone[256];
} peShaderConstant_Skeleton;

void pe_shader_constant_buffer_init(ID3D11Device *device, size_t size, ID3D11Buffer **buffer);
void *pe_shader_constant_begin_map(ID3D11DeviceContext *context, ID3D11Buffer *buffer);
void pe_shader_constant_end_map(ID3D11DeviceContext *context, ID3D11Buffer *buffer);
bool pe_vertex_shader_create(ID3D11Device *device, wchar_t *wchar_file_name, ID3D11VertexShader **vertex_shader, ID3D10Blob **vertex_shader_blob);
bool pe_pixel_shader_create(ID3D11Device *device, wchar_t *wchar_file_name, ID3D11PixelShader **pixel_shader, ID3D10Blob **pixel_shader_blob);

#endif // PE_GRAPHICS_WIN32_H_HEADER_GUARD