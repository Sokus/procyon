#ifndef PE_GRAPHICS_H
#define PE_GRAPHICS_H

#include "HandmadeMath.h"

#include <stdint.h>
#include <stdbool.h>

struct peArena;

// GENERAL

void pe_graphics_init(struct peArena *temp_arena, int window_width, int window_height, const char *window_name);
void pe_graphics_shutdown(void);
void pe_graphics_frame_begin(void);
void pe_graphics_frame_end(bool vsync);
void pe_graphics_projection_perspective(float fovy, float aspect_ratio, float near_z, float far_z);
void pe_graphics_view_lookat(HMM_Vec3 eye, HMM_Vec3 target, HMM_Vec3 up);

int pe_screen_width(void);
int pe_screen_height(void);

union peColor;
void pe_clear_background(union peColor color);

HMM_Mat4 pe_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z);

// COLOR

typedef union peColor {
    struct { uint8_t r, g, b, a; };
    struct { uint32_t rgba; };
} peColor;

union HMM_Vec4;
union HMM_Vec4 pe_color_to_vec4(peColor color);
uint16_t pe_color_to_5650(peColor color);
uint32_t pe_color_to_8888(peColor color);

#define PE_COLOR_WHITE (peColor){ 255, 255, 255, 255 }
#define PE_COLOR_GRAY  (peColor){ 127, 127, 127, 255 }
#define PE_COLOR_RED   (peColor){ 255,   0,   0, 255 }
#define PE_COLOR_GREEN (peColor){   0, 255,   0, 255 }
#define PE_COLOR_BLUE  (peColor){   0,   0, 255, 255 }
#define PE_COLOR_BLACK (peColor){   0,   0,   0,   0 }

// TEXTURES

typedef struct peTexture {
#if defined(_WIN32)
	void *texture_resource; // ID3D11ShaderResourceView
#endif
#if defined(__linux__)
	unsigned int texture_object; // GLuint
#endif
#if defined(PSP)
	void *data;
	int width;
	int height;
	int power_of_two_width;
	int power_of_two_height;
	bool vram;
	int format;
#endif
} peTexture;

peTexture pe_texture_create(struct peArena *temp_arena, void *data, int width, int height, int format);
void pe_texture_bind(peTexture texture);
void pe_texture_bind_default(void);

// CAMERA

typedef struct peCamera {
    HMM_Vec3 position;
    HMM_Vec3 target;
    HMM_Vec3 up;
    float fovy;
} peCamera;

void pe_camera_update(peCamera camera);
struct peRay;
struct peRay pe_get_mouse_ray(HMM_Vec2 mouse, peCamera camera);


#endif // PE_GRAPHICS_H