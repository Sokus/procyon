#ifndef PE_MODEL_H
#define PE_MODEL_H

#include "pe_core.h"
#include "pe_graphics.h"
#include "p3d.h"

#include "HandmadeMath.h"

#include <stdint.h>

struct ID3D11ShaderResourceView;
struct ID3D11Buffer;

typedef struct peMaterial {
    peColor diffuse_color;
#if defined(_WIN32)
    struct ID3D11ShaderResourceView *diffuse_map;
#endif
} peMaterial;

peMaterial pe_default_material(void);

typedef struct peMesh {
    int num_vertex;
    int num_index;

#if defined(_WIN32)
    struct ID3D11Buffer *pos_buffer;
    struct ID3D11Buffer *norm_buffer;
    struct ID3D11Buffer *tex_buffer;
    struct ID3D11Buffer *color_buffer;
    struct ID3D11Buffer *index_buffer;
#endif
} peMesh;

typedef struct peAnimationJoint {
    HMM_Vec3 translation;
    HMM_Quat rotation;
    HMM_Vec3 scale;
} peAnimationJoint;

peAnimationJoint pe_concatenate_animation_joints(peAnimationJoint parent, peAnimationJoint child);

typedef struct peAnimation {
    char name[64];
    uint16_t num_frames;
    peAnimationJoint *frames; // count = num_frames * num_bones
} peAnimation;

typedef struct peModel {
    peArena arena;

    unsigned int num_vertex;
    int num_mesh;
    int num_bone;
    unsigned int *num_index;
    unsigned int *index_offset;
    int *vertex_offset;

    peMaterial *material;

    uint8_t *bone_parent_index;
    HMM_Mat4 *bone_inverse_model_space_pose_matrix;
    peAnimation *animation;

    struct ID3D11Buffer *pos_buffer;
    struct ID3D11Buffer *norm_buffer;
    struct ID3D11Buffer *tex_buffer;
    struct ID3D11Buffer *color_buffer;
    struct ID3D11Buffer *bone_index_buffer;
    struct ID3D11Buffer *bone_weight_buffer;
    struct ID3D11Buffer *index_buffer;
} peModel;

void pe_model_alloc(peModel *model, peAllocator allocator, p3dStaticInfo *p3d_static_info, p3dAnimation *p3d_animation);
peModel pe_model_load(char *file_path);
void pe_model_draw(peModel *model, HMM_Vec3 position, HMM_Vec3 rotation);



#endif // PE_MODEL_H