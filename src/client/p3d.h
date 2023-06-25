#ifndef PROCYON_3D_FORMAT_H
#define PROCYON_3D_FORMAT_H

#include <stdint.h>

// P3D FORMAT:

// 1. static info header
typedef struct p3dStaticInfo {
    char extension_magic[4];
    float scale;
    int32_t num_vertex;
    int32_t num_index;
    uint16_t num_meshes;
} p3dStaticInfo;

// 2. vertices
// int16_t position[3*num_vertex];
// int16_t normal[3*num_vertex];
// uint16_t texcoord[2*num_vertex];

// 3. indices
// uint32_t index[num_index];

// 4. meshes
typedef struct p3dMesh {
    uint32_t num_index;
    uint32_t index_offset;
    uint32_t vertex_offset;
    uint32_t diffuse_color;
} p3dMesh; // meshes[static_info.num_meshes];


// PP3D FORMAT:

//[scale]
//[# of meshes]
//  [vertex type]
//  [vertices] (size dependent on vertex type)
//  [indices] (size dependent on vertex type)
//  [has diffuse color?]
//    [diffuse color]
//  TODO: diffuse color map

#endif // PROCYON_3D_FORMAT_H