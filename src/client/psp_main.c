#include "pe_core.h"
#include "pe_time.h"
#include "pe_input.h"
#include "pe_protocol.h"
#include "pe_net.h"
#include "game/pe_entity.h"
#include "pe_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>

#include "HandmadeMath.h"

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>
#include <pspgu.h>
#include <pspgum.h>

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

static bool should_quit = false;

static unsigned int __attribute__((aligned(16))) list[262144];

static peArena edram_arena;
static peArena temp_arena;

bool pe_should_quit(void) {
	return should_quit;
}

int pe_exit_callback(int arg1, int arg2, void* common) {
	should_quit = true;
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

int pe_exit_callback_thread(SceSize args, void* argp) {
	int cbid = sceKernelCreateCallback("Exit Callback", pe_exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

int pe_setup_callbacks(void) {
	int thid = sceKernelCreateThread("update_thread", pe_exit_callback_thread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}

//
// I/O
//

typedef struct peFileContents {
    peAllocator allocator;
    void *data;
    size_t size;
} peFileContents;

peFileContents pe_file_read_contents(peAllocator allocator, char *file_path, bool zero_terminate) {
	peFileContents result = {0};
	result.allocator = allocator;

	SceUID fuid = sceIoOpen(file_path, PSP_O_RDONLY, 0);
	if (fuid < 0) {
		return result;
	}

	SceIoStat io_stat;
	if (sceIoGetstat(file_path, &io_stat) < 0) {
		sceIoClose(fuid);
		return result;
	}

	size_t total_size = (size_t)(zero_terminate ? io_stat.st_size+1 : io_stat.st_size);
	void *data = pe_alloc(allocator, total_size);
	if (data == NULL) {
		sceIoClose(fuid);
		return result;
	}

	int bytes_read = sceIoRead(fuid, data, io_stat.st_size);
	sceIoClose(fuid);
	PE_ASSERT(bytes_read == io_stat.st_size);

	result.data = data;
	result.size = total_size;
	if (zero_terminate) {
		((uint8_t*)data)[io_stat.st_size] = 0;
	}

	return result;
}

void pe_file_free_contents(peFileContents contents) {
	pe_free(contents.allocator, contents.data);
}

//
//
//

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

typedef struct peColor {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} peColor;

#define PE_COLOR_WHITE (peColor){ 255, 255, 255, 255 }
#define PE_COLOR_GRAY  (peColor){ 127, 127, 127, 255 }
#define PE_COLOR_RED   (peColor){ 255,   0,   0, 255 }
#define PE_COLOR_GREEN (peColor){   0, 255,   0, 255 }
#define PE_COLOR_BLUE  (peColor){   0,   0, 255, 255 }

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

typedef struct peTexture {
	void *data;
	int width;
	int height;
	int power_of_two_width;
	int power_of_two_height;
	bool vram;
	int format;
} peTexture;
static peTexture default_texture;

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

peTexture pe_texture_create(void *data, int width, int height, int format, bool vram) {
	peTexture texture = {0};

	int power_of_two_width = closest_greater_power_of_two(width);
	int power_of_two_height = closest_greater_power_of_two(height);
	unsigned int size = power_of_two_width * power_of_two_height * bytes_per_pixel(format);

	peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);

	unsigned int *data_buffer = pe_alloc_align(pe_arena_allocator(&temp_arena), size, 16);
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

void destroy_texture(peTexture texture) {
	if (texture.vram) {
		// TODO: freeing VRAM
	} else {
		pe_free(pe_heap_allocator(), texture.data);
	}
}

void pe_default_texture_init(void) {
	int texture_width = 8;
	int texture_height = 8;
	size_t texture_data_size = texture_width * texture_height * sizeof(uint16_t);

	peAllocator temp_allocator = pe_arena_allocator(&temp_arena);
	peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);
	{
		uint16_t *texture_data = pe_alloc(temp_allocator, texture_data_size);
		for (int y = 0; y < texture_height; y++) {
			for (int x = 0; x < texture_width; x++) {
				if (y < texture_height/2 && x < texture_width/2 || y >= texture_height/2 && x >= texture_width/2) {
					texture_data[y*texture_width + x] = pe_color_to_5650(PE_COLOR_WHITE);
				} else {
					texture_data[y*texture_width + x] = pe_color_to_5650(PE_COLOR_GRAY);
				}
			}
		}
		default_texture = pe_texture_create(texture_data, texture_width, texture_height, GU_PSM_5650, true);
	}
	pe_temp_arena_memory_end(temp_arena_memory);
}

void use_texture(peTexture texture) {
 	sceGuTexMode(texture.format, 0, 0, 1);
	sceGuTexImage(0, texture.power_of_two_width, texture.power_of_two_height, texture.power_of_two_width, texture.data);
	//sceGuTexEnvColor(0x00FFFFFF);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	//sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuTexScale(1.0f,1.0f);
	sceGuTexOffset(0.0f,0.0f);
}

typedef struct peMaterial {
	bool has_diffuse;
	peColor diffuse_color;
	peTexture diffuse_map;
} peMaterial;

peMaterial pe_default_material(void) {
	peMaterial material = {
		.has_diffuse = false,
		.diffuse_map = default_texture,
	};
	return material;
}

typedef struct VertexTCP
{
    float u, v;
	uint16_t color;
	float x, y, z;
} VertexTCP;

typedef struct VertexTP {
	float u,v;
	float x, y, z;
} VertexTP;

typedef struct VertexP {
	float x, y, z;
} VertexP;

typedef struct Mesh {
	int vertex_count;
	int index_count;

	VertexTCP *vertices;
	uint16_t *indices;
	int vertex_type;
} Mesh;

Mesh gen_mesh_cube(float width, float height, float length, peColor color) {
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
	mesh.vertices = pe_alloc_align(pe_heap_allocator(), vertex_count*sizeof(VertexTCP), 16);
	mesh.indices = pe_alloc_align(pe_heap_allocator(), sizeof(indices), 16);
	mesh.vertex_type = GU_TEXTURE_32BITF|GU_COLOR_5650|GU_VERTEX_32BITF|GU_TRANSFORM_3D|GU_INDEX_16BIT;

	for (int i = 0; i < vertex_count; i += 1) {
		mesh.vertices[i].u = texcoords[2*i];
		mesh.vertices[i].v = texcoords[2*i + 1];
		mesh.vertices[i].color = pe_color_to_5650(color);
		mesh.vertices[i].x = vertices[3*i];
		mesh.vertices[i].y = vertices[3*i + 1];
		mesh.vertices[i].z = vertices[3*i + 2];
	}

	memcpy(mesh.indices, indices, sizeof(indices));

	sceKernelDcacheWritebackInvalidateRange(mesh.vertices, vertex_count*sizeof(VertexTCP));
	sceKernelDcacheWritebackInvalidateRange(mesh.indices, sizeof(indices));

	return mesh;
}

Mesh pe_gen_mesh_quad(float width, float length, peColor color) {
	float vertices[] = {
		-width/2.0f, 0.0f, -length/2.0f,
		-width/2.0f, 0.0f,  length/2.0f,
		 width/2.0f, 0.0f, -length/2.0f,
		 width/2.0f, 0.0f,  length/2.0f,
	};

	// TODO: normals?

	float texcoords[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};

	uint16_t indices[] = {
		0, 1, 2,
		1, 3, 2
	};

	Mesh mesh = {0};

	int vertex_count = PE_COUNT_OF(vertices)/3;
	int index_count = PE_COUNT_OF(indices);
	mesh.vertex_count = vertex_count;
	mesh.index_count = index_count;
	mesh.vertices = pe_alloc_align(pe_heap_allocator(), vertex_count*sizeof(VertexTCP), 16);
	mesh.indices = pe_alloc_align(pe_heap_allocator(), sizeof(indices), 16);
	mesh.vertex_type = GU_TEXTURE_32BITF|GU_COLOR_5650|GU_VERTEX_32BITF|GU_TRANSFORM_3D|GU_INDEX_16BIT;

	for (int i = 0; i < vertex_count; i += 1) {
		mesh.vertices[i].u = texcoords[2*i];
		mesh.vertices[i].v = texcoords[2*i + 1];
		mesh.vertices[i].color = pe_color_to_5650(color);
		mesh.vertices[i].x = vertices[3*i];
		mesh.vertices[i].y = vertices[3*i + 1];
		mesh.vertices[i].z = vertices[3*i + 2];
	}

	memcpy(mesh.indices, indices, sizeof(indices));

	sceKernelDcacheWritebackInvalidateRange(mesh.vertices, vertex_count*sizeof(VertexTCP));
	sceKernelDcacheWritebackInvalidateRange(mesh.indices, sizeof(indices));

	return mesh;
}

void pe_draw_mesh(Mesh mesh, HMM_Vec3 position, HMM_Vec3 rotation) {
	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();
	sceGumLoadIdentity();
	sceGumTranslate((ScePspFVector3 *)&position);
	sceGumRotateXYZ((ScePspFVector3 *)&rotation);
	int count = (mesh.indices != NULL) ? mesh.index_count : mesh.vertex_count;
	sceGumDrawArray(GU_TRIANGLES, mesh.vertex_type, count, mesh.indices, mesh.vertices);
	sceGumPopMatrix();
}

//
// MODELS
//

typedef struct peModel {
	peArena arena;

	int num_mesh;
	struct peMesh {
		int num_vertex;
		int num_index;
		uint32_t diffuse_color;
		int vertex_type;
		void *vertex;
		void *index;
	} *mesh;

	int *num_index;

	int *vertex_type;
	void **index;
	void **vertex;

	uint8_t zero;
} peModel;

#include "pp3d.h"

peModel pe_model_load(char *file_path) {
	peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);
	peAllocator temp_allocator = pe_arena_allocator(&temp_arena);

	peFileContents pp3d_file_contents = pe_file_read_contents(temp_allocator, file_path, false);
	uint8_t *pp3d_file_pointer = pp3d_file_contents.data;

	pp3dStaticInfo *pp3d_static_info = (pp3dStaticInfo*)pp3d_file_pointer;
	pp3d_file_pointer += sizeof(pp3dStaticInfo);

	pp3dMesh *pp3d_mesh_info = (pp3dMesh*)pp3d_file_pointer;
	pp3d_file_pointer += sizeof(pp3dMesh) * pp3d_static_info->num_meshes;

	fprintf(stdout, "STATIC INFO:\n");
	fprintf(stdout, "  extension_magic: %c%c%c%c\n",
		pp3d_static_info->extension_magic[0],
		pp3d_static_info->extension_magic[1],
		pp3d_static_info->extension_magic[2],
		pp3d_static_info->extension_magic[3]
	);
	fprintf(stdout, "  scale: %f\n", pp3d_static_info->scale);
	fprintf(stdout, "  num_meshes: %u\n", pp3d_static_info->num_meshes);

	for (unsigned int m = 0; m < pp3d_static_info->num_meshes; m += 1) {
		fprintf(stdout, "MESH %d:\n", m);
		fprintf(stdout, "  num_vertex: %u\n", pp3d_mesh_info[m].num_vertex);
		fprintf(stdout, "  num_index: %u\n", pp3d_mesh_info[m].num_index);
		fprintf(stdout, "  diffuse_color: %x\n", pp3d_mesh_info[m].diffuse_color);
	}

	peModel model = {0};

	pe_temp_arena_memory_end(temp_arena_memory);
}

void pe_model_free(peModel *model) {
	pe_arena_free(&model->arena);
}

void pe_perspective(float fovy, float aspect, float near, float far) {
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(fovy, aspect, near, far);
}

void pe_view_lookat(HMM_Vec3 eye, HMM_Vec3 target, HMM_Vec3 up) {
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	sceGumLookAt((ScePspFVector3*)&eye, (ScePspFVector3*)&target, (ScePspFVector3*)&up);
}

static float dynamic_2d_draw_depth = (float)65535;
static uint32_t default_gu_color = 0xffffffff;

void pe_draw_point(HMM_Vec3 position, peColor color) {
	size_t vertices_size = sizeof(VertexP);
	VertexP *vertex = sceGuGetMemory(vertices_size);
	vertex->x = position.X;
	vertex->y = position.Y;
	vertex->z = position.Z;

	sceKernelDcacheWritebackInvalidateRange(vertex, vertices_size);

	sceGuColor(pe_color_to_8888(color));
	sceGumDrawArray(GU_POINTS, GU_VERTEX_32BITF|GU_TRANSFORM_3D, 1, NULL, vertex);
	sceGuColor(default_gu_color);
}

void pe_draw_line(HMM_Vec3 start_pos, HMM_Vec3 end_pos, peColor color) {
	size_t vertices_size = 2*sizeof(VertexP);
	VertexP *vertices = sceGuGetMemory(vertices_size);
	vertices[0].x = start_pos.X;
	vertices[0].y = start_pos.Y;
	vertices[0].z = start_pos.Z;
	vertices[1].x = end_pos.X;
	vertices[1].y = end_pos.Y;
	vertices[1].z = end_pos.Z;

	sceKernelDcacheWritebackInvalidateRange(vertices, vertices_size);

	sceGuColor(pe_color_to_8888(color));
	sceGumDrawArray(GU_LINES, GU_VERTEX_32BITF|GU_TRANSFORM_3D, 2, NULL, vertices);
	sceGuColor(default_gu_color);
}

void pe_draw_pixel(float x, float y, peColor color) {
	size_t vertices_size = 1*sizeof(VertexP);
	VertexP *vertices = sceGuGetMemory(vertices_size);
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = dynamic_2d_draw_depth;

	sceKernelDcacheWritebackInvalidateRange(vertices, vertices_size);

	sceGuColor(pe_color_to_8888(color));
	sceGumDrawArray(GU_POINTS, GU_VERTEX_32BITF|GU_TRANSFORM_2D, 1, NULL, vertices);
	sceGuColor(default_gu_color);
}

typedef struct peRect {
	float x;
	float y;
	float width;
	float height;
} peRect;

void pe_draw_rect(peRect rect, peColor color) {
	size_t vertices_size = 2*sizeof(VertexP);
	VertexP *vertices = sceGuGetMemory(vertices_size);
	vertices[0].x = rect.x;              vertices[0].y = rect.y;
	vertices[1].x = rect.x + rect.width; vertices[1].y = rect.y + rect.height;
	vertices[0].z = dynamic_2d_draw_depth;
	vertices[1].z = dynamic_2d_draw_depth;

	sceKernelDcacheWritebackInvalidateRange(vertices, vertices_size);

	sceGuColor(pe_color_to_8888(color));
	sceGumDrawArray(GU_SPRITES, GU_VERTEX_32BITF|GU_TRANSFORM_2D, 2, NULL, vertices);
	sceGuColor(default_gu_color);
}

void pe_gu_init(void) {
	peAllocator edram_allocator = pe_arena_allocator(&edram_arena);

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
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
}

void pe_clear_background(peColor color) {
	sceGuClearColor(pe_color_to_8888(color));
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
}

//
// NET CLIENT STUFF
//

#define CONNECTION_REQUEST_TIME_OUT 20.0f // seconds
#define CONNECTION_REQUEST_SEND_RATE 1.0f // # per second

#define SECONDS_TO_TIME_OUT 10.0f

typedef enum peClientNetworkState {
	peClientNetworkState_Disconnected,
	peClientNetworkState_Connecting,
	peClientNetworkState_Connected,
	peClientNetworkState_Error,
} peClientNetworkState;

static peSocket socket;
peAddress server_address;
peClientNetworkState network_state = peClientNetworkState_Disconnected;
int client_index;
uint32_t entity_index;
uint64_t last_packet_send_time = 0;
uint64_t last_packet_receive_time = 0;

pePacket outgoing_packet = {0};

extern peEntity *entities;

void pe_receive_packets(void) {
	peAddress address;
	pePacket packet = {0};
	peAllocator allocator = pe_heap_allocator();
	while (pe_receive_packet(socket, allocator, &address, &packet)) {
		if (!pe_address_compare(address, server_address)) {
			goto message_cleanup;
		}

		for (int m = 0; m < packet.message_count; m += 1) {
			peMessage message = packet.messages[m];
			switch (message.type) {
				case peMessageType_ConnectionDenied: {
					fprintf(stdout, "connection request denied (reason = %d)\n", message.connection_denied->reason);
					network_state = peClientNetworkState_Error;
				} break;
				case peMessageType_ConnectionAccepted: {
					if (network_state == peClientNetworkState_Connecting) {
						char address_string[256];
						fprintf(stdout, "connected to the server (address = %s)\n", pe_address_to_string(server_address, address_string, sizeof(address_string)));
						network_state = peClientNetworkState_Connected;
						client_index = message.connection_accepted->client_index;
						entity_index = message.connection_accepted->entity_index;
						last_packet_receive_time = pe_time_now();
					} else if (network_state == peClientNetworkState_Connected) {
						PE_ASSERT(client_index == message.connection_accepted->client_index);
						last_packet_receive_time = pe_time_now();
					}
				} break;
				case peMessageType_ConnectionClosed: {
					if (network_state == peClientNetworkState_Connected) {
						fprintf(stdout, "connection closed (reason = %d)\n", message.connection_closed->reason);
						network_state = peClientNetworkState_Error	;
					}
				} break;
				case peMessageType_WorldState: {
					if (network_state == peClientNetworkState_Connected) {
						memcpy(entities, message.world_state->entities, MAX_ENTITY_COUNT*sizeof(peEntity));
					}
				} break;
				default: break;
			}
		}

message_cleanup:
		for (int m = 0; m < packet.message_count; m += 1) {
			pe_message_destroy(allocator, packet.messages[m]);
		}
		packet.message_count = 0;
	}
}

int main(int argc, char* argv[])
{
	pe_setup_callbacks();

	pe_arena_init_from_allocator(&temp_arena, pe_heap_allocator(), PE_MEGABYTES(4));
	pe_arena_init_from_memory(&edram_arena, sceGeEdramGetAddr(), sceGeEdramGetSize());

	pe_gu_init();

	pe_net_init();
	pe_time_init();
	pe_input_init();

	peModel model = pe_model_load("./res/ybot.pp3d");

	pe_allocate_entities();

	peSocketCreateError socket_create_error = pe_socket_create(peSocket_IPv4, 0, &socket);
	fprintf(stdout, "socket create result: %d\n", socket_create_error);

	server_address = pe_address4(192, 168, 128, 81, SERVER_PORT);

	pe_default_texture_init();

	float aspect = (float)pe_screen_width()/(float)pe_screen_height();
	pe_perspective(55.0f, aspect, 0.5f, 1000.0f);

	HMM_Vec3 eye_offset = { 0.0f, 3.0f, 3.0f };
	HMM_Vec3 target = { 0.0f, 0.0f, 0.0f };
	HMM_Vec3 eye = HMM_AddV3(target, eye_offset);
	HMM_Vec3 up = { 0.0f, 1.0f, 0.0f };
	pe_view_lookat(eye, target, up);

	Mesh cube = gen_mesh_cube(1.0f, 1.0f, 1.0f, (peColor){255, 255, 255, 255});
	HMM_Vec3 position = { 0.0f, 0.0f, 0.0f };
	HMM_Vec3 rotation = {0.0f, 0.0f, 0.0f };


	float dt = 1.0f/60.0f;
	while(!pe_should_quit())
	{
		pe_net_update();
		pe_input_update();

		if (network_state != peClientNetworkState_Disconnected) {
			//pe_receive_packets();
		}

		switch (network_state) {
			case peClientNetworkState_Disconnected: {
				fprintf(stdout, "connecting to the server\n");
				network_state = peClientNetworkState_Connecting;
				last_packet_receive_time = pe_time_now();
			} break;
			case peClientNetworkState_Connecting: {
				uint64_t ticks_since_last_received_packet = pe_time_since(last_packet_receive_time);
				float seconds_since_last_received_packet = (float)pe_time_sec(ticks_since_last_received_packet);
				if (seconds_since_last_received_packet > (float)CONNECTION_REQUEST_TIME_OUT) {
					fprintf(stdout, "connection request timed out\n");
					network_state = peClientNetworkState_Error;
					break;
				}
				float connection_request_send_interval = 1.0f / (float)CONNECTION_REQUEST_SEND_RATE;
				uint64_t ticks_since_last_sent_packet = pe_time_since(last_packet_send_time);
				float seconds_since_last_sent_packet = (float)pe_time_sec(ticks_since_last_sent_packet);
				if (seconds_since_last_sent_packet > connection_request_send_interval) {
					peMessage message = pe_message_create(pe_heap_allocator(), peMessageType_ConnectionRequest);
					pe_append_message(&outgoing_packet, message);
				}
			} break;
			case peClientNetworkState_Connected: {
				peMessage message = pe_message_create(pe_heap_allocator(), peMessageType_InputState);
				message.input_state->input.movement.X = pe_input_axis(peGamepadAxis_LeftX);
				message.input_state->input.movement.Y = pe_input_axis(peGamepadAxis_LeftY);
				pe_append_message(&outgoing_packet, message);
			} break;
			default: break;
		}

		if (outgoing_packet.message_count > 0) {
			//pe_send_packet(socket, server_address, &outgoing_packet);
			last_packet_send_time = pe_time_now();
			for (int m = 0; m < outgoing_packet.message_count; m += 1) {
				pe_message_destroy(pe_heap_allocator(), outgoing_packet.messages[m]);
			}
			outgoing_packet.message_count = 0;
		}

		sceGuStart(GU_DIRECT,list);
		sceGuColor(default_gu_color);

		pe_clear_background((peColor){20, 20, 20, 255});

		use_texture(default_texture);

		HMM_Vec3 zero = {0.0f};
		pe_draw_line(zero, (HMM_Vec3){1.0f, 0.0f, 0.0f}, PE_COLOR_RED);
		pe_draw_line(zero, (HMM_Vec3){0.0f, 1.0f, 0.0f}, PE_COLOR_GREEN);
		pe_draw_line(zero, (HMM_Vec3){0.0f, 0.0f, 1.0f}, PE_COLOR_BLUE);

		pe_draw_mesh(cube, HMM_V3(0.0f, 0.0f, 0.0f), HMM_V3(0.0f, 0.0f, 0.0f));

		for (int e = 0; e < MAX_ENTITY_COUNT; e += 1) {
			peEntity *entity = &entities[e];
			if (!entity->active) continue;

			//pe_draw_mesh(cube, entity->position, (HMM_Vec3){ .Y = entity->angle });
		}


		sceGuFinish();
		sceGuSync(0,0);
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();

		pe_free_all(pe_arena_allocator(&temp_arena));
	}

	sceGuTerm();

	sceKernelExitGame();
	return 0;
}