#ifndef PE_GRAPHICS_H
#define PE_GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

#include "math/p_math.h"

#include "HandmadeMath.h"

struct peArena;

// GENERAL

#define PE_LIGHT_VECTOR_DEFAULT (pVec3){ 1.0f, -1.0f, 1.0f }

void pe_graphics_init(struct peArena *temp_arena, int window_width, int window_height);
void pe_graphics_shutdown(void);
void pe_graphics_frame_begin(void);
void pe_graphics_frame_end(bool vsync);

struct peCamera;
void pe_graphics_mode_3d_begin(struct peCamera camera);
void pe_graphics_mode_3d_end(void);

int pe_screen_width(void);
int pe_screen_height(void);

union peColor;
void pe_clear_background(union peColor color);

// MATRICES

typedef enum peMatrixMode {
    peMatrixMode_Projection,
    peMatrixMode_View,
    peMatrixMode_Model,
    peMatrixMode_Count
} peMatrixMode;

void pe_graphics_matrix_mode(peMatrixMode mode);
void pe_graphics_matrix_set(pMat4 *matrix);
void pe_graphics_matrix_identity(void);
void pe_graphics_matrix_update(void);

// TODO: remove in favor of matrix modes
void pe_graphics_matrix_projection(pMat4 *matrix);
void pe_graphics_matrix_view(pMat4 *matrix);

pMat4 pe_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z);
pMat4 pe_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z);

// DYNAMIC DRAW

struct peTexture;
void pe_graphics_draw_point(pVec2 position, union peColor color);
void pe_graphics_draw_point_int(int pos_x, int pos_y, union peColor color);
void pe_graphics_draw_line(pVec2 start_position, pVec2 end_position, union peColor color);
void pe_graphics_draw_rectangle(float x, float y, float width, float height, union peColor color);
void pe_graphics_draw_texture(struct peTexture *texture, float x, float y, union peColor tint);

void pe_graphics_draw_point_3D(pVec3 position, union peColor color);
void pe_graphics_draw_line_3D(pVec3 start_position, pVec3 end_position, union peColor color);

// COLOR

typedef union peColor {
    struct { uint8_t r, g, b, a; };
    struct { uint32_t rgba; };
} peColor;

pVec4 pe_color_to_vec4(peColor color);
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
	int width;
	int height;
#endif
#if defined(__linux__)
	unsigned int texture_object; // GLuint
	int width;
	int height;
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
void pe_texture_disable(void);

// CAMERA

typedef struct peCamera {
    pVec3 position;
    pVec3 target;
    pVec3 up;
    float fovy;
} peCamera;

void pe_camera_update(peCamera camera);
struct peRay;
struct peRay pe_get_mouse_ray(pVec2 mouse, peCamera camera);


#endif // PE_GRAPHICS_H