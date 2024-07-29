#ifndef P_MATH_H_HEADER_GUARD
#define P_MATH_H_HEADER_GUARD

#include <stdbool.h>

#if defined(__PSP__)
#include <psptypes.h>
#include <pspgum.h>
#include <math.h>
#else
#include "HandmadeMath.h"
#endif

#define P_PI 3.14159265358979323846
#define P_PI32 3.14159265359f
#define P_DEG2RAD ((float)(P_PI/180.0))

typedef union pVec2 {
    struct { float x, y; };
    struct { float u, v; };
    struct { float left, right; };
    struct { float width, height; };
    float elements[2];
#if defined(__PSP__)
    ScePspFVector2 _sce;
#else
    HMM_Vec2 _hmm;
#endif
} pVec2;

typedef union pVec3 {
    struct { float x, y, z; };
    struct { float u, v, w; };
    struct { float r, g, b; };
    struct { pVec2 xy; float _ignored0; };
    struct { float _ignored1; pVec2 yz; };
    struct { pVec2 uv; float _ignored2; };
    struct { float _ignored3; pVec2 vw; };
    float elements[3];
#if defined(__PSP__)
    ScePspFVector3 _sce;
#else
    HMM_Vec3 _hmm;
#endif
} pVec3;

typedef union pVec4 {
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    struct { pVec3 xyz; float _ignored0; };
    struct { pVec3 rgb; float _ignored1; };
    struct { pVec2 xy; float _ignored2, _ignored3; };
    struct { float _ignored4; pVec2 yz; float _ignored5; };
    struct { float _ignored6, _ignored7; pVec2 zw; };
    float elements[4];
#if defined(__PSP__)
    ScePspFVector4 _sce;
#else
    HMM_Vec4 _hmm;
#endif
} pVec4;

typedef union pMat4 {
    float elements[4][4];
    pVec4 columns[4];
#if defined(__PSP__)
    ScePspFMatrix4 _sce;
#else
    HMM_Mat4 _hmm;
#endif
} pMat4;

typedef union pQuat {
    struct {
        union {
            struct { float x, y, z; };
            pVec3 xyz;
        };
        float w;
    };
    float elements[4];
#if !defined(__PSP__)
    HMM_Quat _hmm;
#endif
} pQuat;

// FLOATING-POINT MATH FUNCTIONS

static inline float p_sinf(float x) {
    float result = sinf(x);
    return result;
}

static inline float p_cosf(float x) {
    float result = cosf(x);
    return result;
}

static inline float p_tanf(float x) {
    float result = tanf(x);
    return result;
}

static inline float p_sqrtf(float x) {
    float result;
#if defined(PSP)
    result = sqrtf(x);
#else
    result = HMM_SqrtF(x);
#endif
    return result;
}

static inline float p_inv_sqrtf(float x) {
    float result = 1.0f / p_sqrtf(x);
    return result;
}

// VECTOR INITIALIZATION

static inline pVec2 p_vec2(float x, float y) {
    pVec2 result;
#if defined(__PSP__)
    result.x = x;
    result.y = y;
#else
    result._hmm = HMM_V2(x, y);
#endif
    return result;
}

static inline pVec3 p_vec3(float x, float y, float z) {
    pVec3 result;
#if defined(__PSP__)
    result.x = x;
    result.y = y;
    result.z = z;
#else
    result._hmm = HMM_V3(x, y, z);
#endif
    return result;
}

static inline pVec4 p_vec4(float x, float y, float z, float w) {
    pVec4 result;
#if defined(__PSP__)
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
#else
    result._hmm = HMM_V4(x, y, z, w);
#endif
    return result;
}

static inline pVec4 p_vec4v(pVec3 vec3, float w) {
    pVec4 result = p_vec4(vec3.x, vec3.y, vec3.z, w);
    return result;
}

// BINARY VECTOR OPERATIONS

static inline pVec2 p_vec2_add(pVec2 left, pVec2 right) {
    pVec2 result = {
        .x = left.x + right.x,
        .y = left.x + right.y
    };
    return result;
}

static inline pVec3 p_vec3_add(pVec3 left, pVec3 right) {
    pVec3 result = {
        .x = left.x + right.x,
        .y = left.y + right.y,
        .z = left.z + right.z
    };
    return result;
}

static inline pVec4 p_vec4_add(pVec4 left, pVec4 right) {
    pVec4 result;
#if defined(PSP)
    result = (pVec4){
        .x = left.x + right.x,
        .y = left.y + right.y,
        .z = left.z + right.z,
        .w = left.w + right.w
    };
#else
    result._hmm = HMM_AddV4(left._hmm, right._hmm);
#endif
    return result;
}

static inline pVec2 p_vec2_sub(pVec2 left, pVec2 right) {
    pVec2 result = {
        .x = left.x - right.x,
        .y = left.x - right.y
    };
    return result;
}

static inline pVec3 p_vec3_sub(pVec3 left, pVec3 right) {
    pVec3 result = {
        .x = left.x - right.x,
        .y = left.y - right.y,
        .z = left.z - right.z
    };
    return result;
}

static inline pVec4 p_vec4_sub(pVec4 left, pVec4 right) {
    pVec4 result;
#if defined(PSP)
    result = (pVec4){
        .x = left.x - right.x,
        .y = left.y - right.y,
        .z = left.z - right.z,
        .w = left.w - right.w
    };
#else
    result._hmm = HMM_SubV4(left._hmm, right._hmm);
#endif
    return result;
}

static inline pVec2 p_vec2_mul(pVec2 left, pVec2 right) {
    pVec2 result = {
        .x = left.x * right.x,
        .y = left.y * right.y
    };
    return result;
}

static inline pVec2 p_vec2_mul_f(pVec2 left, float right) {
    pVec2 result = {
        .x = left.x * right,
        .y = left.y * right
    };
    return result;
}

static inline pVec3 p_vec3_mul(pVec3 left, pVec3 right) {
    pVec3 result = {
        .x = left.x * right.x,
        .y = left.y * right.y,
        .z = left.z * right.z
    };
    return result;
}

static inline pVec3 p_vec3_mul_f(pVec3 left, float right) {
    pVec3 result = {
        .x = left.x * right,
        .y = left.y * right,
        .z = left.z * right
    };
    return result;
}

static inline pVec4 p_vec4_mul(pVec4 left, pVec4 right) {
    pVec4 result;
#if defined(PSP)
    result = (pVec4){
        .x = left.x * right.x,
        .y = left.y * right.y,
        .z = left.z * right.z,
        .w = left.w * right.w
    };
#else
    result._hmm = HMM_MulV4(left._hmm, right._hmm);
#endif
    return result;
}

static inline pVec4 p_vec4_mul_f(pVec4 left, float right) {
    pVec4 result;
#if defined(PSP)
    result = (pVec4){
        .x = left.x * right,
        .y = left.y * right,
        .z = left.z * right,
        .w = left.w * right
    };
#else
    result._hmm = HMM_MulV4F(left._hmm, right);
#endif
    return result;
}

static inline pVec2 p_vec2_div(pVec2 left, pVec2 right) {
    pVec2 result = {
        .x = left.x / right.x,
        .y = left.y / right.y
    };
    return result;
}

static inline pVec2 p_vec2_div_f(pVec2 left, float right) {
    pVec2 result = {
        .x = left.x / right,
        .y = left.y / right
    };
    return result;
}

static inline pVec3 p_vec3_div(pVec3 left, pVec3 right) {
    pVec3 result = {
        .x = left.x / right.x,
        .y = left.y / right.y,
        .z = left.z / right.z
    };
    return result;
}

static inline pVec3 p_vec3_div_f(pVec3 left, float right) {
    pVec3 result = {
        .x = left.x / right,
        .y = left.y / right,
        .z = left.z / right
    };
    return result;
}

static inline pVec4 p_vec4_div(pVec4 left, pVec4 right) {
    pVec4 result;
#if defined(PSP)
    result = (pVec4){
        .x = left.x / right.x,
        .y = left.y / right.y,
        .z = left.z / right.z,
        .w = left.w / right.w
    };
#else
    result._hmm = HMM_DivV4(left._hmm, right._hmm);
#endif
    return result;
}

static inline pVec4 p_vec4_div_f(pVec4 left, float right) {
    pVec4 result;
#if defined(PSP)
    result = (pVec4){
        .x = left.x / right,
        .y = left.y / right,
        .z = left.z / right,
        .w = left.w / right
    };
#else
    result._hmm = HMM_DivV4F(left._hmm, right);
#endif
    return result;
}

static inline bool p_vec2_eq(pVec2 left, pVec2 right) {
    bool result = (
        left.x == right.x &&
        left.y == right.y
    );
    return result;
}

static inline bool p_vec3_eq(pVec3 left, pVec3 right) {
    bool result = (
        left.x == right.x &&
        left.y == right.y &&
        left.z == right.z
    );
    return result;
}

static inline bool p_vec4_eq(pVec4 left, pVec4 right) {
    bool result = (
        left.x == right.x &&
        left.y == right.y &&
        left.z == right.z &&
        left.w == right.w
    );
    return result;
}

static inline float p_vec2_dot(pVec2 left, pVec2 right) {
    float result = (
        (left.x * right.x) +
        (left.y * right.y)
    );
    return result;
}

static inline float p_vec3_dot(pVec3 left, pVec3 right) {
    float result = (
        (left.x * right.x) +
        (left.y * right.y) +
        (left.z * right.z)
    );
    return result;
}

static inline float p_vec4_dot(pVec4 left, pVec4 right) {
    float result;
#if defined(__PSP__)
    result = (
        (left.x * right.x) +
        (left.y * right.y) +
        (left.z * right.z) +
        (left.w * right.w)
    );
#else
    result = HMM_DotV4(left._hmm, right._hmm);
#endif
    return result;
}

static inline pVec3 p_cross(pVec3 left, pVec3 right) {
    pVec3 result = {
        .x = (left.y * right.z) - (left.z * right.y),
        .y = (left.z * right.x) - (left.x * right.z),
        .z = (left.x * right.y) - (left.y * right.x)
    };
    return result;
}

// UNARY VECTOR OPERATIONS

static inline float p_vec2_len_sqr(pVec2 a) {
    return p_vec2_dot(a, a);
}

static inline float p_vec3_len_sqr(pVec3 a) {
    return p_vec3_dot(a, a);
}

static inline float p_vec4_len_sqr(pVec4 a) {
    return p_vec4_dot(a, a);
}

static inline float p_vec2_len(pVec2 a) {
    return p_sqrtf(p_vec2_len_sqr(a));
}

static inline float p_vec3_len(pVec3 a) {
    return p_sqrtf(p_vec3_len_sqr(a));
}

static inline float p_vec4_len(pVec4 a) {
    return p_sqrtf(p_vec4_len_sqr(a));
}

static inline pVec2 p_vec2_norm(pVec2 a) {
    return p_vec2_mul_f(a, p_inv_sqrtf(p_vec2_dot(a, a)));
}

static inline pVec3 p_vec3_norm(pVec3 a) {
    return p_vec3_mul_f(a, p_inv_sqrtf(p_vec3_dot(a, a)));
}

static inline pVec4 p_vec4_norm(pVec4 a) {
    return p_vec4_mul_f(a, p_inv_sqrtf(p_vec4_dot(a, a)));
}

// 4x4 MATRICES

static inline pMat4 p_mat4(void) {
    pMat4 result = {0};
    return result;
}

static inline pMat4 p_mat4_d(float diagonal) {
    pMat4 result = {0};
    result.elements[0][0] = diagonal;
    result.elements[1][1] = diagonal;
    result.elements[2][2] = diagonal;
    result.elements[3][3] = diagonal;
    return result;
}

static inline pMat4 p_mat4_i(void) {
    pMat4 result;
#if defined(__PSP__)
    gumLoadIdentity(&result._sce);
#else
    result = p_mat4_d(1.0f);
#endif
    return result;
}

static inline pMat4 p_mat4_transpose(pMat4 matrix) {
    pMat4 result;
#if defined(__PSP__)
    result = matrix;
    result.elements[0][1] = matrix.elements[1][0];
    result.elements[0][2] = matrix.elements[2][0];
    result.elements[0][3] = matrix.elements[3][0];
    result.elements[1][0] = matrix.elements[0][1];
    result.elements[1][2] = matrix.elements[2][1];
    result.elements[1][3] = matrix.elements[3][1];
    result.elements[2][1] = matrix.elements[1][2];
    result.elements[2][0] = matrix.elements[0][2];
    result.elements[2][3] = matrix.elements[3][2];
    result.elements[3][1] = matrix.elements[1][3];
    result.elements[3][2] = matrix.elements[2][3];
    result.elements[3][0] = matrix.elements[0][3];
#else
    result._hmm = HMM_TransposeM4(matrix._hmm);
#endif
    return result;
}

static inline pMat4 p_mat4_mul(pMat4 left, pMat4 right) {
    pMat4 result;
#if defined(__PSP__)
    gumMultMatrix(&result._sce, &left._sce, &right._sce);
#else
    result._hmm = HMM_MulM4(left._hmm, right._hmm);
#endif
    return result;
}

static inline pVec4 p_mat4_mul_vec4(pMat4 m, pVec4 v) {
    pVec4 result;
#if defined(__PSP__)
    // TODO: PSP VFPU

    result.x = v.elements[0] * m.columns[0].x;
    result.y = v.elements[0] * m.columns[0].y;
    result.z = v.elements[0] * m.columns[0].z;
    result.w = v.elements[0] * m.columns[0].w;

    result.x += v.elements[1] * m.columns[1].x;
    result.y += v.elements[1] * m.columns[1].y;
    result.z += v.elements[1] * m.columns[1].z;
    result.w += v.elements[1] * m.columns[1].w;

    result.x += v.elements[2] * m.columns[2].x;
    result.y += v.elements[2] * m.columns[2].y;
    result.z += v.elements[2] * m.columns[2].z;
    result.w += v.elements[2] * m.columns[2].w;

    result.x += v.elements[3] * m.columns[3].x;
    result.y += v.elements[3] * m.columns[3].y;
    result.z += v.elements[3] * m.columns[3].z;
    result.w += v.elements[3] * m.columns[3].w;
#else
    result._hmm = HMM_MulM4V4(m._hmm, v._hmm);
#endif
    return result;
}

static inline pMat4 p_mat4_inv_general(pMat4 matrix) {
    pVec3 C01 = p_cross(matrix.columns[0].xyz, matrix.columns[1].xyz);
    pVec3 C23 = p_cross(matrix.columns[2].xyz, matrix.columns[3].xyz);
    pVec3 B10 = p_vec3_sub(p_vec3_mul_f(matrix.columns[0].xyz, matrix.columns[1].w), p_vec3_mul_f(matrix.columns[1].xyz, matrix.columns[0].w));
    pVec3 B32 = p_vec3_sub(p_vec3_mul_f(matrix.columns[2].xyz, matrix.columns[3].w), p_vec3_mul_f(matrix.columns[3].xyz, matrix.columns[2].w));

    float InvDeterminant = 1.0f / (p_vec3_dot(C01, B32) + p_vec3_dot(C23, B10));
    C01 = p_vec3_mul_f(C01, InvDeterminant);
    C23 = p_vec3_mul_f(C23, InvDeterminant);
    B10 = p_vec3_mul_f(B10, InvDeterminant);
    B32 = p_vec3_mul_f(B32, InvDeterminant);

    pMat4 result;
    result.columns[0] = p_vec4v(p_vec3_add(p_cross(matrix.columns[1].xyz, B32), p_vec3_mul_f(C23, matrix.columns[1].w)), -p_vec3_dot(matrix.columns[1].xyz, C23));
    result.columns[1] = p_vec4v(p_vec3_sub(p_cross(B32, matrix.columns[0].xyz), p_vec3_mul_f(C23, matrix.columns[0].w)), +p_vec3_dot(matrix.columns[0].xyz, C23));
    result.columns[2] = p_vec4v(p_vec3_add(p_cross(matrix.columns[3].xyz, B10), p_vec3_mul_f(C01, matrix.columns[3].w)), -p_vec3_dot(matrix.columns[3].xyz, C01));
    result.columns[3] = p_vec4v(p_vec3_sub(p_cross(B10, matrix.columns[2].xyz), p_vec3_mul_f(C01, matrix.columns[2].w)), +p_vec3_dot(matrix.columns[2].xyz, C01));

    return p_mat4_transpose(result);
}

// COMMON GRAPHICS TRANSFORMATIONS

static inline pMat4 p_orthographic_rh_no(float left, float right, float bottom, float top, float p_near, float p_far) {
    pMat4 result = {0};

    result.elements[0][0] = 2.0f / (right - left);
    result.elements[1][1] = 2.0f / (top - bottom);
    result.elements[2][2] = 2.0f / (p_near - p_far);
    result.elements[3][3] = 1.0f;

    result.elements[3][0] = (left + right) / (left - right);
    result.elements[3][1] = (bottom + top) / (bottom - top);
    result.elements[3][2] = (p_near + p_far) / (p_near - p_far);

    return result;
}

static inline pMat4 p_perspective_rh_no(float fov, float aspect_ratio, float p_near, float p_far) {
    pMat4 result = {0};

    float cotangent = 1.0f / p_tanf(fov / 2.0f);
    result.elements[0][0] = cotangent / aspect_ratio;
    result.elements[1][1] = cotangent;
    result.elements[2][3] = -1.0f;

    result.elements[2][2] = (p_near + p_far) / (p_near - p_far);
    result.elements[3][2] = (2.0f * p_near * p_far) / (p_near - p_far);

    return result;
}

static inline pMat4 p_translate(pVec3 translation) {
    pMat4 result = p_mat4_i();
#if defined(__PSP__)
    gumTranslate(&result._sce, &translation._sce);
#else
    result.elements[3][0] = translation.x;
    result.elements[3][1] = translation.y;
    result.elements[3][2] = translation.z;
#endif
    return result;
}

static inline pMat4 p_rotate_rh(float r, pVec3 axis) {
    pMat4 result = p_mat4_i();
    axis = p_vec3_norm(axis);

    float sin_theta = p_sinf(r);
    float cos_theta = p_cosf(r);
    float cos_value = 1.0f - cos_theta;

    result.elements[0][0] = (axis.x * axis.x * cos_value) + cos_theta;
    result.elements[0][1] = (axis.x * axis.y * cos_value) + (axis.z * sin_theta);
    result.elements[0][2] = (axis.x * axis.z * cos_value) - (axis.y * sin_theta);

    result.elements[1][0] = (axis.y * axis.x * cos_value) - (axis.z * sin_theta);
    result.elements[1][1] = (axis.y * axis.y * cos_value) + cos_theta;
    result.elements[1][2] = (axis.y * axis.z * cos_value) + (axis.x * sin_theta);

    result.elements[2][0] = (axis.z * axis.x * cos_value) + (axis.y * sin_theta);
    result.elements[2][1] = (axis.z * axis.y * cos_value) - (axis.x * sin_theta);
    result.elements[2][2] = (axis.z * axis.z * cos_value) + cos_theta;

    return result;
}

static inline pMat4 p_rotate_xyz(pVec3 rotations) {
    pMat4 result;
#if defined(__PSP__)
    gumRotateXYZ(&result._sce, &rotations._sce);
#else
    pMat4 rotate_x = p_rotate_rh(rotations.x, p_vec3(1.0f, 0.0f, 0.0f));
    pMat4 rotate_y = p_rotate_rh(rotations.y, p_vec3(0.0f, 1.0f, 0.0f));
    pMat4 rotate_z = p_rotate_rh(rotations.z, p_vec3(0.0f, 0.0f, 1.0f));
    pMat4 rotate_yx = p_mat4_mul(rotate_y, rotate_x);
    result = p_mat4_mul(rotate_z, rotate_yx);
#endif
    return result;
}

static inline pMat4 p_scale(pVec3 scale) {
    pMat4 result = p_mat4_i();
#if defined(__PSP__)
    gumScale(&result._sce, &scale._sce);
#else
    result.elements[0][0] = scale.x;
    result.elements[1][1] = scale.y;
    result.elements[2][2] = scale.z;
#endif
    return result;
};

static inline pMat4 _p_look_at(pVec3 f, pVec3 s, pVec3 u, pVec3 eye) {
    pMat4 result;

    result.elements[0][0] = s.x;
    result.elements[0][1] = u.x;
    result.elements[0][2] = -f.x;
    result.elements[0][3] = 0.0f;

    result.elements[1][0] = s.y;
    result.elements[1][1] = u.y;
    result.elements[1][2] = -f.y;
    result.elements[1][3] = 0.0f;

    result.elements[2][0] = s.z;
    result.elements[2][1] = u.z;
    result.elements[2][2] = -f.z;
    result.elements[2][3] = 0.0f;

    result.elements[3][0] = -p_vec3_dot(s, eye);
    result.elements[3][1] = -p_vec3_dot(u, eye);
    result.elements[3][2] = p_vec3_dot(f, eye);
    result.elements[3][3] = 1.0f;

    return result;
}

static inline pMat4 p_look_at_rh(pVec3 eye, pVec3 center, pVec3 up) {
    pVec3 f = p_vec3_norm(p_vec3_sub(center, eye));
    pVec3 s = p_vec3_norm(p_cross(f, up));
    pVec3 u = p_cross(s, f);
    pMat4 result = _p_look_at(f, s, u, eye);
    return result;
}

// QUATERNION OPERATIONS

static inline pQuat p_quat(float x, float y, float z, float w) {
    pQuat result;
#if defined(__PSP__)
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
#else
    result._hmm = HMM_Q(x, y, z, w);
#endif
    return result;
}

static inline pQuat p_quat_mul(pQuat left, pQuat right) {
    pQuat result;
#if defined(__PSP__)
    result.x =  right.elements[3] * +left.elements[0];
    result.y =  right.elements[2] * -left.elements[0];
    result.z =  right.elements[1] * +left.elements[0];
    result.w =  right.elements[0] * -left.elements[0];

    result.x += right.elements[2] * +left.elements[1];
    result.y += right.elements[3] * +left.elements[1];
    result.z += right.elements[0] * -left.elements[1];
    result.w += right.elements[1] * -left.elements[1];

    result.x += right.elements[1] * -left.elements[2];
    result.y += right.elements[0] * +left.elements[2];
    result.z += right.elements[3] * +left.elements[2];
    result.w += right.elements[2] * -left.elements[2];

    result.x += right.elements[0] * +left.elements[3];
    result.y += right.elements[1] * +left.elements[3];
    result.z += right.elements[2] * +left.elements[3];
    result.w += right.elements[3] * +left.elements[3];
#else
    result._hmm = HMM_MulQ(left._hmm, right._hmm);
#endif
    return result;
}

static inline pQuat p_quat_norm(pQuat quat) {
    pVec4 vec4 = { quat.x, quat.y, quat.z, quat.w };
    vec4 = p_vec4_norm(vec4);
    pQuat result = { vec4.x, vec4.y, vec4.z, vec4.w };
    return result;
}

static inline pMat4 p_quat_to_mat4(pQuat quat) {
    pMat4 result;

    pQuat quat_norm = p_quat_norm(quat);

    float xx, yy, zz,
          xy, xz, yz,
          wx, wy, wz;

    xx = quat_norm.x * quat_norm.x;
    yy = quat_norm.y * quat_norm.y;
    zz = quat_norm.z * quat_norm.z;
    xy = quat_norm.x * quat_norm.y;
    xz = quat_norm.x * quat_norm.z;
    yz = quat_norm.y * quat_norm.z;
    wx = quat_norm.w * quat_norm.x;
    wy = quat_norm.w * quat_norm.y;
    wz = quat_norm.w * quat_norm.z;

    result.elements[0][0] = 1.0f - 2.0f * (yy + zz);
    result.elements[0][1] = 2.0f * (xy + wz);
    result.elements[0][2] = 2.0f * (xz - wy);
    result.elements[0][3] = 0.0f;

    result.elements[1][0] = 2.0f * (xy - wz);
    result.elements[1][1] = 1.0f - 2.0f * (xx + zz);
    result.elements[1][2] = 2.0f * (yz + wx);
    result.elements[1][3] = 0.0f;

    result.elements[2][0] = 2.0f * (xz + wy);
    result.elements[2][1] = 2.0f * (yz - wx);
    result.elements[2][2] = 1.0f - 2.0f * (xx + yy);
    result.elements[2][3] = 0.0f;

    result.elements[3][0] = 0.0f;
    result.elements[3][1] = 0.0f;
    result.elements[3][2] = 0.0f;
    result.elements[3][3] = 1.0f;

    return result;
}

#endif // P_MATH_H_HEADER_GUARD