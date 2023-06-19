#ifndef PROCYON_3D_FORMAT_H
#define PROCYON_3D_FORMAT_H

#include <stdint.h>

// P3D FORMAT:

// 1. static info header
typedef struct p3dStaticInfo {
    float scale;
    int32_t num_vertex;
    int32_t num_index;
    uint16_t num_meshes;
} p3dStaticInfo;

// 2. vertices
// int16_t position[3*num_vertex];
// int16_t normal[3*num_vertex];
// uint16_t texcoord[2*num_vertex];
// uint32_t color[num_vertex];

// 3. indices
// uint32_t index[num_index];

// 4. meshes
typedef struct p3dMesh {
    uint32_t num_index;
    uint32_t index_offset;

    bool has_diffuse_color;
    uint32_t diffuse_color;

    bool has_diffuse_texture;
    uint32_t diffuse_map_data_offset;
    uint32_t diffuse_map_data_size;
} p3dMesh; // meshes[static_info.num_meshes];

// 5. binary data
// uint8_t binary_data[...]

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