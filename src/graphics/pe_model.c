#include "pe_model.h"

#include "core/pe_core.h"
#include "core/pe_file_io.h"
#include "platform/pe_platform.h"
#include "utility/pe_trace.h"
#include "platform/pe_time.h"

#include "pp3d.h"

#if defined(_WIN32)
	#include "pe_graphics_win32.h"

    #define COBJMACROS
    #include <d3d11.h>
    #include <dxgi1_2.h>

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

#elif defined(__linux__)
	#include "pe_graphics_linux.h"
#elif defined(PSP)
    #include "pe_graphics_psp.h"
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

peAnimationJoint pe_concatenate_animation_joints(peAnimationJoint parent, peAnimationJoint child) {
	peAnimationJoint result;
	result.scale = p_vec3_mul(child.scale, parent.scale);
	result.rotation = p_quat_mul(parent.rotation, child.rotation);
	pMat4 parent_rotation_matrix = p_quat_to_mat4(parent.rotation);
	pVec3 child_scaled_position = p_vec3_mul(child.translation, parent.scale);
	result.translation = p_vec3_add(p_mat4_mul_vec4(parent_rotation_matrix, p_vec4v(child_scaled_position, 1.0f)).xyz, parent.translation);
	return result;
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

static void* pe_push_buffer_pointer(void **ptr, size_t amount, size_t *contents_left) {
    if (amount > *contents_left) return NULL;
    void *result = *ptr;
    *ptr = (uint8_t*)*ptr + amount;
    *contents_left -= amount;
    return result;
}

bool pe_parse_p3d(peArena *arena, peFileContents p3d_file_contents, pp3dFile *p3d) {
	void *p3d_file_pointer = p3d_file_contents.data;
	size_t p3d_file_contents_left = p3d_file_contents.size;

	p3d->static_info = pe_push_buffer_pointer(&p3d_file_pointer, sizeof(pp3dStaticInfo), &p3d_file_contents_left);
	if (!p3d->static_info) return false;

    if (pe_target_platform() == peTargetPlatform_PSP) {
        if (!p3d->static_info->is_portable) return false;
        int8_t bone_weight_size = p3d->static_info->bone_weight_size;
        bool bone_weight_valid = (bone_weight_size == 1 || bone_weight_size == 2 || bone_weight_size == 4);
    	if (!bone_weight_valid) return false;
    } else {
        if (p3d->static_info->is_portable) return false;
    }

	size_t mesh_size = p3d->static_info->num_meshes * sizeof(pp3dMesh);
	p3d->mesh = pe_push_buffer_pointer(&p3d_file_pointer, mesh_size, &p3d_file_contents_left);
	if (!p3d->mesh) return false;

    size_t material_size = p3d->static_info->num_materials * sizeof(pp3dMaterial);
	p3d->material = pe_push_buffer_pointer(&p3d_file_pointer, material_size, &p3d_file_contents_left);
	if (!p3d->material) return false;

    if (p3d->static_info->is_portable) {
    	size_t subskeleton_size = p3d->static_info->num_subskeletons * sizeof(pp3dSubskeleton);
    	p3d->subskeleton = pe_push_buffer_pointer(&p3d_file_pointer, subskeleton_size, &p3d_file_contents_left);
    	if (!p3d->subskeleton) return false;
    }

	size_t animation_size = p3d->static_info->num_animations * sizeof(pp3dAnimation);
	p3d->animation = pe_push_buffer_pointer(&p3d_file_pointer, animation_size, &p3d_file_contents_left);
	if (!p3d->animation) return false;

    if (!p3d->static_info->is_portable) {
    	size_t position_size = 3*p3d->static_info->num_vertex*sizeof(int16_t);
    	size_t normal_size = 3*p3d->static_info->num_vertex*sizeof(int16_t);
    	size_t texcoord_size = 2*p3d->static_info->num_vertex*sizeof(int16_t);
    	size_t bone_index_size = 4*p3d->static_info->num_vertex*sizeof(uint8_t);
    	size_t bone_weight_size = 4*p3d->static_info->num_vertex*sizeof(int16_t);
    	p3d->desktop.position = pe_push_buffer_pointer(&p3d_file_pointer, position_size, &p3d_file_contents_left);
    	p3d->desktop.normal = pe_push_buffer_pointer(&p3d_file_pointer, normal_size, &p3d_file_contents_left);
    	p3d->desktop.texcoord = pe_push_buffer_pointer(&p3d_file_pointer, texcoord_size, &p3d_file_contents_left);
    	p3d->desktop.bone_index = pe_push_buffer_pointer(&p3d_file_pointer, bone_index_size, &p3d_file_contents_left);
    	p3d->desktop.bone_weight = pe_push_buffer_pointer(&p3d_file_pointer, bone_weight_size, &p3d_file_contents_left);
    	if (!p3d->desktop.position) return false;
    	if (!p3d->desktop.normal) return false;
    	if (!p3d->desktop.texcoord) return false;
    	if (!p3d->desktop.bone_index) return false;
    	if (!p3d->desktop.bone_weight) return false;

    	if (p3d->static_info->num_index > 0) {
    		size_t index_size = p3d->static_info->num_index * sizeof(uint32_t);
        	p3d->desktop.index = pe_push_buffer_pointer(&p3d_file_pointer, index_size, &p3d_file_contents_left);
        	if (!p3d->desktop.index) return false;
    	} else {
    		p3d->desktop.index = NULL;
    	}
    }

    if (p3d->static_info->is_portable) {
    	p3d->portable.vertex = (void**)pe_arena_alloc(arena, p3d->static_info->num_meshes * sizeof(void*));
    	p3d->portable.vertex_size = (size_t *)pe_arena_alloc(arena, p3d->static_info->num_meshes * sizeof(size_t));
    	p3d->portable.index = (void**)pe_arena_alloc(arena, p3d->static_info->num_meshes * sizeof(void*));
    	p3d->portable.index_size = pe_arena_alloc(arena, p3d->static_info->num_meshes * sizeof(size_t));
    	for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
    		uint8_t num_weights = p3d->subskeleton[p3d->mesh[m].subskeleton_index].num_bones;
    		bool has_diffuse_map = (
    			p3d->material[p3d->mesh[m].material_index].diffuse_image_file_name[0] != '\0'
    		);
    		size_t single_vertex_size = (
    			num_weights * (size_t)(p3d->static_info->bone_weight_size) + // weights
    			(num_weights % 2) * sizeof(uint8_t) + // weight padding
    			(has_diffuse_map ? 2 * sizeof(int16_t) : 0) + // uv
    			3 * sizeof(int16_t) +         // normal
    			3 * sizeof(int16_t)	          // position
    		);

    		size_t vertex_size = single_vertex_size * p3d->mesh[m].num_vertex;
        	p3d->portable.vertex[m] = pe_push_buffer_pointer(&p3d_file_pointer, vertex_size, &p3d_file_contents_left);
        	if (!p3d->portable.vertex[m]) return false;
    		p3d->portable.vertex_size[m] = vertex_size;

    		if (p3d->mesh[m].num_index > 0) {
    			size_t index_size = p3d->mesh[m].num_index * sizeof(uint16_t);
            	p3d->portable.index[m] = pe_push_buffer_pointer(&p3d_file_pointer, index_size, &p3d_file_contents_left);
            	if (!p3d->portable.index[m]) return false;
    			p3d->portable.index_size[m] = index_size;
    		} else {
    			p3d->portable.index[m] = NULL;
    			p3d->portable.index_size[m] = 0;
    		}
    	}
    }

	size_t bone_parent_index_size = p3d->static_info->num_bones * sizeof(uint16_t);
	p3d->bone_parent_index = pe_push_buffer_pointer(&p3d_file_pointer, bone_parent_index_size, &p3d_file_contents_left);
	if (!p3d->bone_parent_index) return false;

	size_t inverse_model_space_pose_matrix_size = p3d->static_info->num_bones * sizeof(pp3dMatrix);
	p3d->inverse_model_space_pose_matrix = pe_push_buffer_pointer(&p3d_file_pointer, inverse_model_space_pose_matrix_size, &p3d_file_contents_left);
	if (!p3d->inverse_model_space_pose_matrix) return false;

	size_t animation_joint_size = (size_t)p3d->static_info->num_frames_total * (size_t)p3d->static_info->num_bones * sizeof(pp3dAnimationJoint);
	p3d->animation_joint = pe_push_buffer_pointer(&p3d_file_pointer, animation_joint_size, &p3d_file_contents_left);
	if (!p3d->animation_joint) return false;

    return true;
}

void pe_model_alloc_mesh_data(peModel *model, peArena *arena, pp3dFile *p3d) {
#if defined(__PSP__)
    size_t mesh_material_size = p3d->static_info->num_meshes * sizeof(int); // psp
    size_t mesh_subskeleton_size = p3d->static_info->num_meshes * sizeof(int); // psp
    size_t subskeleton_size = p3d->static_info->num_subskeletons * sizeof(peSubskeleton); // psp

    PE_ASSERT(p3d->static_info->is_portable);
    model->psp.mesh_material = pe_arena_alloc(arena, mesh_material_size);
    model->psp.mesh_subskeleton = pe_arena_alloc(arena, mesh_subskeleton_size);
    model->psp.subskeleton = pe_arena_alloc(arena, subskeleton_size);
    size_t psp_vertex_memory_alignment = 16;
    for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
    	model->mesh[m].vertex = pe_arena_alloc_align(arena, p3d->portable.vertex_size[m], psp_vertex_memory_alignment);
    	if (p3d->mesh[m].num_index > 0) {
    		model->mesh[m].index = pe_arena_alloc_align(arena, p3d->portable.index_size[m], psp_vertex_memory_alignment);
    	}
}
#endif
}

void pe_model_alloc(peModel *model, peArena *arena, pp3dFile *p3d) {
    size_t material_size = p3d->static_info->num_meshes * sizeof(peMaterial);
    size_t bone_inverse_model_space_pose_matrix_size = p3d->static_info->num_bones * sizeof(pMat4);
    size_t animation_size = p3d->static_info->num_animations * sizeof(peAnimation);
	size_t bone_parent_index_size = p3d->static_info->num_bones * sizeof(uint16_t);

    size_t num_index_size = p3d->static_info->num_meshes * sizeof(unsigned int); // desktop
    size_t index_offset_size = p3d->static_info->num_meshes * sizeof(unsigned int); // desktop
    size_t vertex_offset_size = p3d->static_info->num_meshes * sizeof(int); // desktop

    size_t mesh_size = p3d->static_info->num_meshes * sizeof(peMesh); // psp

	model->material = pe_arena_alloc(arena, material_size);
	model->bone_inverse_model_space_pose_matrix = pe_arena_alloc(arena, bone_inverse_model_space_pose_matrix_size);

	model->animation = pe_arena_alloc(arena, animation_size);
	for (int a = 0; a < p3d->static_info->num_animations; a += 1) {
		size_t num_animation_joint = p3d->animation[a].num_frames * p3d->static_info->num_bones;
		size_t animation_joint_size = num_animation_joint * sizeof(peAnimationJoint);
		model->animation[a].frames = pe_arena_alloc(arena, animation_joint_size);
	}

    model->bone_parent_index = pe_arena_alloc(arena, bone_parent_index_size);
	model->mesh = pe_arena_alloc(arena, mesh_size);
	pe_model_alloc_mesh_data(model, arena, p3d);
}

static void pe_model_load_mesh_data(peModel *model, peArena *temp_arena, pp3dFile *p3d) {
#if defined(__PSP__)
	for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
		model->psp.mesh_material[m] = p3d->mesh[m].material_index;
		model->psp.mesh_subskeleton[m] = p3d->mesh[m].subskeleton_index;
		model->mesh[m].num_vertex = p3d->mesh[m].num_vertex;
		model->mesh[m].num_index = p3d->mesh[m].num_index;

		model->mesh[m].vertex_type = GU_NORMAL_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_3D;
		if (p3d->material[p3d->mesh[m].material_index].diffuse_image_file_name[0] != '\0') {
			model->mesh[m].vertex_type |= GU_TEXTURE_16BIT;
		}
		uint8_t num_weights = p3d->subskeleton[p3d->mesh[m].subskeleton_index].num_bones;
		if (num_weights > 0) {
			model->mesh[m].vertex_type |= GU_WEIGHTS(num_weights);
			switch (p3d->static_info->bone_weight_size) {
				case 1: model->mesh[m].vertex_type |= GU_WEIGHT_8BIT; break;
				case 2: model->mesh[m].vertex_type |= GU_WEIGHT_16BIT; break;
				case 4: model->mesh[m].vertex_type |= GU_WEIGHT_32BITF; break;
				default: PE_PANIC(); break;
			}
		}

		memcpy(model->mesh[m].vertex, p3d->portable.vertex[m], p3d->portable.vertex_size[m]);

		if (p3d->mesh[m].num_index > 0) {
			model->mesh[m].vertex_type |= GU_INDEX_16BIT;
			memcpy(model->mesh[m].index, p3d->portable.index[m], p3d->portable.index_size[m]);
		}
	}
#endif
#if defined(_WIN32)
    int vertex_offset = 0;
    int index_offset = 0;
    for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
        size_t pos_buffer_size = 3*p3d->mesh[m].num_vertex*sizeof(float);
    	size_t nor_buffer_size = 3*p3d->mesh[m].num_vertex*sizeof(float);
    	size_t tex_buffer_size = 2*p3d->mesh[m].num_vertex*sizeof(float);
    	size_t col_buffer_size = 1*p3d->mesh[m].num_vertex*sizeof(uint32_t);
    	size_t bone_index_buffer_size = 4*p3d->mesh[m].num_vertex*sizeof(uint32_t);
    	size_t bone_weight_buffer_size = 4*p3d->mesh[m].num_vertex*sizeof(float);

        float *pos_buffer = pe_arena_alloc(temp_arena, pos_buffer_size);
        float *nor_buffer = pe_arena_alloc(temp_arena, nor_buffer_size);
        float *tex_buffer = pe_arena_alloc(temp_arena, tex_buffer_size);
        uint32_t *col_buffer = pe_arena_alloc(temp_arena, col_buffer_size);
        uint32_t *bone_index_buffer = pe_arena_alloc(temp_arena, bone_index_buffer_size);
        float *bone_weight_buffer = pe_arena_alloc(temp_arena, bone_weight_buffer_size);

        for (int p = 0; p < 3*p3d->mesh[m].num_vertex; p += 1) {
            pos_buffer[p] = p3d->static_info->scale * pe_int16_to_float(p3d->desktop.position[p+3*vertex_offset], -1.0f, 1.0f);
        }

        for (int n = 0; n < 3*p3d->mesh[m].num_vertex; n += 1) {
            nor_buffer[n] = pe_int16_to_float(p3d->desktop.normal[n+3*vertex_offset], -1.0f, 1.0f);
        }

        for (int t = 0; t < 2*p3d->mesh[m].num_vertex; t += 1) {
            tex_buffer[t] = pe_int16_to_float(p3d->desktop.texcoord[t+2*vertex_offset], -1.0f, 1.0f);
        }

        for (int c = 0; c < p3d->mesh[m].num_vertex; c += 1) {
            col_buffer[c] = PE_COLOR_WHITE.rgba;
        }

        for (int b = 0; b < 4*p3d->mesh[m].num_vertex; b += 1) {
            bone_index_buffer[b] = (uint32_t)p3d->desktop.bone_index[b+4*vertex_offset];
        }

        for (int b = 0; b < 4*p3d->mesh[m].num_vertex; b += 1) {
            bone_weight_buffer[b] = pe_uint16_to_float(p3d->desktop.bone_weight[b+4*vertex_offset], 0.0f, 1.0f);
        }

        model->mesh[m].pos_buffer = pe_d3d11_create_buffer(pos_buffer, (UINT)pos_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
        model->mesh[m].norm_buffer = pe_d3d11_create_buffer(nor_buffer, (UINT)nor_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
        model->mesh[m].tex_buffer = pe_d3d11_create_buffer(tex_buffer, (UINT)tex_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
        model->mesh[m].color_buffer = pe_d3d11_create_buffer(col_buffer, (UINT)col_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
        model->mesh[m].bone_index_buffer = pe_d3d11_create_buffer(bone_index_buffer, (UINT)bone_index_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);
        model->mesh[m].bone_weight_buffer = pe_d3d11_create_buffer(bone_weight_buffer, (UINT)bone_weight_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);

        if (p3d->mesh[m].num_index > 0) {
            size_t mesh_index_size = p3d->mesh[m].num_index * sizeof(uint32_t);
            model->mesh[m].index_buffer = pe_d3d11_create_buffer(p3d->desktop.index+index_offset, (UINT)mesh_index_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0);
        } else {
            model->mesh[m].index_buffer = NULL;
        }

        model->mesh[m].num_vertex = p3d->mesh[m].num_vertex;
        model->mesh[m].num_index = p3d->mesh[m].num_index;

        vertex_offset += p3d->mesh[m].num_vertex;
        index_offset += p3d->mesh[m].num_index;
    }
#elif defined(__linux__)
	size_t pos_buffer_size = 3*p3d->static_info->num_vertex*sizeof(float);
	size_t nor_buffer_size = 3*p3d->static_info->num_vertex*sizeof(float);
	size_t tex_buffer_size = 2*p3d->static_info->num_vertex*sizeof(float);
	size_t col_buffer_size = 1*p3d->static_info->num_vertex*sizeof(uint32_t);
	size_t bone_index_buffer_size = 4*p3d->static_info->num_vertex*sizeof(uint32_t);
	size_t bone_weight_buffer_size = 4*p3d->static_info->num_vertex*sizeof(float);

    float *pos_buffer = pe_arena_alloc(temp_arena, pos_buffer_size);
    float *nor_buffer = pe_arena_alloc(temp_arena, nor_buffer_size);
    float *tex_buffer = pe_arena_alloc(temp_arena, tex_buffer_size);
    uint32_t *col_buffer = pe_arena_alloc(temp_arena, col_buffer_size);
    uint32_t *bone_index_buffer = pe_arena_alloc(temp_arena, bone_index_buffer_size);
    float *bone_weight_buffer = pe_arena_alloc(temp_arena, bone_weight_buffer_size);

    for (unsigned int p = 0; p < 3*p3d->static_info->num_vertex; p += 1) {
        pos_buffer[p] = p3d->static_info->scale * pe_int16_to_float(p3d->desktop.position[p], -1.0f, 1.0f);
    }

    for (unsigned int n = 0; n < 3*p3d->static_info->num_vertex; n += 1) {
        nor_buffer[n] = pe_int16_to_float(p3d->desktop.normal[n], -1.0f, 1.0f);
    }

    for (unsigned int t = 0; t < 2*p3d->static_info->num_vertex; t += 1) {
        tex_buffer[t] = pe_int16_to_float(p3d->desktop.texcoord[t], -1.0f, 1.0f);
    }

    for (unsigned int c = 0; c < p3d->static_info->num_vertex; c += 1) {
        col_buffer[c] = PE_COLOR_WHITE.rgba;
    }

    for (unsigned int b = 0; b < 4*p3d->static_info->num_vertex; b += 1) {
        bone_index_buffer[b] = (uint32_t)p3d->desktop.bone_index[b];
    }

    for (unsigned int b = 0; b < 4*p3d->static_info->num_vertex; b += 1) {
        bone_weight_buffer[b] = pe_uint16_to_float(p3d->desktop.bone_weight[b], 0.0f, 1.0f);
    }

    int vertex_offset = 0;
    int index_offset = 0;
    for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
        size_t mesh_pos_buffer_size = 3 * p3d->mesh[m].num_vertex * sizeof(float);
        size_t mesh_nor_buffer_size = 3 * p3d->mesh[m].num_vertex * sizeof(float);
        size_t mesh_tex_buffer_size = 2 * p3d->mesh[m].num_vertex * sizeof(float);
        size_t mesh_col_buffer_size = 1 * p3d->mesh[m].num_vertex * sizeof(uint32_t);
        size_t mesh_bone_index_buffer_size = 4 * p3d->mesh[m].num_vertex * sizeof(uint32_t);
        size_t mesh_bone_weight_buffer_size = 4 * p3d->mesh[m].num_vertex * sizeof(float);
        size_t mesh_total_size = mesh_pos_buffer_size + mesh_nor_buffer_size + mesh_tex_buffer_size + mesh_col_buffer_size + mesh_bone_index_buffer_size + mesh_bone_weight_buffer_size;

    	glGenVertexArrays(1, &model->mesh[m].vertex_array_object);
        glBindVertexArray(model->mesh[m].vertex_array_object);

        glGenBuffers(1, &model->mesh[m].vertex_buffer_object);
        glBindBuffer(GL_ARRAY_BUFFER, model->mesh[m].vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, mesh_total_size, NULL, GL_STATIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 0,                                                                                                               mesh_pos_buffer_size,         pos_buffer+3*vertex_offset);
        glBufferSubData(GL_ARRAY_BUFFER, mesh_pos_buffer_size,                                                                                            mesh_nor_buffer_size,         nor_buffer+3*vertex_offset);
    	glBufferSubData(GL_ARRAY_BUFFER, mesh_pos_buffer_size+mesh_nor_buffer_size,                                                                       mesh_tex_buffer_size,         tex_buffer+2*vertex_offset);
    	glBufferSubData(GL_ARRAY_BUFFER, mesh_pos_buffer_size+mesh_nor_buffer_size+mesh_tex_buffer_size,                                                  mesh_col_buffer_size,         col_buffer+vertex_offset);
    	glBufferSubData(GL_ARRAY_BUFFER, mesh_pos_buffer_size+mesh_nor_buffer_size+mesh_tex_buffer_size+mesh_col_buffer_size,                             mesh_bone_index_buffer_size,  bone_index_buffer+4*vertex_offset);
    	glBufferSubData(GL_ARRAY_BUFFER, mesh_pos_buffer_size+mesh_nor_buffer_size+mesh_tex_buffer_size+mesh_col_buffer_size+mesh_bone_index_buffer_size, mesh_bone_weight_buffer_size, bone_weight_buffer+4*vertex_offset);

        glVertexAttribPointer( 0, 3, GL_FLOAT,        GL_FALSE, 0,    (void*)0);
        glVertexAttribPointer( 1, 3, GL_FLOAT,        GL_FALSE, 0,    (void*)(mesh_pos_buffer_size));
    	glVertexAttribPointer( 2, 2, GL_FLOAT,        GL_FALSE, 0,    (void*)(mesh_pos_buffer_size+mesh_nor_buffer_size));
    	glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0,    (void*)(mesh_pos_buffer_size+mesh_nor_buffer_size+mesh_tex_buffer_size));
    	glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT,           0,    (void*)(mesh_pos_buffer_size+mesh_nor_buffer_size+mesh_tex_buffer_size+mesh_col_buffer_size));
    	glVertexAttribPointer( 5, 4, GL_FLOAT,        GL_FALSE, 0,    (void*)(mesh_pos_buffer_size+mesh_nor_buffer_size+mesh_tex_buffer_size+mesh_col_buffer_size+mesh_bone_index_buffer_size));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    	glEnableVertexAttribArray(2);
    	glEnableVertexAttribArray(3);
    	glEnableVertexAttribArray(4);
    	glEnableVertexAttribArray(5);

        if (p3d->mesh[m].num_index > 0) {
            size_t mesh_index_size = p3d->mesh[m].num_index * sizeof(uint32_t);
            glGenBuffers(1, &model->mesh[m].element_buffer_object);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->mesh[m].element_buffer_object);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_index_size, p3d->desktop.index+index_offset, GL_STATIC_DRAW);
        } else {
            model->mesh[m].element_buffer_object = 0;
        }

        glBindVertexArray(0);

        model->mesh[m].num_vertex = p3d->mesh[m].num_vertex;
        model->mesh[m].num_index = p3d->mesh[m].num_index;

        vertex_offset += p3d->mesh[m].num_vertex;
        index_offset += p3d->mesh[m].num_index;
    }
#endif

}

peModel pe_model_load(peArena *temp_arena, char *file_path) {
    peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);

	pp3dFile p3d;
	peFileContents p3d_file_contents = pe_file_read_contents(temp_arena, file_path, false);
	pe_parse_p3d(temp_arena, p3d_file_contents, &p3d);

    peModel model = {0};
	peArena model_arena;
    {
		// TODO: this is basically a memory leak (xD)
		// so far we are only loading a single model to test things out
		// so we don't really care, but we should allocate models in
		// another way in the future (pool allocator?)
		size_t model_memory_size = PE_KILOBYTES(512);
		void *model_memory = pe_heap_alloc(PE_KILOBYTES(512));
		pe_arena_init(&model_arena, model_memory, model_memory_size);
        pe_model_alloc(&model, &model_arena, &p3d);
    }

    model.num_mesh = (int)p3d.static_info->num_meshes;
    model.num_material = (int)p3d.static_info->num_materials;
    model.num_bone = (int)p3d.static_info->num_bones;
    model.num_animations = (int)p3d.static_info->num_animations;
    if (pe_target_platform() == peTargetPlatform_PSP) {
	   model.psp.scale = p3d.static_info->scale;
	   model.psp.num_subskeleton = p3d.static_info->num_subskeletons;
    }

    stbi_set_flip_vertically_on_load(false);
	for (int m = 0; m < p3d.static_info->num_materials; m += 1) {
		model.material[m] = pe_default_material();
		model.material[m].diffuse_color.rgba = p3d.material[m].diffuse_color;
		if (p3d.material[m].diffuse_image_file_name[0] != '\0') {
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
			size_t diffuse_map_file_length = strnlen(p3d.material[m].diffuse_image_file_name, sizeof(p3d.material[m].diffuse_image_file_name));
			memcpy(diffuse_texture_path + last_slash_index + 1, p3d.material[m].diffuse_image_file_name, diffuse_map_file_length*sizeof(char));

			int w, h, channels;
			stbi_uc *stbi_data = stbi_load(diffuse_texture_path, &w, &h, &channels, STBI_rgb_alpha);
#if defined(__PSP__)
			model.material[m].diffuse_map = pe_texture_create(temp_arena, stbi_data, w, h, GU_PSM_8888);
#else
			model.material[m].diffuse_map = pe_texture_create(stbi_data, w, h, channels);
#endif
			stbi_image_free(stbi_data);
		}
	}

    if (pe_target_platform() == peTargetPlatform_PSP) {
    	for (int s = 0; s < p3d.static_info->num_subskeletons; s += 1) {
    		model.psp.subskeleton[s].num_bones = p3d.subskeleton[s].num_bones;
    		for (int b = 0; b < p3d.subskeleton[s].num_bones; b += 1) {
    			model.psp.subskeleton[s].bone_indices[b] = p3d.subskeleton[s].bone_indices[b];
    		}
    	}
    }

    for (int a = 0; a < p3d.static_info->num_animations; a += 1) {
        PE_ASSERT(sizeof(model.animation[a].name) == sizeof(p3d.animation[a].name));
        memcpy(model.animation[a].name, p3d.animation[a].name, sizeof(model.animation[a].name));
        model.animation[a].num_frames = p3d.animation[a].num_frames;
    }

    pe_model_load_mesh_data(&model, temp_arena, &p3d);

	memcpy(model.bone_parent_index, p3d.bone_parent_index, p3d.static_info->num_bones * sizeof(uint16_t));
	PE_ASSERT(sizeof(pp3dMatrix) == sizeof(pMat4));
	memcpy(model.bone_inverse_model_space_pose_matrix, p3d.inverse_model_space_pose_matrix, p3d.static_info->num_bones * sizeof(pMat4));

    // NOTE: On desktop platforms size of peAnimationJoint may be different because
    // of SIMD types and their alignment - we need to copy each of the elements separately.
    // On the PSP this is not a concern so we can copy over entire animations and
    // it will align correctly.
	size_t p3d_animation_joint_offset = 0;
    if (pe_target_platform() == peTargetPlatform_PSP) {
        for (int a = 0; a < p3d.static_info->num_animations; a += 1) {
    		size_t num_animation_joints = p3d.static_info->num_bones * p3d.animation[a].num_frames;
    		size_t animation_joints_size = num_animation_joints * sizeof(peAnimationJoint);
    		memcpy(model.animation[a].frames, p3d.animation_joint + p3d_animation_joint_offset, animation_joints_size);
    		p3d_animation_joint_offset += animation_joints_size;
        }
    } else {
        for (int a = 0; a < p3d.static_info->num_animations; a += 1) {
            for (int f = 0; f < model.animation[a].num_frames; f += 1) {
                for (int j = 0; j < p3d.static_info->num_bones; j += 1) {
                    peAnimationJoint *pe_animation_joint = &model.animation[a].frames[f * p3d.static_info->num_bones + j];
                    pp3dAnimationJoint *p3d_animation_joint_ptr = p3d.animation_joint + p3d_animation_joint_offset;
                    memcpy(pe_animation_joint->translation.elements, &p3d_animation_joint_ptr->position, 3*sizeof(float));
                    memcpy(pe_animation_joint->rotation.elements, &p3d_animation_joint_ptr->rotation, 4*sizeof(float));
                    memcpy(pe_animation_joint->scale.elements, &p3d_animation_joint_ptr->scale, 3*sizeof(float));
                    p3d_animation_joint_offset += 1;
                }
            }
        }
    }

#if defined(__PSP__)
	sceKernelDcacheWritebackInvalidateRange(model_arena.physical_start, model_arena.total_allocated);
#endif

	pe_arena_temp_end(temp_arena_memory);

	return model;
}

float frame_time = 1.0f/25.0f;
float current_frame_time = 0.0f;
int frame_index = 0;
uint64_t last_frame_time;
bool last_frame_time_initialized = false;

void pe_model_draw(peModel *model, peArena *temp_arena, pVec3 position, pVec3 rotation) {
	PE_TRACE_FUNCTION_BEGIN();
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
    pMat4 rotate_x = p_rotate_rh(rotation.x, (pVec3){1.0f, 0.0f, 0.0f});
    pMat4 rotate_y = p_rotate_rh(rotation.y, (pVec3){0.0f, 1.0f, 0.0f});
    pMat4 rotate_z = p_rotate_rh(rotation.z, (pVec3){0.0f, 0.0f, 1.0f});
    pMat4 translate = p_translate(position);

    pMat4 model_matrix = p_mat4_mul(p_mat4_mul(p_mat4_mul(translate, rotate_z), rotate_y), rotate_x);
    peShaderConstant_Matrix *constant_model = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.model);
    constant_model->value = model_matrix;
    pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.model);

    peShaderConstant_Light *constant_light = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.light);
    constant_light->do_lighting = true;
    constant_light->light_vector = PE_LIGHT_VECTOR_DEFAULT;
    pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.light);

    {
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

        peShaderConstant_Skeleton *constant_skeleton = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.skeleton);
        constant_skeleton->has_skeleton = true;
        for (int b = 0; b < model->num_bone; b += 1) {
            peAnimationJoint *animation_joint = &model_space_joints[b];
			pMat4 translation = p_translate(animation_joint->translation);
			pMat4 rotation = p_quat_to_mat4(animation_joint->rotation);
			pMat4 scale = p_scale(animation_joint->scale);
			pMat4 transform = p_mat4_mul(translation, p_mat4_mul(scale, rotation));

            pMat4 final_bone_matrix = p_mat4_mul(transform, model->bone_inverse_model_space_pose_matrix[b]);
            constant_skeleton->matrix_bone[b] = final_bone_matrix;
        }
        pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.skeleton);
    }

    ID3D11DeviceContext_IASetPrimitiveTopology(pe_d3d.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (int m = 0; m < model->num_mesh; m += 1) {
        peShaderConstant_Material *constant_material = pe_shader_constant_begin_map(pe_d3d.context, constant_buffers_d3d.material);
        constant_material->diffuse_color = pe_color_to_vec4(model->material[m].diffuse_color);
        pe_shader_constant_end_map(pe_d3d.context, constant_buffers_d3d.material);

		if (model->material[m].has_diffuse_map) {
			pe_texture_bind(model->material[m].diffuse_map);
		} else {
            pe_texture_bind_default();
		}

        ID3D11Buffer *buffs[] = {
            model->mesh[m].pos_buffer,
            model->mesh[m].norm_buffer,
            model->mesh[m].tex_buffer,
            model->mesh[m].color_buffer,
            model->mesh[m].bone_index_buffer,
            model->mesh[m].bone_weight_buffer,
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
        ID3D11DeviceContext_IASetVertexBuffers(pe_d3d.context, 0, 6, buffs, strides, offsets);

        if (model->mesh[m].num_index > 0) {
            ID3D11DeviceContext_IASetIndexBuffer(pe_d3d.context, model->mesh[m].index_buffer, DXGI_FORMAT_R32_UINT, 0);
            ID3D11DeviceContext_DrawIndexed(pe_d3d.context, model->mesh[m].num_index, 0, 0);
        } else {
            ID3D11DeviceContext_Draw(pe_d3d.context, model->mesh[m].num_vertex, 0);
        }

    }
#endif
#if defined(__linux__)
    pMat4 rotate_x = p_rotate_rh(rotation.x, (pVec3){1.0f, 0.0f, 0.0f});
    pMat4 rotate_y = p_rotate_rh(rotation.y, (pVec3){0.0f, 1.0f, 0.0f});
    pMat4 rotate_z = p_rotate_rh(rotation.z, (pVec3){0.0f, 0.0f, 1.0f});
    pMat4 translate = p_translate(position);
    pMat4 model_matrix = p_mat4_mul(p_mat4_mul(p_mat4_mul(translate, rotate_z), rotate_y), rotate_x);

    pMat4 old_model_matrix;
    pe_shader_get_mat4(pe_opengl.shader_program, "matrix_model", &old_model_matrix);
	pe_shader_set_mat4(pe_opengl.shader_program, "matrix_model", &model_matrix);

    {
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


		pMat4 *final_bone_matrix = pe_arena_alloc(temp_arena, model->num_bone * sizeof(pMat4));
        for (int b = 0; b < model->num_bone; b += 1) {
            peAnimationJoint *animation_joint = &model_space_joints[b];
			pMat4 translation = p_translate(animation_joint->translation);
			pMat4 rotation = p_quat_to_mat4(animation_joint->rotation);
			pMat4 scale = p_scale(animation_joint->scale);
			pMat4 transform = p_mat4_mul(translation, p_mat4_mul(scale, rotation));
            final_bone_matrix[b] = p_mat4_mul(transform, model->bone_inverse_model_space_pose_matrix[b]);
        }

		pe_shader_set_bool(pe_opengl.shader_program, "has_skeleton", true);
		pe_shader_set_mat4_array(pe_opengl.shader_program, "matrix_bone", final_bone_matrix, model->num_bone);
    }

    for (int m = 0; m < model->num_mesh; m += 1) {
    	glBindVertexArray(model->mesh[m].vertex_array_object);
        pe_shader_set_vec3(pe_opengl.shader_program, "diffuse_color", pe_color_to_vec4(model->material[m].diffuse_color).rgb);

		if (model->material[m].has_diffuse_map) {
			pe_texture_bind(model->material[m].diffuse_map);
		} else {
            pe_texture_bind_default();
		}

        if (model->mesh[m].num_index > 0) {
            glDrawElements(GL_TRIANGLES, model->mesh[m].num_index, GL_UNSIGNED_INT, (void*)0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, model->mesh[m].num_vertex);
        }
    	glBindVertexArray(0);
	}

	pe_shader_set_mat4(pe_opengl.shader_program, "matrix_model", &old_model_matrix);

#endif
#if defined(PSP)
	peTraceMark tm_sce_gum = PE_TRACE_MARK_BEGIN("SCE GUM");
	sceGumMatrixMode(GU_MODEL);
	sceGumPushMatrix();
	sceGumLoadIdentity();
	sceGumTranslate((ScePspFVector3 *)&position);
	ScePspFVector3 scale_vector = { model->psp.scale, model->psp.scale, model->psp.scale };
	sceGumRotateXYZ((ScePspFVector3 *)&rotation);
	sceGumScale(&scale_vector);
	PE_TRACE_MARK_END(tm_sce_gum);

	peTraceMark tm_concat_anim_joints = PE_TRACE_MARK_BEGIN("concatenate animation joints");
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
	PE_TRACE_MARK_END(tm_concat_anim_joints);

	peTraceMark tm_calc_matrices = PE_TRACE_MARK_BEGIN("calculate matrices");
	pMat4 *final_bone_matrix = pe_arena_alloc(temp_arena, model->num_bone * sizeof(pMat4));
	PE_ASSERT(final_bone_matrix != NULL);
	for (int b = 0; b < model->num_bone; b += 1) {
		peAnimationJoint *animation_joint = &model_space_joints[b];
		pMat4 translation = p_translate(animation_joint->translation);
		pMat4 rotation = p_quat_to_mat4(animation_joint->rotation);
		pMat4 scale = p_scale(animation_joint->scale);
		pMat4 transform = p_mat4_mul(translation, p_mat4_mul(scale, rotation));

		pMat4 *inverse_model_space = &model->bone_inverse_model_space_pose_matrix[b];

		final_bone_matrix[b] = p_mat4_mul(transform, *inverse_model_space);
	}
	PE_TRACE_MARK_END(tm_calc_matrices);

	ScePspFMatrix4 bone_matrix[8];

	int gu_texture_2d_status = sceGuGetStatus(GU_TEXTURE_2D);
	for (int m = 0; m < model->num_mesh; m += 1) {
		peTraceMark tm_draw_mesh = PE_TRACE_MARK_BEGIN("draw mesh");
		peSubskeleton *subskeleton = &model->psp.subskeleton[model->psp.mesh_subskeleton[m]];
		peTraceMark tm_assign_matrices = PE_TRACE_MARK_BEGIN("assign matrices");
		for (int b = 0; b < subskeleton->num_bones; b += 1) {
			sceGuBoneMatrix(b, &final_bone_matrix[subskeleton->bone_indices[b]]._sce);
			//sceGuMorphWeight(b, 1.0f);
		}
		PE_TRACE_MARK_END(tm_assign_matrices);

		peMaterial *mesh_material = &model->material[model->psp.mesh_material[m]];

		peTraceMark tm_diffuse_color = PE_TRACE_MARK_BEGIN("diffuse color");
		uint32_t diffuse_color = mesh_material->diffuse_color.rgba;
		sceGuColor(diffuse_color);
		PE_TRACE_MARK_END(tm_diffuse_color);

		peTraceMark tm_bind_texture = PE_TRACE_MARK_BEGIN("bind texture");
		if (mesh_material->has_diffuse_map) {
            pe_texture_bind(mesh_material->diffuse_map);
		} else {
            pe_texture_bind_default();
		}
		PE_TRACE_MARK_END(tm_bind_texture);

		peTraceMark tm_draw_array = PE_TRACE_MARK_BEGIN("draw array");
		int count = (model->mesh[m].index != NULL) ? model->mesh[m].num_index : model->mesh[m].num_vertex;
		sceGumDrawArray(GU_TRIANGLES, model->mesh[m].vertex_type, count, model->mesh[m].index, model->mesh[m].vertex);
		PE_TRACE_MARK_END(tm_draw_array);
		PE_TRACE_MARK_END(tm_draw_mesh);
	}
	peTraceMark tm_diffuse_color = PE_TRACE_MARK_BEGIN("diffuse color");
	sceGuColor(0xFFFFFFFF);
	PE_TRACE_MARK_END(tm_diffuse_color);
	sceGuSetStatus(GU_TEXTURE_2D, gu_texture_2d_status);
	sceGumPopMatrix();
#endif
	pe_arena_temp_end(temp_arena_memory);
	PE_TRACE_FUNCTION_END();
}