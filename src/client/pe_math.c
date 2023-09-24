#include "pe_math.h"

#include "HandmadeMath.h"

#include <stdbool.h>
#include <float.h>

HMM_Vec3 pe_unproject_vec3(
    HMM_Vec3 v,
    float viewport_x, float viewport_y,
    float viewport_width, float viewport_height,
    float viewport_min_z, float viewport_max_z,
    HMM_Mat4 projection,
    HMM_Mat4 view
) {
    // https://github.com/microsoft/DirectXMath/blob/bec07458c994bd7553638e4d499e17cfedd07831/Extensions/DirectXMathFMA4.h#L229
    HMM_Vec4 d = HMM_V4(-1.0f, 1.0f, 0.0f, 0.0f);

    HMM_Vec4 scale = HMM_V4(viewport_width * 0.5f, -viewport_height * 0.5f, viewport_max_z - viewport_min_z, 1.0f);
    scale = HMM_V4(1.0f/scale.X, 1.0f/scale.Y, 1.0f/scale.Z, 1.0f/scale.W);

    HMM_Vec4 offset = HMM_V4(-viewport_x, -viewport_y, -viewport_min_z, 0.0f);
    offset = HMM_AddV4(HMM_MulV4(scale, offset), d);

    HMM_Mat4 transform = HMM_MulM4(projection, view);
    transform = HMM_InvGeneralM4(transform);

    HMM_Vec4 result = HMM_AddV4(HMM_MulV4(HMM_V4(v.X, v.Y, v.Z, 1.0f), scale), offset);
    result = HMM_MulM4V4(transform, result);

    if (result.W < FLT_EPSILON) {
        result.W = FLT_EPSILON;
    }

    result = HMM_DivV4F(result, result.W);

    return result.XYZ;
}

bool pe_collision_ray_plane(peRay ray, HMM_Vec3 plane_normal, float plane_d, HMM_Vec3 *collision_point) {
    // https://www.cs.princeton.edu/courses/archive/fall00/cs426/lectures/raycast/sld017.htm

    float dot_product = HMM_DotV3(ray.direction, plane_normal);
    if (dot_product == 0.0f) {
        return false;
    }

    float t = -(HMM_DotV3(ray.position, plane_normal) + plane_d) / dot_product;
    if (t < 0.0f) {
        return false;
    }

    *collision_point = HMM_AddV3(ray.position, HMM_MulV3F(ray.direction, t));
    return true;
}