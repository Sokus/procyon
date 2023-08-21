#include "win32_shader.h"

#define COBJMACROS
#include <d3d11.h>
#include <d3dcompiler.h>
#include <winerror.h>

#include <stdio.h>

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
        printf("D3D11: Failed to read shader from file (%x)\n", hr);
        if (error_blob != NULL) {
            printf("\tWith message: %s\n", (char *)ID3D10Blob_GetBufferPointer(error_blob));
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

