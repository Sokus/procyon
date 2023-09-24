#include "pe_graphics.h"
#include "client/pe_graphics_win32.h"
#include "client/pe_window_glfw.h"

#include "pe_core.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <winerror.h>

#pragma comment (lib, "gdi32")
#pragma comment (lib, "user32")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include <stdio.h>

peDirect3D pe_d3d = {0};

// TODO: Wrap those in a struct or something...
ID3D11Buffer *pe_shader_constant_projection_buffer;
ID3D11Buffer *pe_shader_constant_view_buffer;
ID3D11Buffer *pe_shader_constant_model_buffer;
ID3D11Buffer *pe_shader_constant_light_buffer;
ID3D11Buffer *pe_shader_constant_material_buffer;
ID3D11Buffer *pe_shader_constant_skeleton_buffer;

static void pe_create_swapchain_resources(void) {
    HRESULT hr;

    ID3D11Texture2D *render_target;
    hr = IDXGISwapChain1_GetBuffer(pe_d3d.swapchain, 0, &IID_ID3D11Texture2D, &render_target);
    hr = ID3D11Device_CreateRenderTargetView(pe_d3d.device, (ID3D11Resource*)render_target, NULL, &pe_d3d.render_target_view);

    D3D11_TEXTURE2D_DESC depth_buffer_desc;
    ID3D11Texture2D_GetDesc(render_target, &depth_buffer_desc);
    depth_buffer_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D *depth_buffer;
    hr = ID3D11Device_CreateTexture2D(pe_d3d.device, &depth_buffer_desc, NULL, &depth_buffer);
    hr = ID3D11Device_CreateDepthStencilView(pe_d3d.device, (ID3D11Resource*)depth_buffer, NULL, &pe_d3d.depth_stencil_view);

    ID3D11Texture2D_Release(depth_buffer);
    ID3D11Texture2D_Release(render_target);

    ID3D11DeviceContext_OMSetRenderTargets(pe_d3d.context, 1, &pe_d3d.render_target_view, pe_d3d.depth_stencil_view);
}

static void pe_destroy_swapchain_resources(void) {
    ID3D11DeviceContext_OMSetRenderTargets(pe_d3d.context, 0, NULL, NULL);

    PE_ASSERT(pe_d3d.render_target_view != NULL);
    ID3D11RenderTargetView_Release(pe_d3d.render_target_view);
    pe_d3d.render_target_view = NULL;

    PE_ASSERT(pe_d3d.depth_stencil_view != NULL);
    ID3D11DepthStencilView_Release(pe_d3d.depth_stencil_view);
    pe_d3d.render_target_view = NULL;
}

static void d3d11_set_viewport(int width, int height) {
    D3D11_VIEWPORT viewport = {
        .TopLeftX = 0.0f, .TopLeftY = 0.0f,
        .Width = (float)width, .Height = (float)height,
        .MinDepth = 0.0f, .MaxDepth = 1.0f
    };
    ID3D11DeviceContext_RSSetViewports(pe_d3d.context, 1, &viewport);
}

static void glfw_framebuffer_size_proc(GLFWwindow *window, int width, int height) {
    ID3D11DeviceContext_Flush(pe_d3d.context);
    pe_destroy_swapchain_resources();
    HRESULT hr = IDXGISwapChain1_ResizeBuffers(pe_d3d.swapchain, 0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);
    pe_create_swapchain_resources();

    d3d11_set_viewport(width, height);

    pe_glfw.window_width = width;
    pe_glfw.window_height = height;
}

void pe_d3d11_init(void) {
    HRESULT hr;
    HWND hwnd;
    int window_width, window_height;

    // init GLFW
    {
        PE_ASSERT(pe_glfw.initialized);
        glfwSetFramebufferSizeCallback(pe_glfw.window, glfw_framebuffer_size_proc);
        hwnd = glfwGetWin32Window(pe_glfw.window);
        glfwGetWindowSize(pe_glfw.window, &window_width, &window_height);
    }

    // create D3D11 device & context
    {
        UINT flags = 0;
#ifndef NDEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, ARRAYSIZE(levels),
            D3D11_SDK_VERSION, &pe_d3d.device, NULL, &pe_d3d.context
        );
        // make sure device creation succeeeds before continuing
        // for simple applciation you could retry device creation with
        // D3D_DRIVER_TYPE_WARP driver type which enables software rendering
        // (could be useful on broken drivers or remote desktop situations)
#ifndef NDEBUG
        if (!SUCCEEDED(hr) && hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
            flags &= ~D3D11_CREATE_DEVICE_DEBUG;
            hr = D3D11CreateDevice(
                NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, ARRAYSIZE(levels),
                D3D11_SDK_VERSION, &pe_d3d.device, NULL, &pe_d3d.context
            );
        }
#endif
        PE_ASSERT_MSG(SUCCEEDED(hr), "D3D11CreateDevice failed with HRESULT=%x", hr);

#ifndef NDEBUG
        ID3D11InfoQueue* info;
        if (SUCCEEDED(ID3D11Device_QueryInterface(pe_d3d.device, &IID_ID3D11InfoQueue, (void**)&info))) {
            hr = ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            hr = ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
            ID3D11InfoQueue_Release(info);
        } else {
            printf("Sorry no debug layer installed!\n");
        }
#endif
    }

    // create DXGI swap chain
    {
        IDXGIDevice* dxgi_device;
        hr = ID3D11Device_QueryInterface(pe_d3d.device, &IID_IDXGIDevice, (void**)&dxgi_device);

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

        hr = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)pe_d3d.device, hwnd, &desc, NULL, NULL, &pe_d3d.swapchain);

        pe_create_swapchain_resources();

        IDXGIFactory2_Release(factory);
        IDXGIAdapter_Release(dxgi_adapter);
        IDXGIDevice_Release(dxgi_device);
    }

    // Compile shaders
    {
        bool vertex_shader_create_result = pe_vertex_shader_create(pe_d3d.device, L"res/shader.hlsl", &pe_d3d.vertex_shader, &pe_d3d.vs_blob);
        PE_ASSERT(vertex_shader_create_result);
        ID3D11DeviceContext_VSSetShader(pe_d3d.context, pe_d3d.vertex_shader, NULL, 0);

        D3D11_INPUT_ELEMENT_DESC input_element_descs[] = {
            { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,    2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COL", 0, DXGI_FORMAT_R8G8B8A8_UNORM , 3, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 0, DXGI_FORMAT_R32_UINT, 4, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 1, DXGI_FORMAT_R32_UINT, 4, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 2, DXGI_FORMAT_R32_UINT, 4, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 3, DXGI_FORMAT_R32_UINT, 4, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 5, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 1, DXGI_FORMAT_R32_FLOAT, 5, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 2, DXGI_FORMAT_R32_FLOAT, 5, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 3, DXGI_FORMAT_R32_FLOAT, 5, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = ID3D11Device_CreateInputLayout(
            pe_d3d.device,
            input_element_descs,
            PE_COUNT_OF(input_element_descs),
            ID3D10Blob_GetBufferPointer(pe_d3d.vs_blob),
            ID3D10Blob_GetBufferSize(pe_d3d.vs_blob),
            &pe_d3d.input_layout
        );
        ID3D11DeviceContext_IASetInputLayout(pe_d3d.context, pe_d3d.input_layout);

        bool pixel_shader_create_result = pe_pixel_shader_create(pe_d3d.device, L"res/shader.hlsl", &pe_d3d.pixel_shader, &pe_d3d.ps_blob);
        PE_ASSERT(pixel_shader_create_result);
        ID3D11DeviceContext_PSSetShader(pe_d3d.context, pe_d3d.pixel_shader, NULL, 0);
    }

    {
        D3D11_RASTERIZER_DESC rasterizer_desc = {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_BACK,
            .FrontCounterClockwise = TRUE,
        };
        hr = ID3D11Device_CreateRasterizerState(pe_d3d.device, &rasterizer_desc, &pe_d3d.rasterizer_state);
        ID3D11DeviceContext_RSSetState(pe_d3d.context, pe_d3d.rasterizer_state);

        D3D11_SAMPLER_DESC sampler_desc = {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
            .ComparisonFunc = D3D11_COMPARISON_NEVER
        };
        hr = ID3D11Device_CreateSamplerState(pe_d3d.device, &sampler_desc, &pe_d3d.sampler_state);
        ID3D11DeviceContext_PSSetSamplers(pe_d3d.context, 0, 1, &pe_d3d.sampler_state);

        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS
        };
        hr = ID3D11Device_CreateDepthStencilState(pe_d3d.device, &depth_stencil_desc, &pe_d3d.depth_stencil_state);
        ID3D11DeviceContext_OMSetDepthStencilState(pe_d3d.context, pe_d3d.depth_stencil_state, 0);
    }

    // Create constant buffers
    {
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Projection), &pe_shader_constant_projection_buffer);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_View), &pe_shader_constant_view_buffer);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Model), &pe_shader_constant_model_buffer);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Light), &pe_shader_constant_light_buffer);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Material), &pe_shader_constant_material_buffer);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Skeleton), &pe_shader_constant_skeleton_buffer);

        ID3D11Buffer *constant_buffers[] = {
            pe_shader_constant_projection_buffer,
            pe_shader_constant_view_buffer,
            pe_shader_constant_model_buffer,
            pe_shader_constant_light_buffer,
            pe_shader_constant_material_buffer,
            pe_shader_constant_skeleton_buffer,
        };
        ID3D11DeviceContext_VSSetConstantBuffers(pe_d3d.context, 0, PE_COUNT_OF(constant_buffers), constant_buffers);
    }

    ID3D11DeviceContext_OMSetBlendState(pe_d3d.context, NULL, NULL, ~(uint32_t)(0));

    d3d11_set_viewport(window_width, window_height);
}

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
    HRESULT hr = ID3D11Device_CreateBuffer(pe_d3d.device, &buffer_desc, &subresource_data, &buffer);
    return buffer;
}

peTexture pe_texture_create_win32(void *data, UINT width, UINT height, int channels) {
    DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;
    int bytes_per_pixel = 0;
    switch (channels) {
        case 1:
            dxgi_format = DXGI_FORMAT_R8_UNORM;
            bytes_per_pixel = 1;
            break;
        case 4:
            dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            bytes_per_pixel = 4;
            break;
        default: PE_PANIC_MSG("Unsupported channel count: %d\n", channels); break;
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
    hr = ID3D11Device_CreateTexture2D(pe_d3d.device, &texture_desc, &subresource_data, &texture);
    ID3D11ShaderResourceView *texture_view;
    hr = ID3D11Device_CreateShaderResourceView(pe_d3d.device, (ID3D11Resource*)texture, NULL, &texture_view);
    ID3D11Texture2D_Release(texture);

    peTexture result = { .texture_resource = texture_view };
    return result;
}

//
// SHADERS
//

void pe_shader_constant_buffer_init(ID3D11Device *device, size_t size, ID3D11Buffer **buffer) {
    D3D11_BUFFER_DESC constant_buffer_desc = {
        .ByteWidth = (UINT)size,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };

    HRESULT hr = ID3D11Device_CreateBuffer(device, &constant_buffer_desc, 0, buffer);
}

void *pe_shader_constant_begin_map(ID3D11DeviceContext *context, ID3D11Buffer *buffer) {
    D3D11_MAPPED_SUBRESOURCE mapped_subresource;
    HRESULT hr = ID3D11DeviceContext_Map(context, (ID3D11Resource*)buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
    return mapped_subresource.pData;
}

void pe_shader_constant_end_map(ID3D11DeviceContext *context, ID3D11Buffer *buffer) {
    ID3D11DeviceContext_Unmap(context, (ID3D11Resource*)buffer, 0);
}

static bool pe_shader_compile(wchar_t *wchar_file_name, char *entry_point, char *profile, ID3D10Blob **blob) {
    UINT compiler_flags = D3DCOMPILE_ENABLE_STRICTNESS;
    ID3D10Blob *error_blob = NULL;
    HRESULT hr = D3DCompileFromFile(wchar_file_name, NULL, NULL, entry_point, profile, compiler_flags, 0, blob, &error_blob);
    if (!SUCCEEDED(hr)) {
        printf("D3D11: Failed to read shader from file (%x):\n", hr);
        if (error_blob != NULL) {
            printf("%s\n", (char *)ID3D10Blob_GetBufferPointer(error_blob));
            ID3D10Blob_Release(error_blob);
        }
        return false;
    }
    return true;
}

bool pe_vertex_shader_create(ID3D11Device *device, wchar_t *wchar_file_name, ID3D11VertexShader **vertex_shader, ID3D10Blob **vertex_shader_blob) {
    if (!pe_shader_compile(wchar_file_name, "vs_main", "vs_5_0", vertex_shader_blob)) {
        return false;
    }
    void *bytecode = ID3D10Blob_GetBufferPointer(*vertex_shader_blob);
    size_t bytecode_length = ID3D10Blob_GetBufferSize(*vertex_shader_blob);
    HRESULT hr = ID3D11Device_CreateVertexShader(device, bytecode, bytecode_length, NULL, vertex_shader);
    if (!SUCCEEDED(hr)) {
        printf("D3D11: Failed to compile vertex shader (%x)\n", hr);
        ID3D10Blob_Release(*vertex_shader_blob);
        return false;
    }
    return true;
}

bool pe_pixel_shader_create(ID3D11Device *device, wchar_t *wchar_file_name, ID3D11PixelShader **pixel_shader, ID3DBlob **pixel_shader_blob) {
    if (!pe_shader_compile(wchar_file_name, "ps_main", "ps_5_0", pixel_shader_blob)) {
        return false;
    }
    void *bytecode = ID3D10Blob_GetBufferPointer(*pixel_shader_blob);
    size_t bytecode_length = ID3D10Blob_GetBufferSize(*pixel_shader_blob);
    HRESULT hr = ID3D11Device_CreatePixelShader(device, bytecode, bytecode_length, NULL, pixel_shader);
    if (!SUCCEEDED(hr)) {
        printf("D3D11: Failed to compile pixel shader (%x)\n", hr);
        ID3D10Blob_Release(*pixel_shader_blob);
        return false;
    }
    return true;
}

static void pe_scalar_sin_cos(float *out_sin, float *out_cos, float value) {
    // See https://github.com/microsoft/DirectXMath/blob/22e6d747994600e00834faff5fc2a95ab60f1790/Inc/DirectXMathMisc.inl#L2305

    // Map Value to y in [-pi,pi], x = 2*pi*quotient + remainder.
    float quotient = 1.0f/(2.0f * HMM_PI32) * value;
    if (value >= 0.0f) {
        quotient = (float)((int)(quotient + 0.5f));
    } else {
        quotient = (float)((int)(quotient - 0.5f));
    }
    float y = value - 2.0f*HMM_PI32 * quotient;

    // Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
    float sign;
    if (y > HMM_PI32/2.0f) {
        y = HMM_PI32 - y;
        sign = -1.0f;
    } else if (y < -HMM_PI32/2.0f) {
        y = -HMM_PI32 - y;
        sign = -1.0f;
    } else {
        sign = +1.0f;
    }

    float y2 = y * y;

    // 11-degree minimax approximation
    *out_sin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

    // 10-degree minimax approximation
    float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
    *out_cos = sign * p;
}

/*
OLD WIN32 PERSPECTIVE, REPLACED WITH FOVY VERSION
HMM_Mat4 pe_mat4_perspective(float w, float h, float n, float f) {
    // See https://github.com/microsoft/DirectXMath/blob/bec07458c994bd7553638e4d499e17cfedd07831/Inc/DirectXMathMatrix.inl#L2350
    HMM_Mat4 result = {
		2 * n / w,  0,         0,           0,
		0,          2 * n / h, 0,           0,
		0,          0,          f / (n - f), n * f / (n - f),
		0,          0,         -1,           0,
    };
    return result;
}
*/

HMM_Mat4 pe_perspective_win32(float fovy, float aspect_ratio, float near_z, float far_z) {
    // See https://github.com/microsoft/DirectXMath/blob/bec07458c994bd7553638e4d499e17cfedd07831/Inc/DirectXMathMatrix.inl#L2540

    float sin_fov, cos_fov;
    pe_scalar_sin_cos(&sin_fov, &cos_fov, 0.5f*fovy*HMM_DegToRad);

    float height = cos_fov / sin_fov;
    float width = height / aspect_ratio;
    float f_range = far_z / (near_z - far_z);

    HMM_Mat4 result = {
        width, 0.0f,   0.0f,    0.0f,
        0.0f,  height, 0.0f,    0.0f,
        0.0f,  0.0f,   f_range, f_range * near_z,
        0.0f,  0.0f,   -1.0f,   0.0f,
    };
    return result;
}
