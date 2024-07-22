#include "pe_graphics.h"
#include "pe_graphics_psp.h"
#include "core/pe_core.h"
#include "utility/pe_trace.h"

#include <pspge.h>
#include <pspgu.h>
#include <pspdisplay.h> // sceDisplayWaitVblankStart
#include <pspgum.h>
#include <pspkernel.h> // sceKernelDcacheWritebackInvalidateRange

#include <stdint.h>
#include <string.h>

static peArena edram_arena;
static bool gu_initialized = false;

unsigned int __attribute__((aligned(16))) list[262144];

typedef enum peGraphicsMode {
    peGraphicsMode_2D,
    peGraphicsMode_3D,
    peGraphicsMode_Count,
} peGraphicsMode;

struct peGraphics {
    peGraphicsMode mode;
    peMatrixMode matrix_mode;

    pMat4 matrix[peGraphicsMode_Count][peMatrixMode_Count];
    bool matrix_dirty[peGraphicsMode_Count][peMatrixMode_Count];
    bool matrix_model_is_identity[peGraphicsMode_Count];
} graphics = {0};

static unsigned int bytes_per_pixel(unsigned int psm) {
	switch (psm) {
		case GU_PSM_T4: return 0; // FIXME: It's actually 4 bits
		case GU_PSM_T8: return 1;

		case GU_PSM_5650:
		case GU_PSM_5551:
		case GU_PSM_4444:
		case GU_PSM_T16:
			return 2;

		case GU_PSM_8888:
		case GU_PSM_T32:
			return 4;

		default: return 0;
	}
}

void pe_graphics_init_psp(void) {
	pe_arena_init(&edram_arena, sceGeEdramGetAddr(), sceGeEdramGetSize());

	unsigned int framebuffer_size = PSP_BUFF_W * PSP_SCREEN_H * bytes_per_pixel(GU_PSM_5650);
	void *framebuffer0 = pe_arena_alloc_align(&edram_arena, framebuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();
	void *framebuffer1 = pe_arena_alloc_align(&edram_arena, framebuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();

    unsigned int depthbuffer_size = PSP_BUFF_W * PSP_SCREEN_H * bytes_per_pixel(GU_PSM_4444);
	void *depthbuffer = pe_arena_alloc_align(&edram_arena, depthbuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();

	sceGuInit();

	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_5650, framebuffer0, PSP_BUFF_W);
	sceGuDispBuffer(PSP_SCREEN_W, PSP_SCREEN_H, framebuffer1, PSP_BUFF_W);
	sceGuDepthBuffer(depthbuffer, PSP_BUFF_W);
	sceGuOffset(2048 - (PSP_SCREEN_W/2), 2048 - (PSP_SCREEN_H/2));
	sceGuViewport(2048, 2048, PSP_SCREEN_W, PSP_SCREEN_H);
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, PSP_SCREEN_W, PSP_SCREEN_H);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CCW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();

	graphics.mode = peGraphicsMode_2D;
    graphics.matrix_mode = peMatrixMode_Projection;


    for (int gm = 0; gm < peGraphicsMode_Count; gm += 1) {
        for (int mm = 0; mm < peMatrixMode_Count; mm += 1) {
            graphics.matrix[gm][mm] = p_mat4_i();
            graphics.matrix_dirty[gm][mm] = true;
        }
        graphics.matrix_model_is_identity[gm] = true;
    }

    graphics.matrix[peGraphicsMode_2D][peMatrixMode_Projection] = pe_matrix_orthographic(
        0, (float)pe_screen_width(),
        (float)pe_screen_height(), 0,
        0.0f, 1000.0f
    );

    pe_graphics_matrix_update();

    gu_initialized = true;
}

void pe_graphics_shutdown_psp(void) {
	sceGuTerm();
}


void pe_graphics_mode_3d_begin(peCamera camera) {
    pe_graphics_dynamic_draw_flush();

    PE_ASSERT(graphics.mode == peGraphicsMode_2D);
    graphics.mode = peGraphicsMode_3D;

    pe_graphics_matrix_mode(peMatrixMode_Projection);
    float aspect_ratio = (float)pe_screen_width()/(float)pe_screen_height();
    pMat4 matrix_perspective = pe_matrix_perspective(camera.fovy, aspect_ratio, 1.0f, 1000.0f);
    pe_graphics_matrix_set(&matrix_perspective);

    pe_graphics_matrix_mode(peMatrixMode_View);
    pMat4 matrix_view = p_look_at_rh(camera.position, camera.target, camera.up);
    pe_graphics_matrix_set(&matrix_view);

    pe_graphics_matrix_mode(peMatrixMode_Model);
    pe_graphics_matrix_identity();

    pe_graphics_matrix_update();

    sceGuEnable(GU_DEPTH_TEST);
}

void pe_graphics_mode_3d_end(void) {
    pe_graphics_dynamic_draw_flush();

    PE_ASSERT(graphics.mode == peGraphicsMode_3D);
    graphics.mode = peGraphicsMode_2D;

    pe_graphics_matrix_mode(peMatrixMode_View);
    pe_graphics_matrix_identity();
    pe_graphics_matrix_mode(peMatrixMode_Model);
    pe_graphics_matrix_identity();
    graphics.matrix_dirty[peGraphicsMode_2D][peMatrixMode_Projection] = true;

    pe_graphics_matrix_update();

    sceGuDisable(GU_DEPTH_TEST);
}

void pe_graphics_matrix_mode(peMatrixMode mode) {
    PE_ASSERT(mode >= 0);
    PE_ASSERT(mode < peMatrixMode_Count);
    graphics.matrix_mode = mode;
}

void pe_graphics_matrix_set(pMat4 *matrix) {
    memcpy(&graphics.matrix[graphics.mode][graphics.matrix_mode], matrix, sizeof(pMat4));
    graphics.matrix_dirty[graphics.mode][graphics.matrix_mode] = true;
    if (graphics.matrix_mode == peMatrixMode_Model) {
        graphics.matrix_model_is_identity[graphics.mode] = false;
    }
}

void pe_graphics_matrix_identity(void) {
    graphics.matrix[graphics.mode][graphics.matrix_mode] = p_mat4_i();
    graphics.matrix_dirty[graphics.mode][graphics.matrix_mode] = true;
    if (graphics.matrix_mode == peMatrixMode_Model) {
        graphics.matrix_model_is_identity[graphics.mode] = true;
    }
}

void pe_graphics_matrix_update(void) {
    int gu_matrix_modes[peMatrixMode_Count];
    {
        gu_matrix_modes[peMatrixMode_Projection] = GU_PROJECTION;
        gu_matrix_modes[peMatrixMode_View] = GU_VIEW;
        gu_matrix_modes[peMatrixMode_Model] = GU_MODEL;
    }
    for (int matrix_mode = 0; matrix_mode < peMatrixMode_Count; matrix_mode += 1) {
        if (graphics.matrix_dirty[graphics.mode][matrix_mode]) {
            pMat4 *matrix = &graphics.matrix[graphics.mode][matrix_mode];
            sceGumMatrixMode(gu_matrix_modes[matrix_mode]);
            sceGumLoadMatrix(&matrix->_sce);
            graphics.matrix_dirty[graphics.mode][matrix_mode] = false;
        }
    }
    sceGumUpdateMatrix();
}

static unsigned int closest_greater_power_of_two(unsigned int value) {
	if (value == 0 || value > (1 << 31))
		return 0;
	unsigned int power_of_two = 1;
	while (power_of_two < value)
		power_of_two <<= 1;
	return power_of_two;
}

static void copy_texture_data(void *dest, const void *src, const int pW, const int width, const int height) {
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            ((unsigned int*)dest)[x + y * pW] = ((unsigned int *)src)[x + y * width];
        }
    }
}

static void swizzle_fast(u8 *out, const u8 *in, unsigned int width, unsigned int height) {
	unsigned int width_blocks = (width / 16);
	unsigned int height_blocks = (height / 8);

	unsigned int src_pitch = (width - 16) / 4;
	unsigned int src_row = width * 8;

	const u8 *ysrc = in;
	u32 *dst = (u32 *)out;

	for (unsigned int blocky = 0; blocky < height_blocks; ++blocky) {
		const u8 *xsrc = ysrc;
		for (unsigned int blockx = 0; blockx < width_blocks; ++blockx) {
			const u32 *src = (u32 *)xsrc;
			for (unsigned int j = 0; j < 8; ++j)
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
}

peTexture pe_texture_create_psp(peArena *temp_arena, void *data, int width, int height, int format) {
	int power_of_two_width = closest_greater_power_of_two(width);
	int power_of_two_height = closest_greater_power_of_two(height);
	unsigned int size = power_of_two_width * power_of_two_height * bytes_per_pixel(format);

	peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);

	unsigned int *data_buffer = pe_arena_alloc_align(temp_arena, size, 16);
	copy_texture_data(data_buffer, data, power_of_two_width, width, height);

    unsigned int* swizzled_pixels = pe_arena_alloc_align(&edram_arena, size, 16);
    swizzle_fast((u8*)swizzled_pixels, data, power_of_two_width*bytes_per_pixel(format), power_of_two_height);
	sceKernelDcacheWritebackRange(swizzled_pixels, size);

	pe_arena_temp_end(temp_arena_memory);

	peTexture texture = {
		.data = swizzled_pixels,
		.width = width,
		.height = height,
		.power_of_two_width = power_of_two_width,
		.power_of_two_height = power_of_two_height,
		.format = format
	};
    return texture;
}

typedef struct peDynamicDrawVertex {
    pVec2 texcoord;
    uint32_t color;
    pVec3 normal;
    pVec3 position;
} peDynamicDrawVertex;

typedef struct peDynamicDrawBatch {
    int vertex_offset;
    int vertex_count;
    int primitive;
    // int vertex_type;
    peTexture *texture;
} peDynamicDrawBatch;

#define PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT 512
#define PE_MAX_DYNAMIC_DRAW_BATCH_COUNT 256

struct peDynamicDrawState {
    int primitive;
    peTexture *texture;
    pVec2 texcoord;
    peColor color;
    pVec3 normal;

    peDynamicDrawVertex vertex[PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT];
    int vertex_used;

    peDynamicDrawBatch batch[PE_MAX_DYNAMIC_DRAW_BATCH_COUNT];
    int batch_current;
} dynamic_draw = {0};

void pe_graphics_dynamic_draw_flush(void) {
    pe_graphics_dynamic_draw_end_batch();
    if (dynamic_draw.vertex_used > 0) {
        peMatrixMode old_matrix_mode = graphics.matrix_mode;
        pMat4 old_matrix_model = graphics.matrix[graphics.mode][peMatrixMode_Model];
        bool old_matrix_model_is_identity = graphics.matrix_model_is_identity[graphics.mode];

        pe_graphics_matrix_mode(peMatrixMode_Model);
        pe_graphics_matrix_identity();
        pe_graphics_matrix_update();

        sceKernelDcacheWritebackRange(dynamic_draw.vertex, sizeof(peDynamicDrawVertex)*PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT);

        for (int b = 0; b < dynamic_draw.batch_current; b += 1) {
            if (dynamic_draw.batch[b].texture) {
                pe_texture_bind(*dynamic_draw.batch[b].texture);
            } else {
                pe_texture_bind_default();
            }

            int vertex_type = (
                GU_TEXTURE_32BITF|GU_COLOR_8888|GU_NORMAL_32BITF|GU_VERTEX_32BITF|
                (graphics.mode == peGraphicsMode_2D ? GU_TRANSFORM_2D : GU_TRANSFORM_3D)
            );
            sceGuDrawArray(
                dynamic_draw.batch[b].primitive,
                vertex_type,
                dynamic_draw.batch[b].vertex_count,
                NULL,
                dynamic_draw.vertex + dynamic_draw.batch[b].vertex_offset
            );
        }

        pe_graphics_matrix_set(&old_matrix_model);
        pe_graphics_matrix_update();
        pe_graphics_matrix_mode(old_matrix_mode);
        graphics.matrix_model_is_identity[graphics.mode] = old_matrix_model_is_identity;
    }

    dynamic_draw.vertex_used = 0;
    dynamic_draw.batch_current = 0;
    dynamic_draw.batch[0].vertex_count = 0;
}

void pe_graphics_dynamic_draw_end_batch(void) {
    PE_ASSERT(dynamic_draw.batch_current < PE_MAX_DYNAMIC_DRAW_BATCH_COUNT);
    if (dynamic_draw.batch[dynamic_draw.batch_current].vertex_count > 0) {
        dynamic_draw.batch_current += 1;
    }
}

static void pe_graphics_dynamic_draw_new_batch(void) {
    pe_graphics_dynamic_draw_end_batch();
    if (dynamic_draw.batch_current == PE_MAX_DYNAMIC_DRAW_BATCH_COUNT) {
        pe_graphics_dynamic_draw_flush();
    }
    dynamic_draw.batch[dynamic_draw.batch_current].primitive = dynamic_draw.primitive;
    dynamic_draw.batch[dynamic_draw.batch_current].texture = dynamic_draw.texture;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_offset = dynamic_draw.vertex_used;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_count = 0;
}

static void pe_graphics_dynamic_draw_set_texture(peTexture *texture) {
    dynamic_draw.texture = texture;
    if (dynamic_draw.batch[dynamic_draw.batch_current].texture != texture) {
        pe_graphics_dynamic_draw_new_batch();
    }
}

static void pe_graphics_dynamic_draw_set_normal(pVec3 normal) {
    dynamic_draw.normal = normal;
}

static void pe_graphics_dynamic_draw_set_texcoord(pVec2 texcoord) {
    dynamic_draw.texcoord = texcoord;
}

static void pe_graphics_dynamic_draw_set_color(peColor color) {
    dynamic_draw.color = color;
}

static void pe_graphics_dynamic_draw_set_primitive(int primitive) {
    dynamic_draw.primitive = primitive;
    if (dynamic_draw.batch[dynamic_draw.batch_current].primitive != primitive) {
        pe_graphics_dynamic_draw_new_batch();
    }
}

static void pe_graphics_dynamic_draw_begin_primitive_textured(int primitive, peTexture *texture) {
    pe_graphics_dynamic_draw_set_primitive(primitive);
    pe_graphics_dynamic_draw_set_texture(texture);
    pe_graphics_dynamic_draw_set_normal((pVec3){ 0.0f });
    pe_graphics_dynamic_draw_set_texcoord((pVec2){ 0.0f });
    pe_graphics_dynamic_draw_set_color(PE_COLOR_WHITE);
}

static void pe_graphics_dynamic_draw_begin_primitive(int primitive) {
    pe_graphics_dynamic_draw_begin_primitive_textured(primitive, NULL);
}

static int pe_graphics_primitive_vertex_count(int primitive) {
    int result;
    switch (primitive) {
        case GU_POINTS: result = 1; break;
        case GU_LINES: result = 2; break;
        case GU_TRIANGLES: result = 3; break;
        default:
            PE_PANIC_MSG("unsupported primitive: %d\n", primitive);
            break;
    }
    return result;
}

static bool pe_graphics_dynamic_draw_vertex_reserve(int count) {
    bool flushed = false;
    if (dynamic_draw.vertex_used + count > PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT) {
        pe_graphics_dynamic_draw_flush();
        flushed = true;
    }
    return flushed;
}

static void pe_graphics_dynamic_draw_push_vec3(pVec3 position) {
    if (!graphics.matrix_model_is_identity[graphics.mode]) {
        pMat4 *matrix = &graphics.matrix[graphics.mode][peMatrixMode_Model];
        pVec4 position_vec4 = p_vec4v(position, 1.0f);
        pVec4 transformed_position = p_mat4_mul_vec4(*matrix, position_vec4);
        position = transformed_position.xyz;
    }
    int batch_primitive = dynamic_draw.batch[dynamic_draw.batch_current].primitive;
    int batch_vertex_count = dynamic_draw.batch[dynamic_draw.batch_current].vertex_count;
    int primitive_vertex_count = pe_graphics_primitive_vertex_count(batch_primitive);
    int primitive_vertex_left = primitive_vertex_count - (batch_vertex_count % primitive_vertex_count);
    pe_graphics_dynamic_draw_vertex_reserve(primitive_vertex_left);
    peDynamicDrawVertex *vertex = &dynamic_draw.vertex[dynamic_draw.vertex_used];
    vertex->texcoord = dynamic_draw.texcoord;
    vertex->color = pe_color_to_8888(dynamic_draw.color);
    vertex->normal = dynamic_draw.normal;
    vertex->position = position;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_count += 1;
    dynamic_draw.vertex_used += 1;
}

static void pe_graphics_dynamic_draw_push_vec2(pVec2 position) {
    pVec3 vec3 = p_vec3(position.x, position.y, 0.0f);
    pe_graphics_dynamic_draw_push_vec3(vec3);
}

void pe_graphics_draw_point(pVec2 position, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(GU_POINTS);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec2(position);
}

void pe_graphics_draw_point_int(int pos_x, int pos_y, peColor color) {
    pVec2 position = p_vec2((float)pos_x, (float)pos_y);
    pe_graphics_draw_point(position, color);
}

void pe_graphics_draw_line(pVec2 start_position, pVec2 end_position, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(GU_LINES);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec2(start_position);
    pe_graphics_dynamic_draw_push_vec2(end_position);
}

void pe_graphics_draw_rectangle(float x, float y, float width, float height, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(GU_TRIANGLES);
    pe_graphics_dynamic_draw_set_color(color);

    pVec2 positions[4] = {
        p_vec2(x, y), // top left
        p_vec2(x + width, y), // top right
        p_vec2(x, y + height), // bottom left
        p_vec2(x + width, y + height) // bottom_right
    };
    int indices[6] = { 0, 2, 3, 0, 3, 1 };

    for (int i = 0; i < 6; i += 1) {
        pe_graphics_dynamic_draw_push_vec2(positions[indices[i]]);
    }
}

void pe_graphics_draw_texture(peTexture *texture, float x, float y, peColor tint) {
    PE_ASSERT(texture != NULL);
    pe_graphics_dynamic_draw_begin_primitive_textured(GU_TRIANGLES, texture);
    pe_graphics_dynamic_draw_set_color(tint);

    pVec2 positions[4] = {
        p_vec2(x, y), // top left
        p_vec2(x + texture->width, y), // top right
        p_vec2(x, y + texture->height), // bottom left
        p_vec2(x + texture->width, y + texture->height) // bottom_right
    };
    pVec2 texcoords[4] = {
        p_vec2(0.0f, 0.0f), // top left
        p_vec2((float)texture->width, 0.0f), // top right
        p_vec2(0.0f, (float)texture->height), // bottom left
        p_vec2((float)texture->width, (float)texture->height), // bottom right
    };
    int indices[6] = { 0, 2, 3, 0, 3, 1 };

    for (int i = 0; i < 6; i += 1) {
        pe_graphics_dynamic_draw_set_texcoord(texcoords[indices[i]]);
        pe_graphics_dynamic_draw_push_vec2(positions[indices[i]]);
    }
}

void pe_graphics_draw_point_3D(pVec3 position, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(GU_POINTS);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec3(position);
}

void pe_graphics_draw_line_3D(pVec3 start_position, pVec3 end_position, peColor color) {
    pe_graphics_dynamic_draw_begin_primitive(GU_LINES);
    pe_graphics_dynamic_draw_set_color(color);
    pe_graphics_dynamic_draw_push_vec3(start_position);
    pe_graphics_dynamic_draw_push_vec3(end_position);
}
