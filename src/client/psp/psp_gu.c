#include "psp_gu.h"

#include "pe_core.h"

#include <pspge.h>
#include <pspgu.h>
#include <pspdisplay.h> // sceDisplayWaitVblankStart
#include <pspgum.h>

static peArena edram_arena;
static peAllocator edram_allocator;
static bool gu_initialized = false;

unsigned int __attribute__((aligned(16))) list[262144];

void pe_gu_init(void) {
	pe_arena_init_from_memory(&edram_arena, sceGeEdramGetAddr(), sceGeEdramGetSize());
	edram_allocator = pe_arena_allocator(&edram_arena);

	unsigned int framebuffer_size = PSP_BUFF_W * PSP_SCREEN_H * bytes_per_pixel(GU_PSM_5650);
	void *framebuffer0 = pe_alloc_align(edram_allocator, framebuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();
	void *framebuffer1 = pe_alloc_align(edram_allocator, framebuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();

    unsigned int depthbuffer_size = PSP_BUFF_W * PSP_SCREEN_H * bytes_per_pixel(GU_PSM_4444);
	void *depthbuffer = pe_alloc_align(edram_allocator, depthbuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();

	sceGuInit();

	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_5650, framebuffer0, PSP_BUFF_W);
	sceGuDispBuffer(PSP_SCREEN_W, PSP_SCREEN_H, framebuffer1, PSP_BUFF_W);
	sceGuDepthBuffer(depthbuffer, PSP_BUFF_W);
	sceGuOffset(2048 - (PSP_SCREEN_W/2),2048 - (PSP_SCREEN_H/2));
	sceGuViewport(2048,2048,PSP_SCREEN_W,PSP_SCREEN_H);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,PSP_SCREEN_W,PSP_SCREEN_H);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CCW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();

    gu_initialized = true;
}

PE_INLINE peAllocator pe_edram_allocator(void) {
    PE_ASSERT(gu_initialized);
    return edram_allocator;
}

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