#ifndef WIN32_SHADER_H
#define WIN32_SHADER_H

#include <stddef.h>
#include <wchar.h>
#include <stdbool.h>

#include "HandmadeMath.h"

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

typedef struct ID3D11DeviceContext ID3D11DeviceContext;
typedef struct ID3D11Buffer ID3D11Buffer;
typedef struct ID3D11VertexShader ID3D11VertexShader;
typedef struct ID3D11PixelShader ID3D11PixelShader;

struct ID3D11Device;

void pe_shader_constant_buffer_init(struct ID3D11Device *device, size_t size, ID3D11Buffer **buffer);
void *pe_shader_constant_begin_map(ID3D11DeviceContext *context, ID3D11Buffer *buffer);
void pe_shader_constant_end_map(ID3D11DeviceContext *context, ID3D11Buffer *buffer);
bool pe_vertex_shader_create(struct ID3D11Device *device, wchar_t *wchar_file_name, ID3D11VertexShader **vertex_shader, struct ID3D10Blob **vertex_shader_blob);
bool pe_pixel_shader_create(struct ID3D11Device *device, wchar_t *wchar_file_name, ID3D11PixelShader **pixel_shader, struct ID3D10Blob **pixel_shader_blob);

#endif // WIN32_SHADER_H