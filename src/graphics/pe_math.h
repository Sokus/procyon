#ifndef PE_MATH_H_HEADER_GUARD
#define PE_MATH_H_HEADER_GUARD

#include "HandmadeMath.h"

#include <stdbool.h>

HMM_Vec3 pe_unproject_vec3(
    HMM_Vec3 vector,
    float viewport_x, float viewport_y,
    float viewport_width, float viewport_height,
    float viewport_min_z, float viewport_max_z,
    HMM_Mat4 projection,
    HMM_Mat4 view
);

typedef struct peRay {
    HMM_Vec3 position;
    HMM_Vec3 direction;
} peRay;

bool pe_collision_ray_plane(peRay, HMM_Vec3 plane_normal, float plane_d, HMM_Vec3* collision_point);



#endif // PE_MATH_H_HEADER_GUARD