#include "graphics/p_model.h"
#include "graphics/p3d.h"

#warning p_model unimplemented

bool p_parse_p3d_static_info(p3dStaticInfo *static_info) {
    return false;
}

void p_model_alloc_mesh_data(pModel *model, pArena *arena, pModelAllocSize *alloc_size) {
}

void p_model_load_static_info(pModel *model, p3dStaticInfo *static_info) {
}

void p_model_load_mesh_data(pModel *model, p3dFile *p3d) {
}

void p_model_load_skeleton(pModel *model, p3dFile *p3d) {
}

void p_model_load_animations(pModel *model, p3dStaticInfo *static_info, p3dAnimation *animation, p3dAnimationJoint *animation_joint) {
}

void p_model_load_writeback_arena(pArena *model_arena) {
}

void p_model_draw_meshes(pModel *model, pMat4 *final_bone_matrix) {
}
