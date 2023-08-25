#include "pe_model.h"

#include "pe_core.h"
#include "pe_file_io.h"

#include "p3d.h"
#include "pp3d.h"

#if defined(_WIN32)
    #include "win32/win32_d3d.h"
    #include "win32/win32_shader.h"
#elif defined(PSP)
    #include <pspkernel.h> // sceKernelDcacheWritebackInvalidateRange
    #include <pspgu.h>
    #include <pspgum.h>
#endif

#include <string.h>

extern peArena temp_arena;

peMaterial pe_default_material(void) {
    peMaterial material;
#if defined(_WIN32)
    material = (peMaterial){
        .diffuse_color = PE_COLOR_WHITE,
        .diffuse_map = pe_d3d.default_texture_view,
    };
#elif defined(PSP)
    material = (peMaterial){
        .diffuse_color = PE_COLOR_WHITE,
        .has_diffuse_map = false,
    };
#endif
    return material;
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

void pe_model_alloc(peModel *model, peAllocator allocator, p3dStaticInfo *p3d_static_info, p3dAnimation *p3d_animation) {
#if defined(_WIN32)
    size_t num_index_size = p3d_static_info->num_meshes * sizeof(unsigned int);
    size_t index_offset_size = p3d_static_info->num_meshes * sizeof(unsigned int);
    size_t vertex_offset_size = p3d_static_info->num_meshes * sizeof(int);
    size_t material_size = p3d_static_info->num_meshes * sizeof(peMaterial);
    size_t bone_parent_index_size = p3d_static_info->num_bones * sizeof(int);
    size_t bone_inverse_model_space_pose_matrix_size = p3d_static_info->num_bones * sizeof(HMM_Mat4);
    size_t animation_size = p3d_static_info->num_animations * sizeof(peAnimation);

    model->num_index = pe_alloc(allocator, num_index_size);
    model->index_offset = pe_alloc(allocator, index_offset_size);
    model->vertex_offset = pe_alloc(allocator, vertex_offset_size);
    model->material = pe_alloc(allocator, material_size);
    model->bone_parent_index = pe_alloc(allocator, bone_parent_index_size);
    model->bone_inverse_model_space_pose_matrix = pe_alloc(allocator, bone_inverse_model_space_pose_matrix_size);
    model->animation = pe_alloc(allocator, animation_size);

    for (int a = 0; a < p3d_static_info->num_animations; a += 1) {
        unsigned int num_animation_joint = p3d_animation[a].num_frames * p3d_static_info->num_bones;
        size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
        void *frames = pe_alloc(allocator, animation_joint_size);
        if (model->animation) {
            model->animation[a].frames = frames;
        }
    }
#endif
}

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

peModel pe_model_load(char *file_path) {
#if defined(_WIN32)
    peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);
    peAllocator temp_allocator = pe_arena_allocator(&temp_arena);

    peFileContents p3d_file_contents = pe_file_read_contents(temp_allocator, file_path, false);
    uint8_t *p3d_file_pointer = p3d_file_contents.data;

    p3dStaticInfo *p3d_static_info = (void*)p3d_file_pointer;
    p3d_file_pointer += sizeof(p3dStaticInfo);

    p3dMesh *p3d_mesh = (p3dMesh*)p3d_file_pointer;
    p3d_file_pointer += p3d_static_info->num_meshes * sizeof(p3dMesh);

    p3dAnimation *p3d_animation = (p3dAnimation*)p3d_file_pointer;
    p3d_file_pointer += p3d_static_info->num_animations * sizeof(p3dAnimation);

    int16_t *p3d_position = (int16_t*)p3d_file_pointer;
    p3d_file_pointer += 3 * p3d_static_info->num_vertex * sizeof(int16_t);

    int16_t *p3d_normal = (int16_t*)p3d_file_pointer;
    p3d_file_pointer += 3 * p3d_static_info->num_vertex * sizeof(int16_t);

    int16_t *p3d_texcoord = (int16_t*)p3d_file_pointer;
    p3d_file_pointer += 2 * p3d_static_info->num_vertex * sizeof(int16_t);

    uint8_t *p3d_bone_index = (uint8_t*)p3d_file_pointer;
    p3d_file_pointer += 4 * p3d_static_info->num_vertex * sizeof(uint8_t);

    uint16_t *p3d_bone_weight = (uint16_t*)p3d_file_pointer;
    p3d_file_pointer += 4 * p3d_static_info->num_vertex * sizeof(uint16_t);

    uint32_t *p3d_index = (uint32_t*)p3d_file_pointer;
    p3d_file_pointer += p3d_static_info->num_index * sizeof(uint32_t);

    uint8_t *p3d_bone_parent_indices = (uint8_t*)p3d_file_pointer;
    p3d_file_pointer += p3d_static_info->num_bones * sizeof(uint8_t);

    p3dMatrix *p3d_inverse_model_space_pose_matrix = (p3dMatrix*)p3d_file_pointer;
    p3d_file_pointer += p3d_static_info->num_bones * sizeof(p3dMatrix);

    p3dAnimationJoint *p3d_animation_joint = (p3dAnimationJoint*)p3d_file_pointer;
    p3d_file_pointer += (uintptr_t)p3d_static_info->num_frames_total * (uintptr_t)p3d_static_info->num_bones * sizeof(p3dAnimationJoint);

    peModel model = {0};
    {
        peMeasureAllocatorData measure_data = { .alignment = PE_DEFAULT_MEMORY_ALIGNMENT };
        peAllocator measure_allocator = pe_measure_allocator(&measure_data);
        pe_model_alloc(&model, measure_allocator, p3d_static_info, p3d_animation);

        pe_arena_init_from_allocator(&model.arena, pe_heap_allocator(), measure_data.total_allocated);
        peAllocator model_allocator = pe_arena_allocator(&model.arena);
        pe_model_alloc(&model, model_allocator, p3d_static_info, p3d_animation);
    }

    model.num_mesh = (int)p3d_static_info->num_meshes;
    // for P3D number of meshes is always equal to the number of materials
    // (at least it should be... lol)
    model.num_material = (int)p3d_static_info->num_meshes;
    model.num_bone = (int)p3d_static_info->num_bones;
    model.num_animations = (int)p3d_static_info->num_animations;
    model.num_vertex = p3d_static_info->num_vertex;

    for (int m = 0; m < p3d_static_info->num_meshes; m += 1) {
        model.num_index[m] = p3d_mesh[m].num_index;
        model.index_offset[m] = p3d_mesh[m].index_offset;
        PE_ASSERT(p3d_mesh[m].vertex_offset <= INT32_MAX);
        model.vertex_offset[m] = p3d_mesh[m].vertex_offset;

        model.material[m] = pe_default_material();
        model.material[m].diffuse_color.rgba = p3d_mesh[m].diffuse_color;
    }

    for (int a = 0; a < p3d_static_info->num_animations; a += 1) {
        PE_ASSERT(sizeof(model.animation[a].name) == sizeof(p3d_animation[a].name));
        memcpy(model.animation[a].name, p3d_animation[a].name, sizeof(model.animation[a].name));
        model.animation[a].num_frames = p3d_animation[a].num_frames;
    }

    float *pos_buffer = pe_alloc(temp_allocator, 3*p3d_static_info->num_vertex*sizeof(float));
    float *nor_buffer = pe_alloc(temp_allocator, 3*p3d_static_info->num_vertex*sizeof(float));
    float *tex_buffer = pe_alloc(temp_allocator, 2*p3d_static_info->num_vertex*sizeof(float));
    uint32_t *col_buffer = pe_alloc(temp_allocator, p3d_static_info->num_vertex*sizeof(uint32_t));
    uint32_t *bone_index_buffer = pe_alloc(temp_allocator, 4*p3d_static_info->num_vertex*sizeof(uint32_t));
    float *bone_weight_buffer = pe_alloc(temp_allocator, 4*p3d_static_info->num_vertex*sizeof(float));

    for (unsigned int p = 0; p < 3*p3d_static_info->num_vertex; p += 1) {
        pos_buffer[p] = p3d_static_info->scale * pe_int16_to_float(p3d_position[p], -1.0f, 1.0f);
    }

    for (unsigned int n = 0; n < 3*p3d_static_info->num_vertex; n += 1) {
        nor_buffer[n] = pe_int16_to_float(p3d_normal[n], -1.0f, 1.0f);
    }

    for (unsigned int t = 0; t < 2*p3d_static_info->num_vertex; t += 1) {
        tex_buffer[t] = pe_int16_to_float(p3d_texcoord[t], -1.0f, 1.0f);
    }

    for (unsigned int c = 0; c < p3d_static_info->num_vertex; c += 1) {
        col_buffer[c] = PE_COLOR_WHITE.rgba;
    }

    for (unsigned int b = 0; b < 4*p3d_static_info->num_vertex; b += 1) {
        bone_index_buffer[b] = (uint32_t)p3d_bone_index[b];
    }

    for (unsigned int b = 0; b < 4*p3d_static_info->num_vertex; b += 1) {
        bone_weight_buffer[b] = pe_uint16_to_float(p3d_bone_weight[b], 0.0f, 1.0f);
    }

    for (unsigned int b = 0; b < p3d_static_info->num_bones; b += 1) {
        model.bone_parent_index[b] = p3d_bone_parent_indices[b];
    }

    memcpy(model.bone_inverse_model_space_pose_matrix, p3d_inverse_model_space_pose_matrix, p3d_static_info->num_bones*sizeof(p3dMatrix));

    int p3d_animation_joint_offset = 0;
    for (int a = 0; a < p3d_static_info->num_animations; a += 1) {
        for (int f = 0; f < model.animation[a].num_frames; f += 1) {
            for (int j = 0; j < p3d_static_info->num_bones; j += 1) {
                peAnimationJoint *pe_animation_joint = &model.animation[a].frames[f * p3d_static_info->num_bones + j];
                p3dAnimationJoint *p3d_animation_joint_ptr = p3d_animation_joint + p3d_animation_joint_offset;
                memcpy(pe_animation_joint->translation.Elements, &p3d_animation_joint_ptr->position, 3*sizeof(float));
                memcpy(pe_animation_joint->rotation.Elements, &p3d_animation_joint_ptr->rotation, 4*sizeof(float));
                memcpy(pe_animation_joint->scale.Elements, &p3d_animation_joint_ptr->scale, 3*sizeof(float));

                p3d_animation_joint_offset += 1;
            }
        }
    }

    model.pos_buffer = pe_d3d11_create_buffer(pos_buffer, 3*p3d_static_info->num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.norm_buffer = pe_d3d11_create_buffer(nor_buffer, 3*p3d_static_info->num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.tex_buffer = pe_d3d11_create_buffer(tex_buffer, 2*p3d_static_info->num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.color_buffer = pe_d3d11_create_buffer(col_buffer, p3d_static_info->num_vertex*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.bone_index_buffer = pe_d3d11_create_buffer(bone_index_buffer, 4*p3d_static_info->num_vertex*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.bone_weight_buffer = pe_d3d11_create_buffer(bone_weight_buffer, 4*p3d_static_info->num_vertex*sizeof(float), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);
    model.index_buffer = pe_d3d11_create_buffer(p3d_index, p3d_static_info->num_index*sizeof(uint32_t), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER);

    pe_temp_arena_memory_end(temp_arena_memory);

    return model;
#elif defined(PSP)
    peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);
	peAllocator temp_allocator = pe_arena_allocator(&temp_arena);

	peFileContents pp3d_file_contents = pe_file_read_contents(temp_allocator, file_path, false);
	uint8_t *pp3d_file_pointer = pp3d_file_contents.data;

	pp3dStaticInfo *pp3d_static_info = (pp3dStaticInfo*)pp3d_file_pointer;
	pp3d_file_pointer += sizeof(pp3dStaticInfo);

	pp3dMesh *pp3d_mesh_info = (pp3dMesh*)pp3d_file_pointer;
	pp3d_file_pointer += pp3d_static_info->num_meshes * sizeof(pp3dMesh);

	pp3dMaterial *pp3d_material_info = (pp3dMaterial*)pp3d_file_pointer;
	pp3d_file_pointer += pp3d_static_info->num_materials * sizeof(pp3dMaterial);

	pp3dSubskeleton *pp3d_subskeleton_info = (pp3dSubskeleton*)pp3d_file_pointer;
	pp3d_file_pointer += pp3d_static_info->num_subskeletons * sizeof(pp3dSubskeleton);

	pp3dAnimation *pp3d_animation_info = (pp3dAnimation*)pp3d_file_pointer;
	pp3d_file_pointer += pp3d_static_info->num_animations * sizeof(pp3dAnimation);

	// TODO: 8 and 16 bit UVs depending on texture size
	// TODO: 8 and 16 bit indices depending on vertex count

	size_t mesh_size = pp3d_static_info->num_meshes * sizeof(peMesh);
	size_t mesh_material_size = pp3d_static_info->num_meshes * sizeof(int);
	size_t mesh_subskeleton_size = pp3d_static_info->num_meshes * sizeof(int);
	size_t material_size = pp3d_static_info->num_materials * sizeof(peMaterial);
	size_t subskeleton_size = pp3d_static_info->num_subskeletons * sizeof(peSubskeleton);
	size_t bone_parent_index_size = pp3d_static_info->num_bones * sizeof(uint16_t);
	size_t bone_inverse_model_space_pose_matrix_size = pp3d_static_info->num_bones * sizeof(HMM_Mat4);
	size_t animation_size = pp3d_static_info->num_animations * sizeof(peAnimation);

	size_t VERT_MEM_ALIGN = 16;

	peMeasureAllocatorData measure_data = { .alignment = VERT_MEM_ALIGN };
	peAllocator measure_allocator = pe_measure_allocator(&measure_data);
	pe_alloc(measure_allocator, material_size);
	pe_alloc(measure_allocator, subskeleton_size);
	pe_alloc(measure_allocator, bone_parent_index_size);
	pe_alloc(measure_allocator, bone_inverse_model_space_pose_matrix_size);
	pe_alloc(measure_allocator, animation_size);
	pe_alloc(measure_allocator, mesh_size);
	pe_alloc(measure_allocator, mesh_material_size);
	pe_alloc(measure_allocator, mesh_subskeleton_size);

	for (int a = 0; a < pp3d_static_info->num_animations; a += 1) {
		size_t num_animation_joint = pp3d_animation_info[a].num_frames * pp3d_static_info->num_bones;
		size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
		pe_alloc(measure_allocator, animation_joint_size);
	}

	for (int m = 0; m < pp3d_static_info->num_meshes; m += 1) {
		PE_ASSERT(pp3d_mesh_info[m].num_vertex > 0);
		PE_ASSERT(pp3d_mesh_info[m].subskeleton_index >= 0);
		PE_ASSERT(pp3d_mesh_info[m].subskeleton_index < pp3d_static_info->num_subskeletons);
		uint8_t num_weights = pp3d_subskeleton_info[pp3d_mesh_info[m].subskeleton_index].num_bones;
		size_t vertex_size = (
			num_weights * sizeof(float) +
			2 * sizeof(uint16_t) + // uv
			3 * sizeof(uint16_t) + // normal
			3 * sizeof(uint16_t)   // position
		);
		pe_alloc_align(measure_allocator, pp3d_mesh_info[m].num_vertex * vertex_size, VERT_MEM_ALIGN);


		if (pp3d_mesh_info[m].num_index > 0) {
			pe_alloc_align(measure_allocator, pp3d_mesh_info[m].num_index * sizeof(uint16_t), VERT_MEM_ALIGN);
		}
	}

	peModel model = {0};
	pe_arena_init_from_allocator_align(&model.arena, pe_heap_allocator(), measure_data.total_allocated, VERT_MEM_ALIGN);
	peAllocator model_allocator = pe_arena_allocator(&model.arena);

	model.scale = pp3d_static_info->scale;
	model.num_mesh = pp3d_static_info->num_meshes;
	model.num_material = pp3d_static_info->num_materials;
	model.num_subskeleton = pp3d_static_info->num_subskeletons;
	model.num_animations = pp3d_static_info->num_animations;
	model.num_bone = pp3d_static_info->num_bones;

	model.material = pe_alloc(model_allocator, material_size);
	for (int m = 0; m < pp3d_static_info->num_materials; m += 1) {
		model.material[m] = pe_default_material();
		model.material[m].diffuse_color.rgba = pp3d_material_info[m].diffuse_color;
	}

	model.subskeleton = pe_alloc(model_allocator, subskeleton_size);
	for (int s = 0; s < pp3d_static_info->num_subskeletons; s += 1) {
		model.subskeleton[s].num_bones = pp3d_subskeleton_info[s].num_bones;
		for (int b = 0; b < pp3d_subskeleton_info[s].num_bones; b += 1) {
			model.subskeleton[s].bone_indices[b] = pp3d_subskeleton_info[s].bone_indices[b];
		}
	}

	model.animation = pe_alloc(model_allocator, animation_size);
	for (int a = 0; a < pp3d_static_info->num_animations; a += 1) {
		PE_ASSERT(sizeof(model.animation[a].name) == sizeof(pp3d_animation_info[a].name));
		memcpy(model.animation[a].name, pp3d_animation_info[a].name, sizeof(model.animation[a].name));

		model.animation[a].num_frames = pp3d_animation_info[a].num_frames;

		size_t num_animation_joint = pp3d_animation_info[a].num_frames * pp3d_static_info->num_bones;
		size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
		model.animation[a].frames = pe_alloc(model_allocator, animation_joint_size);
	}

	model.bone_parent_index = pe_alloc(model_allocator, bone_parent_index_size);
	model.bone_inverse_model_space_pose_matrix = pe_alloc(model_allocator, bone_inverse_model_space_pose_matrix_size);

	model.mesh = pe_alloc(model_allocator, mesh_size);
	model.mesh_material = pe_alloc(model_allocator, mesh_material_size);
	model.mesh_subskeleton = pe_alloc(model_allocator, mesh_subskeleton_size);
	pe_zero_size(model.mesh, mesh_size);
	for (int m = 0; m < pp3d_static_info->num_meshes; m += 1) {
		model.mesh_material[m] = pp3d_mesh_info[m].material_index;
		model.mesh_subskeleton[m] = pp3d_mesh_info[m].subskeleton_index;
		model.mesh[m].num_vertex = pp3d_mesh_info[m].num_vertex;
		model.mesh[m].num_index = pp3d_mesh_info[m].num_index;

		model.mesh[m].vertex_type = GU_TEXTURE_16BIT|GU_NORMAL_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_3D;
		uint8_t num_weights = pp3d_subskeleton_info[pp3d_mesh_info[m].subskeleton_index].num_bones;
		if (num_weights > 0) {
			model.mesh[m].vertex_type |= GU_WEIGHT_32BITF|GU_WEIGHTS(num_weights);
		}

		size_t vertex_size = (
			num_weights * sizeof(float) +
			2 * sizeof(int16_t) + // uv
			3 * sizeof(int16_t) + // normal
			3 * sizeof(int16_t)	 // position
		);
		size_t total_vertex_size = vertex_size * pp3d_mesh_info[m].num_vertex;
		model.mesh[m].vertex = pe_alloc_align(model_allocator, total_vertex_size, VERT_MEM_ALIGN);
		memcpy(model.mesh[m].vertex, pp3d_file_pointer, total_vertex_size);
		pp3d_file_pointer += total_vertex_size;

		if (pp3d_mesh_info[m].num_index > 0) {
			model.mesh[m].vertex_type |= GU_INDEX_16BIT;
			size_t index_size = pp3d_mesh_info[m].num_index * sizeof(uint16_t);
			model.mesh[m].index = pe_alloc_align(model_allocator, index_size, VERT_MEM_ALIGN);
			memcpy(model.mesh[m].index, pp3d_file_pointer, index_size);
			pp3d_file_pointer += index_size;
		}
	}

	memcpy(model.bone_parent_index, pp3d_file_pointer, bone_parent_index_size);
	pp3d_file_pointer += bone_parent_index_size;

	memcpy(model.bone_inverse_model_space_pose_matrix, pp3d_file_pointer, bone_inverse_model_space_pose_matrix_size);
	pp3d_file_pointer += bone_inverse_model_space_pose_matrix_size;

	for (int a = 0; a < pp3d_static_info->num_animations; a += 1) {
		size_t num_animation_joints = pp3d_static_info->num_bones * pp3d_animation_info[a].num_frames;
		size_t animation_joints_size = num_animation_joints * sizeof(peAnimationJoint);
		memcpy(model.animation[a].frames, pp3d_file_pointer, animation_joints_size);
		pp3d_file_pointer += animation_joints_size;
	}

	sceKernelDcacheWritebackInvalidateRange(model.arena.physical_start, model.arena.total_allocated);

	pe_temp_arena_memory_end(temp_arena_memory);

	return model;
#endif
}

void pe_model_free(peModel *model) {
    pe_arena_free(&model->arena);
}

void pe_model_draw(peModel *model, HMM_Vec3 position, HMM_Vec3 rotation) {
#if defined(_WIN32)
    peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);
    peAllocator temp_allocator = pe_arena_allocator(&temp_arena);

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
        peAnimationJoint *model_space_joints = pe_alloc(temp_allocator, model->num_bone * sizeof(peAnimationJoint));
        peAnimationJoint *animation_joints = &model->animation[0].frames[0 * model->num_bone];
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
        //constant_material->has_diffuse = model->material[m].has_diffuse;
        constant_material->diffuse_color = pe_color_to_vec4(model->material[m].diffuse_color);
        pe_bind_texture(model->material[m].diffuse_map);
        pe_shader_constant_end_map(pe_d3d.context, pe_shader_constant_material_buffer);

        ID3D11DeviceContext_DrawIndexed(pe_d3d.context, model->num_index[m], model->index_offset[m], model->vertex_offset[m]);
    }

    pe_temp_arena_memory_end(temp_arena_memory);
#elif defined(PSP)
    peTempArenaMemory temp_arena_memory = pe_temp_arena_memory_begin(&temp_arena);
	peAllocator temp_allocator = pe_arena_allocator(&temp_arena);

	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();
	sceGumLoadIdentity();
	sceGumTranslate((ScePspFVector3 *)&position);
	ScePspFVector3 scale_vector = { model->scale, model->scale, model->scale };
	sceGumRotateXYZ((ScePspFVector3 *)&rotation);
	sceGumScale(&scale_vector);

	peAnimationJoint *model_space_joints = pe_alloc(temp_allocator, model->num_bone * sizeof(peAnimationJoint));
	peAnimationJoint *animation_joints = &model->animation[0].frames[0 * model->num_bone];
	for (int b = 0; b < model->num_bone; b += 1) {
		if (model->bone_parent_index[b] < UINT16_MAX) {
			peAnimationJoint parent_transform = model_space_joints[model->bone_parent_index[b]];
			model_space_joints[b] = pe_concatenate_animation_joints(parent_transform, animation_joints[b]);
		} else {
			model_space_joints[b] = animation_joints[b];
		}
	}

	ScePspFMatrix4 bone_matrix[8];

	sceGuTexImage(0, 0, 0, 0, NULL);
	for (int m = 0; m < model->num_mesh; m += 1) {
		peSubskeleton *subskeleton = &model->subskeleton[model->mesh_subskeleton[m]];

		for (int b = 0; b < subskeleton->num_bones; b += 1) {
			peAnimationJoint *animation_joint = &model_space_joints[subskeleton->bone_indices[b]];
			HMM_Mat4 translation = HMM_Translate(animation_joint->translation);
			HMM_Mat4 rotation = HMM_QToM4(animation_joint->rotation);
			HMM_Mat4 scale = HMM_Scale(animation_joint->scale);
			HMM_Mat4 transform = HMM_MulM4(translation, HMM_MulM4(scale, rotation));

			HMM_Mat4 *inverse_model_space = &model->bone_inverse_model_space_pose_matrix[subskeleton->bone_indices[b]];

			HMM_Mat4 final_bone_matrix = HMM_MulM4(transform, *inverse_model_space);

			sceGuBoneMatrix(b, (void*)&final_bone_matrix);
			sceGuMorphWeight(b, 1.0f);
		}

		peMaterial *mesh_material = &model->material[model->mesh_material[m]];

		uint32_t diffuse_color = mesh_material->diffuse_color.rgba;
		sceGuColor(diffuse_color);

		if (mesh_material->has_diffuse_map) {
            pe_texture_bind(mesh_material->diffuse_map);
		} else {
            pe_texture_unbind();
		}

		int count = (model->mesh[m].index != NULL) ? model->mesh[m].num_index : model->mesh[m].num_vertex;
		sceGumDrawArray(GU_TRIANGLES, model->mesh[m].vertex_type, count, model->mesh[m].index, model->mesh[m].vertex);
	}
	sceGuColor(0xFFFFFFFF);
	sceGumPopMatrix();

	pe_temp_arena_memory_end(temp_arena_memory);
#endif
}