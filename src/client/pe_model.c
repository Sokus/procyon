#include "pe_model.h"

#include "pe_core.h"
#include "pe_file_io.h"
#include "win32/win32_d3d.h"
#include "win32/win32_shader.h"

extern peArena temp_arena;

peMaterial pe_default_material(void) {
    peMaterial material;
#if defined(_WIN32)
    material = (peMaterial){
        .diffuse_color = PE_COLOR_WHITE,
        .diffuse_map = pe_d3d.default_texture_view,
    };
#endif
    return material;
}

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

    model.num_vertex = p3d_static_info->num_vertex;
    model.num_mesh = (int)p3d_static_info->num_meshes;
    model.num_bone = (int)p3d_static_info->num_bones;

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
}

void pe_model_draw(peModel *model, HMM_Vec3 position, HMM_Vec3 rotation) {
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
}