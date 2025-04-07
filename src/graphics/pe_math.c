#include "pe_math.h"
#include "math/p_math.h"

#include "HandmadeMath.h"

#include <stdbool.h>
#include <float.h>

pVec3 pe_unproject_vec3(
    pVec3 v,
    float viewport_x, float viewport_y,
    float viewport_width, float viewport_height,
    float viewport_min_z, float viewport_max_z,
    pMat4 projection,
    pMat4 view
) {
    // https://github.com/microsoft/DirectXMath/blob/bec07458c994bd7553638e4d499e17cfedd07831/Extensions/DirectXMathFMA4.h#L229
    pVec4 d = p_vec4(-1.0f, 1.0f, 0.0f, 0.0f);

    pVec4 scale = p_vec4(viewport_width * 0.5f, -viewport_height * 0.5f, viewport_max_z - viewport_min_z, 1.0f);
    scale = p_vec4(1.0f/scale.x, 1.0f/scale.y, 1.0f/scale.z, 1.0f/scale.w);

    pVec4 offset = p_vec4(-viewport_x, -viewport_y, -viewport_min_z, 0.0f);
    offset = p_vec4_add(p_vec4_mul(scale, offset), d);

    pMat4 transform = p_mat4_mul(projection, view);
    transform = p_mat4_inv_general(transform);

    pVec4 result = p_vec4_add(p_vec4_mul(p_vec4(v.x, v.y, v.z, 1.0f), scale), offset);
    result = p_mat4_mul_vec4(transform, result);

    if (result.w < FLT_EPSILON) {
        result.w = FLT_EPSILON;
    }

    result = p_vec4_div_f(result, result.w);

    return result.xyz;
}

bool p_collision_ray_plane(pRay ray, pVec3 plane_normal, float plane_d, pVec3 *collision_point) {
    // https://www.cs.princeton.edu/courses/archive/fall00/cs426/lectures/raycast/sld017.htm

    float dot_product = p_vec3_dot(ray.direction, plane_normal);
    if (dot_product == 0.0f) {
        return false;
    }

    float t = -(p_vec3_dot(ray.position, plane_normal) + plane_d) / dot_product;
    if (t < 0.0f) {
        return false;
    }

    *collision_point = p_vec3_add(ray.position, p_vec3_mul_f(ray.direction, t));
    return true;
}
