#ifndef PE_MODEL_H
#define PE_MODEL_H

#include "core/pe_core.h"
#include "math/p_math.h"
#include "pe_graphics.h"

#include "HandmadeMath.h"

#include <stdint.h>
#include <stdbool.h>


#if defined(_WIN32)
struct ID3D11ShaderResourceView;
struct ID3D11Buffer;
#endif

#if defined(__linux__)
#include "glad/glad.h"
#endif

typedef struct peMaterial {
    peColor diffuse_color;
    bool has_diffuse_map;
    peTexture diffuse_map;
} peMaterial;

peMaterial pe_default_material(void);

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

typedef struct peAnimationJoint {
    pVec3 translation;
    pQuat rotation;
    pVec3 scale;
} peAnimationJoint;

peAnimationJoint pe_concatenate_animation_joints(peAnimationJoint parent, peAnimationJoint child);

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
    peString model_path; // ./res/characters/fox (without extension!)

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

typedef struct pp3dFile pp3dFile;

void pe_model_alloc(
    peModel *model,
    peArena *arena,
    pp3dFile *p3d
);
peModel pe_model_load(peArena *temp_arena, char *file_path);
void pe_model_draw(peModel *model, peArena *temp_arena, pVec3 position, pVec3 rotation);



#endif // PE_MODEL_H