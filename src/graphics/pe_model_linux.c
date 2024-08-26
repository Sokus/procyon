#include "graphics/pe_model.h"

#include "graphics/pe_graphics_linux.h"
#include "graphics/p3d.h"
#include "utility/pe_trace.h"

#include "glad/glad.h"

#include <stdbool.h>
#include <string.h>

bool pe_parse_p3d_static_info(p3dStaticInfo *static_info) {
    if (!static_info) return false;
    if (static_info->is_portable) return false;
    return true;
}

void pe_model_alloc_mesh_data(peModel *model, peArena *arena, p3dFile *p3d) {
    size_t mesh_material_size = p3d->static_info->num_meshes * sizeof(int);
    model->mesh_material = pe_arena_alloc(arena, mesh_material_size);
}

void pe_model_load_static_info(peModel *model, p3dStaticInfo *static_info) {
    model->scale = 1.0;
    model->num_mesh = (int)static_info->num_meshes;
    model->num_material = (int)static_info->num_materials;
    model->num_bone = (int)static_info->num_bones;
    model->num_animations = (int)static_info->num_animations;
}

void pe_model_load_mesh_data(peModel *model, peArena *temp_arena, p3dFile *p3d) {
    int vertex_offset = 0;
    int index_offset = 0;
    for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
        peArenaTemp temp_arena_memory = pe_arena_temp_begin(temp_arena);

        size_t vertex_buffer_size = p3d->mesh[m].num_vertex * sizeof(peVertexSkinned);
        peVertexSkinned *vertex_buffer = pe_arena_alloc(temp_arena, vertex_buffer_size);

        for (int v = 0; v < p3d->mesh[m].num_vertex; v += 1) {
            p3dVertex *p3d_vertex = &p3d->desktop.vertex[v+vertex_offset];
            vertex_buffer[v] = pe_vertex_skinned_from_p3d(p3d->desktop.vertex[v+vertex_offset], p3d->static_info->scale);
        }

    	glGenVertexArrays(1, &model->mesh[m].vertex_array_object);
        glBindVertexArray(model->mesh[m].vertex_array_object);

        glGenBuffers(1, &model->mesh[m].vertex_buffer_object);
        glBindBuffer(GL_ARRAY_BUFFER, model->mesh[m].vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, NULL, GL_STATIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_buffer_size, vertex_buffer);

        glVertexAttribPointer( 0, 3, GL_FLOAT,        GL_FALSE, sizeof(peVertexSkinned), (void*)offsetof(peVertexSkinned, position));
        glVertexAttribPointer( 1, 3, GL_FLOAT,        GL_FALSE, sizeof(peVertexSkinned), (void*)offsetof(peVertexSkinned, normal));
    	glVertexAttribPointer( 2, 2, GL_FLOAT,        GL_FALSE, sizeof(peVertexSkinned), (void*)offsetof(peVertexSkinned, texcoord));
    	glVertexAttribPointer( 3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(peVertexSkinned), (void*)offsetof(peVertexSkinned, color));
    	glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT,           sizeof(peVertexSkinned), (void*)offsetof(peVertexSkinned, bone_index));
    	glVertexAttribPointer( 5, 4, GL_FLOAT,        GL_FALSE, sizeof(peVertexSkinned), (void*)offsetof(peVertexSkinned, bone_weight));
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

		model->mesh_material[m] = p3d->mesh[m].material_index;
        model->mesh[m].num_vertex = p3d->mesh[m].num_vertex;
        model->mesh[m].num_index = p3d->mesh[m].num_index;

        vertex_offset += p3d->mesh[m].num_vertex;
        index_offset += p3d->mesh[m].num_index;

        pe_arena_temp_end(temp_arena_memory);
    }
}

void pe_model_load_skeleton(peModel *model, p3dFile *p3d) {
	memcpy(model->bone_parent_index, p3d->bone_parent_index, p3d->static_info->num_bones * sizeof(uint16_t));
	PE_ASSERT(sizeof(p3dMatrix) == sizeof(pMat4));
	memcpy(model->bone_inverse_model_space_pose_matrix, p3d->inverse_model_space_pose_matrix, p3d->static_info->num_bones * sizeof(pMat4));
}

void pe_model_load_animations(peModel *model, p3dStaticInfo *static_info, p3dAnimation *animation, p3dAnimationJoint *animation_joint) {
    for (int a = 0; a < static_info->num_animations; a += 1) {
        PE_ASSERT(sizeof(model->animation[a].name) == sizeof(animation[a].name));
        memcpy(model->animation[a].name, animation[a].name, sizeof(model->animation[a].name));
        model->animation[a].num_frames = animation[a].num_frames;
    }
    // NOTE: On desktop platforms size of peAnimationJoint may be different because
    // of SIMD types and their alignment - we need to copy each of the elements separately.
	size_t p3d_animation_joint_offset = 0;
    for (int a = 0; a < static_info->num_animations; a += 1) {
        for (int f = 0; f < model->animation[a].num_frames; f += 1) {
            for (int j = 0; j < static_info->num_bones; j += 1) {
                peAnimationJoint *pe_animation_joint = &model->animation[a].frames[f * static_info->num_bones + j];
                p3dAnimationJoint *p3d_animation_joint_ptr = animation_joint + p3d_animation_joint_offset;
                memcpy(pe_animation_joint->translation.elements, &p3d_animation_joint_ptr->position, 3*sizeof(float));
                memcpy(pe_animation_joint->rotation.elements, &p3d_animation_joint_ptr->rotation, 4*sizeof(float));
                memcpy(pe_animation_joint->scale.elements, &p3d_animation_joint_ptr->scale, 3*sizeof(float));
                p3d_animation_joint_offset += 1;
            }
        }
    }
}

void pe_model_load_writeback_arena(peArena *model_arena) {
    // NOTE: This does nothing since we only need to invalidate the memory cache on the PSP.
}

void pe_model_draw_meshes(peModel *model, pMat4 *final_bone_matrix) {
    PE_TRACE_FUNCTION_BEGIN();
    pe_shader_set_bool(pe_opengl.shader_program, "has_skeleton", true);
    pe_shader_set_mat4_array(pe_opengl.shader_program, "matrix_bone", final_bone_matrix, model->num_bone);

    for (int m = 0; m < model->num_mesh; m += 1) {
		peTraceMark tm_draw_mesh = PE_TRACE_MARK_BEGIN("draw mesh");
        peMaterial *mesh_material = &model->material[model->mesh_material[m]];

		if (mesh_material->has_diffuse_map) {
            pe_texture_bind(mesh_material->diffuse_map);
		} else {
            pe_texture_bind_default();
		}

		pe_graphics_set_diffuse_color(mesh_material->diffuse_color);

    	glBindVertexArray(model->mesh[m].vertex_array_object);
        if (model->mesh[m].num_index > 0) {
            glDrawElements(GL_TRIANGLES, model->mesh[m].num_index, GL_UNSIGNED_INT, (void*)0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, model->mesh[m].num_vertex);
        }
    	glBindVertexArray(0);
		PE_TRACE_MARK_END(tm_draw_mesh);
    }
    PE_TRACE_FUNCTION_END();
}