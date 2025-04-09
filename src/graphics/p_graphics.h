#ifndef P_GRAPHICS_H
#define P_GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

#include "math/p_math.h"

#include "HandmadeMath.h"

typedef struct pArena pArena;

#define P_LIGHT_VECTOR_DEFAULT (pVec3){ 1.0f, -1.0f, 1.0f }

#define P_COLOR_WHITE (pColor){ 255, 255, 255, 255 }
#define P_COLOR_GRAY  (pColor){ 127, 127, 127, 255 }
#define P_COLOR_RED   (pColor){ 255,   0,   0, 255 }
#define P_COLOR_GREEN (pColor){   0, 255,   0, 255 }
#define P_COLOR_BLUE  (pColor){   0,   0, 255, 255 }
#define P_COLOR_BLACK (pColor){   0,   0,   0,   0 }

typedef enum pGraphicsMode {
    pGraphicsMode_2D,
    pGraphicsMode_3D,
    pGraphicsMode_Count
} pGraphicsMode;

typedef enum pMatrixMode {
    pMatrixMode_Projection,
    pMatrixMode_View,
    pMatrixMode_Model,
    pMatrixMode_Count
} pMatrixMode;

typedef struct pGraphics {
    pGraphicsMode mode;
    pMatrixMode matrix_mode;

    pMat4 matrix[pGraphicsMode_Count][pMatrixMode_Count];
    bool matrix_dirty[pGraphicsMode_Count][pMatrixMode_Count];
    bool matrix_model_is_identity[pGraphicsMode_Count];
} pGraphics;
extern pGraphics p_graphics;

typedef union pColor {
    struct { uint8_t r, g, b, a; };
    struct { uint32_t rgba; };
} pColor;

typedef struct pRect {
    float x;
    float y;
    float width;
    float height;
} pRect;

// pTexture
#if defined(_WIN32)
typedef struct ID3D11ShaderResourceView ID3D11ShaderResourceView;
typedef struct pTexture {
	struct ID3D11ShaderResourceView *texture_resource;
	int width;
	int height;
} pTexture;
#elif defined(__linux__)
typedef unsigned int GLuint;
typedef struct pTexture {
	GLuint texture_object;
	int width;
	int height;
} pTexture;
#elif defined(__PSP__)
typedef struct pTexture {
    void *data;
    int format;
    int width;
    int height;
    int power_of_two_width;
    int power_of_two_height;
    bool swizzle;
    void *palette;
    int palette_length;
    int palette_padded_length;
    int palette_format;
} pTexture;
#endif

typedef enum pPrimitive {
    pPrimitive_Points,
    pPrimitive_Lines,
    pPrimitive_Triangles,
    pPrimitive_TriangleStrip,
    pPrimitive_Count
} pPrimitive;

typedef struct pDynamicDrawVertex {
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
} pDynamicDrawVertex;

typedef struct pDynamicDrawBatch {
    int vertex_offset;
    int vertex_count;
    pPrimitive primitive;
    pTexture *texture;
    bool do_lighting; // unused on the PSP
} pDynamicDrawBatch;

#if defined(__PSP__)
#define P_MAX_DYNAMIC_DRAW_VERTEX_COUNT 2024
#define P_MAX_DYNAMIC_DRAW_BATCH_COUNT 256
#else
#define P_MAX_DYNAMIC_DRAW_VERTEX_COUNT 2024
#define P_MAX_DYNAMIC_DRAW_BATCH_COUNT 512
#endif

typedef struct pDynamicDrawStats {
    int last_vertex_used;
    int peak_vertex_used;
    int last_batch_count;
    int peak_batch_count;
} pDynamicDrawStats;

typedef struct pDynamicDrawState {
    pPrimitive primitive;
    pTexture *texture;
    pVec3 normal;
    pVec2 texcoord;
    pColor color;
    bool do_lighting;

    pDynamicDrawVertex *vertex;
    int vertex_used;

    pDynamicDrawBatch *batch;
    int batch_current;
    int batch_drawn_count;

    pDynamicDrawStats stats;
} pDynamicDrawState;
extern pDynamicDrawState dynamic_draw;

typedef struct pCamera {
    pVec3 position;
    pVec3 target;
    pVec3 up;
    float fovy;
} pCamera;

// GENERAL

void p_graphics_init(int window_width, int window_height);
void p_graphics_shutdown(void);
void p_graphics_set_framebuffer_size(int width, int height);
int p_screen_width(void);
int p_screen_height(void);
void p_clear_background(pColor color);
void p_graphics_frame_begin(void);
void p_graphics_frame_end(void);
void p_graphics_mode_3d_begin(pCamera camera);
void p_graphics_mode_3d_end(void);

void p_graphics_set_depth_test(bool enable);
void p_graphics_set_lighting(bool enable);
void p_graphics_set_light_vector(pVec3 light_vector);
void p_graphics_set_diffuse_color(pColor color);

// MATRICES

void p_graphics_matrix_mode(pMatrixMode mode);
void p_graphics_matrix_set(pMat4 *matrix);
void p_graphics_matrix_identity(void);
void p_graphics_matrix_update(void);

pMat4 p_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z);
pMat4 p_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z);

// COLOR

pVec4 p_color_to_vec4(pColor color);
uint16_t p_color_to_5650(pColor color);
uint32_t p_color_to_8888(pColor color);

// TEXTURE

typedef struct pbmFile pbmFile;
pTexture p_texture_create(void *data, int width, int height);
pTexture p_texture_create_pbm(pbmFile *pbm);
void p_texture_bind(pTexture texture);
void p_texture_bind_default(void);

// DYNAMIC DRAW

int p_graphics_primitive_vertex_count(pPrimitive primitive);
bool p_graphics_primitive_can_continue(pPrimitive primitive);

bool p_graphics_dynamic_draw_vertex_reserve(int count);
void p_graphics_dynamic_draw_new_batch(void);
void p_graphics_dynamic_draw_clear(void);
pDynamicDrawStats p_graphics_dynamic_draw_stats(void);
void p_graphics_dynamic_draw_draw_batches(void);

void p_graphics_dynamic_draw_set_primitive(pPrimitive primitive);
void p_graphics_dynamic_draw_set_texture(pTexture *texture);
void p_graphics_dynamic_draw_set_normal(pVec3 normal);
void p_graphics_dynamic_draw_set_texcoord(pVec2 texcoord);
void p_graphics_dynamic_draw_set_color(pColor color);
void p_graphics_dynamic_draw_do_lighting(bool do_lighting);

void p_graphics_dynamic_draw_begin_primitive(pPrimitive primitive);
void p_graphics_dynamic_draw_begin_primitive_textured(pPrimitive primitive, pTexture *texture);

void p_graphics_dynamic_draw_push_vec3(pVec3 position);
void p_graphics_dynamic_draw_push_vec2(pVec2 position);

void p_graphics_draw_point(pVec2 position, pColor color);
void p_graphics_draw_point_int(int pos_x, int pos_y, pColor color);
void p_graphics_draw_line(pVec2 start_position, pVec2 end_position, pColor color);
void p_graphics_draw_rectangle(float x, float y, float width, float height, pColor color);
void p_graphics_draw_texture(pTexture *texture, float x, float y, float scale, pColor tint);
void p_graphics_draw_texture_ex(pTexture *texture, pRect src, pRect dst, pColor tint);

void p_graphics_draw_point_3D(pVec3 position, pColor color);
void p_graphics_draw_line_3D(pVec3 start_position, pVec3 end_position, pColor color);
void p_graphics_draw_grid_3D(int slices, float spacing);

// CAMERA

typedef struct pRay pRay;
pRay p_get_mouse_ray(pVec2 mouse, pCamera camera);

#endif // P_GRAPHICS_H
