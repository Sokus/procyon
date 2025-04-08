#ifndef P_MODEL_H
#define P_MODEL_H

#include "core/p_string.h"
#include "math/p_math.h"
#include "p_graphics.h"

#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
struct ID3D11ShaderResourceView;
struct ID3D11Buffer;
#endif

#if defined(__linux__)
typedef unsigned int GLuint;
#endif

typedef struct pMaterial {
    pColor diffuse_color;
    bool has_diffuse_map;
    pTexture diffuse_map;
} pMaterial;

typedef struct pMesh {
    int num_vertex;
    int num_index;

#if defined(_WIN32)
    struct ID3D11Buffer *vertex_buffer;
    struct ID3D11Buffer *index_buffer;
#elif defined(__linux__)
    GLuint vertex_array_object;
    GLuint vertex_buffer_object;
    GLuint element_buffer_object;
#elif defined(PSP)
    int vertex_type;
    void *vertex;
    void *index;
#endif
} pMesh;

// desktop
typedef struct pVertexSkinned {
    float position[3];
    float normal[3];
    float texcoord[2];
    uint32_t color;
    uint32_t bone_index[4];
    float bone_weight[4];
} pVertexSkinned;

typedef struct pAnimationJoint {
    pVec3 translation;
    pQuat rotation;
    pVec3 scale;
} pAnimationJoint;

typedef struct pAnimation {
    char name[64];
    uint16_t num_frames;
    pAnimationJoint *frames; // count = num_frames * num_bones
} pAnimation;

typedef struct pSubskeleton {
	uint8_t num_bones;
	uint8_t bone_indices[8];
} pSubskeleton;

typedef struct pModel {
    pString model_path; // ./res/characters/fox (without extension!)

    float scale;
    int num_mesh;
    int num_material;
    int num_bone;
    int num_animations;
#if defined(__PSP__)
    int num_subskeleton;
#endif

    pMesh *mesh;
    pMaterial *material;
    int *mesh_material;

    pMat4 *bone_inverse_model_space_pose_matrix;
    uint16_t *bone_parent_index;
#if defined(__PSP__)
    pSubskeleton *subskeleton;
    int *mesh_subskeleton;
#endif

    pAnimation *animation;
} pModel;

pMaterial p_default_material(void);
pAnimationJoint p_concatenate_animation_joints(pAnimationJoint parent, pAnimationJoint child);

typedef struct p3dFile p3dFile;
typedef struct p3dStaticInfo p3dStaticInfo;
typedef struct p3dVertex p3dVertex;
typedef struct p3dAnimation p3dAnimation;
typedef struct p3dAnimationJoint p3dAnimationJoint;
bool p_parse_p3d_static_info(p3dStaticInfo *static_info);
void p_model_alloc(pModel *model, pArena *arena, p3dFile *p3d);
void p_model_alloc_mesh_data(pModel *model, pArena *arena, p3dFile *p3d);
pModel p_model_load(char *file_path);
void p_model_load_static_info(pModel *model, p3dStaticInfo *static_info);
void p_model_load_mesh_data(pModel *model, p3dFile *p3d);
pVertexSkinned p_vertex_skinned_from_p3d(p3dVertex vertex_p3d, float scale);
void p_model_load_skeleton(pModel *model, p3dFile *file);
void p_model_load_animations(pModel *model, p3dStaticInfo *static_info, p3dAnimation *animation, p3dAnimationJoint *animation_joint);
void p_model_load_writeback_arena(pArena *model_arena);

void p_model_draw(pModel *model, pVec3 position, pVec3 rotation);
void p_model_draw_meshes(pModel *model, pMat4 *final_bone_matrix);

#endif // P_MODEL_H
