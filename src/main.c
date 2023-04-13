#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <malloc.h>

#include <stdint.h>

#include <pspgu.h>
#include <pspgum.h>

PSP_MODULE_INFO("Cube Sample", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)

static unsigned int __attribute__((aligned(16))) list[262144];

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

static unsigned int edram_used = 0;

unsigned int edram_push_offset(unsigned int amount) {
	// TODO: Alignment?
	unsigned int result = edram_used;
	edram_used += amount;
	return result;
}

void *edram_push_size(unsigned int size) {
	// TODO: Alignment?
	void *result = sceGeEdramGetAddr() + (int)edram_used;
	edram_used += size;
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

    unsigned int* swizzled_pixels = NULL;
    if(vram) {
        swizzled_pixels = edram_push_size(size);
    } else {
        swizzled_pixels = (unsigned int *)memalign(16, size);
    }

    swizzle_fast((u8*)swizzled_pixels, data, power_of_two_width * bytes_per_pixel(format), power_of_two_height);

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

typedef struct Vertex
{
    float u, v;
	unsigned int color;
	float x, y, z;
} Vertex;

typedef struct Mesh {
	Vertex *vertices;
	uint16_t *indices;
} Mesh;

Mesh gen_mesh_cube(float width, float height, float length) {
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

	uint32_t colors[] = {
		0xff007af8,
		0xff007af8,
		0xff007af8,
		0xff007af8,
		0xff0229e4,
		0xff0229e4,
		0xff0229e4,
		0xff0229e4,
		0xff11009c,
		0xff11009c,
		0xff11009c,
		0xff11009c,
		0xff681d20,
		0xff681d20,
		0xff681d20,
		0xff681d20,
		0xffa24000,
		0xffa24000,
		0xffa24000,
		0xffa24000,
		0xff117100,
		0xff117100,
		0xff117100,
		0xff117100,
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
	mesh.vertices = memalign(16, sizeof(colors)/sizeof(uint32_t)*sizeof(Vertex));
	mesh.indices = memalign(16, sizeof(indices));
	memcpy(mesh.indices, indices, sizeof(indices));

	for (int i = 0; i < (int)sizeof(colors)/sizeof(uint32_t); i++) {
		mesh.vertices[i].x = vertices[3*i];
		mesh.vertices[i].y = vertices[3*i + 1];
		mesh.vertices[i].z = vertices[3*i + 2];
		mesh.vertices[i].color = colors[i];
		mesh.vertices[i].u = texcoords[2*i];
		mesh.vertices[i].v = texcoords[2*i + 1];
	}


    sceKernelDcacheWritebackInvalidateAll();
	return mesh;
}

int main(int argc, char* argv[])
{
	SetupCallbacks();

	// setup GU

	unsigned int framebuffer_size = BUF_WIDTH * SCR_HEIGHT * bytes_per_pixel(GU_PSM_8888);
    unsigned int depthbuffer_size = BUF_WIDTH * SCR_HEIGHT * bytes_per_pixel(GU_PSM_4444);
    void *framebuffer0 = (void *)edram_push_offset(framebuffer_size);
    void *framebuffer1 = (void *)edram_push_offset(framebuffer_size);
    void *depthbuffer = (void *)edram_push_offset(depthbuffer_size);

	sceGuInit();

	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_8888, framebuffer0, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, framebuffer1, BUF_WIDTH);
	sceGuDepthBuffer(depthbuffer, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
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

	int texture_width = 8;
	int texture_height = 8;
	size_t texture_data_size = texture_width*texture_height*sizeof(uint32_t);
	uint32_t *texture_data = malloc(texture_data_size);
	for (int y = 0; y < texture_height; y++) {
		for (int x = 0; x < texture_width; x++) {
			if (y < texture_height/2 && x < texture_width/2 || y >= texture_height/2 && x >= texture_width/2) {
				texture_data[y*texture_width + x] = 0xFFFFFFFF;
			} else {
				texture_data[y*texture_width + x] = 0xFF7F7F7F;
			}
		}
	}

	Texture texture = create_texture(texture_data, texture_width, texture_height, GU_PSM_8888, true);
	free(texture_data);

	int val = 0;

	Mesh mesh = gen_mesh_cube(1.0f, 1.0f, 1.0f);

	while(running())
	{
		sceGuStart(GU_DIRECT,list);

		// clear screen

		sceGuClearColor(0xff151515);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

		// setup matrices for cube

		sceGumMatrixMode(GU_PROJECTION);
		sceGumLoadIdentity();
		sceGumPerspective(75.0f,16.0f/9.0f,0.5f,1000.0f);

		sceGumMatrixMode(GU_VIEW);
		sceGumLoadIdentity();

		sceGumMatrixMode(GU_MODEL);
		sceGumLoadIdentity();
		{
			ScePspFVector3 pos = { 0, 0, -2.5f };
			ScePspFVector3 rot = { val * 0.79f * (GU_PI/180.0f), val * 0.98f * (GU_PI/180.0f), val * 1.32f * (GU_PI/180.0f) };
			sceGumTranslate(&pos);
			sceGumRotateXYZ(&rot);
		}

		// setup texture

		sceGuTexMode(texture.format, 0, 0, 1);
		sceGuTexImage(0, texture.power_of_two_width, texture.power_of_two_height, texture.power_of_two_width, texture.data);
		sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
		sceGuTexEnvColor(0xffffff);
		//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
		sceGuTexFilter(GU_NEAREST,GU_NEAREST);
		sceGuTexScale(1.0f,1.0f);
		sceGuTexOffset(0.0f,0.0f);
		sceGuAmbientColor(0xffffffff);

		// draw cube

		//sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,12*3,0,vertices);
		int vertex_type = GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D|GU_INDEX_16BIT;
		sceGumDrawArray(GU_TRIANGLES, vertex_type, 12*3, mesh.indices, mesh.vertices);

		sceGuFinish();
		sceGuSync(0,0);

		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();

		val++;
	}

	sceGuTerm();

	sceKernelExitGame();
	return 0;
}