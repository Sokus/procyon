#ifndef PE_WIN32_D3D_H
#define PE_WIN32_D3D_H

#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_2.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "GLFW/glfw3.h"

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

ID3D11ShaderResourceView *pe_texture_upload(void *data, unsigned width, unsigned height, int format);
void pe_bind_texture(ID3D11ShaderResourceView *texture);

ID3D11Buffer *pe_d3d11_create_buffer(void *data, UINT byte_width, D3D11_USAGE usage, UINT bind_flags);

void pe_create_swapchain_resources(void);
void pe_destroy_swapchain_resources(void);

void d3d11_set_viewport(int width, int height);

void pe_d3d11_init(void);

#endif // PE_WIN32_D3D_H