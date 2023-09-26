#include "pe_graphics.h"

#include "pe_core.h"
#include "pe_math.h"
#include "pe_platform.h"
#include "pe_temp_arena.h"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

	#include "pe_window_glfw.h"
	#include "pe_graphics_win32.h"
#elif defined(PSP)
	#include "pe_graphics_psp.h"

    #include <pspdisplay.h> // sceDisplayWaitVblankStart
    #include <pspgu.h>
	#include <pspgum.h>
#elif defined(__linux__)
    #include "pe_window_glfw.h"
	#include "pe_graphics_linux.h"

	#define GLFW_INCLUDE_NONE
	#include "GLFW/glfw3.h"

	#include "glad/glad.h"
#endif

#include "HandmadeMath.h"

#include <stdbool.h>

static peTexture default_texture;

void pe_graphics_init(int window_width, int window_height, const char *window_name) {
#if defined(_WIN32)
    pe_glfw_init(window_width, window_height, window_name);
    pe_d3d11_init();
#elif defined(PSP)
    pe_graphics_init_psp();
#elif defined(__linux__)
	pe_glfw_init(window_width, window_height, window_name);
	pe_graphics_init_linux();
#endif

#if defined(_WIN32) || defined(__linux__)
	// init default texture
	{
		uint32_t texture_data[] = { 0xFFFFFFFF };
		default_texture = pe_texture_create(texture_data, 1, 1, 4);
	}
#endif
}

void pe_graphics_shutdown(void) {
#if defined(_WIN32) || defined(__linux__)
	pe_glfw_shutdown();
#endif
}

void pe_graphics_frame_begin(void) {
#if defined(PSP)
    sceGuStart(GU_DIRECT, list);
#endif
}

void pe_graphics_frame_end(bool vsync) {
#if defined(_WIN32)
    UINT sync_interval = (vsync ? 1 : 0);
    IDXGISwapChain1_Present(pe_d3d.swapchain, sync_interval, 0);
#elif defined(PSP)
    sceGuFinish();
    sceGuSync(0, 0);
    if (vsync) {
        sceDisplayWaitVblankStart();
    }
    sceGuSwapBuffers();
#elif defined(__linux__)
	int interval = (vsync ? 1 : 0);
	glfwSwapInterval(interval);
	glfwSwapBuffers(pe_glfw.window);
#endif
}

void pe_graphics_projection_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
#if defined(_WIN32)
    peShaderConstant_Projection *projection = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_projection_buffer);
	projection->matrix = pe_matrix_perspective(fovy, aspect_ratio, near_z, far_z);
	pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_projection_buffer);
#elif defined(__linux__)
	HMM_Mat4 matrix_perspective = pe_matrix_perspective(fovy, aspect_ratio, near_z, far_z);
	pe_shader_set_mat4(pe_opengl.shader_program, "matrix_projection", &matrix_perspective);
#elif defined(PSP)
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(fovy, aspect_ratio, near_z, far_z);
#endif
}

void pe_graphics_view_lookat(HMM_Vec3 eye, HMM_Vec3 target, HMM_Vec3 up) {
#if defined(_WIN32)
    peShaderConstant_View *constant_view = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_view_buffer);
    constant_view->matrix = HMM_LookAt_RH(eye, target, up);
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_view_buffer);
#elif defined(__linux__)
	HMM_Mat4 matrix_view = HMM_LookAt_RH(eye, target, up);
    pe_shader_set_mat4(pe_opengl.shader_program, "matrix_view", &matrix_view);
#elif defined(PSP)
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	ScePspFVector3 sce_eye = { .x = eye.X, .y = eye.Y, .z = eye.Z };
	ScePspFVector3 sce_target = { .x = target.X, .y = target.Y, .z = target.Z };
	ScePspFVector3 sce_up = { .x = up.X, .y = up.Y, .z = up.Z };
	sceGumLookAt(&sce_eye, &sce_target, &sce_up);
#endif
}

int pe_screen_width(void) {
#if defined(_WIN32) || defined(__linux__)
    return pe_glfw.window_width;
#elif defined(PSP)
    return PSP_SCREEN_W;
#endif
}

int pe_screen_height(void) {
#if defined(_WIN32) || defined(__linux__)
    return pe_glfw.window_height;
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

HMM_Mat4 pe_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
#if defined(_WIN32)
	return pe_perspective_win32(fovy, aspect_ratio, near_z, far_z);
#else
	return HMM_Perspective_RH_NO(fovy * HMM_DegToRad, aspect_ratio, near_z, far_z);
#endif
}

HMM_Vec4 pe_color_to_vec4(peColor color) {
    HMM_Vec4 vec4 = {
        .R = (float)color.r / 255.0f,
        .G = (float)color.g / 255.0f,
        .B = (float)color.b / 255.0f,
        .A = (float)color.a / 255.0f,
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

peTexture pe_texture_create(void *data, int width, int height, int format) {
#if defined(_WIN32)
	return pe_texture_create_win32(data, (UINT)width, (UINT)height, format);
#elif defined(__linux__)
	return pe_texture_create_linux(data, width, height, format);
#elif defined(PSP)
	return pe_texture_create_psp(data, width, height, format);
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

void pe_camera_update(peCamera camera) {
    pe_graphics_projection_perspective(camera.fovy, (float)pe_screen_width()/(float)pe_screen_height(), 1.0f, 1000.0f);
    pe_graphics_view_lookat(camera.position, camera.target, camera.up);
}

peRay pe_get_mouse_ray(HMM_Vec2 mouse, peCamera camera) {
    float window_width_float = (float)pe_screen_width();
    float window_height_float = (float)pe_screen_height();
    HMM_Mat4 projection = pe_matrix_perspective(55.0f, window_width_float/window_height_float, 1.0f, 1000.0f);
    HMM_Mat4 view = HMM_LookAt_RH(camera.position, camera.target, camera.up);

    HMM_Vec3 near_point = pe_unproject_vec3(HMM_V3(mouse.X, mouse.Y, 0.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    HMM_Vec3 far_point = pe_unproject_vec3(HMM_V3(mouse.X, mouse.Y, 1.0f), 0.0f, 0.0f, window_width_float, window_height_float, 0.0f, 1.0f, projection, view);
    HMM_Vec3 direction = HMM_NormV3(HMM_SubV3(far_point, near_point));
    peRay ray = {
        .position = camera.position,
        .direction = direction,
    };
    return ray;
}