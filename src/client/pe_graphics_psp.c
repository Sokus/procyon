#include "pe_graphics.h"
#include "pe_graphics_psp.h"
#include "pe_core.h"
#include "pe_temp_arena.h"

#include <pspge.h>
#include <pspgu.h>
#include <pspdisplay.h> // sceDisplayWaitVblankStart
#include <pspgum.h>
#include <pspkernel.h> // sceKernelDcacheWritebackInvalidateRange


static peArena edram_arena;
static bool gu_initialized = false;

unsigned int __attribute__((aligned(16))) list[262144];

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

    gu_initialized = true;
}

void pe_graphics_shutdown_psp(void) {
	sceGuTerm();
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

peTexture pe_texture_create_psp(void *data, int width, int height, int format) {
	int power_of_two_width = closest_greater_power_of_two(width);
	int power_of_two_height = closest_greater_power_of_two(height);
	unsigned int size = power_of_two_width * power_of_two_height * bytes_per_pixel(format);

	peArenaTemp temp_arena_memory = pe_arena_temp_begin(pe_temp_arena());

	unsigned int *data_buffer = pe_arena_alloc_align(pe_temp_arena(), size, 16);
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