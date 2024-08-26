#ifndef PE_GRAPHICS_H
#define PE_GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

#include "math/p_math.h"

#include "HandmadeMath.h"

typedef struct peArena peArena;

#define PE_LIGHT_VECTOR_DEFAULT (pVec3){ 1.0f, -1.0f, 1.0f }

#define PE_COLOR_WHITE (peColor){ 255, 255, 255, 255 }
#define PE_COLOR_GRAY  (peColor){ 127, 127, 127, 255 }
#define PE_COLOR_RED   (peColor){ 255,   0,   0, 255 }
#define PE_COLOR_GREEN (peColor){   0, 255,   0, 255 }
#define PE_COLOR_BLUE  (peColor){   0,   0, 255, 255 }
#define PE_COLOR_BLACK (peColor){   0,   0,   0,   0 }

typedef enum peGraphicsMode {
    peGraphicsMode_2D,
    peGraphicsMode_3D,
    peGraphicsMode_Count
} peGraphicsMode;

typedef enum peMatrixMode {
    peMatrixMode_Projection,
    peMatrixMode_View,
    peMatrixMode_Model,
    peMatrixMode_Count
} peMatrixMode;

typedef struct peGraphics {
    peGraphicsMode mode;
    peMatrixMode matrix_mode;

    pMat4 matrix[peGraphicsMode_Count][peMatrixMode_Count];
    bool matrix_dirty[peGraphicsMode_Count][peMatrixMode_Count];
    bool matrix_model_is_identity[peGraphicsMode_Count];
} peGraphics;
extern peGraphics pe_graphics;

typedef union peColor {
    struct { uint8_t r, g, b, a; };
    struct { uint32_t rgba; };
} peColor;

#if defined(_WIN32)
typedef struct ID3D11ShaderResourceView ID3D11ShaderResourceView;
#elif defined(__linux__)
typedef unsigned int GLuint;
#endif
typedef struct peTexture {
#if defined(_WIN32)
	struct ID3D11ShaderResourceView *texture_resource;
	int width;
	int height;
#elif defined(__linux__)
	GLuint texture_object; // GLuint
	int width;
	int height;
#elif defined(PSP)
	void *data;
	int width;
	int height;
	int power_of_two_width;
	int power_of_two_height;
	bool vram;
	int format;
#endif
} peTexture;

typedef enum pePrimitive {
    pePrimitive_Points,
    pePrimitive_Lines,
    pePrimitive_Triangles,
    pePrimitive_Count
} pePrimitive;

typedef struct peDynamicDrawVertex {
#if defined(_WIN32)
    pVec3 position;
    pVec3 normal;
    pVec2 texcoord;
    uint32_t color;
#elif defined(__PSP__)
    pVec2 texcoord;
    uint32_t color;
    pVec3 normal;
    pVec3 position;
#elif defined(__linux__)
    pVec3 position;
    pVec3 normal;
    pVec2 texcoord;
    uint32_t color;
#endif
} peDynamicDrawVertex;

typedef struct peDynamicDrawBatch {
    int vertex_offset;
    int vertex_count;
    pePrimitive primitive;
    peTexture *texture;
    bool do_lighting; // unused on the PSP
} peDynamicDrawBatch;

#if defined(__PSP__)
#define PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT 512
#define PE_MAX_DYNAMIC_DRAW_BATCH_COUNT 256
#else
#define PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT 1024
#define PE_MAX_DYNAMIC_DRAW_BATCH_COUNT 512
#endif

typedef struct peDynamicDrawState {
    pePrimitive primitive;
    peTexture *texture;
    pVec3 normal;
    pVec2 texcoord;
    peColor color;
    bool do_lighting;

    peDynamicDrawVertex vertex[PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT];
    int vertex_used;

    peDynamicDrawBatch batch[PE_MAX_DYNAMIC_DRAW_BATCH_COUNT];
    int batch_current;
    int batch_drawn_count;
} peDynamicDrawState;
extern peDynamicDrawState dynamic_draw;

typedef struct peCamera {
    pVec3 position;
    pVec3 target;
    pVec3 up;
    float fovy;
} peCamera;

// GENERAL

void pe_graphics_init(peArena *temp_arena, int window_width, int window_height);
void pe_graphics_shutdown(void);
int pe_screen_width(void);
int pe_screen_height(void);
void pe_clear_background(peColor color);
void pe_graphics_frame_begin(void);
void pe_graphics_frame_end(bool vsync);
void pe_graphics_mode_3d_begin(peCamera camera);
void pe_graphics_mode_3d_end(void);

void pe_graphics_set_depth_test(bool enable);
void pe_graphics_set_lighting(bool enable);
void pe_graphics_set_light_vector(pVec3 light_vector);
void pe_graphics_set_diffuse_color(peColor color);

// MATRICES

void pe_graphics_matrix_mode(peMatrixMode mode);
void pe_graphics_matrix_set(pMat4 *matrix);
void pe_graphics_matrix_identity(void);
void pe_graphics_matrix_update(void);

pMat4 pe_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z);
pMat4 pe_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z);

// COLOR

pVec4 pe_color_to_vec4(peColor color);
uint16_t pe_color_to_5650(peColor color);
uint32_t pe_color_to_8888(peColor color);

// TEXTURE

peTexture pe_texture_create(peArena *temp_arena, void *data, int width, int height);
void pe_texture_bind(peTexture texture);
void pe_texture_bind_default(void);

// DYNAMIC DRAW

int pe_graphics_primitive_vertex_count(pePrimitive primitive);

bool pe_graphics_dynamic_draw_vertex_reserve(int count);
void pe_graphics_dynamic_draw_new_batch(void);
void pe_graphics_dynamic_draw_clear(void);
void pe_graphics_dynamic_draw_draw_batches(void);

void pe_graphics_dynamic_draw_set_primitive(pePrimitive primitive);
void pe_graphics_dynamic_draw_set_texture(peTexture *texture);
void pe_graphics_dynamic_draw_set_normal(pVec3 normal);
void pe_graphics_dynamic_draw_set_texcoord(pVec2 texcoord);
void pe_graphics_dynamic_draw_set_color(peColor color);
void pe_graphics_dynamic_draw_do_lighting(bool do_lighting);

void pe_graphics_dynamic_draw_begin_primitive(pePrimitive primitive);
void pe_graphics_dynamic_draw_begin_primitive_textured(pePrimitive primitive, peTexture *texture);

void pe_graphics_dynamic_draw_push_vec3(pVec3 position);
void pe_graphics_dynamic_draw_push_vec2(pVec2 position);

void pe_graphics_draw_point(pVec2 position, peColor color);
void pe_graphics_draw_point_int(int pos_x, int pos_y, peColor color);
void pe_graphics_draw_line(pVec2 start_position, pVec2 end_position, peColor color);
void pe_graphics_draw_rectangle(float x, float y, float width, float height, peColor color);
void pe_graphics_draw_texture(peTexture *texture, float x, float y, peColor tint);

void pe_graphics_draw_point_3D(pVec3 position, peColor color);
void pe_graphics_draw_line_3D(pVec3 start_position, pVec3 end_position, peColor color);
void pe_graphics_draw_grid_3D(int slices, float spacing);

// CAMERA

typedef struct peRay peRay;
peRay pe_get_mouse_ray(pVec2 mouse, peCamera camera);

#endif // PE_GRAPHICS_H