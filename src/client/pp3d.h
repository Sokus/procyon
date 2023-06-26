#ifndef PROCYON_PORTABLE_3D_FORMAT_H
#define PROCYON_PORTABLE_3D_FORMAT_H

#include <stdint.h>

// 1. static info header
typedef struct pp3dStaticInfo {
    char extension_magic[4];
    float scale;
    uint16_t num_meshes;
} pp3dStaticInfo;

// 2. mesh infos
typedef struct pp3dMesh {
    uint16_t num_vertex;
    uint16_t num_index;
    uint32_t diffuse_color;
} pp3dMesh; // meshes[static_info.num_meshes];

// 3. mesh vertices
typedef struct pp3dVertex {
    uint16_t uv[2];
    uint16_t normal[3];
    uint16_t vertex[3];
} pp3dVertex; // vertices[mesh[m].num_vertex];

// 4. mesh indices // if (mesh[m].num_index > 0)
// uint16_t indices[mesh[m].num_index];

// ... 3 and 4 repeated for every mesh



#endif // PROCYON_PORTABLE_3D_FORMAT_H