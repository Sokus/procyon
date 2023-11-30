#ifndef PROCYON_3D_FORMAT_H
#define PROCYON_3D_FORMAT_H

#include <stdint.h>
#include <stdbool.h>

// 1. static info header
typedef struct p3dStaticInfo {
    char extension_magic[4];
    float scale;
    uint32_t num_vertex;
    uint32_t num_index;
    uint16_t num_meshes;
    uint16_t num_animations;
    uint16_t num_frames_total;
    uint8_t num_bones;
    uint8_t alignment; // align to 24 bytes
} p3dStaticInfo;
// p3dStaticInfo static_info;

// 2. meshes
typedef struct p3dMesh {
    uint32_t num_index;
    uint32_t index_offset;
    uint32_t vertex_offset;
    uint32_t diffuse_color;
    char diffuse_map_file_name[48];
} p3dMesh;
// p3dMesh meshes[static_info.num_meshes];

// 3. animations
typedef struct p3dAnimation {
    char name[64];
    uint16_t num_frames;
} p3dAnimation;
// p3dAnimation animations[static_info.num_animations];

// 4. vertices
// int16_t position[3*static_info.num_vertex];
// int16_t normal[3*static_info.num_vertex];
// int16_t texcoord[2*static_info.num_vertex];
// uint8_t bone_index[4*static_info.num_vertex];
// uint16_t bone_weight[4*static_info.num_vertex];

// 5. indices
// uint32_t index[num_index];

// 6. bone parent indices
// uint8_t bone_parent_indices[static_info.num_bones];

// 7. bone inverse model sapce pose matrices
typedef struct p3dMatrix {
    float elements[4][4];
} p3dMatrix;
// p3dMatrix inverse_model_space_pose_matrix[static_info.num_bones];

// 8. animation frame infos
typedef struct p3dAnimationJoint {
    struct { float x, y, z; } position;
    struct { float x, y, z, w; } rotation;
    struct { float x, y, z; } scale;
} p3dAnimationJoint;
// p3dAnimationJoint animation_joints[static_info.num_frames_total * static_info.num_bones];

#endif // PROCYON_3D_FORMAT_H