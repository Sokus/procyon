#include "pe_graphics.h"
#include "pe_graphics_win32.h"

#include "core/p_defines.h"
#include "core/p_assert.h"
#include "core/p_heap.h"
#include "utility/pe_trace.h"

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

#include <stdio.h>

void* pe_hwnd = NULL;
peDirect3D pe_d3d = {0};
peDynamicDrawStateD3D dynamic_draw_d3d = {0};
peShaderConstantBuffersD3D constant_buffers_d3d;

peTexture default_texture = {0};

//
// GENERAL IMPLEMENTATIONS
//

void pe_graphics_init(peArena *temp_arena, int window_width, int window_height) {
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
        P_ASSERT_MSG(SUCCEEDED(hr), "D3D11CreateDevice failed with HRESULT=%x", hr);
#ifndef NDEBUG
        ID3D11InfoQueue* info;
        if (SUCCEEDED(ID3D11Device_QueryInterface(pe_d3d.device, &IID_ID3D11InfoQueue, (void**)&info))) {
            hr = ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            hr = ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
            ID3D11InfoQueue_Release(info);
        } else {
            printf("D3D11: No debug layer installed!\n");
        }
#endif
    }

    // create DXGI swap chain
    {
        IDXGIDevice* dxgi_device;
        IDXGIAdapter* dxgi_adapter;
        IDXGIFactory2* factory;
        hr = ID3D11Device_QueryInterface(pe_d3d.device, &IID_IDXGIDevice, (void**)&dxgi_device);
        hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
        hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, (void**)&factory);

        DXGI_SWAP_CHAIN_DESC1 desc =
        {
            .Width = window_width,
            .Height = window_height,
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
            .Stereo = FALSE,
            .SampleDesc = {
                .Count = 1,
                .Quality = 0,
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .Scaling = DXGI_SCALING_STRETCH,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
            .Flags = 0,
        };
        HWND hwnd = pe_hwnd;
        hr = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)pe_d3d.device, hwnd, &desc, NULL, NULL, &pe_d3d.swapchain);
        IDXGIFactory2_Release(factory);
        IDXGIAdapter_Release(dxgi_adapter);
        IDXGIDevice_Release(dxgi_device);

        pe_create_swapchain_resources();
    }

    {
        D3D11_RASTERIZER_DESC rasterizer_desc = {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_BACK,
            .FrontCounterClockwise = TRUE,
        };
        hr = ID3D11Device_CreateRasterizerState(pe_d3d.device, &rasterizer_desc, &pe_d3d.rasterizer_state);
        ID3D11DeviceContext_RSSetState(pe_d3d.context, pe_d3d.rasterizer_state);
    }

    {
        D3D11_SAMPLER_DESC sampler_desc = {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
            .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
            .ComparisonFunc = D3D11_COMPARISON_NEVER
        };
        hr = ID3D11Device_CreateSamplerState(pe_d3d.device, &sampler_desc, &pe_d3d.sampler_state);
        ID3D11DeviceContext_PSSetSamplers(pe_d3d.context, 0, 1, &pe_d3d.sampler_state);
    }

    {
        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc_enabled = {
            .DepthEnable = TRUE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = FALSE,
        };
        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc_disabled = {
            .DepthEnable = FALSE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
            .DepthFunc = D3D11_COMPARISON_ALWAYS,
            .StencilEnable = FALSE,
        };
        hr = ID3D11Device_CreateDepthStencilState(pe_d3d.device, &depth_stencil_desc_enabled, &pe_d3d.depth_stencil_state_enabled);
        hr = ID3D11Device_CreateDepthStencilState(pe_d3d.device, &depth_stencil_desc_disabled, &pe_d3d.depth_stencil_state_disabled);
        ID3D11DeviceContext_OMSetDepthStencilState(pe_d3d.context, pe_d3d.depth_stencil_state_enabled, 0);
    }

    {
        D3D11_BLEND_DESC blend_desc = {
            .RenderTarget = {
                {
                    .BlendEnable = TRUE,
                    .SrcBlend = D3D11_BLEND_SRC_ALPHA,
                    .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                    .BlendOp = D3D11_BLEND_OP_ADD,
                    .SrcBlendAlpha = D3D11_BLEND_ONE,
                    .DestBlendAlpha = D3D11_BLEND_ZERO,
                    .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                    .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
                }
            }
        };
        hr = ID3D11Device_CreateBlendState(pe_d3d.device, &blend_desc, &pe_d3d.blend_state);
        FLOAT blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        UINT sample_mask = 0xFFFFFFFF;
        ID3D11DeviceContext_OMSetBlendState(pe_d3d.context, pe_d3d.blend_state, blend_factor, sample_mask);
    }

    d3d11_set_viewport(window_width, window_height);

    // compile shaders
    {
        bool vertex_shader_create_result = pe_vertex_shader_create(pe_d3d.device, L"res/shader.hlsl", &pe_d3d.vertex_shader, &pe_d3d.vs_blob);
        P_ASSERT(vertex_shader_create_result);
        ID3D11DeviceContext_VSSetShader(pe_d3d.context, pe_d3d.vertex_shader, NULL, 0);

        bool pixel_shader_create_result = pe_pixel_shader_create(pe_d3d.device, L"res/shader.hlsl", &pe_d3d.pixel_shader, &pe_d3d.ps_blob);
        P_ASSERT(pixel_shader_create_result);
        ID3D11DeviceContext_PSSetShader(pe_d3d.context, pe_d3d.pixel_shader, NULL, 0);
    }

    // create input layout
    {
        D3D11_INPUT_ELEMENT_DESC input_element_descs[] = {
            { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COL", 0, DXGI_FORMAT_R8G8B8A8_UNORM , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 1, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 2, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 3, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 1, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 2, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 3, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        hr = ID3D11Device_CreateInputLayout(
            pe_d3d.device,
            input_element_descs,
            P_COUNT_OF(input_element_descs),
            ID3D10Blob_GetBufferPointer(pe_d3d.vs_blob),
            ID3D10Blob_GetBufferSize(pe_d3d.vs_blob),
            &pe_d3d.input_layout
        );
        ID3D11DeviceContext_IASetInputLayout(pe_d3d.context, pe_d3d.input_layout);
    }

    // create constant buffers
    {
        peShaderConstantBuffersD3D *cb = &constant_buffers_d3d;
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Matrix), &cb->projection);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Matrix), &cb->view);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Matrix), &cb->model);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Light), &cb->light);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_LightVector), &cb->light_vector);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Material), &cb->material);
        pe_shader_constant_buffer_init(pe_d3d.device, sizeof(peShaderConstant_Skeleton), &cb->skeleton);

        ID3D11Buffer *constant_buffers[] = {
            constant_buffers_d3d.projection,
            constant_buffers_d3d.view,
            constant_buffers_d3d.model,
            constant_buffers_d3d.light,
            constant_buffers_d3d.light_vector,
            constant_buffers_d3d.material,
            constant_buffers_d3d.skeleton,
        };
        P_ASSERT(P_COUNT_OF(constant_buffers) <= 15);
        ID3D11DeviceContext_VSSetConstantBuffers(pe_d3d.context, 0, P_COUNT_OF(constant_buffers), constant_buffers);
    }

    // init dynamic_draw
    {
        dynamic_draw.vertex = pe_heap_alloc(PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT * sizeof(peDynamicDrawVertex));
        dynamic_draw.batch = pe_heap_alloc(PE_MAX_DYNAMIC_DRAW_BATCH_COUNT * sizeof(peDynamicDrawBatch));

    	dynamic_draw_d3d.vertex_buffer = pe_d3d11_create_buffer(
            dynamic_draw.vertex,
            PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT*sizeof(peDynamicDrawVertex),
            D3D11_USAGE_DYNAMIC,
            D3D11_BIND_VERTEX_BUFFER,
            D3D11_CPU_ACCESS_WRITE
        );

        D3D11_INPUT_ELEMENT_DESC input_element_descs[] = {
            { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COL", 0, DXGI_FORMAT_R8G8B8A8_UNORM , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 0, DXGI_FORMAT_R32_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 1, DXGI_FORMAT_R32_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 2, DXGI_FORMAT_R32_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEINDEX", 3, DXGI_FORMAT_R32_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 0, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 1, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 2, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT", 3, DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        hr = ID3D11Device_CreateInputLayout(
            pe_d3d.device,
            input_element_descs,
            P_COUNT_OF(input_element_descs),
            ID3D10Blob_GetBufferPointer(pe_d3d.vs_blob),
            ID3D10Blob_GetBufferSize(pe_d3d.vs_blob),
            &dynamic_draw_d3d.input_layout
        );

    }

    // init matrices
    {
        pe_graphics.mode = peGraphicsMode_2D;
        pe_graphics.matrix_mode = peMatrixMode_Projection;

        for (int gm = 0; gm < peGraphicsMode_Count; gm += 1) {
            for (int mm = 0; mm < peMatrixMode_Count; mm += 1) {
                pe_graphics.matrix[gm][mm] = p_mat4_i();
                pe_graphics.matrix_dirty[gm][mm] = true;
            }
            pe_graphics.matrix_model_is_identity[gm] = true;
        }

        pe_graphics.matrix[peGraphicsMode_2D][peMatrixMode_Projection] = pe_matrix_orthographic(
            0, (float)window_width,
            (float)window_height, 0,
            0.0f, 1000.0f
        );

        pe_graphics_matrix_update();
    }

    {
        pe_graphics_set_lighting(true);
        pe_graphics_set_light_vector(PE_LIGHT_VECTOR_DEFAULT);
    }

    // init default texture
	{
		uint32_t texture_data[] = { 0xFFFFFFFF };
		default_texture = pe_texture_create(temp_arena, texture_data, 1, 1);
	}

    pe_d3d.framebuffer_width = window_width;
    pe_d3d.framebuffer_height = window_height;
}

void pe_graphics_shutdown(void) {
    pe_heap_free(dynamic_draw.vertex);
    pe_heap_free(dynamic_draw.batch);
}

void pe_graphics_set_framebuffer_size(int width, int height) {
    ID3D11DeviceContext_Flush(pe_d3d.context);
    pe_destroy_swapchain_resources();
    HRESULT hr = IDXGISwapChain1_ResizeBuffers(pe_d3d.swapchain, 0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);
    pe_create_swapchain_resources();

    d3d11_set_viewport(width, height);

    pe_d3d.framebuffer_width = width;
    pe_d3d.framebuffer_height = height;
}

int pe_screen_width(void) {
    return pe_d3d.framebuffer_width;
}

int pe_screen_height(void) {
    return pe_d3d.framebuffer_height;
}

void pe_clear_background(peColor color) {
    pVec4 color_converted_vec4 = pe_color_to_vec4(color);
    ID3D11DeviceContext_ClearRenderTargetView(pe_d3d.context, pe_d3d.render_target_view, color_converted_vec4.elements);
    ID3D11DeviceContext_ClearDepthStencilView(pe_d3d.context, pe_d3d.depth_stencil_view, D3D11_CLEAR_DEPTH, 1, 0);
}

void pe_graphics_frame_begin(void) {
    pe_graphics.matrix[peGraphicsMode_2D][peMatrixMode_Projection] = pe_matrix_orthographic(
        0, (float)pe_screen_width(),
        (float)pe_screen_height(), 0,
        0.0f, 1000.0f
    );
    pe_graphics_matrix_update();
}

void pe_graphics_frame_end(void) {
}

void pe_graphics_set_depth_test(bool enable) {
    ID3D11DepthStencilState *depth_stencil_state;
    if (enable) {
        depth_stencil_state = pe_d3d.depth_stencil_state_enabled;
    } else {
        depth_stencil_state = pe_d3d.depth_stencil_state_disabled;
    }
    ID3D11DeviceContext_OMSetDepthStencilState(pe_d3d.context, depth_stencil_state, 0);
}

void pe_graphics_set_lighting(bool enable) {
    peShaderConstant_Light *constant_light = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.light);
    constant_light->do_lighting = enable;
    pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.light);
}

void pe_graphics_set_light_vector(pVec3 light_vector) {
    peShaderConstant_LightVector *constant_light_vector = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.light_vector);
    constant_light_vector->light_vector = light_vector;
    pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.light_vector);
}

void pe_graphics_set_diffuse_color(peColor color) {
    peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.material);
    constant_material->diffuse_color = pe_color_to_vec4(color);
    pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.material);
}

void pe_graphics_matrix_update(void) {
    ID3D11Buffer *shader_constant_buffers[peMatrixMode_Count] = {
        constant_buffers_d3d.projection,
        constant_buffers_d3d.view,
        constant_buffers_d3d.model,
    };
    for (int matrix_mode = 0; matrix_mode < peMatrixMode_Count; matrix_mode += 1) {
        if (pe_graphics.matrix_dirty[pe_graphics.mode][matrix_mode]) {
            ID3D11Buffer *constant_buffer = shader_constant_buffers[matrix_mode];
            peShaderConstant_Matrix *constant_matrix = pe_shader_constant_begin_map(pe_d3d.context, constant_buffer);
            constant_matrix->value = pe_graphics.matrix[pe_graphics.mode][matrix_mode];
            pe_shader_constant_end_map(pe_d3d.context, constant_buffer);
            pe_graphics.matrix_dirty[pe_graphics.mode][matrix_mode] = false;
        }
    }
}

pMat4 pe_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
    return p_perspective_rh_zo(fovy * P_DEG2RAD, aspect_ratio, near_z, far_z);
}

pMat4 pe_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
    return p_orthographic_rh_zo(left, right, bottom, top, near_z, far_z);
}

void pe_graphics_dynamic_draw_draw_batches(void) {
    PE_TRACE_FUNCTION_BEGIN();
    if (dynamic_draw.vertex_used > 0) {
        peMatrixMode old_matrix_mode = pe_graphics.matrix_mode;
        pMat4 old_matrix_model = pe_graphics.matrix[pe_graphics.mode][peMatrixMode_Model];
        bool old_matrix_model_is_identity = pe_graphics.matrix_model_is_identity[pe_graphics.mode];

        pe_graphics_matrix_mode(peMatrixMode_Model);
        pe_graphics_matrix_identity();
        pe_graphics_matrix_update();

        peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.material);
        constant_material->diffuse_color = pe_color_to_vec4(PE_COLOR_WHITE);
        pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.material);

        peShaderConstant_Skeleton *constant_skeleton = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.skeleton);
        constant_skeleton->has_skeleton = false;
        pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.skeleton);

        peDynamicDrawVertex *vertex_buffer_map = pe_shader_constant_begin_map(pe_d3d.context, dynamic_draw_d3d.vertex_buffer);
        memcpy(vertex_buffer_map, dynamic_draw.vertex, dynamic_draw.vertex_used*sizeof(peDynamicDrawVertex));
        pe_shader_constant_end_map(pe_d3d.context, dynamic_draw_d3d.vertex_buffer);

        uint32_t vertex_buffer_strides[] = { sizeof(peDynamicDrawVertex) };
        uint32_t vertex_buffer_offsets[] = { 0 };
        ID3D11DeviceContext_IASetVertexBuffers(pe_d3d.context, 0, 1, &dynamic_draw_d3d.vertex_buffer, vertex_buffer_strides, vertex_buffer_offsets);

        ID3D11DeviceContext_IASetInputLayout(pe_d3d.context, dynamic_draw_d3d.input_layout);
        if (pe_graphics.mode == peGraphicsMode_2D) {
            ID3D11DeviceContext_OMSetDepthStencilState(pe_d3d.context, pe_d3d.depth_stencil_state_disabled, 0);
        }

        bool previous_do_lighting = false;
        for (int b = dynamic_draw.batch_drawn_count; b <= dynamic_draw.batch_current; b += 1) {
            P_ASSERT(dynamic_draw.batch[b].vertex_count > 0);

            peTexture *texture = dynamic_draw.batch[b].texture;
            if (!texture) texture = &default_texture;
            pe_texture_bind(*texture);

            if (b == 0 || previous_do_lighting != dynamic_draw.batch[b].do_lighting) {
                pe_graphics_set_lighting(dynamic_draw.batch[b].do_lighting);
                pe_graphics_set_light_vector(PE_LIGHT_VECTOR_DEFAULT);
                previous_do_lighting = dynamic_draw.batch[b].do_lighting;
            }

            D3D11_PRIMITIVE_TOPOLOGY primitive_d3d11;
            switch (dynamic_draw.batch[b].primitive) {
                case pePrimitive_Points: primitive_d3d11 = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
                case pePrimitive_Lines: primitive_d3d11 = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
                case pePrimitive_Triangles: primitive_d3d11 = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
                case pePrimitive_TriangleStrip: primitive_d3d11 = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
                default: primitive_d3d11 = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED; break;
            }

            ID3D11DeviceContext_IASetPrimitiveTopology(pe_d3d.context, primitive_d3d11);
            ID3D11DeviceContext_Draw(pe_d3d.context, dynamic_draw.batch[b].vertex_count, dynamic_draw.batch[b].vertex_offset);
        }

        ID3D11DeviceContext_IASetInputLayout(pe_d3d.context, pe_d3d.input_layout);
        if (pe_graphics.mode == peGraphicsMode_2D) {
            ID3D11DeviceContext_OMSetDepthStencilState(pe_d3d.context, pe_d3d.depth_stencil_state_enabled, 0);
        }

        pe_graphics_matrix_set(&old_matrix_model);
        pe_graphics_matrix_update();
        pe_graphics_matrix_mode(old_matrix_mode);
        pe_graphics.matrix_model_is_identity[pe_graphics.mode] = old_matrix_model_is_identity;

        dynamic_draw.batch_drawn_count = dynamic_draw.batch_current + 1;
    }
    PE_TRACE_FUNCTION_END();
}

void pe_graphics_dynamic_draw_push_vec3(pVec3 position) {
    if (!pe_graphics.matrix_model_is_identity[pe_graphics.mode]) {
        pMat4 *matrix = &pe_graphics.matrix[pe_graphics.mode][peMatrixMode_Model];
        pVec4 position_vec4 = p_vec4v(position, 1.0f);
        pVec4 transformed_position = p_mat4_mul_vec4(*matrix, position_vec4);
        position = transformed_position.xyz;
    }
    pePrimitive batch_primitive = dynamic_draw.batch[dynamic_draw.batch_current].primitive;
    int batch_vertex_count = dynamic_draw.batch[dynamic_draw.batch_current].vertex_count;
    int primitive_vertex_count = pe_graphics_primitive_vertex_count(batch_primitive);
    if (primitive_vertex_count > 0) {
        int primitive_vertex_left = primitive_vertex_count - (batch_vertex_count % primitive_vertex_count);
        pe_graphics_dynamic_draw_vertex_reserve(primitive_vertex_left);
    }
    peDynamicDrawVertex *vertex = &dynamic_draw.vertex[dynamic_draw.vertex_used];
    vertex->position = position;
    vertex->normal = dynamic_draw.normal;
    vertex->texcoord = dynamic_draw.texcoord;
    vertex->color = dynamic_draw.color.rgba;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_count += 1;
    dynamic_draw.vertex_used += 1;
}

ID3D11Buffer *pe_d3d11_create_buffer(void *data, UINT byte_width, D3D11_USAGE usage, UINT bind_flags, D3D11_CPU_ACCESS_FLAG cpu_access_flags) {
    D3D11_BUFFER_DESC buffer_desc = {
        .ByteWidth = byte_width,
        .Usage = usage,
        .BindFlags = bind_flags,
        .CPUAccessFlags = cpu_access_flags,
    };
    D3D11_SUBRESOURCE_DATA subresource_data = {
        .pSysMem = data,
        .SysMemPitch = byte_width,
    };
    ID3D11Buffer *buffer = NULL;
    HRESULT hr = ID3D11Device_CreateBuffer(pe_d3d.device, &buffer_desc, &subresource_data, &buffer);
    return buffer;
}

peTexture pe_texture_create(peArena *temp_arena, void *data, int width, int height) {
    DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;
    int bytes_per_pixel = 0;
    const int channels = 4;
    switch (channels) {
        case 1:
            dxgi_format = DXGI_FORMAT_R8_UNORM;
            bytes_per_pixel = 1;
            break;
        case 4:
            dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
            bytes_per_pixel = 4;
            break;
        default: P_PANIC_MSG("Unsupported channel count: %d\n", channels); break;
    }
    D3D11_TEXTURE2D_DESC texture_desc = {
        .Width = (UINT)width,
        .Height = (UINT)height,
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

    peTexture result = {
        .texture_resource = texture_view,
        .width = width,
        .height = height,
    };
    return result;
}

void pe_texture_bind(peTexture texture) {
	ID3D11DeviceContext_PSSetShaderResources(pe_d3d.context, 0, 1, (ID3D11ShaderResourceView *const *)&texture.texture_resource);
}

void pe_texture_bind_default(void) {
	pe_texture_bind(default_texture);
}

//
// INTERNAL IMPLEMENTATIONS
//

static void pe_create_swapchain_resources(void) {
    HRESULT hr;

    ID3D11Texture2D *render_target;
    hr = IDXGISwapChain1_GetBuffer(pe_d3d.swapchain, 0, &IID_ID3D11Texture2D, &render_target);

    D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc = {
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
    };
    hr = ID3D11Device_CreateRenderTargetView(pe_d3d.device, (ID3D11Resource*)render_target, &render_target_view_desc, &pe_d3d.render_target_view);

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

    P_ASSERT(pe_d3d.render_target_view != NULL);
    ID3D11RenderTargetView_Release(pe_d3d.render_target_view);
    pe_d3d.render_target_view = NULL;

    P_ASSERT(pe_d3d.depth_stencil_view != NULL);
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

void pe_framebuffer_size_callback_win32(int width, int height) {
    ID3D11DeviceContext_Flush(pe_d3d.context);
    pe_destroy_swapchain_resources();
    HRESULT hr = IDXGISwapChain1_ResizeBuffers(pe_d3d.swapchain, 0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);
    pe_create_swapchain_resources();

    d3d11_set_viewport(width, height);

    pe_d3d.framebuffer_width = width;
    pe_d3d.framebuffer_height = height;
}

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
