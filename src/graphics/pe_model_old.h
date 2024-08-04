// NOTE: This is being kept just for reference, I don't want to delete
// the code yet but it really annoyed me while in pe_model.c

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
    uint32_t vertex_buffer_offsets[] = { 0, 0, 0, 0 };

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
#endif
