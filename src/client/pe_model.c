#include "pe_model.h"

#include "pe_core.h"
#include "pe_file_io.h"
#include "pe_platform.h"
#include "pe_profile.h"

#include "p3d.h"
#include "pp3d.h"

#if defined(_WIN32)
	#include "pe_graphics_win32.h"
#elif defined(__linux__)
	#include "pe_graphics_linux.h"
#elif defined(PSP)
    #include <pspkernel.h> // sceKernelDcacheWritebackInvalidateRange
    #include <pspgu.h>
    #include <pspgum.h>
#endif

#include "stb_image.h"

#include <string.h>

peMaterial pe_default_material(void) {
    peMaterial result = {
		.diffuse_color = PE_COLOR_WHITE,
		.has_diffuse_map = false
	};
	return result;
}

// FIXME: This needs a rework probably, it wasn't really modified since I
// started working on the 3D model format (P3D).
#if 0 // WIN32
peMesh pe_gen_mesh_cube(float width, float height, float length) {
    float pos[] = {
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

    float norm[] = {
		 0.0, 0.0, 1.0,  0.0, 0.0, 1.0,  0.0, 0.0, 1.0,  0.0, 0.0, 1.0,
		 0.0, 0.0,-1.0,  0.0, 0.0,-1.0,  0.0, 0.0,-1.0,  0.0, 0.0,-1.0,
		 0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,
		 0.0,-1.0, 0.0,  0.0,-1.0, 0.0,  0.0,-1.0, 0.0,  0.0,-1.0, 0.0,
		 1.0, 0.0, 0.0,  1.0, 0.0, 0.0,  1.0, 0.0, 0.0,  1.0, 0.0, 0.0,
		-1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0,
    };

    float tex[] = {
		0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0,
		1.0, 0.0,  1.0, 1.0,  0.0, 1.0,  0.0, 0.0,
		0.0, 1.0,  0.0, 0.0,  1.0, 0.0,  1.0, 1.0,
		1.0, 1.0,  0.0, 1.0,  0.0, 0.0,  1.0, 0.0,
		1.0, 0.0,  1.0, 1.0,  0.0, 1.0,  0.0, 0.0,
		0.0, 0.0,  1.0, 0.0,  1.0, 1.0,  0.0, 1.0,
    };

    uint32_t color[24];
    for (int i = 0; i < PE_COUNT_OF(color); i += 1) {
        color[i] = 0xffffffff;
    }

    uint32_t index[] = {
		0,  1,  2,  0,  2,  3,
		4,  5,  6,  4,  6,  7,
		8,  9, 10,  8, 10, 11,
	   12, 13, 14, 12, 14, 15,
	   16, 17, 18, 16, 18, 19,
	   20, 21, 22, 20, 22, 23,
    };

    peMesh mesh = {0};
    int num_vertex = PE_COUNT_OF(pos)/3;
    int num_index = PE_COUNT_OF(index);
    mesh.pos_buffer = pe_d3d11_create_buffer(pos, 3*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.norm_buffer = pe_d3d11_create_buffer(norm, 3*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.tex_buffer = pe_d3d11_create_buffer(tex, 2*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.color_buffer = pe_d3d11_create_buffer(color, num_vertex*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.index_buffer = pe_d3d11_create_buffer(index, num_index*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER);
    mesh.num_vertex = num_vertex;
    mesh.num_index = num_index;
    return mesh;
}

peMesh pe_gen_mesh_quad(float width, float length) {
	float pos[] = {
		-width/2.0f, 0.0f, -length/2.0f,
		-width/2.0f, 0.0f,  length/2.0f,
		 width/2.0f, 0.0f, -length/2.0f,
		 width/2.0f, 0.0f,  length/2.0f,
	};


    float norm[] = {
		0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,
    };

	float tex[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};

    uint32_t color[6];
    for (int i = 0; i < PE_COUNT_OF(color); i += 1) {
        color[i] = 0xffffffff;
    }

	uint32_t index[] = {
		0, 1, 2,
		1, 3, 2
	};

    peMesh mesh = {0};
    int num_vertex = PE_COUNT_OF(pos)/3;
    int num_index = PE_COUNT_OF(index);
    mesh.pos_buffer = pe_d3d11_create_buffer(pos, 3*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.norm_buffer = pe_d3d11_create_buffer(norm, 3*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.tex_buffer = pe_d3d11_create_buffer(tex, 2*num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.color_buffer = pe_d3d11_create_buffer(color, num_vertex*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    mesh.index_buffer = pe_d3d11_create_buffer(index, num_index*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER);
    mesh.num_vertex = num_vertex;
    mesh.num_index = num_index;
    return mesh;
}

void pe_draw_mesh(peMesh *mesh, peMaterial material, HMM_Vec3 position, HMM_Vec3 rotation) {
    HMM_Mat4 rotate_x = HMM_Rotate_RH(rotation.X, (HMM_Vec3){1.0f, 0.0f, 0.0f});
    HMM_Mat4 rotate_y = HMM_Rotate_RH(rotation.Y, (HMM_Vec3){0.0f, 1.0f, 0.0f});
    HMM_Mat4 rotate_z = HMM_Rotate_RH(rotation.Z, (HMM_Vec3){0.0f, 0.0f, 1.0f});
    HMM_Mat4 translate = HMM_Translate(position);

    HMM_Mat4 model_matrix = HMM_MulM4(HMM_MulM4(HMM_MulM4(translate, rotate_z), rotate_y), rotate_x);
    peShaderConstant_Model *constant_model = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_model_buffer);
    constant_model->matrix = model_matrix;
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_model_buffer);

    peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_material_buffer);
    //constant_material->has_diffuse = material.has_diffuse;
    constant_material->diffuse_color = pe_color_to_vec4(material.diffuse_color);
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_material_buffer);

    pe_bind_texture(material.diffuse_map);

    ID3D11Buffer *vertex_buffers[] = {
        mesh->pos_buffer,
        mesh->norm_buffer,
        mesh->tex_buffer,
        mesh->color_buffer,
    };
    uint32_t vertex_buffer_strides[] = {
        3 * sizeof(float), // positions
        3 * sizeof(float), // normals
        2 * sizeof(float), // texcoords
        sizeof(uint32_t),
    };
    uint32_t vertex_buffer_offsets[] = { 0, 0, 0, 0};

    ID3D11DeviceContext_IASetPrimitiveTopology(pe_d3d.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetVertexBuffers(pe_d3d.context, 0, PE_COUNT_OF(vertex_buffers),
        vertex_buffers, vertex_buffer_strides, vertex_buffer_offsets);
    ID3D11DeviceContext_IASetIndexBuffer(pe_d3d.context, mesh->index_buffer, DXGI_FORMAT_R32_UINT, 0);

    ID3D11DeviceContext_DrawIndexed(pe_d3d.context, mesh->num_index, 0, 0);
}
#endif

// FIXME: This needs a rework probably, it wasn't really modified since I
// started working on the 3D model format (PP3D).
#if 0 // PSP
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
#endif

peAnimationJoint pe_concatenate_animation_joints(peAnimationJoint parent, peAnimationJoint child) {
	peAnimationJoint result;
	result.scale = HMM_MulV3(child.scale, parent.scale);
	result.rotation = HMM_MulQ(parent.rotation, child.rotation);
	HMM_Mat4 parent_rotation_matrix = HMM_QToM4(parent.rotation);
	HMM_Vec3 child_scaled_position = HMM_MulV3(child.translation, parent.scale);
	result.translation = HMM_AddV3(HMM_MulM4V4(parent_rotation_matrix, HMM_V4V(child_scaled_position, 1.0f)).XYZ, parent.translation);
	return result;
}

#if defined(_WIN32) || defined(__linux__)
void pe_model_alloc(peModel *model, peArena *arena, p3dStaticInfo *p3d_static_info, p3dAnimation *p3d_animation) {
    size_t num_index_size = p3d_static_info->num_meshes * sizeof(unsigned int);
    size_t index_offset_size = p3d_static_info->num_meshes * sizeof(unsigned int);
    size_t vertex_offset_size = p3d_static_info->num_meshes * sizeof(int);
    size_t material_size = p3d_static_info->num_meshes * sizeof(peMaterial);
    size_t bone_parent_index_size = p3d_static_info->num_bones * sizeof(int);
    size_t bone_inverse_model_space_pose_matrix_size = p3d_static_info->num_bones * sizeof(HMM_Mat4);
    size_t animation_size = p3d_static_info->num_animations * sizeof(peAnimation);

    model->num_index = pe_arena_alloc(arena, num_index_size);
    model->index_offset = pe_arena_alloc(arena, index_offset_size);
    model->vertex_offset = pe_arena_alloc(arena, vertex_offset_size);
    model->material = pe_arena_alloc(arena, material_size);
    model->bone_parent_index = pe_arena_alloc(arena, bone_parent_index_size);
    model->bone_inverse_model_space_pose_matrix = pe_arena_alloc(arena, bone_inverse_model_space_pose_matrix_size);

    model->animation = pe_arena_alloc(arena, animation_size);
    for (int a = 0; a < p3d_static_info->num_animations; a += 1) {
        unsigned int num_animation_joint = p3d_animation[a].num_frames * p3d_static_info->num_bones;
        size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
        model->animation[a].frames = pe_arena_alloc(arena, animation_joint_size);
    }
}
#endif

#define VERT_MEM_ALIGN 16

#if defined(PSP)
void pe_model_alloc_psp(peModel *model, peArena *arena, pp3dStaticInfo *pp3d_static_info, pp3dMesh *pp3d_mesh, pp3dAnimation *pp3d_animation, size_t *pp3d_vertex_size, size_t *pp3d_index_size) {
	size_t material_size = pp3d_static_info->num_materials * sizeof(peMaterial);
	size_t subskeleton_size = pp3d_static_info->num_subskeletons * sizeof(peSubskeleton);
	size_t animation_size = pp3d_static_info->num_animations * sizeof(peAnimation);
	size_t bone_parent_index_size = pp3d_static_info->num_bones * sizeof(uint16_t);
	size_t bone_inverse_model_space_pose_matrix_size = pp3d_static_info->num_bones * sizeof(HMM_Mat4);
	size_t mesh_size = pp3d_static_info->num_meshes * sizeof(peMesh);
	size_t mesh_material_size = pp3d_static_info->num_meshes * sizeof(int);
	size_t mesh_subskeleton_size = pp3d_static_info->num_meshes * sizeof(int);

	model->material = pe_arena_alloc(arena, material_size);
	model->subskeleton = pe_arena_alloc(arena, subskeleton_size);

	model->animation = pe_arena_alloc(arena, animation_size);
	for (int a = 0; a < pp3d_static_info->num_animations; a += 1) {
		size_t num_animation_joint = pp3d_animation[a].num_frames * pp3d_static_info->num_bones;
		size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
		model->animation[a].frames = pe_arena_alloc(arena, animation_joint_size);
	}

	model->bone_parent_index = pe_arena_alloc(arena, bone_parent_index_size);
	model->bone_inverse_model_space_pose_matrix = pe_arena_alloc(arena, bone_inverse_model_space_pose_matrix_size);

	model->mesh = pe_arena_alloc(arena, mesh_size);
	model->mesh_material = pe_arena_alloc(arena, mesh_material_size);
	model->mesh_subskeleton = pe_arena_alloc(arena, mesh_subskeleton_size);

	for (int m = 0; m < pp3d_static_info->num_meshes; m += 1) {
		model->mesh[m].vertex = pe_arena_alloc_align(arena, pp3d_vertex_size[m], VERT_MEM_ALIGN);

		if (pp3d_mesh[m].num_index > 0) {
			model->mesh[m].index = pe_arena_alloc_align(arena, pp3d_index_size[m], VERT_MEM_ALIGN);
		}
	}
}
#endif

static float pe_int16_to_float(int16_t value, float a, float b) {
    float float_negative_one_to_one;
    if (value >= 0) {
        float_negative_one_to_one = (float)value / (float)INT16_MAX;
    } else {
        float_negative_one_to_one = -1.0f * (float)value / (float)INT16_MIN;
    }
    PE_ASSERT(b > a);
    float float_a_to_b = (float_negative_one_to_one + 1.0f) * (b - a) / 2.0f + a;
    return float_a_to_b;
}

static float pe_uint16_to_float(uint16_t value, float a, float b) {
    float float_zero_to_one = (float)value / (float)UINT16_MAX;
    float float_a_to_b = float_zero_to_one * (b - a) + a;
    return float_a_to_b;
}

#if defined(PSP)

bool pe_parse_pp3d(peArena *arena, peFileContents pp3d_file_contents, pp3dFile *pp3d) {
	uint8_t *pp3d_file_pointer = pp3d_file_contents.data;
	size_t pp3d_file_contents_left = pp3d_file_contents.size;

	if (sizeof(pp3dStaticInfo) > pp3d_file_contents_left) return false;
	pp3d->static_info = (pp3dStaticInfo *)pp3d_file_pointer;
	pp3d_file_pointer += sizeof(pp3dStaticInfo);
	pp3d_file_contents_left -= sizeof(pp3dStaticInfo);
	{
		int8_t bone_weight_size = pp3d->static_info->bone_weight_size;
		if (!(bone_weight_size == 1 || bone_weight_size == 2 || bone_weight_size == 4)) {
			return false;
		}
	}

	size_t mesh_size = pp3d->static_info->num_meshes * sizeof(pp3dMesh);
	if (mesh_size > pp3d_file_contents_left) return false;
	pp3d->mesh = (pp3dMesh *)pp3d_file_pointer;
	pp3d_file_pointer += mesh_size;
	pp3d_file_contents_left -= mesh_size;

	size_t material_size = pp3d->static_info->num_materials * sizeof(pp3dMaterial);
	if (material_size > pp3d_file_contents_left) return false;
	pp3d->material = (pp3dMaterial *)pp3d_file_pointer;
	pp3d_file_pointer += material_size;
	pp3d_file_contents_left -= material_size;

	size_t subskeleton_size = pp3d->static_info->num_subskeletons * sizeof(pp3dSubskeleton);
	if (subskeleton_size > pp3d_file_contents_left) return false;
	pp3d->subskeleton = (pp3dSubskeleton *)pp3d_file_pointer;
	pp3d_file_pointer += subskeleton_size;
	pp3d_file_contents_left -= subskeleton_size;

	size_t animation_size = pp3d->static_info->num_animations * sizeof(pp3dAnimation);
	if (animation_size > pp3d_file_contents_left) return false;
	pp3d->animation = (pp3dAnimation *)pp3d_file_pointer;
	pp3d_file_pointer += animation_size;
	pp3d_file_contents_left -= animation_size;

	pp3d->vertex = (void**)pe_arena_alloc(arena, pp3d->static_info->num_meshes * sizeof(void*));
	pp3d->vertex_size = (size_t *)pe_arena_alloc(arena, pp3d->static_info->num_meshes * sizeof(size_t));
	pp3d->index = (void**)pe_arena_alloc(arena, pp3d->static_info->num_meshes * sizeof(void*));
	pp3d->index_size = pe_arena_alloc(arena, pp3d->static_info->num_meshes * sizeof(size_t));
	for (int m = 0; m < pp3d->static_info->num_meshes; m += 1) {
		uint8_t num_weights = pp3d->subskeleton[pp3d->mesh[m].subskeleton_index].num_bones;
		bool has_diffuse_map = (
			pp3d->material[pp3d->mesh[m].material_index].diffuse_image_file_name[0] != '\0'
		);
		size_t single_vertex_size = (
			num_weights * (size_t)(pp3d->static_info->bone_weight_size) + // weights
			(num_weights % 2) * sizeof(uint8_t) + // weight padding
			(has_diffuse_map ? 2 * sizeof(int16_t) : 0) + // uv
			3 * sizeof(int16_t) +         // normal
			3 * sizeof(int16_t)	          // position
		);

		size_t vertex_size = single_vertex_size * pp3d->mesh[m].num_vertex;
		if (vertex_size > pp3d_file_contents_left) return false;
		pp3d->vertex[m] = pp3d_file_pointer;
		pp3d->vertex_size[m] = vertex_size;
		pp3d_file_pointer += vertex_size;
		pp3d_file_contents_left -= vertex_size;

		if (pp3d->mesh[m].num_index > 0) {
			size_t index_size = pp3d->mesh[m].num_index * sizeof(uint16_t);
			if (index_size > pp3d_file_contents_left) return false;
			pp3d->index[m] = pp3d_file_pointer;
			pp3d->index_size[m] = index_size;
			pp3d_file_pointer += index_size;
			pp3d_file_contents_left -= index_size;
		} else {
			pp3d->index[m] = NULL;
			pp3d->index_size[m] = 0;
		}
	}

	size_t bone_parent_index_size = pp3d->static_info->num_bones * sizeof(uint16_t);
	if (bone_parent_index_size > pp3d_file_contents_left) return false;
	pp3d->bone_parent_index = (uint16_t *)pp3d_file_pointer;
	pp3d_file_pointer += bone_parent_index_size;
	pp3d_file_contents_left -= bone_parent_index_size;

	size_t inverse_model_space_pose_matrix_size = pp3d->static_info->num_bones * sizeof(pp3dMatrix);
	if (inverse_model_space_pose_matrix_size > pp3d_file_contents_left) return false;
	pp3d->inverse_model_space_pose_matrix = (pp3dMatrix *)pp3d_file_pointer;
	pp3d_file_pointer += inverse_model_space_pose_matrix_size;
	pp3d_file_contents_left -= inverse_model_space_pose_matrix_size;

	size_t animation_joint_size = (size_t)pp3d->static_info->num_frames_total * (size_t)pp3d->static_info->num_bones * sizeof(pp3dAnimationJoint);
	if (animation_joint_size > pp3d_file_contents_left) return false;
	pp3d->animation_joint = (pp3dAnimationJoint *)pp3d_file_pointer;
	pp3d_file_pointer += animation_joint_size;
	pp3d_file_contents_left -= animation_joint_size;

	return true;
}

static peModel pe_model_load_psp(peArena *temp_arena, const char *file_path) {
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);

	pp3dFile pp3d;
	peFileContents pp3d_file_contents = pe_file_read_contents(temp_arena, file_path, false);
	pe_parse_pp3d(temp_arena, pp3d_file_contents, &pp3d);

    peModel model = {0};
	peArena model_arena;
    {
		// TODO: this is basically a memory leak (xD)
		// so far we are only loading a single model to test things out
		// so we don't really care, but we should allocate models in
		// another way in the future (pool allocator?)
		size_t model_memory_size = PE_KILOBYTES(512);
		void *model_memory = pe_heap_alloc_align(PE_KILOBYTES(512), VERT_MEM_ALIGN);
		pe_arena_init(&model_arena, model_memory, model_memory_size);
        pe_model_alloc_psp(&model, &model_arena, pp3d.static_info, pp3d.mesh, pp3d.animation, pp3d.vertex_size, pp3d.index_size);
    }

	// TODO: 8 and 16 bit UVs depending on texture size
	// TODO: 8 and 16 bit indices depending on vertex count

	model.scale = pp3d.static_info->scale;
	model.num_mesh = pp3d.static_info->num_meshes;
	model.num_material = pp3d.static_info->num_materials;
	model.num_subskeleton = pp3d.static_info->num_subskeletons;
	model.num_animations = pp3d.static_info->num_animations;
	model.num_bone = pp3d.static_info->num_bones;

    stbi_set_flip_vertically_on_load(false);
	for (int m = 0; m < pp3d.static_info->num_materials; m += 1) {
		model.material[m] = pe_default_material();
		model.material[m].diffuse_color.rgba = pp3d.material[m].diffuse_color;
		if (pp3d.material[m].diffuse_image_file_name[0] != '\0') {
			model.material[m].has_diffuse_map = true;

			int last_slash_index = 0;
			for (int c = 0; c < 256; c += 1) {
				if (file_path[c] == '\0') {
					break;
				}
				if (file_path[c] == '/') {
					last_slash_index = c;
				}
			}
			char diffuse_texture_path[512] = {0};
			memcpy(diffuse_texture_path, file_path, (last_slash_index+1) * sizeof(char));
			size_t diffuse_map_file_length = strnlen(pp3d.material[m].diffuse_image_file_name, sizeof(pp3d.material[m].diffuse_image_file_name));
			memcpy(diffuse_texture_path + last_slash_index + 1, pp3d.material[m].diffuse_image_file_name, diffuse_map_file_length*sizeof(char));

			int w, h, channels;
			stbi_uc *stbi_data = stbi_load(diffuse_texture_path, &w, &h, &channels, STBI_rgb_alpha);
			model.material[m].diffuse_map = pe_texture_create(temp_arena, stbi_data, w, h, GU_PSM_8888);
			stbi_image_free(stbi_data);
		}
	}

	for (int s = 0; s < pp3d.static_info->num_subskeletons; s += 1) {
		model.subskeleton[s].num_bones = pp3d.subskeleton[s].num_bones;
		for (int b = 0; b < pp3d.subskeleton[s].num_bones; b += 1) {
			model.subskeleton[s].bone_indices[b] = pp3d.subskeleton[s].bone_indices[b];
		}
	}

	for (int a = 0; a < pp3d.static_info->num_animations; a += 1) {
		PE_ASSERT(sizeof(model.animation[a].name) == sizeof(pp3d.animation[a].name));
		memcpy(model.animation[a].name, pp3d.animation[a].name, sizeof(model.animation[a].name));

		model.animation[a].num_frames = pp3d.animation[a].num_frames;
	}

	for (int m = 0; m < pp3d.static_info->num_meshes; m += 1) {
		model.mesh_material[m] = pp3d.mesh[m].material_index;
		model.mesh_subskeleton[m] = pp3d.mesh[m].subskeleton_index;
		model.mesh[m].num_vertex = pp3d.mesh[m].num_vertex;
		model.mesh[m].num_index = pp3d.mesh[m].num_index;

		model.mesh[m].vertex_type = GU_NORMAL_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_3D;
		if (pp3d.material[pp3d.mesh[m].material_index].diffuse_image_file_name[0] != '\0') {
			model.mesh[m].vertex_type |= GU_TEXTURE_16BIT;
		}
		uint8_t num_weights = pp3d.subskeleton[pp3d.mesh[m].subskeleton_index].num_bones;
		if (num_weights > 0) {
			model.mesh[m].vertex_type |= GU_WEIGHTS(num_weights);
			switch (pp3d.static_info->bone_weight_size) {
				case 1: model.mesh[m].vertex_type |= GU_WEIGHT_8BIT; break;
				case 2: model.mesh[m].vertex_type |= GU_WEIGHT_16BIT; break;
				case 4: model.mesh[m].vertex_type |= GU_WEIGHT_32BITF; break;
				default: PE_PANIC(); break;
			}
		}

		memcpy(model.mesh[m].vertex, pp3d.vertex[m], pp3d.vertex_size[m]);

		if (pp3d.mesh[m].num_index > 0) {
			model.mesh[m].vertex_type |= GU_INDEX_16BIT;
			memcpy(model.mesh[m].index, pp3d.index[m], pp3d.index_size[m]);
		}
	}

	memcpy(model.bone_parent_index, pp3d.bone_parent_index, pp3d.static_info->num_bones * sizeof(uint16_t));

	memcpy(model.bone_inverse_model_space_pose_matrix, pp3d.inverse_model_space_pose_matrix, pp3d.static_info->num_bones * sizeof(HMM_Mat4));

	int pp3d_animation_joint_offset = 0;
	for (int a = 0; a < pp3d.static_info->num_animations; a += 1) {
		size_t num_animation_joints = pp3d.static_info->num_bones * pp3d.animation[a].num_frames;
		size_t animation_joints_size = num_animation_joints * sizeof(peAnimationJoint);
		memcpy(model.animation[a].frames, pp3d.animation_joint + pp3d_animation_joint_offset, animation_joints_size);
		pp3d_animation_joint_offset += animation_joints_size;
	}

	sceKernelDcacheWritebackInvalidateRange(model_arena.physical_start, model_arena.total_allocated);

	pe_arena_temp_end(temp_arena_memory);

	return model;
}
#endif // #if defined(PSP)

bool pe_parse_p3d(peFileContents p3d_file_contents, p3dFile *p3d) {
	uint8_t *p3d_file_pointer = p3d_file_contents.data;
	size_t p3d_file_contents_left = p3d_file_contents.size;

	if (p3d_file_contents_left < sizeof(p3dStaticInfo)) return false;
	p3d->static_info = (p3dStaticInfo*)p3d_file_pointer;
	p3d_file_pointer += sizeof(p3dStaticInfo);
	p3d_file_contents_left -= sizeof(p3dStaticInfo);

	size_t mesh_size = p3d->static_info->num_meshes * sizeof(p3dMesh);
	if (p3d_file_contents_left < mesh_size) return false;
	p3d->mesh = (p3dMesh*)p3d_file_pointer;
	p3d_file_pointer += mesh_size;
	p3d_file_contents_left -= mesh_size;

	size_t animation_size = p3d->static_info->num_animations * sizeof(p3dAnimation);
	if (p3d_file_contents_left < animation_size) return false;
	p3d->animation = (p3dAnimation*)p3d_file_pointer;
	p3d_file_pointer += animation_size;
	p3d_file_contents_left -= animation_size;

	size_t position_size = 3 * p3d->static_info->num_vertex * sizeof(int16_t);
	if (p3d_file_contents_left < position_size) return false;
	p3d->position = (int16_t*)p3d_file_pointer;
	p3d_file_pointer += position_size;
	p3d_file_contents_left -= position_size;

	size_t normal_size = 3 * p3d->static_info->num_vertex * sizeof(int16_t);
	if (p3d_file_contents_left < normal_size) return false;
	p3d->normal = (int16_t*)p3d_file_pointer;
	p3d_file_pointer += normal_size;
	p3d_file_contents_left -= normal_size;

	size_t texcoord_size = 2 * p3d->static_info->num_vertex * sizeof(int16_t);
	if (p3d_file_contents_left < texcoord_size) return false;
	p3d->texcoord = (int16_t*)p3d_file_pointer;
	p3d_file_pointer += texcoord_size;
	p3d_file_contents_left -= texcoord_size;

	size_t bone_index_size = 4 * p3d->static_info->num_vertex * sizeof(uint8_t);
	if (p3d_file_contents_left < bone_index_size) return false;
	p3d->bone_index = (uint8_t *)p3d_file_pointer;
	p3d_file_pointer += bone_index_size;
	p3d_file_contents_left -= bone_index_size;

	size_t bone_weight_size = 4 * p3d->static_info->num_vertex * sizeof(uint16_t);
	if (p3d_file_contents_left < bone_weight_size) return false;
	p3d->bone_weight = (uint16_t *)p3d_file_pointer;
	p3d_file_pointer += bone_weight_size;
	p3d_file_contents_left -= bone_weight_size;

	size_t index_size = p3d->static_info->num_index * sizeof(uint32_t);
	if (p3d_file_contents_left < index_size) return false;
	p3d->index = (uint32_t *)p3d_file_pointer;
	p3d_file_pointer += index_size;
	p3d_file_contents_left -= index_size;

	size_t bone_parent_index_size = p3d->static_info->num_bones * sizeof(uint8_t);
	if (p3d_file_contents_left < bone_parent_index_size) return false;
	p3d->bone_parent_index = (uint8_t *)p3d_file_pointer;
	p3d_file_pointer += bone_parent_index_size;
	p3d_file_contents_left -= bone_parent_index_size;

	size_t inverse_model_space_pose_matrix_size = p3d->static_info->num_bones * sizeof(p3dMatrix);
	if (p3d_file_contents_left < inverse_model_space_pose_matrix_size) return false;
	p3d->inverse_model_space_pose_matrix = (p3dMatrix *)p3d_file_pointer;
	p3d_file_pointer += inverse_model_space_pose_matrix_size;
	p3d_file_contents_left -= inverse_model_space_pose_matrix_size;

	size_t animation_joint_size = (size_t)p3d->static_info->num_frames_total * (size_t)p3d->static_info->num_bones * sizeof(p3dAnimationJoint);
	if (p3d_file_contents_left < animation_joint_size) return false;
	p3d->animation_joint = (p3dAnimationJoint *)p3d_file_pointer;
	p3d_file_pointer += animation_joint_size;
	p3d_file_contents_left -= animation_joint_size;

	return true;
}

peModel pe_model_load(peArena *temp_arena, char *file_path) {
#if defined(_WIN32) || defined(__linux__)
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);

	p3dFile p3d;
    peFileContents p3d_file_contents = pe_file_read_contents(temp_arena, file_path, false);
	pe_parse_p3d(p3d_file_contents, &p3d);

    peModel model = {0};
    {
		// TODO: this is basically a memory leak (xD)
		// so far we are only loading a single model to test things out
		// so we don't really care, but we should allocate models in
		// another way in the future (pool allocator?)
		peArena model_arena;
		size_t model_memory_size = PE_KILOBYTES(512);
		pe_arena_init(&model_arena, pe_heap_alloc(model_memory_size), model_memory_size);
        pe_model_alloc(&model, &model_arena, p3d.static_info, p3d.animation);
    }

    model.num_mesh = (int)p3d.static_info->num_meshes;
    // for P3D number of meshes is always equal to the number of materials
    // (at least it should be... lol)
    model.num_material = (int)p3d.static_info->num_meshes;
    model.num_bone = (int)p3d.static_info->num_bones;
    model.num_animations = (int)p3d.static_info->num_animations;
    model.num_vertex = p3d.static_info->num_vertex;

    stbi_set_flip_vertically_on_load(false);
    for (int m = 0; m < p3d.static_info->num_meshes; m += 1) {
        model.num_index[m] = p3d.mesh[m].num_index;
        model.index_offset[m] = p3d.mesh[m].index_offset;
        PE_ASSERT(p3d.mesh[m].vertex_offset <= INT32_MAX);
        model.vertex_offset[m] = p3d.mesh[m].vertex_offset;

        model.material[m] = pe_default_material();
        model.material[m].diffuse_color.rgba = p3d.mesh[m].diffuse_color;
		if (p3d.mesh[m].diffuse_map_file_name[0] != '\0') {
			model.material[m].has_diffuse_map = true;

			int last_slash_index = 0;
			for (int c = 0; c < 256; c += 1) {
				if (file_path[c] == '\0') {
					break;
				}
				if (file_path[c] == '/') {
					last_slash_index = c;
				}
			}
			char diffuse_texture_path[512] = {0};
			memcpy(diffuse_texture_path, file_path, (last_slash_index+1) * sizeof(char));
			size_t diffuse_map_file_length = strnlen(p3d.mesh[m].diffuse_map_file_name, sizeof(p3d.mesh[m].diffuse_map_file_name));
			memcpy(diffuse_texture_path + last_slash_index + 1, p3d.mesh[m].diffuse_map_file_name, diffuse_map_file_length*sizeof(char));

			int w, h, channels;
			stbi_uc *stbi_data = stbi_load(diffuse_texture_path, &w, &h, &channels, STBI_rgb_alpha);
			model.material[m].diffuse_map = pe_texture_create(temp_arena, stbi_data, w, h, channels);
			stbi_image_free(stbi_data);
		}
    }

    for (int a = 0; a < p3d.static_info->num_animations; a += 1) {
        PE_ASSERT(sizeof(model.animation[a].name) == sizeof(p3d.animation[a].name));
        memcpy(model.animation[a].name, p3d.animation[a].name, sizeof(model.animation[a].name));
        model.animation[a].num_frames = p3d.animation[a].num_frames;
    }

	size_t pos_buffer_size = 3*p3d.static_info->num_vertex*sizeof(float);
	size_t nor_buffer_size = 3*p3d.static_info->num_vertex*sizeof(float);
	size_t tex_buffer_size = 2*p3d.static_info->num_vertex*sizeof(float);
	size_t bone_index_buffer_size = 4*p3d.static_info->num_vertex*sizeof(uint32_t);
	size_t bone_weight_buffer_size = 4*p3d.static_info->num_vertex*sizeof(float);

    float *pos_buffer = pe_arena_alloc(temp_arena, pos_buffer_size);
    float *nor_buffer = pe_arena_alloc(temp_arena, nor_buffer_size);
    float *tex_buffer = pe_arena_alloc(temp_arena, tex_buffer_size);
    uint32_t *col_buffer = pe_arena_alloc(temp_arena, p3d.static_info->num_vertex*sizeof(uint32_t));
    uint32_t *bone_index_buffer = pe_arena_alloc(temp_arena, bone_index_buffer_size);
    float *bone_weight_buffer = pe_arena_alloc(temp_arena, bone_weight_buffer_size);

    for (unsigned int p = 0; p < 3*p3d.static_info->num_vertex; p += 1) {
        pos_buffer[p] = p3d.static_info->scale * pe_int16_to_float(p3d.position[p], -1.0f, 1.0f);
    }

    for (unsigned int n = 0; n < 3*p3d.static_info->num_vertex; n += 1) {
        nor_buffer[n] = pe_int16_to_float(p3d.normal[n], -1.0f, 1.0f);
    }

    for (unsigned int t = 0; t < 2*p3d.static_info->num_vertex; t += 1) {
        tex_buffer[t] = pe_int16_to_float(p3d.texcoord[t], -1.0f, 1.0f);
    }

    for (unsigned int c = 0; c < p3d.static_info->num_vertex; c += 1) {
        col_buffer[c] = PE_COLOR_WHITE.rgba;
    }

    for (unsigned int b = 0; b < 4*p3d.static_info->num_vertex; b += 1) {
        bone_index_buffer[b] = (uint32_t)p3d.bone_index[b];
    }

    for (unsigned int b = 0; b < 4*p3d.static_info->num_vertex; b += 1) {
        bone_weight_buffer[b] = pe_uint16_to_float(p3d.bone_weight[b], 0.0f, 1.0f);
    }

    for (unsigned int b = 0; b < p3d.static_info->num_bones; b += 1) {
        model.bone_parent_index[b] = p3d.bone_parent_index[b];
    }

    memcpy(model.bone_inverse_model_space_pose_matrix, p3d.inverse_model_space_pose_matrix, p3d.static_info->num_bones*sizeof(p3dMatrix));

    int p3d_animation_joint_offset = 0;
    for (int a = 0; a < p3d.static_info->num_animations; a += 1) {
        for (int f = 0; f < model.animation[a].num_frames; f += 1) {
            for (int j = 0; j < p3d.static_info->num_bones; j += 1) {
                peAnimationJoint *pe_animation_joint = &model.animation[a].frames[f * p3d.static_info->num_bones + j];
                p3dAnimationJoint *p3d_animation_joint_ptr = p3d.animation_joint + p3d_animation_joint_offset;
                memcpy(pe_animation_joint->translation.Elements, &p3d_animation_joint_ptr->position, 3*sizeof(float));
                memcpy(pe_animation_joint->rotation.Elements, &p3d_animation_joint_ptr->rotation, 4*sizeof(float));
                memcpy(pe_animation_joint->scale.Elements, &p3d_animation_joint_ptr->scale, 3*sizeof(float));

                p3d_animation_joint_offset += 1;
            }
        }
    }

	size_t p3d_index_size = p3d.static_info->num_index * sizeof(*p3d.index);
#if defined(_WIN32)
    model.pos_buffer = pe_d3d11_create_buffer(pos_buffer, (UINT)pos_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.norm_buffer = pe_d3d11_create_buffer(nor_buffer, (UINT)nor_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.tex_buffer = pe_d3d11_create_buffer(tex_buffer, (UINT)tex_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.color_buffer = pe_d3d11_create_buffer(col_buffer, p3d.static_info->num_vertex*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.bone_index_buffer = pe_d3d11_create_buffer(bone_index_buffer, (UINT)bone_index_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.bone_weight_buffer = pe_d3d11_create_buffer(bone_weight_buffer, (UINT)bone_weight_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.index_buffer = pe_d3d11_create_buffer(p3d.index, (UINT)p3d_index_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER);
#endif
#if defined(__linux__)
	size_t total_size = pos_buffer_size + nor_buffer_size + tex_buffer_size + bone_index_buffer_size + bone_weight_buffer_size;

	glGenVertexArrays(1, &model.vertex_array_object);
    glBindVertexArray(model.vertex_array_object);

    glGenBuffers(1, &model.vertex_buffer_object);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, total_size, NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0,                                                                      pos_buffer_size, pos_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size,                                                        nor_buffer_size, nor_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size+nor_buffer_size,                                        tex_buffer_size, tex_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size+nor_buffer_size+tex_buffer_size,                        bone_index_buffer_size, bone_index_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, pos_buffer_size+nor_buffer_size+tex_buffer_size+bone_index_buffer_size, bone_weight_buffer_size, bone_weight_buffer);

    glVertexAttribPointer( 0, 3, GL_FLOAT,        GL_FALSE, 0,    (void*)0);
    glVertexAttribPointer( 1, 3, GL_FLOAT,        GL_FALSE, 0,    (void*)(pos_buffer_size));
	glVertexAttribPointer( 2, 2, GL_FLOAT,        GL_FALSE, 0,    (void*)(pos_buffer_size+nor_buffer_size));
	glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT,           0,    (void*)(pos_buffer_size+nor_buffer_size+tex_buffer_size));
	glVertexAttribPointer( 4, 4, GL_FLOAT,        GL_FALSE, 0,    (void*)(pos_buffer_size+nor_buffer_size+tex_buffer_size+bone_index_buffer_size));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);

    glGenBuffers(1, &model.element_buffer_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.element_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, p3d_index_size, p3d.index, GL_STATIC_DRAW);

    glBindVertexArray(0);
#endif
    pe_arena_temp_end(temp_arena_memory);

    return model;
#elif defined(PSP)
	return pe_model_load_psp(temp_arena, file_path);
#endif
}

#include "pe_time.h"

float frame_time = 1.0f/25.0f;
float current_frame_time = 0.0f;
int frame_index = 0;
uint64_t last_frame_time;
bool last_frame_time_initialized = false;

void pe_model_draw(peModel *model, peArena *temp_arena, HMM_Vec3 position, HMM_Vec3 rotation) {
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);
	if (!last_frame_time_initialized) {
		last_frame_time = pe_time_now();
		last_frame_time_initialized = true;
	}
	current_frame_time += (float)pe_time_sec(pe_time_laptime(&last_frame_time));
	if (current_frame_time >= frame_time) {
		frame_index = (frame_index + 1) % model->animation[0].num_frames;
		current_frame_time -= frame_time;
	}

#if defined(_WIN32)
    HMM_Mat4 rotate_x = HMM_Rotate_RH(rotation.X, (HMM_Vec3){1.0f, 0.0f, 0.0f});
    HMM_Mat4 rotate_y = HMM_Rotate_RH(rotation.Y, (HMM_Vec3){0.0f, 1.0f, 0.0f});
    HMM_Mat4 rotate_z = HMM_Rotate_RH(rotation.Z, (HMM_Vec3){0.0f, 0.0f, 1.0f});
    HMM_Mat4 translate = HMM_Translate(position);

    HMM_Mat4 model_matrix = HMM_MulM4(HMM_MulM4(HMM_MulM4(translate, rotate_z), rotate_y), rotate_x);
    peShaderConstant_Model *constant_model = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_model_buffer);
    constant_model->matrix = model_matrix;
    pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_model_buffer);

    ID3D11Buffer *buffs[] = {
        model->pos_buffer,
        model->norm_buffer,
        model->tex_buffer,
        model->color_buffer,
        model->bone_index_buffer,
        model->bone_weight_buffer,
    };
    uint32_t strides[] = {
        3*sizeof(float),
        3*sizeof(float),
        2*sizeof(float),
        sizeof(uint32_t),
        4*sizeof(uint32_t),
        4*sizeof(float),
    };
    uint32_t offsets[6] = {0, 0, 0, 0, 0, 0};

    ID3D11DeviceContext_IASetPrimitiveTopology(pe_d3d.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetVertexBuffers(pe_d3d.context, 0, 6, buffs, strides, offsets);
    ID3D11DeviceContext_IASetIndexBuffer(pe_d3d.context, model->index_buffer, DXGI_FORMAT_R32_UINT, 0);

    {
        peAnimationJoint *model_space_joints = pe_arena_alloc(temp_arena, model->num_bone * sizeof(peAnimationJoint));
        peAnimationJoint *animation_joints = &model->animation[0].frames[frame_index * model->num_bone];
        for (int b = 0; b < model->num_bone; b += 1) {
            if (model->bone_parent_index[b] < UINT8_MAX) {
                peAnimationJoint parent_transform = model_space_joints[model->bone_parent_index[b]];
                model_space_joints[b] = pe_concatenate_animation_joints(parent_transform, animation_joints[b]);
            } else {
                model_space_joints[b] = animation_joints[b];
            }
        }

        peShaderConstant_Skeleton *constant_skeleton = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_skeleton_buffer);
        constant_skeleton->has_skeleton = true;
        for (int b = 0; b < model->num_bone; b += 1) {
            peAnimationJoint *animation_joint = &model_space_joints[b];
			HMM_Mat4 translation = HMM_Translate(animation_joint->translation);
			HMM_Mat4 rotation = HMM_QToM4(animation_joint->rotation);
			HMM_Mat4 scale = HMM_Scale(animation_joint->scale);
			HMM_Mat4 transform = HMM_MulM4(translation, HMM_MulM4(scale, rotation));

            HMM_Mat4 final_bone_matrix = HMM_MulM4(transform, model->bone_inverse_model_space_pose_matrix[b]);
            constant_skeleton->matrix_bone[b] = final_bone_matrix;
        }
        pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_skeleton_buffer);
    }

    for (int m = 0; m < model->num_mesh; m += 1) {
        peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(pe_d3d.context, pe_shader_constant_material_buffer);
        constant_material->diffuse_color = pe_color_to_vec4(model->material[m].diffuse_color);
        pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_material_buffer);

		if (model->material[m].has_diffuse_map) {
			pe_texture_bind(model->material[m].diffuse_map);
		} else {
            pe_texture_bind_default();
		}

        ID3D11DeviceContext_DrawIndexed(pe_d3d.context, model->num_index[m], model->index_offset[m], model->vertex_offset[m]);
    }
#endif
#if defined(__linux__)
    HMM_Mat4 rotate_x = HMM_Rotate_RH(rotation.X, (HMM_Vec3){1.0f, 0.0f, 0.0f});
    HMM_Mat4 rotate_y = HMM_Rotate_RH(rotation.Y, (HMM_Vec3){0.0f, 1.0f, 0.0f});
    HMM_Mat4 rotate_z = HMM_Rotate_RH(rotation.Z, (HMM_Vec3){0.0f, 0.0f, 1.0f});
    HMM_Mat4 translate = HMM_Translate(position);
    HMM_Mat4 model_matrix = HMM_MulM4(HMM_MulM4(HMM_MulM4(translate, rotate_z), rotate_y), rotate_x);
	pe_shader_set_mat4(pe_opengl.shader_program, "matrix_model", &model_matrix);

    {
        peAnimationJoint *model_space_joints = pe_arena_alloc(temp_arena, model->num_bone * sizeof(peAnimationJoint));
        peAnimationJoint *animation_joints = &model->animation[0].frames[frame_index * model->num_bone];
        for (int b = 0; b < model->num_bone; b += 1) {
            if (model->bone_parent_index[b] < UINT8_MAX) {
                peAnimationJoint parent_transform = model_space_joints[model->bone_parent_index[b]];
                model_space_joints[b] = pe_concatenate_animation_joints(parent_transform, animation_joints[b]);
            } else {
                model_space_joints[b] = animation_joints[b];
            }
        }


		HMM_Mat4 *final_bone_matrix = pe_arena_alloc(temp_arena, model->num_bone * sizeof(HMM_Mat4));
        for (int b = 0; b < model->num_bone; b += 1) {
            peAnimationJoint *animation_joint = &model_space_joints[b];
			HMM_Mat4 translation = HMM_Translate(animation_joint->translation);
			HMM_Mat4 rotation = HMM_QToM4(animation_joint->rotation);
			HMM_Mat4 scale = HMM_Scale(animation_joint->scale);
			HMM_Mat4 transform = HMM_MulM4(translation, HMM_MulM4(scale, rotation));
            final_bone_matrix[b] = HMM_MulM4(transform, model->bone_inverse_model_space_pose_matrix[b]);
        }

		pe_shader_set_bool(pe_opengl.shader_program, "has_skeleton", true);
		pe_shader_set_mat4_array(pe_opengl.shader_program, "matrix_bone", final_bone_matrix, model->num_bone);
    }

	glBindVertexArray(model->vertex_array_object);
	for (int m = 0; m < model->num_mesh; m += 1) {
		if (model->material[m].has_diffuse_map) {
			pe_texture_bind(model->material[m].diffuse_map);
		} else {
            pe_texture_bind_default();
		}

		glDrawElementsBaseVertex(
			GL_TRIANGLES, model->num_index[m], GL_UNSIGNED_INT,
			(void*)(model->index_offset[m]*sizeof(uint32_t)), model->vertex_offset[m]
		);
	}
	glBindVertexArray(0);
#endif
#if defined(PSP)
	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();
	sceGumLoadIdentity();
	sceGumTranslate((ScePspFVector3 *)&position);
	ScePspFVector3 scale_vector = { model->scale, model->scale, model->scale };
	sceGumRotateXYZ((ScePspFVector3 *)&rotation);
	sceGumScale(&scale_vector);

	pe_profile_region_begin(peProfileRegion_ModelDraw_ConcatenateAnimationJoints);
	peAnimationJoint *model_space_joints = pe_arena_alloc(temp_arena, model->num_bone * sizeof(peAnimationJoint));
	peAnimationJoint *animation_joints = &model->animation[0].frames[frame_index * model->num_bone];
	for (int b = 0; b < model->num_bone; b += 1) {
		if (model->bone_parent_index[b] < UINT16_MAX) {
			peAnimationJoint parent_transform = model_space_joints[model->bone_parent_index[b]];
			model_space_joints[b] = pe_concatenate_animation_joints(parent_transform, animation_joints[b]);
		} else {
			model_space_joints[b] = animation_joints[b];
		}
	}
	pe_profile_region_end(peProfileRegion_ModelDraw_ConcatenateAnimationJoints);

	pe_profile_region_begin(peProfileRegion_ModelDraw_CalculateMatrices);
	HMM_Mat4 *final_bone_matrix = pe_arena_alloc(temp_arena, model->num_bone * sizeof(HMM_Mat4));
	PE_ASSERT(final_bone_matrix != NULL);
	for (int b = 0; b < model->num_bone; b += 1) {
		peAnimationJoint *animation_joint = &model_space_joints[b];
		HMM_Mat4 translation = HMM_Translate(animation_joint->translation);
		HMM_Mat4 rotation = HMM_QToM4(animation_joint->rotation);
		HMM_Mat4 scale = HMM_Scale(animation_joint->scale);
		HMM_Mat4 transform = HMM_MulM4(translation, HMM_MulM4(scale, rotation));

		HMM_Mat4 *inverse_model_space = &model->bone_inverse_model_space_pose_matrix[b];

		final_bone_matrix[b] = HMM_MulM4(transform, *inverse_model_space);
	}
	pe_profile_region_end(peProfileRegion_ModelDraw_CalculateMatrices);

	ScePspFMatrix4 bone_matrix[8];

	for (int m = 0; m < model->num_mesh; m += 1) {
		peSubskeleton *subskeleton = &model->subskeleton[model->mesh_subskeleton[m]];
		pe_profile_region_begin(peProfileRegion_ModelDraw_AssignMatrices);
		for (int b = 0; b < subskeleton->num_bones; b += 1) {
			sceGuBoneMatrix(b, (void*)&final_bone_matrix[subskeleton->bone_indices[b]]);
			sceGuMorphWeight(b, 1.0f);
		}
		pe_profile_region_end(peProfileRegion_ModelDraw_AssignMatrices);

		peMaterial *mesh_material = &model->material[model->mesh_material[m]];

		uint32_t diffuse_color = mesh_material->diffuse_color.rgba;
		sceGuColor(diffuse_color);

		pe_profile_region_begin(peProfileRegion_ModelDraw_BindTexture);
		if (mesh_material->has_diffuse_map) {
            pe_texture_bind(mesh_material->diffuse_map);
		} else {
            pe_texture_bind_default();
		}
		pe_profile_region_end(peProfileRegion_ModelDraw_BindTexture);

		pe_profile_region_begin(peProfileRegion_ModelDraw_DrawArray);
		int count = (model->mesh[m].index != NULL) ? model->mesh[m].num_index : model->mesh[m].num_vertex;
		sceGumDrawArray(GU_TRIANGLES, model->mesh[m].vertex_type, count, model->mesh[m].index, model->mesh[m].vertex);
		pe_profile_region_end(peProfileRegion_ModelDraw_DrawArray);
	}
	sceGuColor(0xFFFFFFFF);
	sceGumPopMatrix();
#endif
	pe_arena_temp_end(temp_arena_memory);
}