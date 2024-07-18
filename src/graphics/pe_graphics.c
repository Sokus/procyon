#include "pe_graphics.h"

#include "core/pe_core.h"
#include "pe_math.h"
#include "platform/pe_platform.h"
#include "platform/pe_window.h"
#include "utility/pe_trace.h"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

	#include "pe_graphics_win32.h"
#elif defined(PSP)
	#include "pe_graphics_psp.h"

    #include <pspdisplay.h> // sceDisplayWaitVblankStart
    #include <pspgu.h>
	#include <pspgum.h>
#elif defined(__linux__)
	#include "pe_graphics_linux.h"

	#include "glad/glad.h"
#endif

#include "HandmadeMath.h"

#include <stdbool.h>

peTexture default_texture;

void pe_graphics_init(peArena *temp_arena, int window_width, int window_height) {
#if defined(_WIN32)
	HWND hwnd = pe_window_get_win32_window();
	pe_graphics_init_win32(hwnd, window_width, window_height);
#elif defined(PSP)
    pe_graphics_init_psp();
#elif defined(__linux__)
	pe_graphics_init_linux(temp_arena, window_width, window_height);

#endif
#if defined(_WIN32) || defined(__linux__)
	// init default texture
	{
		uint32_t texture_data[] = { 0xFFFFFFFF };
		default_texture = pe_texture_create(temp_arena, texture_data, 1, 1, 4);
	}
#endif
}

void pe_graphics_shutdown(void) {
#if defined(PSP)
	pe_graphics_shutdown_psp();
#endif
}

void pe_graphics_frame_begin(void) {
#if defined(PSP)
    sceGuStart(GU_DIRECT, list);
#endif
}

void pe_graphics_frame_end(bool vsync) {
	PE_TRACE_FUNCTION_BEGIN();
#if defined(_WIN32)
    UINT sync_interval = (vsync ? 1 : 0);
    IDXGISwapChain1_Present(pe_d3d.swapchain, sync_interval, 0);
#elif defined(PSP)
    sceGuFinish();
	peTraceMark tm_sync = PE_TRACE_MARK_BEGIN("sceGuSync");
    sceGuSync(0, 0);
	PE_TRACE_MARK_END(tm_sync);
    if (vsync) {
        sceDisplayWaitVblankStart();
    }
    sceGuSwapBuffers();
#elif defined(__linux__)
    pe_graphics_dynamic_draw_end_batch();
    pe_graphics_dynamic_draw_flush();
	pe_window_swap_buffers(vsync);
#endif
	PE_TRACE_FUNCTION_END();
}

void pe_graphics_matrix_projection(pMat4 *matrix) {
#if defined(_WIN32)
    peShaderConstant_Projection *projection = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_projection_buffer);
	projection->matrix = (*matrix)._hmm;
	pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_projection_buffer);
#elif defined(__linux__)
	pe_shader_set_mat4(pe_opengl.shader_program, "matrix_projection", matrix);
#elif defined(PSP)
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadMatrix(&matrix->_sce);
#endif
}

void pe_graphics_matrix_view(pMat4 *matrix) {
#if defined(_WIN32)
    peShaderConstant_View *constant_view = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_view_buffer);
    constant_view->matrix = (*matrix)._hmm;
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_view_buffer);
#elif defined(__linux__)
    pe_shader_set_mat4(pe_opengl.shader_program, "matrix_view", matrix);
#elif defined(PSP)
	sceGumMatrixMode(GU_VIEW);
    sceGumLoadMatrix(&matrix->_sce);
#endif
}

int pe_screen_width(void) {
#if defined(_WIN32)
    return pe_d3d.framebuffer_width;
#elif defined(__linux__)
	return pe_opengl.framebuffer_width;
#elif defined(PSP)
    return PSP_SCREEN_W;
#endif
}

int pe_screen_height(void) {
#if defined(_WIN32)
    return pe_d3d.framebuffer_height;
#elif defined(__linux__)
	return pe_opengl.framebuffer_height;
#elif defined(PSP)
    return PSP_SCREEN_H;
#endif
}

void pe_clear_background(peColor color) {
#if defined(_WIN32)
    float r = (float)color.r / 255.0f;
    float g = (float)color.g / 255.0f;
    float b = (float)color.b / 255.0f;

    // FIXME: render a sprite to get exact background color
    FLOAT clear_color[4] = { powf(r, 2.0f), powf(g, 2.0f), powf(b, 2.0f), 1.0f };
    ID3D11DeviceContext_ClearRenderTargetView(pe_d3d.context, pe_d3d.render_target_view, clear_color);
    ID3D11DeviceContext_ClearDepthStencilView(pe_d3d.context, pe_d3d.depth_stencil_view, D3D11_CLEAR_DEPTH, 1, 0);
#elif defined(PSP)
	sceGuClearColor(pe_color_to_8888(color));
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
#elif defined(__linux__)
	float r = (float)color.r / 255.0f;
	float g = (float)color.g / 255.0f;
	float b = (float)color.b / 255.0f;
	float a = (float)color.a / 255.0f;
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

pMat4 pe_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
#if defined(_WIN32)
	return pe_perspective_win32(fovy, aspect_ratio, near_z, far_z);
#else
	return p_perspective_rh_no(fovy * P_DEG2RAD, aspect_ratio, near_z, far_z);
#endif
}

pMat4 pe_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
#if defined(_WIN32)
    PE_UNIMPLEMENTED();
    return p_mat4_i();
#else
    return p_orthographic_rh_no(left, right, bottom, top, near_z, far_z);
#endif
}

pVec4 pe_color_to_vec4(peColor color) {
    pVec4 vec4 = {
        .r = (float)color.r / 255.0f,
        .g = (float)color.g / 255.0f,
        .b = (float)color.b / 255.0f,
        .a = (float)color.a / 255.0f,
    };
    return vec4;
}

uint16_t pe_color_to_5650(peColor color) {
	uint16_t max_5_bit = (1U << 5) - 1;
	uint16_t max_6_bit = (1U << 6) - 1;
	uint16_t max_8_bit = (1U << 8) - 1;
	uint16_t r = (((uint16_t)color.r * max_5_bit) / max_8_bit) & max_5_bit;
	uint16_t g = (((uint16_t)color.g * max_6_bit) / max_8_bit) & max_6_bit;
	uint16_t b = (((uint16_t)color.b * max_5_bit) / max_8_bit) & max_5_bit;
	uint16_t result = b << 11 | g << 5 | r;
	return result;
}

uint32_t pe_color_to_8888(peColor color) {
	uint32_t r = (uint32_t)color.r;
	uint32_t g = (uint32_t)color.g;
	uint32_t b = (uint32_t)color.b;
	uint32_t a = (uint32_t)color.a;
	uint32_t result = r | g << 8 | b << 16 | a << 24;
	return result;
}

peTexture pe_texture_create(peArena *temp_arena, void *data, int width, int height, int format) {
#if defined(_WIN32)
	return pe_texture_create_win32(data, (UINT)width, (UINT)height, format);
#elif defined(__linux__)
	return pe_texture_create_linux(data, width, height, format);
#elif defined(PSP)
	return pe_texture_create_psp(temp_arena, data, width, height, format);
#endif
}

void pe_texture_bind(peTexture texture) {
#if defined(_WIN32)
	ID3D11DeviceContext_PSSetShaderResources(pe_d3d.context, 0, 1, (ID3D11ShaderResourceView *const *)&texture.texture_resource);
#elif defined(__linux__)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.texture_object);
#elif defined(PSP)
	sceGuEnable(GU_TEXTURE_2D);
 	sceGuTexMode(texture.format, 0, 0, 1);
	sceGuTexImage(0, texture.power_of_two_width, texture.power_of_two_height, texture.power_of_two_width, texture.data);
	//sceGuTexEnvColor(0x00FFFFFF);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f);
#endif
}

void pe_texture_bind_default(void) {
#if defined(_WIN32) || defined(__linux__)
	pe_texture_bind(default_texture);
#elif defined(PSP)
	sceGuDisable(GU_TEXTURE_2D);
#endif
}

void pe_texture_disable(void) {
#if defined(__linux__)
    glBindTexture(GL_TEXTURE_2D, 0);
#else
    PE_UNIMPLEMENTED();
#endif
}

void pe_camera_update(peCamera camera) {
    pMat4 matrix_perspective = pe_matrix_perspective(camera.fovy, (float)pe_screen_width()/(float)pe_screen_height(), 1.0f, 1000.0f);
    pe_graphics_matrix_projection(&matrix_perspective);
    pMat4 matrix_lookat = p_look_at_rh(camera.position, camera.target, camera.up);
    pe_graphics_matrix_view(&matrix_lookat);
}

peRay pe_get_mouse_ray(pVec2 mouse, peCamera camera) {
    float window_width_float = (float)pe_screen_width();
    float window_height_float = (float)pe_screen_height();
    pMat4 projection = pe_matrix_perspective(55.0f, window_width_float/window_height_float, 1.0f, 1000.0f);
    pMat4 view = p_look_at_rh(camera.position, camera.target, camera.up);

    pVec3 near_point = pe_unproject_vec3(p_vec3(mouse.x, mouse.y, 0.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    pVec3 far_point = pe_unproject_vec3(p_vec3(mouse.x, mouse.y, 1.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    pVec3 direction = p_vec3_norm(p_vec3_sub(far_point, near_point));
    peRay ray = {
        .position = camera.position,
        .direction = direction,
    };
    return ray;
}