#include "pe_graphics.h"
#include "pe_graphics_psp.h"
#include "core/p_assert.h"
#include "core/p_arena.h"
#include "utility/pe_trace.h"

#include <pspge.h>
#include <pspgu.h>
#include <pspdisplay.h> // sceDisplayWaitVblankStart
#include <pspgum.h>
#include <pspkernel.h> // sceKernelDcacheWritebackInvalidateRange

#include <stdint.h>
#include <string.h>

#define PSP_BUFF_W (512)
#define PSP_SCREEN_W (480)
#define PSP_SCREEN_H (272)

static peArena edram_arena;
static bool gu_initialized = false;

static unsigned int __attribute__((aligned(16))) list[262144];

//
// GENERAL IMPLEMENTATIONS
//

void pe_graphics_init(peArena *, int, int) {
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
    sceGuEnable(GU_BLEND); // TODO: verify performance impact
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();

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
        0, (float)pe_screen_width(),
        (float)pe_screen_height(), 0,
        0.0f, 1000.0f
    );

    pe_graphics_matrix_update();

    gu_initialized = true;
}

void pe_graphics_shutdown(void) {
	sceGuTerm();
}

int pe_screen_width(void) {
    return PSP_SCREEN_W;
}

int pe_screen_height(void) {
    return PSP_SCREEN_H;
}

void pe_clear_background(peColor color) {
	sceGuClearColor(pe_color_to_8888(color));
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
}

void pe_graphics_frame_begin(void) {
    sceGuStart(GU_DIRECT, list);
}

void pe_graphics_frame_end(bool vsync) {
	PE_TRACE_FUNCTION_BEGIN();
	pe_graphics_dynamic_draw_draw_batches();
    sceGuFinish();
	peTraceMark tm_sync = PE_TRACE_MARK_BEGIN("sceGuSync");
    sceGuSync(0, 0);
	PE_TRACE_MARK_END(tm_sync);
    if (vsync) {
        sceDisplayWaitVblankStart();
    }
    sceGuSwapBuffers();
    pe_graphics_dynamic_draw_clear();
	PE_TRACE_FUNCTION_END();
}

void pe_graphics_set_depth_test(bool enable) {
    if (enable) {
        sceGuEnable(GU_DEPTH_TEST);
    } else {
        sceGuDisable(GU_DEPTH_TEST);
    }
}

void pe_graphics_set_lighting(bool enable) {

}

void pe_graphics_set_light_vector(pVec3 light_vector) {

}

void pe_graphics_set_diffuse_color(peColor color) {
    // TODO: change only the diffuse color and not everything
    sceGuColor(color.rgba);
}

void pe_graphics_matrix_update(void) {
    int gu_matrix_modes[peMatrixMode_Count];
    {
        gu_matrix_modes[peMatrixMode_Projection] = GU_PROJECTION;
        gu_matrix_modes[peMatrixMode_View] = GU_VIEW;
        gu_matrix_modes[peMatrixMode_Model] = GU_MODEL;
    }
    for (int matrix_mode = 0; matrix_mode < peMatrixMode_Count; matrix_mode += 1) {
        if (pe_graphics.matrix_dirty[pe_graphics.mode][matrix_mode]) {
            pMat4 *matrix = &pe_graphics.matrix[pe_graphics.mode][matrix_mode];
            sceGumMatrixMode(gu_matrix_modes[matrix_mode]);
            sceGumLoadMatrix(&matrix->_sce);
            pe_graphics.matrix_dirty[pe_graphics.mode][matrix_mode] = false;
        }
    }
    sceGumUpdateMatrix();
}

pMat4 pe_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
	return p_perspective_rh_no(fovy * P_DEG2RAD, aspect_ratio, near_z, far_z);
}

pMat4 pe_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
    return p_orthographic_rh_no(left, right, bottom, top, near_z, far_z);
}

peTexture pe_texture_create(peArena *temp_arena, void *data, int width, int height) {
    const int format = GU_PSM_8888;
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

void pe_texture_bind(peTexture texture) {
    PE_TRACE_FUNCTION_BEGIN();
	sceGuEnable(GU_TEXTURE_2D);
 	sceGuTexMode(texture.format, 0, 0, 1);
	sceGuTexImage(0, texture.power_of_two_width, texture.power_of_two_height, texture.power_of_two_width, texture.data);
	//sceGuTexEnvColor(0x00FFFFFF);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f);
	PE_TRACE_FUNCTION_END();
}

void pe_texture_bind_default(void) {
	sceGuDisable(GU_TEXTURE_2D);
}

void pe_graphics_dynamic_draw_draw_batches(void) {
    if (dynamic_draw.vertex_used > 0) {
        peMatrixMode old_matrix_mode = pe_graphics.matrix_mode;
        pMat4 old_matrix_model = pe_graphics.matrix[pe_graphics.mode][peMatrixMode_Model];
        bool old_matrix_model_is_identity = pe_graphics.matrix_model_is_identity[pe_graphics.mode];

        pe_graphics_matrix_mode(peMatrixMode_Model);
        pe_graphics_matrix_identity();
        pe_graphics_matrix_update();

        sceKernelDcacheWritebackRange(dynamic_draw.vertex, sizeof(peDynamicDrawVertex)*PE_MAX_DYNAMIC_DRAW_VERTEX_COUNT);

        for (int b = dynamic_draw.batch_drawn_count; b <= dynamic_draw.batch_current; b += 1) {
            P_ASSERT(dynamic_draw.batch[b].vertex_count > 0);

            if (dynamic_draw.batch[b].texture) {
                pe_texture_bind(*dynamic_draw.batch[b].texture);
            } else {
                pe_texture_bind_default();
            }

            int primitive_sce;
            switch (dynamic_draw.batch[b].primitive) {
                case pePrimitive_Points: primitive_sce = GU_POINTS; break;
                case pePrimitive_Lines: primitive_sce = GU_LINES; break;
                case pePrimitive_Triangles: primitive_sce = GU_TRIANGLES; break;
                default: break;
            }
            int vertex_type = (
                GU_TEXTURE_32BITF|GU_COLOR_8888|GU_NORMAL_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_3D
            );
            sceGuDrawArray(
                primitive_sce,
                vertex_type,
                dynamic_draw.batch[b].vertex_count,
                NULL,
                dynamic_draw.vertex + dynamic_draw.batch[b].vertex_offset
            );
        }

        pe_graphics_matrix_set(&old_matrix_model);
        pe_graphics_matrix_update();
        pe_graphics_matrix_mode(old_matrix_mode);
        pe_graphics.matrix_model_is_identity[pe_graphics.mode] = old_matrix_model_is_identity;

        dynamic_draw.batch_drawn_count = dynamic_draw.batch_current + 1;
    }
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

//
// INTERNAL IMPLEMENTATIONS
//

unsigned int bytes_per_pixel(unsigned int psm) {
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

unsigned int closest_greater_power_of_two(unsigned int value) {
	if (value == 0 || value > (1 << 31))
		return 0;
	unsigned int power_of_two = 1;
	while (power_of_two < value)
		power_of_two <<= 1;
	return power_of_two;
}

void copy_texture_data(void *dest, const void *src, const int pW, const int width, const int height) {
    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            ((unsigned int*)dest)[x + y * pW] = ((unsigned int *)src)[x + y * width];
        }
    }
}

void swizzle_fast(u8 *out, const u8 *in, unsigned int width, unsigned int height) {
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
