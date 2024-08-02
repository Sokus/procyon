#ifndef PE_GRAPHICS_WIN32_H_HEADER_GUARD
#define PE_GRAPHICS_WIN32_H_HEADER_GUARD

#include "math/p_math.h"

#include <wchar.h>
#include <stdbool.h>

// forward declare all the D3D gar... stuff
typedef struct ID3D11Device ID3D11Device;
typedef struct ID3D11DeviceContext ID3D11DeviceContext;
typedef struct IDXGISwapChain1 IDXGISwapChain1;
typedef struct ID3D11RenderTargetView ID3D11RenderTargetView;
typedef struct ID3D11DepthStencilView ID3D11DepthStencilView;
typedef struct ID3D11VertexShader ID3D11VertexShader;
typedef struct ID3D11PixelShader ID3D11PixelShader;
typedef struct ID3D10Blob ID3D10Blob;
typedef struct ID3D11InputLayout ID3D11InputLayout;
typedef struct ID3D11RasterizerState ID3D11RasterizerState;
typedef struct ID3D11SamplerState ID3D11SamplerState;
typedef struct ID3D11DepthStencilState ID3D11DepthStencilState;
typedef struct ID3D11BlendState ID3D11BlendState;
typedef struct ID3D11Buffer ID3D11Buffer;
typedef enum D3D11_USAGE D3D11_USAGE;
typedef enum D3D11_CPU_ACCESS_FLAG D3D11_CPU_ACCESS_FLAG;
typedef unsigned int UINT;

typedef struct peDirect3D {
    int framebuffer_width;
    int framebuffer_height;

    ID3D11Device *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain1 *swapchain;
    ID3D11RenderTargetView *render_target_view;
    ID3D11DepthStencilView *depth_stencil_view;

    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader *pixel_shader;
    ID3D10Blob *vs_blob, *ps_blob;

    ID3D11InputLayout *input_layout;

    ID3D11RasterizerState *rasterizer_state;
    ID3D11SamplerState *sampler_state;
    ID3D11DepthStencilState *depth_stencil_state_enabled;
    ID3D11DepthStencilState *depth_stencil_state_disabled;

    ID3D11BlendState *blend_state;
} peDirect3D;
extern peDirect3D pe_d3d;

typedef struct peDynamicDrawStateD3D {
    ID3D11Buffer *vertex_buffer;
    ID3D11InputLayout *input_layout;
} peDynamicDrawStateD3D;
extern peDynamicDrawStateD3D dynamic_draw_d3d;

typedef struct peShaderConstantBuffersD3D {
    ID3D11Buffer *projection;
    ID3D11Buffer *view;
    ID3D11Buffer *model;
    ID3D11Buffer *light;
    ID3D11Buffer *material;
    ID3D11Buffer *skeleton;
} peShaderConstantBuffersD3D;
extern peShaderConstantBuffersD3D constant_buffers_d3d;

typedef __declspec(align(16)) struct peShaderConstant_Matrix {
    pMat4 value;
} peShaderConstant_Matrix;

typedef __declspec(align(16)) struct peShaderConstant_Light {
    bool do_lighting;
    pVec3 light_vector;
} peShaderConstant_Light;

typedef __declspec(align(16)) struct peShaderConstant_Material {
    pVec4 diffuse_color;
} peShaderConstant_Material;

typedef __declspec(align(16)) struct peShaderConstant_Skeleton {
    bool has_skeleton;
    pMat4 matrix_bone[256];
} peShaderConstant_Skeleton;

void pe_create_swapchain_resources(void);
void d3d11_set_viewport(int width, int height);

bool pe_vertex_shader_create(ID3D11Device *device, wchar_t *wchar_file_name, ID3D11VertexShader **vertex_shader, ID3D10Blob **vertex_shader_blob);
bool pe_pixel_shader_create(ID3D11Device *device, wchar_t *wchar_file_name, ID3D11PixelShader **pixel_shader, ID3D10Blob **pixel_shader_blob);
void pe_shader_constant_buffer_init(ID3D11Device *device, size_t size, ID3D11Buffer **buffer);
void *pe_shader_constant_begin_map(ID3D11DeviceContext *context, ID3D11Buffer *buffer);
void pe_shader_constant_end_map(ID3D11DeviceContext *context, ID3D11Buffer *buffer);

ID3D11Buffer *pe_d3d11_create_buffer(void *data, UINT byte_width, D3D11_USAGE usage, UINT bind_flags, D3D11_CPU_ACCESS_FLAG cpu_access_flags);

void pe_framebuffer_size_callback_win32(int width, int height);

typedef struct peTexture peTexture;
peTexture pe_texture_create(void *data, UINT width, UINT height, int channels);
void pe_texture_bind(peTexture texture);
void pe_texture_bind_default(void);

#endif // PE_GRAPHICS_WIN32_H_HEADER_GUARD