#ifndef PE_MATH_H_HEADER_GUARD
#define PE_MATH_H_HEADER_GUARD

#include <stdbool.h>

#include "math/p_math.h"

#include "HandmadeMath.h"

pVec3 pe_unproject_vec3(
    pVec3 vector,
    float viewport_x, float viewport_y,
    float viewport_width, float viewport_height,
    float viewport_min_z, float viewport_max_z,
    pMat4 projection,
    pMat4 view
);

typedef struct peRay {
    pVec3 position;
    pVec3 direction;
} peRay;

bool pe_collision_ray_plane(peRay ray, pVec3 plane_normal, float plane_d, pVec3* collision_point);



#endif // PE_MATH_H_HEADER_GUARD