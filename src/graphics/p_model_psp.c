#include "graphics/p_model.h"
#include "graphics/p3d.h"
#include "core/p_assert.h"
#include "utility/pe_trace.h"

#include <stdbool.h>
#include <string.h>

#include <pspkernel.h> // sceKernelDcacheWritebackInvalidateRange
#include <pspgu.h>

bool p_parse_p3d_static_info(p3dStaticInfo *static_info) {
    if (!static_info) return false;
    if (!static_info->is_portable) return false;
    int8_t bone_weight_size = static_info->bone_weight_size;
    bool bone_weight_valid = (bone_weight_size == 1 || bone_weight_size == 2 || bone_weight_size == 4);
	if (!bone_weight_valid) return false;
    return true;
}

void p_model_alloc_mesh_data(pModel *model, pArena *arena, p3dFile *p3d) {
    size_t mesh_material_size = p3d->static_info->num_meshes * sizeof(int);
    model->mesh_material = p_arena_alloc(arena, mesh_material_size);

    P_ASSERT(p3d->static_info->is_portable);
    size_t mesh_subskeleton_size = p3d->static_info->num_meshes * sizeof(int); // psp
    size_t subskeleton_size = p3d->static_info->num_subskeletons * sizeof(pSubskeleton); // psp
    model->mesh_subskeleton = p_arena_alloc(arena, mesh_subskeleton_size);
    model->subskeleton = p_arena_alloc(arena, subskeleton_size);
    size_t psp_vertex_memory_alignment = 16;
    for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
    	model->mesh[m].vertex = p_arena_alloc_align(arena, p3d->portable.vertex_size[m], psp_vertex_memory_alignment);
    	if (p3d->mesh[m].num_index > 0) {
    		model->mesh[m].index = p_arena_alloc_align(arena, p3d->portable.index_size[m], psp_vertex_memory_alignment);
    	}
    }
}

void p_model_load_static_info(pModel *model, p3dStaticInfo *static_info) {
    model->scale = static_info->scale;
    model->num_mesh = (int)static_info->num_meshes;
    model->num_material = (int)static_info->num_materials;
    model->num_bone = (int)static_info->num_bones;
    model->num_animations = (int)static_info->num_animations;
    model->num_subskeleton = static_info->num_subskeletons;
}

void p_model_load_mesh_data(pModel *model, pArena *temp_arena, p3dFile *p3d) {
	for (int m = 0; m < p3d->static_info->num_meshes; m += 1) {
    	model->mesh_material[m] = p3d->mesh[m].material_index;
		model->mesh_subskeleton[m] = p3d->mesh[m].subskeleton_index;
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
				default: P_PANIC(); break;
			}
		}

		memcpy(model->mesh[m].vertex, p3d->portable.vertex[m], p3d->portable.vertex_size[m]);

		if (p3d->mesh[m].num_index > 0) {
			model->mesh[m].vertex_type |= GU_INDEX_16BIT;
			memcpy(model->mesh[m].index, p3d->portable.index[m], p3d->portable.index_size[m]);
		}
	}
}

void p_model_load_skeleton(pModel *model, p3dFile *p3d) {
	memcpy(model->bone_parent_index, p3d->bone_parent_index, p3d->static_info->num_bones * sizeof(uint16_t));
	P_ASSERT(sizeof(p3dMatrix) == sizeof(pMat4));
	memcpy(model->bone_inverse_model_space_pose_matrix, p3d->inverse_model_space_pose_matrix, p3d->static_info->num_bones * sizeof(pMat4));
	for (int s = 0; s < p3d->static_info->num_subskeletons; s += 1) {
		model->subskeleton[s].num_bones = p3d->subskeleton[s].num_bones;
		for (int b = 0; b < p3d->subskeleton[s].num_bones; b += 1) {
			model->subskeleton[s].bone_indices[b] = p3d->subskeleton[s].bone_indices[b];
		}
	}
}

void p_model_load_animations(pModel *model, p3dStaticInfo *static_info, p3dAnimation *animation, p3dAnimationJoint *animation_joint) {
    for (int a = 0; a < static_info->num_animations; a += 1) {
        P_ASSERT(sizeof(model->animation[a].name) == sizeof(animation[a].name));
        memcpy(model->animation[a].name, animation[a].name, sizeof(model->animation[a].name));
        model->animation[a].num_frames = animation[a].num_frames;
    }
	size_t p3d_animation_joint_offset = 0;
    for (int a = 0; a < static_info->num_animations; a += 1) {
		size_t num_animation_joints = static_info->num_bones * animation[a].num_frames;
		size_t animation_joints_size = num_animation_joints * sizeof(pAnimationJoint);
		memcpy(model->animation[a].frames, animation_joint + p3d_animation_joint_offset, animation_joints_size);
		p3d_animation_joint_offset += animation_joints_size;
    }
}

void p_model_load_writeback_arena(pArena *model_arena) {
	sceKernelDcacheWritebackInvalidateRange(model_arena->physical_start, model_arena->total_allocated);
}

void p_model_draw_meshes(pModel *model, pMat4 *final_bone_matrix) {
    P_TRACE_FUNCTION_BEGIN();
    for (int m = 0; m < model->num_mesh; m += 1) {
		pTraceMark tm_draw_mesh = P_TRACE_MARK_BEGIN("draw mesh");
        pMaterial *mesh_material = &model->material[model->mesh_material[m]];

		if (mesh_material->has_diffuse_map) {
            p_texture_bind(mesh_material->diffuse_map);
		} else {
            p_texture_bind_default();
		}

		p_graphics_set_diffuse_color(mesh_material->diffuse_color);

        pTraceMark tm_assign_matrices = P_TRACE_MARK_BEGIN("assign matrices");
        pSubskeleton *subskeleton = &model->subskeleton[model->mesh_subskeleton[m]];
		for (int b = 0; b < subskeleton->num_bones; b += 1) {
            pMat4 *final_subskeleton_bone_matrix = &final_bone_matrix[subskeleton->bone_indices[b]];
            sceGuBoneMatrix(b, &final_subskeleton_bone_matrix->_sce);
		}
        P_TRACE_MARK_END(tm_assign_matrices);

		pTraceMark tm_draw_array = P_TRACE_MARK_BEGIN("sceGumDrawArray");
		int count = (model->mesh[m].index != NULL) ? model->mesh[m].num_index : model->mesh[m].num_vertex;
		sceGumDrawArray(GU_TRIANGLES, model->mesh[m].vertex_type, count, model->mesh[m].index, model->mesh[m].vertex);
		P_TRACE_MARK_END(tm_draw_array);
		P_TRACE_MARK_END(tm_draw_mesh);
    }
    P_TRACE_FUNCTION_END();
}
