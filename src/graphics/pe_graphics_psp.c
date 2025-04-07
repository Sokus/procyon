#include "pe_graphics.h"
#include "pe_graphics_psp.h"
#include "core/p_assert.h"
#include "core/p_arena.h"
#include "core/p_heap.h"
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

static pArena edram_arena;
static bool gu_initialized = false;

static unsigned int __attribute__((aligned(16))) list[262144];

//
// GENERAL IMPLEMENTATIONS
//

void p_graphics_init(pArena *, int, int) {
	p_arena_init(&edram_arena, sceGeEdramGetAddr(), sceGeEdramGetSize());

	unsigned int framebuffer_size = p_pixel_bytes(PSP_BUFF_W*PSP_SCREEN_H, GU_PSM_5650);
	void *framebuffer0 = p_arena_alloc_align(&edram_arena, framebuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();
	void *framebuffer1 = p_arena_alloc_align(&edram_arena, framebuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();

    unsigned int depthbuffer_size = p_pixel_bytes(PSP_BUFF_W*PSP_SCREEN_H, GU_PSM_4444);
	void *depthbuffer = p_arena_alloc_align(&edram_arena, depthbuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();

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

	p_graphics.mode = pGraphicsMode_2D;
    p_graphics.matrix_mode = pMatrixMode_Projection;

    for (int gm = 0; gm < pGraphicsMode_Count; gm += 1) {
        for (int mm = 0; mm < pMatrixMode_Count; mm += 1) {
            p_graphics.matrix[gm][mm] = p_mat4_i();
            p_graphics.matrix_dirty[gm][mm] = true;
        }
        p_graphics.matrix_model_is_identity[gm] = true;
    }

    p_graphics.matrix[pGraphicsMode_2D][pMatrixMode_Projection] = p_matrix_orthographic(
        0, (float)p_screen_width(),
        (float)p_screen_height(), 0,
        0.0f, 1000.0f
    );

    p_graphics_matrix_update();

    dynamic_draw.vertex = p_heap_alloc(P_MAX_DYNAMIC_DRAW_VERTEX_COUNT * sizeof(pDynamicDrawVertex));
    dynamic_draw.batch = p_heap_alloc(P_MAX_DYNAMIC_DRAW_BATCH_COUNT * sizeof(pDynamicDrawBatch));

    gu_initialized = true;
}

void p_graphics_shutdown(void) {
    p_heap_free(dynamic_draw.vertex);
    p_heap_free(dynamic_draw.batch);
	sceGuTerm();
}

int p_screen_width(void) {
    return PSP_SCREEN_W;
}

int p_screen_height(void) {
    return PSP_SCREEN_H;
}

void p_clear_background(pColor color) {
	sceGuClearColor(p_color_to_8888(color));
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
}

void p_graphics_frame_begin(void) {
    sceGuStart(GU_DIRECT, list);
}

void p_graphics_frame_end(void) {
    // TODO: check if sceGuFinish is even worth tracing
	peTraceMark tm_finish = PE_TRACE_MARK_BEGIN("sceGuFinish");
    sceGuFinish();
    PE_TRACE_MARK_END(tm_finish);
	peTraceMark tm_sync = PE_TRACE_MARK_BEGIN("sceGuSync");
    sceGuSync(0, 0);
	PE_TRACE_MARK_END(tm_sync);
}

void p_graphics_set_depth_test(bool enable) {
    if (enable) {
        sceGuEnable(GU_DEPTH_TEST);
    } else {
        sceGuDisable(GU_DEPTH_TEST);
    }
}

void p_graphics_set_lighting(bool enable) {

}

void p_graphics_set_light_vector(pVec3 light_vector) {

}

void p_graphics_set_diffuse_color(pColor color) {
    // TODO: change only the diffuse color and not everything
    sceGuColor(color.rgba);
}

void p_graphics_matrix_update(void) {
    int gu_matrix_modes[pMatrixMode_Count];
    {
        gu_matrix_modes[pMatrixMode_Projection] = GU_PROJECTION;
        gu_matrix_modes[pMatrixMode_View] = GU_VIEW;
        gu_matrix_modes[pMatrixMode_Model] = GU_MODEL;
    }
    for (int matrix_mode = 0; matrix_mode < pMatrixMode_Count; matrix_mode += 1) {
        if (p_graphics.matrix_dirty[p_graphics.mode][matrix_mode]) {
            pMat4 *matrix = &p_graphics.matrix[p_graphics.mode][matrix_mode];
            sceGumMatrixMode(gu_matrix_modes[matrix_mode]);
            sceGumLoadMatrix(&matrix->_sce);
            p_graphics.matrix_dirty[p_graphics.mode][matrix_mode] = false;
        }
    }
    sceGumUpdateMatrix();
}

pMat4 p_matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
	return p_perspective_rh_no(fovy * P_DEG2RAD, aspect_ratio, near_z, far_z);
}

pMat4 p_matrix_orthographic(float left, float right, float bottom, float top, float near_z, float far_z) {
    return p_orthographic_rh_no(left, right, bottom, top, near_z, far_z);
}

pTexture p_texture_create(pArena *temp_arena, void *data, int width, int height) {
    const int format = GU_PSM_8888;
	int pow2_width = closest_greater_power_of_two(width);
	int pow2_height = closest_greater_power_of_two(height);
	int block_height = 8 * ((height + 7) / 8);

	int byte_width = p_pixel_bytes(width, format);
	int pow2_byte_width = p_pixel_bytes(pow2_width, format);
	int size = p_pixel_bytes(pow2_width * block_height, format);

    unsigned int* swizzled_pixels = p_arena_alloc_align(&edram_arena, size, 16);
    p_swizzle(swizzled_pixels, data, byte_width, pow2_byte_width, height);
    sceKernelDcacheWritebackRange(swizzled_pixels, size);

	pTexture texture = {
		.data = swizzled_pixels,
		.width = width,
		.height = height,
		.power_of_two_width = pow2_width,
		.power_of_two_height = pow2_height,
		.format = format,
		.swizzle = true,
	};
    return texture;
}

#include "graphics/p_pbm.h"

pTexture p_texture_create_pbm(pArena *temp_arena, pbmFile *pbm) {
    int width = pbm->static_info->width;
    int height = pbm->static_info->height;
    int power_of_two_width = closest_greater_power_of_two(width);
    int power_of_two_height = closest_greater_power_of_two(height);
	int block_size_height = 8 * ((height + 7) / 8);

	int pixel_format = (pbm->static_info->bits_per_pixel == 8 ? GU_PSM_T8 : GU_PSM_T4);
	int byte_width = p_pixel_bytes(width, pixel_format);
	int power_of_two_byte_width = p_pixel_bytes(power_of_two_width, pixel_format);
	int pixel_size = p_pixel_bytes(power_of_two_width * block_size_height, pixel_format);

    unsigned int* swizzled_pixels = p_arena_alloc_align(&edram_arena, pixel_size, 16);
    p_swizzle(swizzled_pixels, pbm->index, byte_width, power_of_two_byte_width, height);
    sceKernelDcacheWritebackRange(swizzled_pixels, pixel_size);

    int palette_format = 0;
    switch (pbm->static_info->palette_format) {
        case pbmColorFormat_5650: palette_format = GU_PSM_5650; break;
        case pbmColorFormat_5551: palette_format = GU_PSM_5551; break;
        case pbmColorFormat_4444: palette_format = GU_PSM_4444; break;
        case pbmColorFormat_8888: palette_format = GU_PSM_8888; break;
        default: P_PANIC(); break;
    }

    int palette_padded_length = 8 * ((pbm->static_info->palette_length + 7) / 8);
    int palette_size = p_pixel_bytes(palette_padded_length, palette_format);
    void *palette = p_arena_alloc_align(&edram_arena, palette_size, 16);
    memcpy(palette, pbm->palette, palette_size);
    sceKernelDcacheWritebackRange(palette, palette_size);

	pTexture texture = {
		.data = swizzled_pixels,
		.format = pixel_format,
		.width = width,
		.height = height,
		.power_of_two_width = power_of_two_width,
		.power_of_two_height = power_of_two_height,
		.swizzle = true,
		.palette = palette,
		.palette_length = pbm->static_info->palette_length,
		.palette_padded_length = palette_padded_length,
		.palette_format = palette_format,
	};
    return texture;
}

void p_texture_bind(pTexture texture) {
    PE_TRACE_FUNCTION_BEGIN();
	sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(texture.format, 0, 0, texture.swizzle);
    // sceGuTexMode(texture.format, 0, 0, 0);
	sceGuTexImage(0, texture.power_of_two_width, texture.power_of_two_height, texture.power_of_two_width, texture.data);
	//sceGuTexEnvColor(0x00FFFFFF);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	float tex_scale_u = (float)texture.width/(float)texture.power_of_two_width;
    float tex_scale_v = (float)texture.height/(float)texture.power_of_two_height;
    // float tex_scale_v = 1.0f;
    sceGuTexScale(tex_scale_u, tex_scale_v);
	sceGuTexOffset(0.0f,0.0f);

    if (texture.palette_length != 0) {
        P_ASSERT(texture.palette_length > 0);
        P_ASSERT(texture.palette_length <= texture.palette_padded_length);
        P_ASSERT(texture.palette_padded_length % 8 == 0);
        P_ASSERT(texture.format == GU_PSM_T4 || texture.format == GU_PSM_T8);
        int max_palette_length = (
            texture.format == GU_PSM_T4 ? 16 :
            texture.format == GU_PSM_T8 ? 256 : 0
        );
        P_ASSERT(texture.palette_length <= max_palette_length);
        sceGuClutMode(texture.palette_format, 0, 0xFF, 0);
        int palette_num_blocks = texture.palette_padded_length / 8;
        sceGuClutLoad(palette_num_blocks, texture.palette);
    }

	PE_TRACE_FUNCTION_END();
}

void p_texture_bind_default(void) {
	sceGuDisable(GU_TEXTURE_2D);
}

void p_graphics_dynamic_draw_draw_batches(void) {
    if (dynamic_draw.vertex_used > 0) {
        PE_TRACE_FUNCTION_BEGIN();
        pMatrixMode old_matrix_mode = p_graphics.matrix_mode;
        pMat4 old_matrix_model = p_graphics.matrix[p_graphics.mode][pMatrixMode_Model];
        bool old_matrix_model_is_identity = p_graphics.matrix_model_is_identity[p_graphics.mode];

        p_graphics_matrix_mode(pMatrixMode_Model);
        p_graphics_matrix_identity();
        p_graphics_matrix_update();

        sceKernelDcacheWritebackRange(dynamic_draw.vertex, sizeof(pDynamicDrawVertex)*P_MAX_DYNAMIC_DRAW_VERTEX_COUNT);

        for (int b = dynamic_draw.batch_drawn_count; b <= dynamic_draw.batch_current; b += 1) {
            peTraceMark tm_draw_batch = PE_TRACE_MARK_BEGIN("draw batch");
            P_ASSERT(dynamic_draw.batch[b].vertex_count > 0);

            if (dynamic_draw.batch[b].texture) {
                p_texture_bind(*dynamic_draw.batch[b].texture);
            } else {
                p_texture_bind_default();
            }

            int primitive_sce;
            switch (dynamic_draw.batch[b].primitive) {
                case pPrimitive_Points: primitive_sce = GU_POINTS; break;
                case pPrimitive_Lines: primitive_sce = GU_LINES; break;
                case pPrimitive_Triangles: primitive_sce = GU_TRIANGLES; break;
                case pPrimitive_TriangleStrip: primitive_sce = GU_TRIANGLE_STRIP; break;
                default: break;
            }
            int gu_transform = p_graphics.mode == pGraphicsMode_3D ? GU_TRANSFORM_3D : GU_TRANSFORM_3D;
            int vertex_type = (
                GU_TEXTURE_32BITF|GU_COLOR_8888|GU_NORMAL_32BITF|GU_VERTEX_32BITF|gu_transform
            );
            sceGuDrawArray(
                primitive_sce,
                vertex_type,
                dynamic_draw.batch[b].vertex_count,
                NULL,
                dynamic_draw.vertex + dynamic_draw.batch[b].vertex_offset
            );
            PE_TRACE_MARK_END(tm_draw_batch);
        }

        p_graphics_matrix_set(&old_matrix_model);
        p_graphics_matrix_update();
        p_graphics_matrix_mode(old_matrix_mode);
        p_graphics.matrix_model_is_identity[p_graphics.mode] = old_matrix_model_is_identity;

        dynamic_draw.batch_drawn_count = dynamic_draw.batch_current + 1;
        PE_TRACE_FUNCTION_END();
    }
}

void p_graphics_dynamic_draw_push_vec3(pVec3 position) {
    if (!p_graphics.matrix_model_is_identity[p_graphics.mode]) {
        pMat4 *matrix = &p_graphics.matrix[p_graphics.mode][pMatrixMode_Model];
        pVec4 position_vec4 = p_vec4v(position, 1.0f);
        pVec4 transformed_position = p_mat4_mul_vec4(*matrix, position_vec4);
        position = transformed_position.xyz;
    }
    pPrimitive batch_primitive = dynamic_draw.batch[dynamic_draw.batch_current].primitive;
    int batch_vertex_count = dynamic_draw.batch[dynamic_draw.batch_current].vertex_count;
    int primitive_vertex_count = p_graphics_primitive_vertex_count(batch_primitive);
    if (primitive_vertex_count > 0) {
        int primitive_vertex_left = primitive_vertex_count - (batch_vertex_count % primitive_vertex_count);
        p_graphics_dynamic_draw_vertex_reserve(primitive_vertex_left);
    }
    pDynamicDrawVertex *vertex = &dynamic_draw.vertex[dynamic_draw.vertex_used];
    vertex->texcoord = dynamic_draw.texcoord;
    vertex->color = p_color_to_8888(dynamic_draw.color);
    vertex->normal = dynamic_draw.normal;
    vertex->position = position;
    dynamic_draw.batch[dynamic_draw.batch_current].vertex_count += 1;
    dynamic_draw.vertex_used += 1;
}

//
// INTERNAL IMPLEMENTATIONS
//

int p_pixel_bytes(int count, int format) {
    P_ASSERT(count >= 0);
    int result = 0;
	switch (format)
	{
        case GU_PSM_T4: result = count / 2; break;
		case GU_PSM_T8: result = count; break;

		case GU_PSM_5650:
		case GU_PSM_5551:
		case GU_PSM_4444:
		case GU_PSM_T16:
			result = 2 * count; break;

		case GU_PSM_8888:
		case GU_PSM_T32:
			result = 4 * count; break;

		default: P_PANIC_MSG("Unsupported pixel format: %d", format); break;
	}
    return result;
}

unsigned int closest_greater_power_of_two(unsigned int value) {
	if (value == 0 || value > (1 << 31))
		return 0;
	unsigned int power_of_two = 1;
	while (power_of_two < value)
		power_of_two <<= 1;
	return power_of_two;
}

/*
    ####################. .:. . . .:
    #  B0  |   B1  |   B2  :   B3  :
    #      |       |   #. .:. . . .:
    #------|-------|---#---:-------:
    #  B4  |   B5  |   B6 .:. .B7 .:
    ####################   :       :
    . . . .:. . . .:. . . .:. . . .:
*/
void p_swizzle(void *out, void *in, int byte_width, int pow2_byte_width, int height) {
    PE_TRACE_FUNCTION_BEGIN();
    const int BLOCK_WIDTH = 16;
    const int BLOCK_HEIGHT = 8;
    int full_block_count_x = byte_width / BLOCK_WIDTH;
    int full_block_count_y = height / BLOCK_HEIGHT;
    int block_count_x = pow2_byte_width / BLOCK_WIDTH;
    int block_count_y = (height + BLOCK_HEIGHT-1) / BLOCK_HEIGHT;
    int right_block_width = byte_width % BLOCK_WIDTH;
    int bottom_block_height = height % BLOCK_HEIGHT;

    struct Block_Data_Row {
        uint8_t buffer[BLOCK_WIDTH];
    };
    P_STATIC_ASSERT(sizeof(struct Block_Data_Row) == BLOCK_WIDTH);

    uint8_t *out_ptr = out;
    uint8_t *in_row_ptr = in;
    for (int block_y = 0; block_y < full_block_count_y; block_y += 1) {
        uint8_t *in_col_ptr = in_row_ptr;
        for (int block_x = 0; block_x < full_block_count_x; block_x += 1) {
            uint8_t *in_block_ptr = in_col_ptr;
            for (int block_row = 0; block_row < BLOCK_HEIGHT; block_row += 1) {
                memcpy(out_ptr, in_block_ptr, BLOCK_WIDTH);
                out_ptr += BLOCK_WIDTH;
                in_block_ptr += byte_width;
            }
            in_col_ptr += BLOCK_WIDTH;
        }
        if (full_block_count_x < block_count_x) {
            uint8_t *in_block_ptr = in_col_ptr;
            for (int block_row = 0; block_row < BLOCK_HEIGHT; block_row += 1) {
                memcpy(out_ptr, in_block_ptr, right_block_width);
                out_ptr += BLOCK_WIDTH;
                in_block_ptr += byte_width;
            }
            out_ptr += (block_count_x-full_block_count_x-1)*BLOCK_WIDTH*BLOCK_HEIGHT;
        }
        in_row_ptr += byte_width*BLOCK_HEIGHT;
    }
    for (int block_y = full_block_count_y; block_y < block_count_y; block_y += 1) {
        uint8_t *in_col_ptr = in_row_ptr;
        for (int block_x = 0; block_x < full_block_count_x; block_x += 1) {
            uint8_t *in_block_ptr = in_col_ptr;
            for (int block_row = 0; block_row < bottom_block_height; block_row += 1) {
                memcpy(out_ptr, in_block_ptr, BLOCK_WIDTH);
                out_ptr += BLOCK_WIDTH;
                in_block_ptr += byte_width;
            }
            out_ptr += (BLOCK_HEIGHT - bottom_block_height) * BLOCK_WIDTH;
            in_col_ptr += BLOCK_WIDTH;
        }
        if (full_block_count_x < block_count_x) {
            uint8_t *in_block_ptr = in_col_ptr;
            for (int block_row = 0; block_row < bottom_block_height; block_row += 1) {
                memcpy(out_ptr, in_block_ptr, right_block_width);
                out_ptr += BLOCK_WIDTH;
                in_block_ptr += byte_width;
            }
            out_ptr += (BLOCK_HEIGHT - bottom_block_height) * BLOCK_WIDTH;
            in_col_ptr += BLOCK_WIDTH;
        }
    }
    PE_TRACE_FUNCTION_END();
}

static void copy_texture_data(void *dest, const void *src, const int rows, size_t pitch, size_t pitch_pow2) {
    PE_TRACE_FUNCTION_BEGIN();
    for (int r = 0; r < rows; r += 1) {
        memcpy((uint8_t*)dest + r * pitch_pow2, (uint8_t*)src + r * pitch, pitch);
    }
    PE_TRACE_FUNCTION_END();
}

static void swizzle_fast(void *out, void *in, int width, int height) {
    PE_TRACE_FUNCTION_BEGIN();
	int width_blocks = width / 16;
	int height_blocks = height / 8;

	int src_pitch = (width - 16) / 4;
	int src_row = width * 8;

	uint8_t *ysrc = in;
	uint32_t *dst = (uint32_t *)out;

	for (int blocky = 0; blocky < height_blocks; ++blocky) {
		uint8_t *xsrc = ysrc;
		for (int blockx = 0; blockx < width_blocks; ++blockx) {
			uint32_t *src = (uint32_t *)xsrc;
			for (int j = 0; j < 8; ++j)
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
	PE_TRACE_FUNCTION_END();
}
