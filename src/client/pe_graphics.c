#include "pe_graphics.h"

#include "pe_core.h"
#include "pe_platform.h"

#if defined(_WIN32)
    #include "win32/win32_d3d.h"
    #include "pe_window_glfw.h"

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(PSP)
	#include "pe_graphics_psp.h"

    #include <pspkernel.h> // sceKernelDcacheWritebackInvalidateRange
    #include <pspdisplay.h> // sceDisplayWaitVblankStart
    #include <pspgu.h>
#endif

#include "HandmadeMath.h"

#include <stdbool.h>

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

#if defined(PSP)

static unsigned int closest_greater_power_of_two(unsigned int value) {
	if (value == 0 || value > (1 << 31))
		return 0;
	unsigned int power_of_two = 1;
	while (power_of_two < value)
		power_of_two <<= 1;
	return power_of_two;

}

static void copy_texture_data(void *dest, const void *src, const int pW, const int width, const int height)
{
    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++)
        {
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

peTexture pe_texture_create(void *data, int width, int height, int format, bool vram) {
	peTexture texture = {0};

	int power_of_two_width = closest_greater_power_of_two(width);
	int power_of_two_height = closest_greater_power_of_two(height);
	unsigned int size = power_of_two_width * power_of_two_height * bytes_per_pixel(format);

	peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(pe_temp_arena());

	unsigned int *data_buffer = pe_alloc_align(pe_temp_allocator(), size, 16);
	copy_texture_data(data_buffer, data, power_of_two_width, width, height);

	peAllocator texture_allocator = vram ? pe_edram_allocator() : pe_heap_allocator();
    unsigned int* swizzled_pixels = pe_alloc_align(texture_allocator, size, 16);
    swizzle_fast((u8*)swizzled_pixels, data, power_of_two_width*bytes_per_pixel(format), power_of_two_height);

	pe_temp_arena_memory_end(temp_arena_memory);

    texture.data = swizzled_pixels;
	texture.width = width;
	texture.height = height;
	texture.power_of_two_width = power_of_two_width;
	texture.power_of_two_height = power_of_two_height;
	texture.vram = vram;
	texture.format = format;

    sceKernelDcacheWritebackInvalidateAll();

    return texture;
}

void pe_texture_destroy(peTexture texture) {
	if (texture.vram) {
		// TODO: freeing VRAM
	} else {
		pe_free(pe_heap_allocator(), texture.data);
	}
}

void pe_texture_bind(peTexture texture) {
	sceGuEnable(GU_TEXTURE_2D);
 	sceGuTexMode(texture.format, 0, 0, 1);
	sceGuTexImage(0, texture.power_of_two_width, texture.power_of_two_height, texture.power_of_two_width, texture.data);
	//sceGuTexEnvColor(0x00FFFFFF);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f);
}

void pe_texture_unbind(void) {
	sceGuDisable(GU_TEXTURE_2D);
}
#endif

void pe_graphics_init(int window_width, int window_height, const char *window_name) {
#if defined(_WIN32)
    pe_glfw_init(window_width, window_height, "Procyon");
    pe_d3d11_init();
#elif defined(PSP)
    pe_graphics_init_psp();
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
#endif
}


int pe_screen_width(void) {
#if defined(_WIN32)
    return pe_glfw.window_width;
#elif defined(PSP)
    return PSP_SCREEN_W;
#else
	#error unimplemented
#endif
}

int pe_screen_height(void) {
#if defined(_WIN32)
    return pe_glfw.window_height;
#elif defined(PSP)
    return PSP_SCREEN_H;
#else
    #error unimplemented
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
#else
	#error unimplemented
#endif
}