#ifndef PE_MODEL_H
#define PE_MODEL_H

#include "pe_core.h"
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
#if defined(_WIN32)
    struct ID3D11ShaderResourceView *diffuse_map;
#elif defined(__linux__)
    bool has_diffuse_map;
    peTexture diffuse_map;
#elif defined(PSP)
    bool has_diffuse_map;
    peTexture diffuse_map;
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
#elif defined(PSP)
    int vertex_type;
    void *vertex;
    void *index;
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

#if defined(PSP)
typedef struct peSubskeleton {
	uint8_t num_bones;
	uint8_t bone_indices[8];
} peSubskeleton;
#endif

typedef struct peModel {
    int num_mesh;
    int num_material;
    int num_bone;
    int num_animations;

    peMaterial *material;
    HMM_Mat4 *bone_inverse_model_space_pose_matrix;
    peAnimation *animation;

#if defined(_WIN32) || defined(__linux__)
    unsigned int num_vertex;
    unsigned int *num_index;
    unsigned int *index_offset;
    int *vertex_offset;
    uint8_t *bone_parent_index;
#endif

#if defined(_WIN32)
    struct ID3D11Buffer *pos_buffer;
    struct ID3D11Buffer *norm_buffer;
    struct ID3D11Buffer *tex_buffer;
    struct ID3D11Buffer *color_buffer;
    struct ID3D11Buffer *bone_index_buffer;
    struct ID3D11Buffer *bone_weight_buffer;
    struct ID3D11Buffer *index_buffer;
#endif

#if defined(__linux__)
    GLuint vertex_array_object;
    GLuint vertex_buffer_object;
    GLuint element_buffer_object;
#endif

#if defined(PSP)
    float scale;
    peMesh *mesh;
    int *mesh_material;
    int *mesh_subskeleton;
    uint16_t *bone_parent_index; // TODO: Change this to uint8_t
    int num_subskeleton;
    peSubskeleton *subskeleton;
#endif
} peModel;

struct p3dStaticInfo;
struct p3dAnimation;

void pe_model_alloc(peModel *model, peArena *arena, struct p3dStaticInfo *p3d_static_info, struct p3dAnimation *p3d_animation);
peModel pe_model_load(char *file_path);
void pe_model_draw(peModel *model, HMM_Vec3 position, HMM_Vec3 rotation);



#endif // PE_MODEL_H