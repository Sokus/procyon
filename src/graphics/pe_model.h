#ifndef PE_MODEL_H
#define PE_MODEL_H

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

typedef struct peMaterial {
    pColor diffuse_color;
    bool has_diffuse_map;
    pTexture diffuse_map;
} peMaterial;

typedef struct peMesh {
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
} peMesh;

// desktop
typedef struct peVertexSkinned {
    float position[3];
    float normal[3];
    float texcoord[2];
    uint32_t color;
    uint32_t bone_index[4];
    float bone_weight[4];
} peVertexSkinned;

typedef struct peAnimationJoint {
    pVec3 translation;
    pQuat rotation;
    pVec3 scale;
} peAnimationJoint;

typedef struct peAnimation {
    char name[64];
    uint16_t num_frames;
    peAnimationJoint *frames; // count = num_frames * num_bones
} peAnimation;

typedef struct peSubskeleton {
	uint8_t num_bones;
	uint8_t bone_indices[8];
} peSubskeleton;

typedef struct peModel {
    pString model_path; // ./res/characters/fox (without extension!)

    float scale;
    int num_mesh;
    int num_material;
    int num_bone;
    int num_animations;
#if defined(__PSP__)
    int num_subskeleton;
#endif

    peMesh *mesh;
    peMaterial *material;
    int *mesh_material;

    pMat4 *bone_inverse_model_space_pose_matrix;
    uint16_t *bone_parent_index;
#if defined(__PSP__)
    peSubskeleton *subskeleton;
    int *mesh_subskeleton;
#endif

    peAnimation *animation;
} peModel;

peMaterial pe_default_material(void);
peAnimationJoint pe_concatenate_animation_joints(peAnimationJoint parent, peAnimationJoint child);

typedef struct p3dFile p3dFile;
typedef struct p3dStaticInfo p3dStaticInfo;
typedef struct p3dVertex p3dVertex;
typedef struct p3dAnimation p3dAnimation;
typedef struct p3dAnimationJoint p3dAnimationJoint;
bool pe_parse_p3d_static_info(p3dStaticInfo *static_info);
void pe_model_alloc(peModel *model, pArena *arena, p3dFile *p3d);
void pe_model_alloc_mesh_data(peModel *model, pArena *arena, p3dFile *p3d);
peModel pe_model_load(pArena *temp_arena, char *file_path);
void pe_model_load_static_info(peModel *model, p3dStaticInfo *static_info);
void pe_model_load_mesh_data(peModel *model, pArena *temp_arena, p3dFile *p3d);
peVertexSkinned pe_vertex_skinned_from_p3d(p3dVertex vertex_p3d, float scale);
void pe_model_load_skeleton(peModel *model, p3dFile *file);
void pe_model_load_animations(peModel *model, p3dStaticInfo *static_info, p3dAnimation *animation, p3dAnimationJoint *animation_joint);
void pe_model_load_writeback_arena(pArena *model_arena);

void pe_model_draw(peModel *model, pArena *temp_arena, pVec3 position, pVec3 rotation);
void pe_model_draw_meshes(peModel *model, pMat4 *final_bone_matrix);

#endif // PE_MODEL_H
