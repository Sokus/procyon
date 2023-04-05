#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdarg.h>
#include <pspiofilemgr.h>

#include "pe_time.h"
#include "pe_input.h"

#include <stdint.h>

#include <pspgu.h>
#include <pspgum.h>
#include <math.h>

#include "pe_core.h"

PSP_MODULE_INFO("Procyon", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define PSP_BUFF_W (512)
#define PSP_SCREEN_W (480)
#define PSP_SCREEN_H (272)

int pe_screen_width() {
	return PSP_SCREEN_W;
}

int pe_screen_height() {
	return PSP_SCREEN_H;
}

static unsigned int __attribute__((aligned(16))) list[262144];

static peArena edram_arena;
static peArena temp_arena;
static peAllocator temp_allocator;

static int exit_request = 0;

int running() {
    return !exit_request;
}

int exit_callback(int arg1, int arg2, void* common) {
	exit_request = 1;
	return 0;
}

unsigned int closest_greater_power_of_two(unsigned int value) {
	if (value == 0 || value > (1 << 31))
		return 0;
	unsigned int power_of_two = 1;
	while (power_of_two < value)
		power_of_two <<= 1;
	return power_of_two;

}

int CallbackThread(SceSize args, void* argp) {
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

int SetupCallbacks(void) {
	int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}

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

typedef struct Color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} Color;

#define PE_COLOR_WHITE (Color){ 255, 255, 255, 255 }
#define PE_COLOR_GRAY  (Color){ 127, 127, 127, 255 }

uint16_t pe_color_to_5650(Color color) {
	uint16_t max_5_bit = (1U << 5) - 1;
	uint16_t max_6_bit = (1U << 6) - 1;
	uint16_t max_8_bit = (1U << 8) - 1;
	uint16_t r = (((uint16_t)color.r * max_5_bit) / max_8_bit) & max_5_bit;
	uint16_t g = (((uint16_t)color.g * max_6_bit) / max_8_bit) & max_6_bit;
	uint16_t b = (((uint16_t)color.b * max_5_bit) / max_8_bit) & max_5_bit;
	uint16_t result = b << 11 | g << 5 | r;
	return result;
}

typedef struct Texture {
	void *data;
	int width;
	int height;
	int power_of_two_width;
	int power_of_two_height;
	bool vram;
	int format;
} Texture;
static Texture default_texture;

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

Texture create_texture(void *data, int width, int height, int format, bool vram) {
	Texture texture = {0};

	int power_of_two_width = closest_greater_power_of_two(width);
	int power_of_two_height = closest_greater_power_of_two(height);
	unsigned int size = power_of_two_width * power_of_two_height * bytes_per_pixel(format);

	peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);

	unsigned int *data_buffer = pe_alloc_align(temp_allocator, size, 16);
	copy_texture_data(data_buffer, data, power_of_two_width, width, height);

	peAllocator texture_allocator = vram ? pe_arena_allocator(&edram_arena) : pe_heap_allocator();
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

void destroy_texture(Texture texture) {
	if (texture.vram) {
		// TODO: freeing VRAM
	} else {
		pe_free(pe_heap_allocator(), texture.data);
	}
}

void pe_load_texture_default(void) {
	peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);

	int texture_width = 8;
	int texture_height = 8;
	size_t texture_data_size = texture_width * texture_height * sizeof(uint16_t);
	uint16_t *texture_data = pe_alloc(pe_arena_allocator(&temp_arena), texture_data_size);
	for (int y = 0; y < texture_height; y++) {
		for (int x = 0; x < texture_width; x++) {
			if (y < texture_height/2 && x < texture_width/2 || y >= texture_height/2 && x >= texture_width/2) {
				texture_data[y*texture_width + x] = pe_color_to_5650(PE_COLOR_WHITE);
			} else {
				texture_data[y*texture_width + x] = pe_color_to_5650(PE_COLOR_GRAY);
			}
		}
	}
	default_texture = create_texture(texture_data, texture_width, texture_height, GU_PSM_5650, true);

	pe_temp_arena_memory_end(temp_arena_memory);
}

void use_texture(Texture texture) {
 	sceGuTexMode(texture.format, 0, 0, 1);
	sceGuTexImage(0, texture.power_of_two_width, texture.power_of_two_height, texture.power_of_two_width, texture.data);
	//sceGuTexEnvColor(0x00FFFFFF);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f);
}

typedef struct Vertex
{
    float u, v;
	uint16_t color;
	float x, y, z;
} Vertex;

typedef struct Mesh {
	int vertex_count;
	int index_count;

	Vertex *vertices;
	uint16_t *indices;
	int vertex_type;
} Mesh;

Mesh gen_mesh_cube(float width, float height, float length, Color color) {
	float vertices[] = {
		-width/2.0f, -height/2.0f,  length/2.0f,
		 width/2.0f, -height/2.0f,  length/2.0f,
		 width/2.0f,  height/2.0f,  length/2.0f,
		-width/2.0f,  height/2.0f,  length/2.0f,
		-width/2.0f, -height/2.0f, -length/2.0f,
		-width/2.0f,  height/2.0f, -length/2.0f,
		 width/2.0f,  height/2.0f, -length/2.0f,
		 width/2.0f, -height/2.0f, -length/2.0f,
		-width/2.0f,  height/2.0f, -length/2.0f,
		-width/2.0f,  height/2.0f,  length/2.0f,
		 width/2.0f,  height/2.0f,  length/2.0f,
		 width/2.0f,  height/2.0f, -length/2.0f,
		-width/2.0f, -height/2.0f, -length/2.0f,
		 width/2.0f, -height/2.0f, -length/2.0f,
		 width/2.0f, -height/2.0f,  length/2.0f,
		-width/2.0f, -height/2.0f,  length/2.0f,
		 width/2.0f, -height/2.0f, -length/2.0f,
		 width/2.0f,  height/2.0f, -length/2.0f,
		 width/2.0f,  height/2.0f,  length/2.0f,
		 width/2.0f, -height/2.0f,  length/2.0f,
		-width/2.0f, -height/2.0f, -length/2.0f,
		-width/2.0f, -height/2.0f,  length/2.0f,
		-width/2.0f,  height/2.0f,  length/2.0f,
		-width/2.0f,  height/2.0f, -length/2.0f,
	};

#if 0
	float normals[] = {
		 0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
		 0.0f, 0.0f,-1.0f,  0.0f, 0.0f,-1.0f,  0.0f, 0.0f,-1.0f,  0.0f, 0.0f,-1.0f,
		 0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
		 0.0f,-1.0f, 0.0f,  0.0f,-1.0f, 0.0f,  0.0f,-1.0f, 0.0f,  0.0f,-1.0f, 0.0f,
		 1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
	};
#endif

	float texcoords[] = {
		0.0f, 0.0f,  1.0f, 0.0f,  1.0, 1.0f,  0.0f, 1.0f,
		1.0f, 0.0f,  1.0f, 1.0f,  0.0, 1.0f,  0.0f, 0.0f,
		0.0f, 1.0f,  0.0f, 0.0f,  1.0, 0.0f,  1.0f, 1.0f,
		1.0f, 1.0f,  0.0f, 1.0f,  0.0, 0.0f,  1.0f, 0.0f,
		1.0f, 0.0f,  1.0f, 1.0f,  0.0, 1.0f,  0.0f, 0.0f,
		0.0f, 0.0f,  1.0f, 0.0f,  1.0, 1.0f,  0.0f, 1.0f,
	};

	uint16_t indices[] = {
		0,  1,  2,  0,  2,  3,
		4,  5,  6,  4,  6,  7,
		8,  9, 10,  8, 10, 11,
	   12, 13, 14, 12, 14, 15,
	   16, 17, 18, 16, 18, 19,
	   20, 21, 22, 20, 22, 23,
    };

	Mesh mesh = {0};

	int vertex_count = PE_COUNT_OF(vertices)/3; // NOTE: 3 floats per vertex
	int index_count = PE_COUNT_OF(indices);
	mesh.vertex_count = vertex_count;
	mesh.index_count = index_count;
	mesh.vertices = pe_alloc_align(pe_heap_allocator(), vertex_count*sizeof(Vertex), 16);
	mesh.indices = pe_alloc_align(pe_heap_allocator(), sizeof(indices), 16);
	memcpy(mesh.indices, indices, sizeof(indices));
	mesh.vertex_type = GU_TEXTURE_32BITF|GU_COLOR_5650|GU_VERTEX_32BITF|GU_TRANSFORM_3D|GU_INDEX_16BIT;

	for (int i = 0; i < vertex_count; i += 1) {
		mesh.vertices[i].x = vertices[3*i];
		mesh.vertices[i].y = vertices[3*i + 1];
		mesh.vertices[i].z = vertices[3*i + 2];
		mesh.vertices[i].color = pe_color_to_5650(color);
		mesh.vertices[i].u = texcoords[2*i];
		mesh.vertices[i].v = texcoords[2*i + 1];
	}

    sceKernelDcacheWritebackInvalidateAll();
	return mesh;
}

void pe_draw_mesh(Mesh mesh) {
	int count = (mesh.indices != NULL) ? mesh.index_count : mesh.vertex_count;
	sceGumDrawArray(GU_TRIANGLES, mesh.vertex_type, count, mesh.indices, mesh.vertices);
}

typedef struct peCamera {
	ScePspFVector3 eye;
	ScePspFVector3 target;
	ScePspFVector3 up;
	float fovy;
} peCamera;

void pe_camera_update(peCamera camera) {
	float aspect = (float)pe_screen_width() / (float)pe_screen_height();
	float near = 0.5f;
	float far = 1000.0f;
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(camera.fovy, aspect, near, far);
	//sceGumPerspective(75.0f,16.0f/9.0f,0.5f,1000.0f);

	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	sceGumLookAt(&camera.eye, &camera.target, &camera.up);
}

int main(int argc, char* argv[])
{
	pe_arena_init_from_allocator(&temp_arena, pe_heap_allocator(), PE_MEGABYTES(4));
	temp_allocator = pe_arena_allocator(&temp_arena);

	SetupCallbacks();

	pe_arena_init_from_memory(&edram_arena, sceGeEdramGetAddr(), sceGeEdramGetSize());
	peAllocator edram_allocator = pe_arena_allocator(&edram_arena);

	unsigned int framebuffer_size = PSP_BUFF_W * PSP_SCREEN_H * bytes_per_pixel(GU_PSM_5650);
	void *framebuffer0 = pe_alloc_align(edram_allocator, framebuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();
	void *framebuffer1 = pe_alloc_align(edram_allocator, framebuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();

    unsigned int depthbuffer_size = PSP_BUFF_W * PSP_SCREEN_H * bytes_per_pixel(GU_PSM_4444);
	void *depthbuffer = pe_alloc_align(edram_allocator, depthbuffer_size, 4) - (uintptr_t)sceGeEdramGetAddr();

	pe_time_init();
	pe_input_init();

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
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	int val = 0;

	pe_load_texture_default();
	Mesh mesh = gen_mesh_cube(1.0f, 1.0f, 1.0f, (Color){255, 255, 255, 255});

	peCamera camera;
	ScePspFVector3 eye_offset = (ScePspFVector3){ 0.0f, 3.0f, 3.0f };
	camera.target = (ScePspFVector3){ 0.0f, 0.0f, 0.0f };
	camera.eye.x = camera.target.x + eye_offset.x;
	camera.eye.y = camera.target.y + eye_offset.y;
	camera.eye.z = camera.target.z + eye_offset.z;
	camera.up = (ScePspFVector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 55.0f;

	float dt = 1.0f/60.0f;

	ScePspFVector3 position = {0};

	float angle = 0.0;

	while(running())
	{
		sceGuStart(GU_DIRECT,list);

		sceGuClearColor(0xff151515);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

		pe_input_update();

		position.x += dt * pe_input_axis(peGamepadAxis_LeftX);
		position.z += dt * pe_input_axis(peGamepadAxis_LeftY);

		if (pe_input_axis(peGamepadAxis_LeftX) != 0.0 || pe_input_axis(peGamepadAxis_LeftY) != 0.0) {
			angle = atan2f(pe_input_axis(peGamepadAxis_LeftX), pe_input_axis(peGamepadAxis_LeftY));
		}

		//camera.target = position;
		//camera.eye.x = camera.target.x + eye_offset.x;
		//camera.eye.y = camera.target.y + eye_offset.y;
		//camera.eye.z = camera.target.z + eye_offset.z;

		pe_camera_update(camera);

		use_texture(default_texture);

		sceGumMatrixMode(GU_MODEL);
		sceGumLoadIdentity();
		sceGumTranslate(&position);
		sceGumRotateY(angle);
		pe_draw_mesh(mesh);

		sceGuFinish();
		sceGuSync(0,0);

		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();

		val++;

		pe_free_all(temp_allocator);
	}

	sceGuTerm();

	sceKernelExitGame();
	return 0;
}