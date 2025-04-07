#include "graphics/pe_model.h"

#include "graphics/p_graphics_win32.h"
#include "graphics/p3d.h"
#include "utility/pe_trace.h"

#include <stdbool.h>

#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_2.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool pe_parse_p3d_static_info(p3dStaticInfo *static_info) {
    if (!static_info) return false;
    if (static_info->is_portable) return false;
    return true;
}

void pe_model_alloc_mesh_data(peModel *model, pArena *arena, p3dFile *p3d) {
    size_t mesh_material_size = p3d->static_info->num_meshes * sizeof(int);
    model->mesh_material = p_arena_alloc(arena, mesh_material_size);
}

void pe_model_load_static_info(peModel *model, p3dStaticInfo *static_info) {
    model->scale = 1.0;
    model->num_mesh = (int)static_info->num_meshes;
    model->num_material = (int)static_info->num_materials;
    model->num_bone = (int)static_info->num_bones;
    model->num_animations = (int)static_info->num_animations;
}

void pe_model_load_mesh_data(peModel *model, pArena *temp_arena, p3dFile *p3d) {
    int vertex_offset = 0;
    int index_offset = 0;
    for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
        pArenaTemp temp_arena_memory = p_arena_temp_begin(temp_arena);

        size_t vertex_buffer_size = p3d->mesh[m].num_vertex * sizeof(peVertexSkinned);
        peVertexSkinned *vertex_buffer = p_arena_alloc(temp_arena, vertex_buffer_size);

        for (int v = 0; v < p3d->mesh[m].num_vertex; v += 1) {
            p3dVertex *p3d_vertex = &p3d->desktop.vertex[v+vertex_offset];
            vertex_buffer[v] = pe_vertex_skinned_from_p3d(p3d->desktop.vertex[v+vertex_offset], p3d->static_info->scale);
        }

        model->mesh[m].vertex_buffer = p_d3d11_create_buffer(vertex_buffer, (UINT)vertex_buffer_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0);

        if (p3d->mesh[m].num_index > 0) {
            size_t mesh_index_size = p3d->mesh[m].num_index * sizeof(uint32_t);
            model->mesh[m].index_buffer = p_d3d11_create_buffer(p3d->desktop.index+index_offset, (UINT)mesh_index_size, D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0);
        } else {
            model->mesh[m].index_buffer = NULL;
        }

		model->mesh_material[m] = p3d->mesh[m].material_index;
        model->mesh[m].num_vertex = p3d->mesh[m].num_vertex;
        model->mesh[m].num_index = p3d->mesh[m].num_index;

        vertex_offset += p3d->mesh[m].num_vertex;
        index_offset += p3d->mesh[m].num_index;

        p_arena_temp_end(temp_arena_memory);
    }
}

void pe_model_load_skeleton(peModel *model, p3dFile *p3d) {
	memcpy(model->bone_parent_index, p3d->bone_parent_index, p3d->static_info->num_bones * sizeof(uint16_t));
	P_ASSERT(sizeof(p3dMatrix) == sizeof(pMat4));
	memcpy(model->bone_inverse_model_space_pose_matrix, p3d->inverse_model_space_pose_matrix, p3d->static_info->num_bones * sizeof(pMat4));
}

void pe_model_load_animations(peModel *model, p3dStaticInfo *static_info, p3dAnimation *animation, p3dAnimationJoint *animation_joint) {
    for (int a = 0; a < static_info->num_animations; a += 1) {
        P_ASSERT(sizeof(model->animation[a].name) == sizeof(animation[a].name));
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

void pe_model_load_writeback_arena(pArena *model_arena) {
    // NOTE: This does nothing since we only need to invalidate the memory cache on the PSP.
}

void pe_model_draw_meshes(peModel *model, pMat4 *final_bone_matrix) {
    PE_TRACE_FUNCTION_BEGIN();
    pShaderConstant_Skeleton *constant_skeleton = p_shader_constant_begin_map(p_d3d.context, constant_buffers_d3d.skeleton);
    constant_skeleton->has_skeleton = true;
    memcpy(constant_skeleton->matrix_bone, final_bone_matrix, model->num_bone * sizeof(pMat4));
    p_shader_constant_end_map(p_d3d.context, constant_buffers_d3d.skeleton);

    for (int m = 0; m < model->num_mesh; m += 1) {
        peTraceMark tm_draw_mesh = PE_TRACE_MARK_BEGIN("draw mesh");
        peMaterial *mesh_material = &model->material[model->mesh_material[m]];

		if (mesh_material->has_diffuse_map) {
            p_texture_bind(mesh_material->diffuse_map);
		} else {
            p_texture_bind_default();
		}

		p_graphics_set_diffuse_color(mesh_material->diffuse_color);

        ID3D11DeviceContext_IASetPrimitiveTopology(p_d3d.context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D11Buffer *buffs[] = { model->mesh[m].vertex_buffer };
        uint32_t strides[] = { sizeof(peVertexSkinned) };
        uint32_t offsets[] = {0};
        P_ASSERT(P_COUNT_OF(buffs) == P_COUNT_OF(strides));
        P_ASSERT(P_COUNT_OF(buffs) == P_COUNT_OF(offsets));
        ID3D11DeviceContext_IASetVertexBuffers(p_d3d.context, 0, P_COUNT_OF(buffs), buffs, strides, offsets);
        if (model->mesh[m].num_index > 0) {
            ID3D11DeviceContext_IASetIndexBuffer(p_d3d.context, model->mesh[m].index_buffer, DXGI_FORMAT_R32_UINT, 0);
            ID3D11DeviceContext_DrawIndexed(p_d3d.context, model->mesh[m].num_index, 0, 0);
        } else {
            ID3D11DeviceContext_Draw(p_d3d.context, model->mesh[m].num_vertex, 0);
        }
		PE_TRACE_MARK_END(tm_draw_mesh);
    }
    PE_TRACE_FUNCTION_END();
}
