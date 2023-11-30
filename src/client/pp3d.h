#ifndef PROCYON_PORTABLE_3D_FORMAT_H
#define PROCYON_PORTABLE_3D_FORMAT_H

#include <stdint.h>


// 1. static info header
typedef struct pp3dStaticInfo {
    char extension_magic[4];
    float scale;
    // TODO: All those could technically be uint8_t
    uint16_t num_meshes;
    uint16_t num_materials;
    uint16_t num_subskeletons;
    uint16_t num_animations;
    uint16_t num_bones;
    uint16_t num_frames_total;
} pp3dStaticInfo;
// pp3dStaticInfo static_info;

// 2. mesh infos
typedef struct pp3dMesh {
    uint16_t material_index;
    uint16_t subskeleton_index;
    uint16_t num_vertex;
    uint16_t num_index;
} pp3dMesh;
// pp3dMesh meshes[static_info.num_meshes];

// 3. material infos
typedef struct pp3dMaterial {
    uint32_t diffuse_color;
    char diffuse_image_file_name[48];
} pp3dMaterial;
// pp3dMaterial materials[static_info.num_materials];

// 4. subskeleton infos
typedef struct pp3dSubskeleton {
    uint8_t num_bones;
    uint8_t bone_indices[8];
    uint8_t alignment[3];
} pp3dSubskeleton;
// pp3dSubskeleton subskeletons[static_info.num_subskeletons];

// 5. animation infos
typedef struct pp3dAnimation {
    char name[64];
    uint16_t num_frames;
    uint16_t alignment;
} pp3dAnimation;
// pp3dAnimation animations[static_info.num_animations];

// 6.
//   6.1. vertices
// typedef struct pp3dVertex {
//     float bone_weights[0-8]; // depends on `subskeletons[mesh[m].subskeleton_index].num_bones`
//     uint16_t uv[2];
//     uint16_t normal[3];
//     uint16_t position[3];
// } pp3dVertex;
// pp3dVertex vertices[mesh[m].num_vertex];
//
//   6.2. indices - if (mesh[m].num_index > 0)
// uint16_t indices[mesh[m].num_index];

// 7. bone parent indices
// uint16_t bone_parent_indices[static_info.num_bones]

// 8. bone inverse model space pose matrices
typedef struct pp3dMatrix {
    float elements[4][4];
} pp3dMatrix;
// pp3dMatrix inverse_model_space_pose_matrix[static_info.num_bones];

// 9. animation frame infos
typedef struct pp3dAnimationJoint {
    struct { float x, y, z; } position;
    struct { float x, y, z, w; } rotation;
    struct { float x, y, z; } scale;
} pp3dAnimationJoint;
// pp3dAnimationJoint animation_joints[static_info.num_frames_total * static_info.num_bones];



#endif // PROCYON_PORTABLE_3D_FORMAT_H