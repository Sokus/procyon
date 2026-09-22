/*
    NOTE(sokus):
    Full credit for this file goes to:
    https://github.com/HandmadeMath/HandmadeMath
    What has been done here was just a simple find and replace
    to match this project's syntax style.
*/

#ifndef P_MATH_H_HEADER_GUARD
#define P_MATH_H_HEADER_GUARD

#if !defined(__cplusplus)
	#if (defined(_MSC_VER) && _MSC_VER < 1800) || (!defined(_MSC_VER) && !defined(__STDC_VERSION__))
		#ifndef true
		#define true  (0 == 0)
		#endif
		#ifndef false
		#define false (0 != 0)
		#endif
		typedef signed int bool;
	#else
		#include <stdbool.h>
	#endif
#endif

#ifdef P_MATH_NO_SSE
# warning "P_MATH_NO_SSE is deprecated, use P_MATH_NO_SIMD instead"
# define P_MATH_NO_SIMD
#endif

/* let's figure out if SSE is really available (unless disabled anyway)
   (it isn't on non-x86/x86_64 platforms or even x86 without explicit SSE support)
   => only use "#ifdef P_MATH__USE_SSE" to check for SSE support below this block! */
#ifndef P_MATH_NO_SIMD
# ifdef _MSC_VER /* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#  if defined(_M_AMD64) || ( defined(_M_IX86_FP) && _M_IX86_FP >= 1 )
#   define P_MATH__USE_SSE 1
#  endif
# else /* not MSVC, probably GCC, clang, icc or something that doesn't support SSE anyway */
#  ifdef __SSE__ /* they #define __SSE__ if it's supported */
#   define P_MATH__USE_SSE 1
#  endif /*  __SSE__ */
# endif /* not _MSC_VER */
# ifdef __ARM_NEON
#  define P_MATH__USE_NEON 1
# endif /* NEON Supported */
#endif /* #ifndef P_MATH_NO_SIMD */

#if (!defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
# define P_MATH__USE_C11_GENERICS 1
#endif

#ifdef P_MATH__USE_SSE
# include <xmmintrin.h>
#endif

#ifdef P_MATH__USE_NEON
# include <arm_neon.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif

#if defined(__GNUC__) || defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wfloat-equal"
# pragma GCC diagnostic ignored "-Wmissing-braces"
# ifdef __clang__
#  pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# endif
#endif

#if defined(__GNUC__) || defined(__clang__)
# define P_MATH_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
# define P_MATH_DEPRECATED(msg) __declspec(deprecated(msg))
#else
# define P_MATH_DEPRECATED(msg)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(P_MATH_USE_DEGREES) \
    && !defined(P_MATH_USE_TURNS) \
    && !defined(P_MATH_USE_RADIANS)
# define P_MATH_USE_RADIANS
#endif

#define P_PI 3.14159265358979323846
#define P_PI32 3.14159265359f
#define P_DEG180 180.0
#define P_DEG18032 180.0f
#define P_TURNHALF 0.5
#define P_TURNHALF32 0.5f
#define P_RAD2DEG ((float)(P_DEG180/P_PI))
#define P_RAD2TURN ((float)(P_TURNHALF/P_PI))
#define P_DEG2RAD ((float)(P_PI/P_DEG180))
#define P_DEG2TURN ((float)(P_TURNHALF/P_DEG180))
#define P_TURN2RAD ((float)(P_PI/P_TURNHALF))
#define P_TURN2DEG ((float)(P_DEG180/P_TURNHALF))

#if defined(P_MATH_USE_RADIANS)
# define P_ANGLE_RAD(a) (a)
# define P_ANGLE_DEG(a) ((a)*P_DEG2RAD)
# define P_ANGLE_TURN(a) ((a)*P_TURN2RAD)
#elif defined(P_MATH_USE_DEGREES)
# define P_ANGLE_RAD(a) ((a)*P_RAD2DEG)
# define P_ANGLE_DEG(a) (a)
# define P_ANGLE_TURN(a) ((a)*P_TURN2DEG)
#elif defined(P_MATH_USE_TURNS)
# define P_ANGLE_RAD(a) ((a)*P_RAD2TURN)
# define P_ANGLE_DEG(a) ((a)*P_DEG2TURN)
# define P_ANGLE_TURN(a) (a)
#endif

#if defined(__PSP__)
#include <psptypes.h>
#include <pspgum.h>
#endif

#if !defined(P_MATH_PROVIDE_MATH_FUNCTIONS)
# include <math.h>
# define P_SINF sinf
# define P_COSF cosf
# define P_TANF tanf
# define P_SQRTF sqrtf
# define P_ACOSF acosf
#endif

#if !defined(P_ANGLE_USER_TO_INTERNAL)
# define P_ANGLE_USER_TO_INTERNAL(a) (p_angle_to_rad(a))
#endif

#if !defined(P_ANGLE_INTERNAL_TO_USER)
# if defined(P_MATH_USE_RADIANS)
#  define P_ANGLE_INTERNAL_TO_USER(a) (a)
# elif defined(P_MATH_USE_DEGREES)
#  define P_ANGLE_INTERNAL_TO_USER(a) ((a)*P_RAD2DEG)
# elif defined(P_MATH_USE_TURNS)
#  define P_ANGLE_INTERNAL_TO_USER(a) ((a)*P_RAD2TURN)
# endif
#endif

#define P_SIGN(x) ((x) < 0 ? -1 : 1)
#define P_MIN(a, b) ((a) > (b) ? (b) : (a))
#define P_MAX(a, b) ((a) < (b) ? (b) : (a))
#define P_ABS(a) ((a) > 0 ? (a) : -(a))
#define P_MOD(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))
#define P_SQUARE(x) ((x) * (x))

typedef union pVec2 {
    struct { float x, y; };
    struct { float u, v; };
    struct { float left, right; };
    struct { float width, height; };
    float elements[2];
#if defined(__PSP__)
    ScePspFVector2 _sce;
#endif
#ifdef __cplusplus
    inline float &operator[](int index) { return elements[index]; }
    inline const float& operator[](int index) const { return elements[index]; }
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
#endif
#ifdef __cplusplus
    inline float &operator[](int index) { return elements[index]; }
    inline const float &operator[](int index) const { return elements[index]; }
#endif
} pVec3;

typedef union pVec4 {
    struct {
        union {
            pVec3 xyz;
            struct { float x, y, z; };
        };
        float w;
    };
    struct {
        union {
            pVec3 rgb;
            struct { float r, g, b; };
        };
        float a;
    };
    struct { pVec2 xy; float _ignored0; float _ignored1; };
    struct { float _ignored2; pVec2 yz; float _ignored3; };
    struct { float _ignored4; float _ignored5; pVec2 zw; };
    float elements[4];
#ifdef P_MATH__USE_SSE
    __m128 sse;
#endif
#ifdef P_MATH__USE_NEON
    float32x4_t neon;
#endif
#if defined(__PSP__)
    ScePspFVector4 _sce;
#endif
#ifdef __cplusplus
    inline float &operator[](int index) { return elements[index]; }
    inline const float &operator[](int index) const { return elements[index]; }
#endif
} pVec4;

typedef union pMat2 {
    float elements[2][2];
    pVec2 columns[2];
#if defined(__PSP__)
    ScePspFMatrix2 _sce;
#endif
#ifdef __cplusplus
    inline pVec2 &operator[](int index) { return columns[index]; }
    inline const pVec2 &operator[](int index) const { return columns[index]; }
#endif
} pMat2;

typedef union pMat3 {
    float elements[3][3];
    pVec3 columns[3];
#if defined(__PSP__)
    ScePspFMatrix3 _sce;
#endif
#ifdef __cplusplus
    inline pVec3 &operator[](int index) { return columns[index]; }
    inline const pVec3 &operator[](int index) const { return columns[index]; }
#endif
} pMat3;

typedef union pMat4 {
    float elements[4][4];
    pVec4 columns[4];
#if defined(__PSP__)
    ScePspFMatrix4 _sce;
#endif
#ifdef __cplusplus
    inline pVec4 &operator[](int index) { return columns[index]; }
    inline const pVec4 &operator[](int index) const { return columns[index]; }
#endif
} pMat4;

typedef union pQuat {
    struct {
        union {
            pVec3 xyz;
            struct { float x, y, z; };
        };
        float w;
    };
    float elements[4];
#ifdef P_MATH__USE_SSE
    __m128 sse;
#endif
#ifdef P_MATH__USE_NEON
    float32x4_t neon;
#endif
} pQuat;

/*
 * Angle unit conversion functions
 */
static inline float p_angle_to_rad(float angle) {
#if defined(P_MATH_USE_RADIANS)
    float result = angle;
#elif defined(P_MATH_USE_DEGREES)
    float result = angle * P_DEG2RAD;
#elif defined(P_MATH_USE_TURNS)
    float result = angle * P_TURN2RAD;
#endif
    return result;
}

static inline float p_angle_to_deg(float angle) {
#if defined(P_MATH_USE_RADIANS)
    float result = angle * P_RAD2DEG;
#elif defined(P_MATH_USE_DEGREES)
    float result = angle;
#elif defined(P_MATH_USE_TURNS)
    float result = angle * P_TURN2DEG;
#endif
    return result;
}

static inline float p_angle_to_turn(float angle) {
#if defined(P_MATH_USE_RADIANS)
    float result = angle * P_RAD2TURN;
#elif defined(P_MATH_USE_DEGREES)
    float result = angle * P_DEG2TURN;
#elif defined(P_MATH_USE_TURNS)
    float result = angle;
#endif
    return result;
}

/*
 * Floating-point math functions
 */

static inline float p_sinf(float angle) {
    return P_SINF(P_ANGLE_USER_TO_INTERNAL(angle));
}

static inline float p_cosf(float angle) {
    return P_COSF(P_ANGLE_USER_TO_INTERNAL(angle));
}

static inline float p_tanf(float angle) {
    return P_TANF(P_ANGLE_USER_TO_INTERNAL(angle));
}

static inline float p_acosf(float arg) {
    return P_ANGLE_INTERNAL_TO_USER(P_ACOSF(arg));
}

static inline float p_sqrtf(float x) {
    float result;
#ifdef P_MATH__USE_SSE
    __m128 in = _mm_set_ss(x);
    __m128 out = _mm_sqrt_ss(in);
    result = _mm_cvtss_f32(out);
#elif defined(P_MATH__USE_NEON)
    float32x4_t in = vdupq_n_f32(x);
    float32x4_t out = vsqrtq_f32(in);
    result = vgetq_lane_f32(out, 0);
#else
    result = P_SQRTF(x);
#endif
    return result;
}

static inline float p_inv_sqrtf(float Float) {
    float result;
    result = 1.0f/p_sqrtf(Float);
    return result;
}


/*
 * Utility functions
 */

static inline float p_lerp(float a, float time, float b) {
    return (1.0f - time) * a + time * b;
}

static inline float p_clamp(float min, float value, float max) {
    float result = value;
    if (result < min) {
        result = min;
    }
    if (result > max) {
        result = max;
    }
    return result;
}


/*
 * v initialization
 */

static inline pVec2 p_vec2(float x, float y) {
    pVec2 result;
    result.x = x;
    result.y = y;
    return result;
}

static inline pVec3 p_vec3(float x, float y, float z) {
    pVec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

static inline pVec4 p_vec4(float x, float y, float z, float w) {
    pVec4 result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_setr_ps(x, y, z, w);
#elif defined(P_MATH__USE_NEON)
    float32x4_t v = {x, y, z, w};
    result.neon = v;
#else
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
#endif
    return result;
}

static inline pVec4 p_vec4v(pVec3 v, float w) {
    pVec4 result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_setr_ps(v.x, v.y, v.z, w);
#elif defined(P_MATH__USE_NEON)
    float32x4_t v = {v.x, v.y, v.z, w};
    result.neon = v;
#else
    result.xyz = v;
    result.w = w;
#endif
    return result;
}


/*
 * Binary vector operations
 */

static inline pVec2 p_vec2_add(pVec2 left, pVec2 right) {
    pVec2 result;
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    return result;
}

static inline pVec3 p_vec3_add(pVec3 left, pVec3 right) {
    pVec3 result;
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    return result;
}

static inline pVec4 p_vec4_add(pVec4 left, pVec4 right) {
    pVec4 result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_add_ps(left.sse, right.sse);
#elif defined(P_MATH__USE_NEON)
    result.neon = vaddq_f32(left.neon, right.neon);
#else
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    result.w = left.w + right.w;
#endif
    return result;
}

static inline pVec2 p_vec2_sub(pVec2 left, pVec2 right) {
    pVec2 result;
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    return result;
}

static inline pVec3 p_vec3_sub(pVec3 left, pVec3 right) {
    pVec3 result;
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    result.z = left.z - right.z;
    return result;
}

static inline pVec4 p_vec4_sub(pVec4 left, pVec4 right) {
    pVec4 result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_sub_ps(left.sse, right.sse);
#elif defined(P_MATH__USE_NEON)
    result.neon = vsubq_f32(left.neon, right.neon);
#else
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    result.z = left.z - right.z;
    result.w = left.w - right.w;
#endif
    return result;
}

static inline pVec2 p_vec2_mul(pVec2 left, pVec2 right) {
    pVec2 result;
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    return result;
}

static inline pVec2 p_vec2_mul_f(pVec2 left, float right) {
    pVec2 result;
    result.x = left.x * right;
    result.y = left.y * right;
    return result;
}

static inline pVec3 p_vec3_mul(pVec3 left, pVec3 right) {
    pVec3 result;
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    result.z = left.z * right.z;
    return result;
}

static inline pVec3 p_vec3_mul_f(pVec3 left, float right) {
    pVec3 result;
    result.x = left.x * right;
    result.y = left.y * right;
    result.z = left.z * right;
    return result;
}

static inline pVec4 p_vec4_mul(pVec4 left, pVec4 right) {
    pVec4 result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_mul_ps(left.sse, right.sse);
#elif defined(P_MATH__USE_NEON)
    result.neon = vmulq_f32(left.neon, right.neon);
#else
    result.x = left.x * right.x;
    result.y = left.y * right.y;
    result.z = left.z * right.z;
    result.w = left.w * right.w;
#endif
    return result;
}

static inline pVec4 p_vec4_mul_f(pVec4 left, float right) {
    pVec4 result;
#ifdef P_MATH__USE_SSE
    __m128 scalar = _mm_set1_ps(right);
    result.sse = _mm_mul_ps(left.sse, scalar);
#elif defined(P_MATH__USE_NEON)
    result.neon = vmulq_n_f32(left.neon, right);
#else
    result.x = left.x * right;
    result.y = left.y * right;
    result.z = left.z * right;
    result.w = left.w * right;
#endif
    return result;
}

static inline pVec2 p_vec2_div(pVec2 left, pVec2 right) {
    pVec2 result;
    result.x = left.x / right.x;
    result.y = left.y / right.y;
    return result;
}

static inline pVec2 p_vec2_div_f(pVec2 left, float right) {
    pVec2 result;
    result.x = left.x / right;
    result.y = left.y / right;
    return result;
}

static inline pVec3 p_vec3_div(pVec3 left, pVec3 right) {
    pVec3 result;
    result.x = left.x / right.x;
    result.y = left.y / right.y;
    result.z = left.z / right.z;
    return result;
}

static inline pVec3 p_vec3_div_f(pVec3 left, float right) {
    pVec3 result;
    result.x = left.x / right;
    result.y = left.y / right;
    result.z = left.z / right;
    return result;
}

static inline pVec4 p_vec4_div(pVec4 left, pVec4 right) {
    pVec4 result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_div_ps(left.sse, right.sse);
#elif defined(P_MATH__USE_NEON)
    result.neon = vdivq_f32(left.neon, right.neon);
#else
    result.x = left.x / right.x;
    result.y = left.y / right.y;
    result.z = left.z / right.z;
    result.w = left.w / right.w;
#endif
    return result;
}

static inline pVec4 p_vec4_div_f(pVec4 left, float right) {
    pVec4 result;
#ifdef P_MATH__USE_SSE
    __m128 scalar = _mm_set1_ps(right);
    result.sse = _mm_div_ps(left.sse, scalar);
#elif defined(P_MATH__USE_NEON)
    float32x4_t scalar = vdupq_n_f32(right);
    result.neon = vdivq_f32(left.neon, scalar);
#else
    result.x = left.x / right;
    result.y = left.y / right;
    result.z = left.z / right;
    result.w = left.w / right;
#endif
    return result;
}

static inline bool p_vec2_eq(pVec2 left, pVec2 right) {
    return left.x == right.x && left.y == right.y;
}

static inline bool p_vec3_eq(pVec3 left, pVec3 right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static inline bool p_vec4_eq(pVec4 left, pVec4 right) {
    return left.x == right.x && left.y == right.y && left.z == right.z && left.w == right.w;
}

static inline float p_vec2_dot(pVec2 left, pVec2 right) {
    return (left.x * right.x) + (left.y * right.y);
}

static inline float p_vec3_dot(pVec3 left, pVec3 right) {
    return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

static inline float p_vec4_dot(pVec4 left, pVec4 right) {
    float result;
    // NOTE(zak): IN the future if we wanna check what version SSE is support
    // we can use _mm_dp_ps (4.3) but for now we will use the old way.
    // Or a r = _mm_mul_ps(v1, v2), r = _mm_hadd_ps(r, r), r = _mm_hadd_ps(r, r) for SSE3
#ifdef P_MATH__USE_SSE
    __m128 sse_result_one = _mm_mul_ps(left.sse, right.sse);
    __m128 sse_result_two = _mm_shuffle_ps(sse_result_one, sse_result_one, _MM_SHUFFLE(2, 3, 0, 1));
    sse_result_one = _mm_add_ps(sse_result_one, sse_result_two);
    sse_result_two = _mm_shuffle_ps(sse_result_one, sse_result_one, _MM_SHUFFLE(0, 1, 2, 3));
    sse_result_one = _mm_add_ps(sse_result_one, sse_result_two);
    _mm_store_ss(&result, sse_result_one);
#elif defined(P_MATH__USE_NEON)
    float32x4_t neon_multiply_result = vmulq_f32(left.neon, right.neon);
    float32x4_t neon_half_add = vpaddq_f32(neon_multiply_result, neon_multiply_result);
    float32x4_t neon_full_add = vpaddq_f32(neon_half_add, neon_half_add);
    result = vgetq_lane_f32(neon_full_add, 0);
#else
    result = ((left.x * right.x) + (left.z * right.z)) + ((left.y * right.y) + (left.w * right.w));
#endif
    return result;
}

static inline pVec3 p_cross(pVec3 left, pVec3 right) {
    pVec3 result;
    result.x = (left.y * right.z) - (left.z * right.y);
    result.y = (left.z * right.x) - (left.x * right.z);
    result.z = (left.x * right.y) - (left.y * right.x);
    return result;
}


/*
 * Unary vector operations
 */

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

/*
 * Utility vector functions
 */

static inline pVec2 p_lerp_vec2(pVec2 a, float t, pVec2 b) {
    return p_vec2_add(p_vec2_mul_f(a, 1.0f - t), p_vec2_mul_f(b, t));
}

static inline pVec3 p_lerp_vec3(pVec3 a, float t, pVec3 b) {
    return p_vec3_add(p_vec3_mul_f(a, 1.0f - t), p_vec3_mul_f(b, t));
}

static inline pVec4 p_lerp_vec4(pVec4 a, float t, pVec4 b) {
    return p_vec4_add(p_vec4_mul_f(a, 1.0f - t), p_vec4_mul_f(b, t));
}

/*
 * SSE stuff
 */

static inline pVec4 p_linear_combine_vec4_mat4(pVec4 left, pMat4 right) {
    pVec4 result;
    // TODO: Utilize PSP VFPU
#ifdef P_MATH__USE_SSE
    result.sse = _mm_mul_ps(_mm_shuffle_ps(left.sse, left.sse, 0x00), right.columns[0].sse);
    result.sse = _mm_add_ps(result.sse, _mm_mul_ps(_mm_shuffle_ps(left.sse, left.sse, 0x55), right.columns[1].sse));
    result.sse = _mm_add_ps(result.sse, _mm_mul_ps(_mm_shuffle_ps(left.sse, left.sse, 0xaa), right.columns[2].sse));
    result.sse = _mm_add_ps(result.sse, _mm_mul_ps(_mm_shuffle_ps(left.sse, left.sse, 0xff), right.columns[3].sse));
#elif defined(P_MATH__USE_NEON)
    result.neon = vmulq_laneq_f32(right.columns[0].neon, left.neon, 0);
    result.neon = vfmaq_laneq_f32(result.neon, right.columns[1].neon, left.neon, 1);
    result.neon = vfmaq_laneq_f32(result.neon, right.columns[2].neon, left.neon, 2);
    result.neon = vfmaq_laneq_f32(result.neon, right.columns[3].neon, left.neon, 3);
#else
    result.x = left.elements[0] * right.columns[0].x;
    result.y = left.elements[0] * right.columns[0].y;
    result.z = left.elements[0] * right.columns[0].z;
    result.w = left.elements[0] * right.columns[0].w;

    result.x += left.elements[1] * right.columns[1].x;
    result.y += left.elements[1] * right.columns[1].y;
    result.z += left.elements[1] * right.columns[1].z;
    result.w += left.elements[1] * right.columns[1].w;

    result.x += left.elements[2] * right.columns[2].x;
    result.y += left.elements[2] * right.columns[2].y;
    result.z += left.elements[2] * right.columns[2].z;
    result.w += left.elements[2] * right.columns[2].w;

    result.x += left.elements[3] * right.columns[3].x;
    result.y += left.elements[3] * right.columns[3].y;
    result.z += left.elements[3] * right.columns[3].z;
    result.w += left.elements[3] * right.columns[3].w;
#endif
    return result;
}

/*
 * 2x2 Matrices
 */

static inline pMat2 p_mat2(void) {
    pMat2 result = {0};
    return result;
}

static inline pMat2 p_mat2_d(float diagonal) {
    pMat2 result = {0};
    result.elements[0][0] = diagonal;
    result.elements[1][1] = diagonal;
    return result;
}

static inline pMat2 p_mat2_transpose(pMat2 matrix) {
    pMat2 result = matrix;
    result.elements[0][1] = matrix.elements[1][0];
    result.elements[1][0] = matrix.elements[0][1];
    return result;
}

static inline pMat2 p_mat2_add(pMat2 left, pMat2 right) {
    pMat2 result;
    result.elements[0][0] = left.elements[0][0] + right.elements[0][0];
    result.elements[0][1] = left.elements[0][1] + right.elements[0][1];
    result.elements[1][0] = left.elements[1][0] + right.elements[1][0];
    result.elements[1][1] = left.elements[1][1] + right.elements[1][1];
    return result;
}

static inline pMat2 p_mat2_sub(pMat2 left, pMat2 right) {
    pMat2 result;
    result.elements[0][0] = left.elements[0][0] - right.elements[0][0];
    result.elements[0][1] = left.elements[0][1] - right.elements[0][1];
    result.elements[1][0] = left.elements[1][0] - right.elements[1][0];
    result.elements[1][1] = left.elements[1][1] - right.elements[1][1];
    return result;
}

static inline pVec2 p_mat2_mul_vec2(pMat2 matrix, pVec2 v) {
    pVec2 result;
    result.x = v.elements[0] * matrix.columns[0].x;
    result.y = v.elements[0] * matrix.columns[0].y;
    result.x += v.elements[1] * matrix.columns[1].x;
    result.y += v.elements[1] * matrix.columns[1].y;
    return result;
}

static inline pMat2 p_mat2_mul(pMat2 left, pMat2 right) {
    pMat2 result;
    result.columns[0] = p_mat2_mul_vec2(left, right.columns[0]);
    result.columns[1] = p_mat2_mul_vec2(left, right.columns[1]);
    return result;
}

static inline pMat2 p_mat2_mul_f(pMat2 matrix, float scalar) {
    pMat2 result;
    result.elements[0][0] = matrix.elements[0][0] * scalar;
    result.elements[0][1] = matrix.elements[0][1] * scalar;
    result.elements[1][0] = matrix.elements[1][0] * scalar;
    result.elements[1][1] = matrix.elements[1][1] * scalar;
    return result;
}

static inline pMat2 p_mat2_div_f(pMat2 matrix, float scalar) {
    pMat2 result;
    result.elements[0][0] = matrix.elements[0][0] / scalar;
    result.elements[0][1] = matrix.elements[0][1] / scalar;
    result.elements[1][0] = matrix.elements[1][0] / scalar;
    result.elements[1][1] = matrix.elements[1][1] / scalar;
    return result;
}

static inline float p_mat2_determinant(pMat2 matrix) {
    return matrix.elements[0][0]*matrix.elements[1][1] - matrix.elements[0][1]*matrix.elements[1][0];
}


static inline pMat2 p_mat2_inv_general(pMat2 matrix) {
    pMat2 result;
    float inv_determinant = 1.0f / p_mat2_determinant(matrix);
    result.elements[0][0] = inv_determinant * +matrix.elements[1][1];
    result.elements[1][1] = inv_determinant * +matrix.elements[0][0];
    result.elements[0][1] = inv_determinant * -matrix.elements[0][1];
    result.elements[1][0] = inv_determinant * -matrix.elements[1][0];
    return result;
}

/*
 * 3x3 Matrices
 */

static inline pMat3 p_mat3(void) {
    pMat3 result = {0};
    return result;
}

static inline pMat3 p_mat3_d(float diagonal) {
    pMat3 result = {0};
    result.elements[0][0] = diagonal;
    result.elements[1][1] = diagonal;
    result.elements[2][2] = diagonal;
    return result;
}

static inline pMat3 p_mat3_transpose(pMat3 matrix) {
    pMat3 result = matrix;
    result.elements[0][1] = matrix.elements[1][0];
    result.elements[0][2] = matrix.elements[2][0];
    result.elements[1][0] = matrix.elements[0][1];
    result.elements[1][2] = matrix.elements[2][1];
    result.elements[2][1] = matrix.elements[1][2];
    result.elements[2][0] = matrix.elements[0][2];
    return result;
}

static inline pMat3 p_mat3_add(pMat3 left, pMat3 right) {
    pMat3 result;
    result.elements[0][0] = left.elements[0][0] + right.elements[0][0];
    result.elements[0][1] = left.elements[0][1] + right.elements[0][1];
    result.elements[0][2] = left.elements[0][2] + right.elements[0][2];
    result.elements[1][0] = left.elements[1][0] + right.elements[1][0];
    result.elements[1][1] = left.elements[1][1] + right.elements[1][1];
    result.elements[1][2] = left.elements[1][2] + right.elements[1][2];
    result.elements[2][0] = left.elements[2][0] + right.elements[2][0];
    result.elements[2][1] = left.elements[2][1] + right.elements[2][1];
    result.elements[2][2] = left.elements[2][2] + right.elements[2][2];
    return result;
}

static inline pMat3 p_mat3_sub(pMat3 left, pMat3 right) {
    pMat3 result;
    result.elements[0][0] = left.elements[0][0] - right.elements[0][0];
    result.elements[0][1] = left.elements[0][1] - right.elements[0][1];
    result.elements[0][2] = left.elements[0][2] - right.elements[0][2];
    result.elements[1][0] = left.elements[1][0] - right.elements[1][0];
    result.elements[1][1] = left.elements[1][1] - right.elements[1][1];
    result.elements[1][2] = left.elements[1][2] - right.elements[1][2];
    result.elements[2][0] = left.elements[2][0] - right.elements[2][0];
    result.elements[2][1] = left.elements[2][1] - right.elements[2][1];
    result.elements[2][2] = left.elements[2][2] - right.elements[2][2];
    return result;
}

static inline pVec3 p_mat3_mul_v(pMat3 matrix, pVec3 v) {
    pVec3 result;
    result.x = v.elements[0] * matrix.columns[0].x;
    result.y = v.elements[0] * matrix.columns[0].y;
    result.z = v.elements[0] * matrix.columns[0].z;

    result.x += v.elements[1] * matrix.columns[1].x;
    result.y += v.elements[1] * matrix.columns[1].y;
    result.z += v.elements[1] * matrix.columns[1].z;

    result.x += v.elements[2] * matrix.columns[2].x;
    result.y += v.elements[2] * matrix.columns[2].y;
    result.z += v.elements[2] * matrix.columns[2].z;
    return result;
}

static inline pMat3 p_mat3_mul(pMat3 left, pMat3 right) {
    pMat3 result;
    result.columns[0] = p_mat3_mul_v(left, right.columns[0]);
    result.columns[1] = p_mat3_mul_v(left, right.columns[1]);
    result.columns[2] = p_mat3_mul_v(left, right.columns[2]);
    return result;
}

static inline pMat3 p_mat3_mul_f(pMat3 matrix, float scalar) {
    pMat3 result;
    result.elements[0][0] = matrix.elements[0][0] * scalar;
    result.elements[0][1] = matrix.elements[0][1] * scalar;
    result.elements[0][2] = matrix.elements[0][2] * scalar;
    result.elements[1][0] = matrix.elements[1][0] * scalar;
    result.elements[1][1] = matrix.elements[1][1] * scalar;
    result.elements[1][2] = matrix.elements[1][2] * scalar;
    result.elements[2][0] = matrix.elements[2][0] * scalar;
    result.elements[2][1] = matrix.elements[2][1] * scalar;
    result.elements[2][2] = matrix.elements[2][2] * scalar;
    return result;
}

static inline pMat3 p_mat3_div_f(pMat3 matrix, float scalar) {
    pMat3 result;
    result.elements[0][0] = matrix.elements[0][0] / scalar;
    result.elements[0][1] = matrix.elements[0][1] / scalar;
    result.elements[0][2] = matrix.elements[0][2] / scalar;
    result.elements[1][0] = matrix.elements[1][0] / scalar;
    result.elements[1][1] = matrix.elements[1][1] / scalar;
    result.elements[1][2] = matrix.elements[1][2] / scalar;
    result.elements[2][0] = matrix.elements[2][0] / scalar;
    result.elements[2][1] = matrix.elements[2][1] / scalar;
    result.elements[2][2] = matrix.elements[2][2] / scalar;
    return result;
}

static inline float p_mat3_determinant(pMat3 matrix) {
    pMat3 cross;
    cross.columns[0] = p_cross(matrix.columns[1], matrix.columns[2]);
    cross.columns[1] = p_cross(matrix.columns[2], matrix.columns[0]);
    cross.columns[2] = p_cross(matrix.columns[0], matrix.columns[1]);
    return p_vec3_dot(cross.columns[2], matrix.columns[2]);
}

static inline pMat3 p_mat3_inv_general(pMat3 matrix) {
    pMat3 cross;
    cross.columns[0] = p_cross(matrix.columns[1], matrix.columns[2]);
    cross.columns[1] = p_cross(matrix.columns[2], matrix.columns[0]);
    cross.columns[2] = p_cross(matrix.columns[0], matrix.columns[1]);
    float inv_determinant = 1.0f / p_vec3_dot(cross.columns[2], matrix.columns[2]);
    pMat3 result;
    result.columns[0] = p_vec3_mul_f(cross.columns[0], inv_determinant);
    result.columns[1] = p_vec3_mul_f(cross.columns[1], inv_determinant);
    result.columns[2] = p_vec3_mul_f(cross.columns[2], inv_determinant);
    return p_mat3_transpose(result);
}

/*
 * 4x4 Matrices
 */

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
#ifdef P_MATH__USE_SSE
    result = matrix;
    _MM_TRANSPOSE4_PS(result.columns[0].sse, result.columns[1].sse, result.columns[2].sse, result.columns[3].sse);
#elif defined(P_MATH__USE_NEON)
    float32x4x4_t transposed = vld4q_f32((float*)matrix.columns);
    result.columns[0].neon = transposed.val[0];
    result.columns[1].neon = transposed.val[1];
    result.columns[2].neon = transposed.val[2];
    result.columns[3].neon = transposed.val[3];
#else
    result.elements[0][0] = matrix.elements[0][0];
    result.elements[0][1] = matrix.elements[1][0];
    result.elements[0][2] = matrix.elements[2][0];
    result.elements[0][3] = matrix.elements[3][0];
    result.elements[1][0] = matrix.elements[0][1];
    result.elements[1][1] = matrix.elements[1][1];
    result.elements[1][2] = matrix.elements[2][1];
    result.elements[1][3] = matrix.elements[3][1];
    result.elements[2][0] = matrix.elements[0][2];
    result.elements[2][1] = matrix.elements[1][2];
    result.elements[2][2] = matrix.elements[2][2];
    result.elements[2][3] = matrix.elements[3][2];
    result.elements[3][0] = matrix.elements[0][3];
    result.elements[3][1] = matrix.elements[1][3];
    result.elements[3][2] = matrix.elements[2][3];
    result.elements[3][3] = matrix.elements[3][3];
#endif
    return result;
}

static inline pMat4 p_mat4_add(pMat4 left, pMat4 right) {
    pMat4 result;
    result.columns[0] = p_vec4_add(left.columns[0], right.columns[0]);
    result.columns[1] = p_vec4_add(left.columns[1], right.columns[1]);
    result.columns[2] = p_vec4_add(left.columns[2], right.columns[2]);
    result.columns[3] = p_vec4_add(left.columns[3], right.columns[3]);
    return result;
}

static inline pMat4 p_mat4_sub(pMat4 left, pMat4 right) {
    pMat4 result;
    result.columns[0] = p_vec4_sub(left.columns[0], right.columns[0]);
    result.columns[1] = p_vec4_sub(left.columns[1], right.columns[1]);
    result.columns[2] = p_vec4_sub(left.columns[2], right.columns[2]);
    result.columns[3] = p_vec4_sub(left.columns[3], right.columns[3]);
    return result;
}

static inline pMat4 p_mat4_mul(pMat4 left, pMat4 right) {
    pMat4 result;
#if defined(__PSP__)
    gumMultMatrix(&result._sce, &left._sce, &right._sce);
#else
    result.columns[0] = p_linear_combine_vec4_mat4(right.columns[0], left);
    result.columns[1] = p_linear_combine_vec4_mat4(right.columns[1], left);
    result.columns[2] = p_linear_combine_vec4_mat4(right.columns[2], left);
    result.columns[3] = p_linear_combine_vec4_mat4(right.columns[3], left);
#endif
    return result;
}

static inline pMat4 p_mat4_mul_f(pMat4 matrix, float scalar) {
    pMat4 result;
 #ifdef P_MATH__USE_SSE
    __m128 sse_scalar = _mm_set1_ps(scalar);
    result.columns[0].sse = _mm_mul_ps(matrix.columns[0].sse, sse_scalar);
    result.columns[1].sse = _mm_mul_ps(matrix.columns[1].sse, sse_scalar);
    result.columns[2].sse = _mm_mul_ps(matrix.columns[2].sse, sse_scalar);
    result.columns[3].sse = _mm_mul_ps(matrix.columns[3].sse, sse_scalar);
#elif defined(P_MATH__USE_NEON)
    result.columns[0].neon = vmulq_n_f32(matrix.columns[0].neon, scalar);
    result.columns[1].neon = vmulq_n_f32(matrix.columns[1].neon, scalar);
    result.columns[2].neon = vmulq_n_f32(matrix.columns[2].neon, scalar);
    result.columns[3].neon = vmulq_n_f32(matrix.columns[3].neon, scalar);
#else
    result.elements[0][0] = matrix.elements[0][0] * scalar;
    result.elements[0][1] = matrix.elements[0][1] * scalar;
    result.elements[0][2] = matrix.elements[0][2] * scalar;
    result.elements[0][3] = matrix.elements[0][3] * scalar;
    result.elements[1][0] = matrix.elements[1][0] * scalar;
    result.elements[1][1] = matrix.elements[1][1] * scalar;
    result.elements[1][2] = matrix.elements[1][2] * scalar;
    result.elements[1][3] = matrix.elements[1][3] * scalar;
    result.elements[2][0] = matrix.elements[2][0] * scalar;
    result.elements[2][1] = matrix.elements[2][1] * scalar;
    result.elements[2][2] = matrix.elements[2][2] * scalar;
    result.elements[2][3] = matrix.elements[2][3] * scalar;
    result.elements[3][0] = matrix.elements[3][0] * scalar;
    result.elements[3][1] = matrix.elements[3][1] * scalar;
    result.elements[3][2] = matrix.elements[3][2] * scalar;
    result.elements[3][3] = matrix.elements[3][3] * scalar;
#endif
    return result;
}

static inline pVec4 p_mat4_mul_vec4(pMat4 matrix, pVec4 v) {
    return p_linear_combine_vec4_mat4(v, matrix);
}

static inline pMat4 p_mat4_div_f(pMat4 matrix, float scalar) {
    pMat4 result;
#ifdef P_MATH__USE_SSE
    __m128 sse_scalar = _mm_set1_ps(scalar);
    result.columns[0].sse = _mm_div_ps(matrix.columns[0].sse, sse_scalar);
    result.columns[1].sse = _mm_div_ps(matrix.columns[1].sse, sse_scalar);
    result.columns[2].sse = _mm_div_ps(matrix.columns[2].sse, sse_scalar);
    result.columns[3].sse = _mm_div_ps(matrix.columns[3].sse, sse_scalar);
#elif defined(P_MATH__USE_NEON)
    float32x4_t NEONScalar = vdupq_n_f32(scalar);
    result.columns[0].neon = vdivq_f32(matrix.columns[0].neon, NEONScalar);
    result.columns[1].neon = vdivq_f32(matrix.columns[1].neon, NEONScalar);
    result.columns[2].neon = vdivq_f32(matrix.columns[2].neon, NEONScalar);
    result.columns[3].neon = vdivq_f32(matrix.columns[3].neon, NEONScalar);
#else
    result.elements[0][0] = matrix.elements[0][0] / scalar;
    result.elements[0][1] = matrix.elements[0][1] / scalar;
    result.elements[0][2] = matrix.elements[0][2] / scalar;
    result.elements[0][3] = matrix.elements[0][3] / scalar;
    result.elements[1][0] = matrix.elements[1][0] / scalar;
    result.elements[1][1] = matrix.elements[1][1] / scalar;
    result.elements[1][2] = matrix.elements[1][2] / scalar;
    result.elements[1][3] = matrix.elements[1][3] / scalar;
    result.elements[2][0] = matrix.elements[2][0] / scalar;
    result.elements[2][1] = matrix.elements[2][1] / scalar;
    result.elements[2][2] = matrix.elements[2][2] / scalar;
    result.elements[2][3] = matrix.elements[2][3] / scalar;
    result.elements[3][0] = matrix.elements[3][0] / scalar;
    result.elements[3][1] = matrix.elements[3][1] / scalar;
    result.elements[3][2] = matrix.elements[3][2] / scalar;
    result.elements[3][3] = matrix.elements[3][3] / scalar;
#endif
    return result;
}

static inline float p_mat4_determinant(pMat4 matrix) {
    pVec3 c01 = p_cross(matrix.columns[0].xyz, matrix.columns[1].xyz);
    pVec3 c23 = p_cross(matrix.columns[2].xyz, matrix.columns[3].xyz);
    pVec3 b10 = p_vec3_sub(p_vec3_mul_f(matrix.columns[0].xyz, matrix.columns[1].w), p_vec3_mul_f(matrix.columns[1].xyz, matrix.columns[0].w));
    pVec3 b32 = p_vec3_sub(p_vec3_mul_f(matrix.columns[2].xyz, matrix.columns[3].w), p_vec3_mul_f(matrix.columns[3].xyz, matrix.columns[2].w));
    return p_vec3_dot(c01, b32) + p_vec3_dot(c23, b10);
}

// Returns a general-purpose inverse of an pMat4. Note that special-purpose inverses of many transformations
// are available and will be more efficient.
static inline pMat4 p_mat4_inv_general(pMat4 matrix) {
    pVec3 c01 = p_cross(matrix.columns[0].xyz, matrix.columns[1].xyz);
    pVec3 c23 = p_cross(matrix.columns[2].xyz, matrix.columns[3].xyz);
    pVec3 b10 = p_vec3_sub(p_vec3_mul_f(matrix.columns[0].xyz, matrix.columns[1].w), p_vec3_mul_f(matrix.columns[1].xyz, matrix.columns[0].w));
    pVec3 b23 = p_vec3_sub(p_vec3_mul_f(matrix.columns[2].xyz, matrix.columns[3].w), p_vec3_mul_f(matrix.columns[3].xyz, matrix.columns[2].w));
    float inv_determinant = 1.0f / (p_vec3_dot(c01, b23) + p_vec3_dot(c23, b10));
    c01 = p_vec3_mul_f(c01, inv_determinant);
    c23 = p_vec3_mul_f(c23, inv_determinant);
    b10 = p_vec3_mul_f(b10, inv_determinant);
    b23 = p_vec3_mul_f(b23, inv_determinant);
    pMat4 result;
    result.columns[0] = p_vec4v(p_vec3_add(p_cross(matrix.columns[1].xyz, b23), p_vec3_mul_f(c23, matrix.columns[1].w)), -p_vec3_dot(matrix.columns[1].xyz, c23));
    result.columns[1] = p_vec4v(p_vec3_sub(p_cross(b23, matrix.columns[0].xyz), p_vec3_mul_f(c23, matrix.columns[0].w)), +p_vec3_dot(matrix.columns[0].xyz, c23));
    result.columns[2] = p_vec4v(p_vec3_add(p_cross(matrix.columns[3].xyz, b10), p_vec3_mul_f(c01, matrix.columns[3].w)), -p_vec3_dot(matrix.columns[3].xyz, c01));
    result.columns[3] = p_vec4v(p_vec3_sub(p_cross(b10, matrix.columns[2].xyz), p_vec3_mul_f(c01, matrix.columns[2].w)), +p_vec3_dot(matrix.columns[2].xyz, c01));
    return p_mat4_transpose(result);
}

/*
 * Common graphics transformations
 */

// Produces a right-handed orthographic projection matrix with z ranging from -1 to 1 (the GL convention).
// left, right, bottom, and top specify the coordinates of their respective clipping planes.
// near and far specify the distances to the near and far clipping planes.
static inline pMat4 p_orthographic_rh_no(float left, float right, float bottom, float top, float near, float far) {
    pMat4 result = {0};
    result.elements[0][0] = 2.0f / (right - left);
    result.elements[1][1] = 2.0f / (top - bottom);
    result.elements[2][2] = 2.0f / (near - far);
    result.elements[3][3] = 1.0f;
    result.elements[3][0] = (left + right) / (left - right);
    result.elements[3][1] = (bottom + top) / (bottom - top);
    result.elements[3][2] = (near + far) / (near - far);
    return result;
}

// Produces a right-handed orthographic projection matrix with z ranging from 0 to 1 (the DirectX convention).
// left, right, bottom, and top specify the coordinates of their respective clipping planes.
// near and far specify the distances to the near and far clipping planes.
static inline pMat4 p_orthographic_rh_zo(float left, float right, float bottom, float top, float near, float far) {
    pMat4 result = {0};
    result.elements[0][0] = 2.0f / (right - left);
    result.elements[1][1] = 2.0f / (top - bottom);
    result.elements[2][2] = 1.0f / (near - far);
    result.elements[3][3] = 1.0f;
    result.elements[3][0] = (left + right) / (left - right);
    result.elements[3][1] = (bottom + top) / (bottom - top);
    result.elements[3][2] = (near) / (near - far);
    return result;
}

// Produces a left-handed orthographic projection matrix with z ranging from -1 to 1 (the GL convention).
// left, right, bottom, and top specify the coordinates of their respective clipping planes.
// near and far specify the distances to the near and far clipping planes.
static inline pMat4 p_orthographic_lh_no(float left, float right, float bottom, float top, float near, float far) {
    pMat4 result = p_orthographic_rh_no(left, right, bottom, top, near, far);
    result.elements[2][2] = -result.elements[2][2];
    return result;
}

// Produces a left-handed orthographic projection matrix with z ranging from 0 to 1 (the DirectX convention).
// left, right, bottom, and top specify the coordinates of their respective clipping planes.
// near and far specify the distances to the near and far clipping planes.
static inline pMat4 p_orthographic_lh_zo(float left, float right, float bottom, float top, float near, float far) {
    pMat4 result = p_orthographic_rh_zo(left, right, bottom, top, near, far);
    result.elements[2][2] = -result.elements[2][2];
    return result;
}

// Returns an inverse for the given orthographic projection matrix. Works for all orthographic
// projection matrices, regardless of handedness or NDC convention.
static inline pMat4 p_inv_orthographic(pMat4 ortho_matrix) {
    pMat4 result = {0};
    result.elements[0][0] = 1.0f / ortho_matrix.elements[0][0];
    result.elements[1][1] = 1.0f / ortho_matrix.elements[1][1];
    result.elements[2][2] = 1.0f / ortho_matrix.elements[2][2];
    result.elements[3][3] = 1.0f;
    result.elements[3][0] = -ortho_matrix.elements[3][0] * result.elements[0][0];
    result.elements[3][1] = -ortho_matrix.elements[3][1] * result.elements[1][1];
    result.elements[3][2] = -ortho_matrix.elements[3][2] * result.elements[2][2];
    return result;
}

static inline pMat4 p_perspective_rh_no(float fov, float aspect_ratio, float near, float far) {
    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
    pMat4 result = {0};
    float cotangent = 1.0f / p_tanf(fov / 2.0f);
    result.elements[0][0] = cotangent / aspect_ratio;
    result.elements[1][1] = cotangent;
    result.elements[2][3] = -1.0f;
    result.elements[2][2] = (near + far) / (near - far);
    result.elements[3][2] = (2.0f * near * far) / (near - far);
    return result;
}

static inline pMat4 p_perspective_rh_zo(float fov, float aspect_ratio, float near, float far) {
    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
    pMat4 result = {0};
    float cotangent = 1.0f / p_tanf(fov / 2.0f);
    result.elements[0][0] = cotangent / aspect_ratio;
    result.elements[1][1] = cotangent;
    result.elements[2][3] = -1.0f;
    result.elements[2][2] = (far) / (near - far);
    result.elements[3][2] = (near * far) / (near - far);
    return result;
}

static inline pMat4 p_perspective_lh_no(float fov, float aspect_ratio, float near, float far) {
    pMat4 result = p_perspective_rh_no(fov, aspect_ratio, near, far);
    result.elements[2][2] = -result.elements[2][2];
    result.elements[2][3] = -result.elements[2][3];
    return result;
}

static inline pMat4 p_perspective_lh_zo(float fov, float aspect_ratio, float near, float far) {
    pMat4 result = p_perspective_rh_zo(fov, aspect_ratio, near, far);
    result.elements[2][2] = -result.elements[2][2];
    result.elements[2][3] = -result.elements[2][3];
    return result;
}

static inline pMat4 p_inv_perspective_rh(pMat4 perspective_matrix) {
    pMat4 result = {0};
    result.elements[0][0] = 1.0f / perspective_matrix.elements[0][0];
    result.elements[1][1] = 1.0f / perspective_matrix.elements[1][1];
    result.elements[2][2] = 0.0f;
    result.elements[2][3] = 1.0f / perspective_matrix.elements[3][2];
    result.elements[3][3] = perspective_matrix.elements[2][2] * result.elements[2][3];
    result.elements[3][2] = perspective_matrix.elements[2][3];
    return result;
}

static inline pMat4 p_inv_perspective_lh(pMat4 perspective_matrix) {
    pMat4 result = {0};
    result.elements[0][0] = 1.0f / perspective_matrix.elements[0][0];
    result.elements[1][1] = 1.0f / perspective_matrix.elements[1][1];
    result.elements[2][2] = 0.0f;
    result.elements[2][3] = 1.0f / perspective_matrix.elements[3][2];
    result.elements[3][3] = perspective_matrix.elements[2][2] * -result.elements[2][3];
    result.elements[3][2] = perspective_matrix.elements[2][3];
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

static inline pMat4 p_inv_translate(pMat4 translation_matrix) {
    pMat4 result = translation_matrix;
    result.elements[3][0] = -result.elements[3][0];
    result.elements[3][1] = -result.elements[3][1];
    result.elements[3][2] = -result.elements[3][2];
    return result;
}

static inline pMat4 p_rotate_rh(float angle, pVec3 axis) {
    pMat4 result = p_mat4_i();
    axis = p_vec3_norm(axis);
    float sin_theta = p_sinf(angle);
    float cos_theta = p_cosf(angle);
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

static inline pMat4 p_rotate_lh(float angle, pVec3 axis) {
    /* NOTE(lcf): matrix will be inverse/transpose of RH. */
    return p_rotate_rh(-angle, axis);
}

static inline pMat4 p_rotate_xyz(pVec3 rotations) {
    pMat4 result;
#if defined(__PSP__)
    gumLoadIdentity(&result._sce);
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

static inline pMat4 p_inv_rotate(pMat4 RotationMatrix) {
    return p_mat4_transpose(RotationMatrix);
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
}

static inline pMat4 p_inv_scale(pMat4 scale_matrix) {
    pMat4 result = scale_matrix;
    result.elements[0][0] = 1.0f / result.elements[0][0];
    result.elements[1][1] = 1.0f / result.elements[1][1];
    result.elements[2][2] = 1.0f / result.elements[2][2];
    return result;
}

static inline pMat4 _p_look_at(pVec3 f,  pVec3 s, pVec3 u,  pVec3 eye) {
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
    return _p_look_at(f, s, u, eye);
}

static inline pMat4 p_look_at_lh(pVec3 eye, pVec3 center, pVec3 up) {
    pVec3 f = p_vec3_norm(p_vec3_sub(eye, center));
    pVec3 s = p_vec3_norm(p_cross(f, up));
    pVec3 u = p_cross(s, f);
    return _p_look_at(f, s, u, eye);
}

static inline pMat4 p_inv_look_at(pMat4 matrix) {
    pMat4 result;
    pMat3 Rotation = {0};
    Rotation.columns[0] = matrix.columns[0].xyz;
    Rotation.columns[1] = matrix.columns[1].xyz;
    Rotation.columns[2] = matrix.columns[2].xyz;
    Rotation = p_mat3_transpose(Rotation);
    result.columns[0] = p_vec4v(Rotation.columns[0], 0.0f);
    result.columns[1] = p_vec4v(Rotation.columns[1], 0.0f);
    result.columns[2] = p_vec4v(Rotation.columns[2], 0.0f);
    result.columns[3] = p_vec4_mul_f(matrix.columns[3], -1.0f);
    result.elements[3][0] = -1.0f * matrix.elements[3][0] /
        (Rotation.elements[0][0] + Rotation.elements[0][1] + Rotation.elements[0][2]);
    result.elements[3][1] = -1.0f * matrix.elements[3][1] /
        (Rotation.elements[1][0] + Rotation.elements[1][1] + Rotation.elements[1][2]);
    result.elements[3][2] = -1.0f * matrix.elements[3][2] /
        (Rotation.elements[2][0] + Rotation.elements[2][1] + Rotation.elements[2][2]);
    result.elements[3][3] = 1.0f;
    return result;
}

/*
 * Quaternion operations
 */

static inline pQuat p_quat(float x, float y, float z, float w) {
    pQuat result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_setr_ps(x, y, z, w);
#elif defined(P_MATH__USE_NEON)
    float32x4_t v = { x, y, z, w };
    result.neon = v;
#else
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
#endif
    return result;
}

static inline pQuat p_quat_vec4(pVec4 v) {
    pQuat result;
#ifdef P_MATH__USE_SSE
    result.sse = v.sse;
#elif defined(P_MATH__USE_NEON)
    result.neon = v.neon;
#else
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    result.w = v.w;
#endif
    return result;
}

static inline pQuat p_quat_add(pQuat left, pQuat right) {
    pQuat result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_add_ps(left.sse, right.sse);
#elif defined(P_MATH__USE_NEON)
    result.neon = vaddq_f32(left.neon, right.neon);
#else
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    result.z = left.z + right.z;
    result.w = left.w + right.w;
#endif
    return result;
}

static inline pQuat p_quat_sub(pQuat left, pQuat right) {
    pQuat result;
#ifdef P_MATH__USE_SSE
    result.sse = _mm_sub_ps(left.sse, right.sse);
#elif defined(P_MATH__USE_NEON)
    result.neon = vsubq_f32(left.neon, right.neon);
#else
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    result.z = left.z - right.z;
    result.w = left.w - right.w;
#endif
    return result;
}

static inline pQuat p_quat_mul(pQuat left, pQuat right) {
    pQuat result;
#ifdef P_MATH__USE_SSE
    __m128 sse_result_one = _mm_xor_ps(_mm_shuffle_ps(left.sse, left.sse, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.f, -0.f, 0.f, -0.f));
    __m128 sse_result_two = _mm_shuffle_ps(right.sse, right.sse, _MM_SHUFFLE(0, 1, 2, 3));
    __m128 sse_result_three = _mm_mul_ps(sse_result_two, sse_result_one);
    sse_result_one = _mm_xor_ps(_mm_shuffle_ps(left.sse, left.sse, _MM_SHUFFLE(1, 1, 1, 1)) , _mm_setr_ps(0.f, 0.f, -0.f, -0.f));
    sse_result_two = _mm_shuffle_ps(right.sse, right.sse, _MM_SHUFFLE(1, 0, 3, 2));
    sse_result_three = _mm_add_ps(sse_result_three, _mm_mul_ps(sse_result_two, sse_result_one));
    sse_result_one = _mm_xor_ps(_mm_shuffle_ps(left.sse, left.sse, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.f, 0.f, 0.f, -0.f));
    sse_result_two = _mm_shuffle_ps(right.sse, right.sse, _MM_SHUFFLE(2, 3, 0, 1));
    sse_result_three = _mm_add_ps(sse_result_three, _mm_mul_ps(sse_result_two, sse_result_one));
    sse_result_one = _mm_shuffle_ps(left.sse, left.sse, _MM_SHUFFLE(3, 3, 3, 3));
    sse_result_two = _mm_shuffle_ps(right.sse, right.sse, _MM_SHUFFLE(3, 2, 1, 0));
    result.sse = _mm_add_ps(sse_result_three, _mm_mul_ps(sse_result_two, sse_result_one));
#elif defined(P_MATH__USE_NEON)
    float32x4_t right1032 = vrev64q_f32(right.neon);
    float32x4_t right3210 = vcombine_f32(vget_high_f32(right1032), vget_low_f32(right1032));
    float32x4_t right2301 = vrev64q_f32(right3210);
    float32x4_t first_sign = {1.0f, -1.0f, 1.0f, -1.0f};
    result.neon = vmulq_f32(right3210, vmulq_f32(vdupq_laneq_f32(left.neon, 0), first_sign));
    float32x4_t second_sign = {1.0f, 1.0f, -1.0f, -1.0f};
    result.neon = vfmaq_f32(result.neon, right2301, vmulq_f32(vdupq_laneq_f32(left.neon, 1), second_sign));
    float32x4_t third_sign = {-1.0f, 1.0f, 1.0f, -1.0f};
    result.neon = vfmaq_f32(result.neon, right1032, vmulq_f32(vdupq_laneq_f32(left.neon, 2), third_sign));
    result.neon = vfmaq_laneq_f32(result.neon, right.neon, left.neon, 3);
#else
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
#endif
    return result;
}

static inline pQuat p_quat_mul_f(pQuat left, float multiplicative) {
    pQuat result;
#ifdef P_MATH__USE_SSE
    __m128 scalar = _mm_set1_ps(multiplicative);
    result.sse = _mm_mul_ps(left.sse, scalar);
#elif defined(P_MATH__USE_NEON)
    result.neon = vmulq_n_f32(left.neon, multiplicative);
#else
    result.x = left.x * multiplicative;
    result.y = left.y * multiplicative;
    result.z = left.z * multiplicative;
    result.w = left.w * multiplicative;
#endif
    return result;
}

static inline pQuat p_quat_div_f(pQuat left, float divnd) {
    pQuat result;
#ifdef P_MATH__USE_SSE
    __m128 scalar = _mm_set1_ps(divnd);
    result.sse = _mm_div_ps(left.sse, scalar);
#elif defined(P_MATH__USE_NEON)
    float32x4_t scalar = vdupq_n_f32(divnd);
    result.neon = vdivq_f32(left.neon, scalar);
#else
    result.x = left.x / divnd;
    result.y = left.y / divnd;
    result.z = left.z / divnd;
    result.w = left.w / divnd;
#endif
    return result;
}

static inline float p_quat_dot(pQuat left, pQuat right) {
    float result;
#ifdef P_MATH__USE_SSE
    __m128 sse_result_one = _mm_mul_ps(left.sse, right.sse);
    __m128 sse_result_two = _mm_shuffle_ps(sse_result_one, sse_result_one, _MM_SHUFFLE(2, 3, 0, 1));
    sse_result_one = _mm_add_ps(sse_result_one, sse_result_two);
    sse_result_two = _mm_shuffle_ps(sse_result_one, sse_result_one, _MM_SHUFFLE(0, 1, 2, 3));
    sse_result_one = _mm_add_ps(sse_result_one, sse_result_two);
    _mm_store_ss(&result, sse_result_one);
#elif defined(P_MATH__USE_NEON)
    float32x4_t neon_multiply_result = vmulq_f32(left.neon, right.neon);
    float32x4_t neon_half_add = vpaddq_f32(neon_multiply_result, neon_multiply_result);
    float32x4_t neon_full_add = vpaddq_f32(neon_half_add, neon_half_add);
    result = vgetq_lane_f32(neon_full_add, 0);
#else
    result = ((left.x * right.x) + (left.z * right.z)) + ((left.y * right.y) + (left.w * right.w));
#endif
    return result;
}

static inline pQuat p_quat_inv(pQuat left) {
    pQuat result;
    result.x = -left.x;
    result.y = -left.y;
    result.z = -left.z;
    result.w = left.w;
    return p_quat_div_f(result, (p_quat_dot(left, left)));
}

static inline pQuat p_quat_norm(pQuat quat) {
    /* NOTE(lcf): Take advantage of SSE implementation in p_vec4_norm */
    pVec4 vec = {quat.x, quat.y, quat.z, quat.w};
    vec = p_vec4_norm(vec);
    pQuat result = {vec.x, vec.y, vec.z, vec.w};
    return result;
}

static inline pQuat _p_quat_mix(pQuat left, float mix_left, pQuat right, float mix_right) {
    pQuat result;
#ifdef P_MATH__USE_SSE
    __m128 scalar_left = _mm_set1_ps(mix_left);
    __m128 scalar_right = _mm_set1_ps(mix_right);
    __m128 sse_result_one = _mm_mul_ps(left.sse, scalar_left);
    __m128 sse_result_two = _mm_mul_ps(right.sse, scalar_right);
    result.sse = _mm_add_ps(sse_result_one, sse_result_two);
#elif defined(P_MATH__USE_NEON)
    float32x4_t scaled_left = vmulq_n_f32(left.neon, mix_left);
    float32x4_t scaled_right = vmulq_n_f32(right.neon, mix_right);
    result.neon = vaddq_f32(scaled_left, scaled_right);
#else
    result.x = left.x*mix_left + right.x*mix_right;
    result.y = left.y*mix_left + right.y*mix_right;
    result.z = left.z*mix_left + right.z*mix_right;
    result.w = left.w*mix_left + right.w*mix_right;
#endif
    return result;
}

static inline pQuat p_quat_nlerp(pQuat left, float t, pQuat right) {
    pQuat result = _p_quat_mix(left, 1.0f-t, right, t);
    result = p_quat_norm(result);
    return result;
}

static inline pQuat p_quat_slerp(pQuat left, float t, pQuat right) {
    pQuat result;
    float cos_theta = p_quat_dot(left, right);
    if (cos_theta < 0.0f) { /* NOTE(lcf): Take shortest path on Hyper-sphere */
        cos_theta = -cos_theta;
        right = p_quat(-right.x, -right.y, -right.z, -right.w);
    }
    /* NOTE(lcf): Use Normalized Linear interpolation when vectors are roughly not L.I. */
    if (cos_theta > 0.9995f) {
        result = p_quat_nlerp(left, t, right);
    } else {
        float angle = p_acosf(cos_theta);
        float mix_left = p_sinf((1.0f - t) * angle);
        float mix_right = p_sinf(t * angle);
        result = _p_quat_mix(left, mix_left, right, mix_right);
        result = p_quat_norm(result);
    }
    return result;
}

static inline pMat4 p_quat_to_mat4(pQuat left) {
    pMat4 result;
    pQuat normalized_q = p_quat_norm(left);
    float xx, yy, zz,
          xy, xz, yz,
          wx, wy, wz;
    xx = normalized_q.x * normalized_q.x;
    yy = normalized_q.y * normalized_q.y;
    zz = normalized_q.z * normalized_q.z;
    xy = normalized_q.x * normalized_q.y;
    xz = normalized_q.x * normalized_q.z;
    yz = normalized_q.y * normalized_q.z;
    wx = normalized_q.w * normalized_q.x;
    wy = normalized_q.w * normalized_q.y;
    wz = normalized_q.w * normalized_q.z;
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

// This method taken from Mike Day at Insomniac Games.
// https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
//
// Note that as mentioned at the top of the paper, the paper assumes the matrix
// would be *post*-multiplied to a vector to rotate it, meaning the matrix is
// the transpose of what we're dealing with. But, because our matrices are
// stored in column-major order, the indices *appear* to match the paper.
//
// For example, m12 in the paper is row 1, column 2. We need to transpose it to
// row 2, column 1. But, because the column comes first when referencing
// elements, it looks like m.elements[1][2].
//
// Don't be confused! Or if you must be confused, at least trust this
// comment. :)
static inline pQuat p_mat4_to_quat_rh(pMat4 m) {
    float t;
    pQuat q;
    if (m.elements[2][2] < 0.0f) {
        if (m.elements[0][0] > m.elements[1][1]) {
            t = 1 + m.elements[0][0] - m.elements[1][1] - m.elements[2][2];
            q = p_quat(
                t,
                m.elements[0][1] + m.elements[1][0],
                m.elements[2][0] + m.elements[0][2],
                m.elements[1][2] - m.elements[2][1]
            );
        } else {
            t = 1 - m.elements[0][0] + m.elements[1][1] - m.elements[2][2];
            q = p_quat(
                m.elements[0][1] + m.elements[1][0],
                t,
                m.elements[1][2] + m.elements[2][1],
                m.elements[2][0] - m.elements[0][2]
            );
        }
    } else {
        if (m.elements[0][0] < -m.elements[1][1]) {
            t = 1 - m.elements[0][0] - m.elements[1][1] + m.elements[2][2];
            q = p_quat(
                m.elements[2][0] + m.elements[0][2],
                m.elements[1][2] + m.elements[2][1],
                t,
                m.elements[0][1] - m.elements[1][0]
            );
        } else {
            t = 1 + m.elements[0][0] + m.elements[1][1] + m.elements[2][2];
            q = p_quat(
                m.elements[1][2] - m.elements[2][1],
                m.elements[2][0] - m.elements[0][2],
                m.elements[0][1] - m.elements[1][0],
                t
            );
        }
    }
    q = p_quat_mul_f(q, 0.5f / p_sqrtf(t));
    return q;
}

static inline pQuat p_mat4_to_quat_lh(pMat4 m) {
    float t;
    pQuat q;
    if (m.elements[2][2] < 0.0f) {
        if (m.elements[0][0] > m.elements[1][1]) {
            t = 1 + m.elements[0][0] - m.elements[1][1] - m.elements[2][2];
            q = p_quat(
                t,
                m.elements[0][1] + m.elements[1][0],
                m.elements[2][0] + m.elements[0][2],
                m.elements[2][1] - m.elements[1][2]
            );
        } else {
            t = 1 - m.elements[0][0] + m.elements[1][1] - m.elements[2][2];
            q = p_quat(
                m.elements[0][1] + m.elements[1][0],
                t,
                m.elements[1][2] + m.elements[2][1],
                m.elements[0][2] - m.elements[2][0]
            );
        }
    } else {
        if (m.elements[0][0] < -m.elements[1][1]) {
            t = 1 - m.elements[0][0] - m.elements[1][1] + m.elements[2][2];
            q = p_quat(
                m.elements[2][0] + m.elements[0][2],
                m.elements[1][2] + m.elements[2][1],
                t,
                m.elements[1][0] - m.elements[0][1]
            );
        } else {
            t = 1 + m.elements[0][0] + m.elements[1][1] + m.elements[2][2];
            q = p_quat(
                m.elements[2][1] - m.elements[1][2],
                m.elements[0][2] - m.elements[2][0],
                m.elements[1][0] - m.elements[0][2],
                t
            );
        }
    }
    q = p_quat_mul_f(q, 0.5f / p_sqrtf(t));
    return q;
}


static inline pQuat p_quat_from_axis_angle_rh(pVec3 axis, float angle) {
    pQuat result;
    pVec3 axis_normalized = p_vec3_norm(axis);
    float sine_of_rotation = p_sinf(angle / 2.0f);
    result.xyz = p_vec3_mul_f(axis_normalized, sine_of_rotation);
    result.w = p_cosf(angle / 2.0f);
    return result;
}

static inline pQuat p_quat_from_axis_angle_lh(pVec3 axis, float angle) {
    return p_quat_from_axis_angle_rh(axis, -angle);
}

static inline pQuat p_quat_from_norm_pair(pVec3 left, pVec3 right) {
    pQuat result;
    result.xyz = p_cross(left, right);
    result.w = 1.0f + p_vec3_dot(left, right);
    return p_quat_norm(result);
}

static inline pQuat p_quat_from_vec_pair(pVec3 left, pVec3 right) {
    return p_quat_from_norm_pair(p_vec3_norm(left), p_vec3_norm(right));
}

static inline pVec2 p_vec2_rotate(pVec2 v, float angle) {
    float sin_a = p_sinf(angle);
    float cos_a = p_cosf(angle);
    return p_vec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}

// implementation from
// https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/
static inline pVec3 p_vec3_rotate_quat(pVec3 v, pQuat q) {
    pVec3 t = p_vec3_mul_f(p_cross(q.xyz, v), 2);
    return p_vec3_add(v, p_vec3_add(p_vec3_mul_f(t, q.w), p_cross(q.xyz, t)));
}

static inline pVec3 p_vec3_rotate_axis_angle_lh(pVec3 v, pVec3 axis, float angle) {
    return p_vec3_rotate_quat(v, p_quat_from_axis_angle_lh(axis, angle));
}

static inline pVec3 p_vec3_rotate_axis_angle_rh(pVec3 v, pVec3 axis, float angle) {
    return p_vec3_rotate_quat(v, p_quat_from_axis_angle_rh(axis, angle));
}


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

static inline float p_len(pVec2 a) { return p_vec2_len(a); }
static inline float p_len(pVec3 a) { return p_vec3_len(a); }
static inline float p_len(pVec4 a) { return p_vec4_len(a); }

static inline float p_len_sqr(pVec2 a) { return p_vec2_len_sqr(a); }
static inline float p_len_sqr(pVec3 a) { return p_vec3_len_sqr(a); }
static inline float p_len_sqr(pVec4 a) { return p_vec4_len_sqr(a); }

static inline pVec2 p_norm(pVec2 a) { return p_vec2_norm(a); }
static inline pVec3 p_norm(pVec3 a) { return p_vec3_norm(a); }
static inline pVec4 p_norm(pVec4 a) { return p_vec4_norm(a); }
static inline pQuat p_norm(pQuat a) { return p_quat_norm(a); }

static inline float p_dot(pVec2 left, pVec2 VecTwo) { return p_vec2_dot(left, VecTwo); }
static inline float p_dot(pVec3 left, pVec3 VecTwo) { return p_vec3_dot(left, VecTwo); }
static inline float p_dot(pVec4 left, pVec4 VecTwo) { return p_vec4_dot(left, VecTwo); }

static inline pVec2 p_lerp(pVec2 left, float t, pVec2 right) { return p_lerp_vec2(left, t, right); }
static inline pVec3 p_lerp(pVec3 left, float t, pVec3 right) { return p_lerp_vec3(left, t, right); }
static inline pVec4 p_lerp(pVec4 left, float t, pVec4 right) { return p_lerp_vec4(left, t, right); }

static inline pMat2 p_transpose(pMat2 matrix) { return p_mat2_transpose(matrix); }
static inline pMat3 p_transpose(pMat3 matrix) { return p_mat3_transpose(matrix); }
static inline pMat4 p_transpose(pMat4 matrix) { return p_mat4_transpose(matrix); }

static inline float p_determinant(pMat2 matrix) { return p_mat2_determinant(matrix); }
static inline float p_determinant(pMat3 matrix) { return p_mat3_determinant(matrix); }
static inline float p_determinant(pMat4 matrix) { return p_mat4_determinant(matrix); }

static inline pMat2 p_inv_general(pMat2 matrix) { return p_mat2_inv_general(matrix); }
static inline pMat3 p_inv_general(pMat3 matrix) { return p_mat3_inv_general(matrix); }
static inline pMat4 p_inv_general(pMat4 matrix) { return p_mat4_inv_general(matrix); }

static inline float p_dot(pQuat quat_one, pQuat quat_two) { return p_quat_dot(quat_one, quat_two); }

static inline pVec2 p_add(pVec2 left, pVec2 right) { return p_vec2_add(left, right); }
static inline pVec3 p_add(pVec3 left, pVec3 right) { return p_vec3_add(left, right); }
static inline pVec4 p_add(pVec4 left, pVec4 right) { return p_vec4_add(left, right); }
static inline pMat2 p_add(pMat2 left, pMat2 right) { return p_mat2_add(left, right); }
static inline pMat3 p_add(pMat3 left, pMat3 right) { return p_mat3_add(left, right); }
static inline pMat4 p_add(pMat4 left, pMat4 right) { return p_mat4_add(left, right); }
static inline pQuat p_add(pQuat left, pQuat right) { return p_quat_add(left, right); }

static inline pVec2 p_sub(pVec2 left, pVec2 right) { return p_vec2_sub(left, right); }
static inline pVec3 p_sub(pVec3 left, pVec3 right) { return p_vec3_sub(left, right); }
static inline pVec4 p_sub(pVec4 left, pVec4 right) { return p_vec4_sub(left, right); }
static inline pMat2 p_sub(pMat2 left, pMat2 right) { return p_mat2_sub(left, right); }
static inline pMat3 p_sub(pMat3 left, pMat3 right) { return p_mat3_sub(left, right); }
static inline pMat4 p_sub(pMat4 left, pMat4 right) { return p_mat4_sub(left, right); }
static inline pQuat p_sub(pQuat left, pQuat right) { return p_quat_sub(left, right); }

static inline pVec2 p_mul(pVec2 left, pVec2 right) { return p_vec2_mul(left, right); }
static inline pVec2 p_mul(pVec2 left, float right) { return p_vec2_mul_f(left, right); }
static inline pVec3 p_mul(pVec3 left, pVec3 right) { return p_vec3_mul(left, right); }
static inline pVec3 p_mul(pVec3 left, float right) { return p_vec3_mul_f(left, right); }
static inline pVec4 p_mul(pVec4 left, pVec4 right) { return p_vec4_mul(left, right); }
static inline pVec4 p_mul(pVec4 left, float right) { return p_vec4_mul_f(left, right); }
static inline pMat2 p_mul(pMat2 left, pMat2 right) { return p_mat2_mul(left, right); }
static inline pMat3 p_mul(pMat3 left, pMat3 right) { return p_mat3_mul(left, right); }
static inline pMat4 p_mul(pMat4 left, pMat4 right) { return p_mat4_mul(left, right); }
static inline pMat2 p_mul(pMat2 left, float right) { return p_mat2_mul_f(left, right); }
static inline pMat3 p_mul(pMat3 left, float right) { return p_mat3_mul_f(left, right); }
static inline pMat4 p_mul(pMat4 left, float right) { return p_mat4_mul_f(left, right); }
static inline pVec2 p_mul(pMat2 matrix, pVec2 v) { return p_mat2_mul_vec2(matrix, v); }
static inline pVec3 p_mul(pMat3 matrix, pVec3 v) { return p_mat3_mul_v(matrix, v); }
static inline pVec4 p_mul(pMat4 matrix, pVec4 v) { return p_mat4_mul_vec4(matrix, v); }
static inline pQuat p_mul(pQuat left, pQuat right) { return p_quat_mul(left, right); }
static inline pQuat p_mul(pQuat left, float right) { return p_quat_mul_f(left, right); }

static inline pVec2 p_div(pVec2 left, pVec2 right) { return p_vec2_div(left, right); }
static inline pVec2 p_div(pVec2 left, float right) { return p_vec2_div_f(left, right); }
static inline pVec3 p_div(pVec3 left, pVec3 right) { return p_vec3_div(left, right); }
static inline pVec3 p_div(pVec3 left, float right) { return p_vec3_div_f(left, right); }
static inline pVec4 p_div(pVec4 left, pVec4 right) { return p_vec4_div(left, right); }
static inline pVec4 p_div(pVec4 left, float right) { return p_vec4_div_f(left, right); }
static inline pMat2 p_div(pMat2 left, float right) { return p_mat2_div_f(left, right); }
static inline pMat3 p_div(pMat3 left, float right) { return p_mat3_div_f(left, right); }
static inline pMat4 p_div(pMat4 left, float right) { return p_mat4_div_f(left, right); }
static inline pQuat p_div(pQuat left, float right) { return p_quat_div_f(left, right); }

static inline bool p_eq(pVec2 left, pVec2 right) { return p_vec2_eq(left, right); }
static inline bool p_eq(pVec3 left, pVec3 right) { return p_vec3_eq(left, right); }
static inline bool p_eq(pVec4 left, pVec4 right) { return p_vec4_eq(left, right); }

static inline pVec2 operator+(pVec2 left, pVec2 right) { return p_vec2_add(left, right); }
static inline pVec3 operator+(pVec3 left, pVec3 right) { return p_vec3_add(left, right); }
static inline pVec4 operator+(pVec4 left, pVec4 right) { return p_vec4_add(left, right); }
static inline pMat2 operator+(pMat2 left, pMat2 right) { return p_mat2_add(left, right); }
static inline pMat3 operator+(pMat3 left, pMat3 right) { return p_mat3_add(left, right); }
static inline pMat4 operator+(pMat4 left, pMat4 right) { return p_mat4_add(left, right); }
static inline pQuat operator+(pQuat left, pQuat right) { return p_quat_add(left, right); }

static inline pVec2 operator-(pVec2 left, pVec2 right) { return p_vec2_sub(left, right); }
static inline pVec3 operator-(pVec3 left, pVec3 right) { return p_vec3_sub(left, right); }
static inline pVec4 operator-(pVec4 left, pVec4 right) { return p_vec4_sub(left, right); }
static inline pMat2 operator-(pMat2 left, pMat2 right) { return p_mat2_sub(left, right); }
static inline pMat3 operator-(pMat3 left, pMat3 right) { return p_mat3_sub(left, right); }
static inline pMat4 operator-(pMat4 left, pMat4 right) { return p_mat4_sub(left, right); }
static inline pQuat operator-(pQuat left, pQuat right) { return p_quat_sub(left, right); }

static inline pVec2 operator*(pVec2 left, pVec2 right) { return p_vec2_mul(left, right); }
static inline pVec3 operator*(pVec3 left, pVec3 right) { return p_vec3_mul(left, right); }
static inline pVec4 operator*(pVec4 left, pVec4 right) { return p_vec4_mul(left, right); }
static inline pMat2 operator*(pMat2 left, pMat2 right) { return p_mat2_mul(left, right); }
static inline pMat3 operator*(pMat3 left, pMat3 right) { return p_mat3_mul(left, right); }
static inline pMat4 operator*(pMat4 left, pMat4 right) { return p_mat4_mul(left, right); }
static inline pQuat operator*(pQuat left, pQuat right) { return p_quat_mul(left, right); }
static inline pVec2 operator*(pVec2 left, float right) { return p_vec2_mul_f(left, right); }
static inline pVec3 operator*(pVec3 left, float right) { return p_vec3_mul_f(left, right); }
static inline pVec4 operator*(pVec4 left, float right) { return p_vec4_mul_f(left, right); }
static inline pMat2 operator*(pMat2 left, float right) { return p_mat2_mul_f(left, right); }
static inline pMat3 operator*(pMat3 left, float right) { return p_mat3_mul_f(left, right); }
static inline pMat4 operator*(pMat4 left, float right) { return p_mat4_mul_f(left, right); }
static inline pQuat operator*(pQuat left, float right) { return p_quat_mul_f(left, right); }
static inline pVec2 operator*(float left, pVec2 right) { return p_vec2_mul_f(right, left); }
static inline pVec3 operator*(float left, pVec3 right) { return p_vec3_mul_f(right, left); }
static inline pVec4 operator*(float left, pVec4 right) { return p_vec4_mul_f(right, left); }
static inline pMat2 operator*(float left, pMat2 right) { return p_mat2_mul_f(right, left); }
static inline pMat3 operator*(float left, pMat3 right) { return p_mat3_mul_f(right, left); }
static inline pMat4 operator*(float left, pMat4 right) { return p_mat4_mul_f(right, left); }
static inline pQuat operator*(float left, pQuat right) { return p_quat_mul_f(right, left); }
static inline pVec2 operator*(pMat2 matrix, pVec2 v) { return p_mat2_mul_vec2(matrix, v); }
static inline pVec3 operator*(pMat3 matrix, pVec3 v) { return p_mat3_mul_v(matrix, v); }
static inline pVec4 operator*(pMat4 matrix, pVec4 v) { return p_mat4_mul_vec4(matrix, v); }

static inline pVec2 operator/(pVec2 left, pVec2 right) { return p_vec2_div(left, right); }
static inline pVec3 operator/(pVec3 left, pVec3 right) { return p_vec3_div(left, right); }
static inline pVec4 operator/(pVec4 left, pVec4 right) { return p_vec4_div(left, right); }
static inline pVec2 operator/(pVec2 left, float right) { return p_vec2_div_f(left, right); }
static inline pVec3 operator/(pVec3 left, float right) { return p_vec3_div_f(left, right); }
static inline pVec4 operator/(pVec4 left, float right) { return p_vec4_div_f(left, right); }
static inline pMat4 operator/(pMat4 left, float right) { return p_mat4_div_f(left, right); }
static inline pMat3 operator/(pMat3 left, float right) { return p_mat3_div_f(left, right); }
static inline pMat2 operator/(pMat2 left, float right) { return p_mat2_div_f(left, right); }
static inline pQuat operator/(pQuat left, float right) { return p_quat_div_f(left, right); }

static inline pVec2 &operator+=(pVec2 &left, pVec2 right) { return left = left + right; }
static inline pVec3 &operator+=(pVec3 &left, pVec3 right) { return left = left + right; }
static inline pVec4 &operator+=(pVec4 &left, pVec4 right) { return left = left + right; }
static inline pMat2 &operator+=(pMat2 &left, pMat2 right) { return left = left + right; }
static inline pMat3 &operator+=(pMat3 &left, pMat3 right) { return left = left + right; }
static inline pMat4 &operator+=(pMat4 &left, pMat4 right) { return left = left + right; }
static inline pQuat &operator+=(pQuat &left, pQuat right) { return left = left + right; }

static inline pVec2 &operator-=(pVec2 &left, pVec2 right) { return left = left - right; }
static inline pVec3 &operator-=(pVec3 &left, pVec3 right) { return left = left - right; }
static inline pVec4 &operator-=(pVec4 &left, pVec4 right) { return left = left - right; }
static inline pMat2 &operator-=(pMat2 &left, pMat2 right) { return left = left - right; }
static inline pMat3 &operator-=(pMat3 &left, pMat3 right) { return left = left - right; }
static inline pMat4 &operator-=(pMat4 &left, pMat4 right) { return left = left - right; }
static inline pQuat &operator-=(pQuat &left, pQuat right) { return left = left - right; }

static inline pVec2 &operator*=(pVec2 &left, pVec2 right) { return left = left * right; }
static inline pVec3 &operator*=(pVec3 &left, pVec3 right) { return left = left * right; }
static inline pVec4 &operator*=(pVec4 &left, pVec4 right) { return left = left * right; }
static inline pVec2 &operator*=(pVec2 &left, float right) { return left = left * right; }
static inline pVec3 &operator*=(pVec3 &left, float right) { return left = left * right; }
static inline pVec4 &operator*=(pVec4 &left, float right) { return left = left * right; }
static inline pMat2 &operator*=(pMat2 &left, float right) { return left = left * right; }
static inline pMat3 &operator*=(pMat3 &left, float right) { return left = left * right; }
static inline pMat4 &operator*=(pMat4 &left, float right) { return left = left * right; }
static inline pQuat &operator*=(pQuat &left, float right) { return left = left * right; }

static inline pVec2 &operator/=(pVec2 &left, pVec2 right) { return left = left / right; }
static inline pVec3 &operator/=(pVec3 &left, pVec3 right) { return left = left / right; }
static inline pVec4 &operator/=(pVec4 &left, pVec4 right) { return left = left / right; }
static inline pVec2 &operator/=(pVec2 &left, float right) { return left = left / right; }
static inline pVec3 &operator/=(pVec3 &left, float right) { return left = left / right; }
static inline pVec4 &operator/=(pVec4 &left, float right) { return left = left / right; }
static inline pMat4 &operator/=(pMat4 &left, float right) { return left = left / right; }
static inline pQuat &operator/=(pQuat &left, float right) { return left = left / right; }

static inline bool operator==(pVec2 left, pVec2 right) { return p_vec2_eq(left, right); }
static inline bool operator==(pVec3 left, pVec3 right) { return p_vec3_eq(left, right); }
static inline bool operator==(pVec4 left, pVec4 right) { return p_vec4_eq(left, right); }

static inline bool operator!=(pVec2 left, pVec2 right) { return !p_vec2_eq(left, right); }
static inline bool operator!=(pVec3 left, pVec3 right) { return !p_vec3_eq(left, right); }
static inline bool operator!=(pVec4 left, pVec4 right) { return !p_vec4_eq(left, right); }

static inline pVec2 operator-(pVec2 in) {
    pVec2 result;
    result.x = -in.x;
    result.y = -in.y;
    return result;
}

static inline pVec3 operator-(pVec3 in) {
    pVec3 result;
    result.x = -in.x;
    result.y = -in.y;
    result.z = -in.z;
    return result;
}

static inline pVec4 operator-(pVec4 in) {
    pVec4 result;
#if P_MATH__USE_SSE
    result.sse = _mm_xor_ps(in.sse, _mm_set1_ps(-0.0f));
#elif defined(P_MATH__USE_NEON)
    float32x4_t Zero = vdupq_n_f32(0.0f);
    result.neon = vsubq_f32(Zero, in.neon);
#else
    result.x = -in.x;
    result.y = -in.y;
    result.z = -in.z;
    result.w = -in.w;
#endif
    return result;
}

#endif /* __cplusplus*/

#ifdef P_MATH__USE_C11_GENERICS

void __hmm_invalid_generic();

#define p_add(a, b) _Generic((a), \
    pVec2: p_vec2_add, \
    pVec3: p_vec3_add, \
    pVec4: p_vec4_add, \
    pMat2: p_mat2_add, \
    pMat3: p_mat3_add, \
    pMat4: p_mat4_add, \
    pQuat: p_quat_add  \
)(a, b)

#define p_sub(a, b) _Generic((a), \
    pVec2: p_vec2_sub, \
    pVec3: p_vec3_sub, \
    pVec4: p_vec4_sub, \
    pMat2: p_mat2_sub, \
    pMat3: p_mat3_sub, \
    pMat4: p_mat4_sub, \
    pQuat: p_quat_sub  \
)(a, b)

#define p_mul(a, b) _Generic((b), \
    float: _Generic((a), \
        pVec2: p_vec2_mul_f, \
        pVec3: p_vec3_mul_f, \
        pVec4: p_vec4_mul_f, \
        pMat2: p_mat2_mul_f, \
        pMat3: p_mat3_mul_f, \
        pMat4: p_mat4_mul_f, \
        pQuat: p_quat_mul_f, \
        default: __hmm_invalid_generic \
    ), \
    pVec2: _Generic((a), \
        pVec2: p_vec2_mul,      \
        pMat2: p_mat2_mul_vec2, \
        default: __hmm_invalid_generic \
    ), \
    pVec3: _Generic((a), \
        pVec3: p_vec3_mul,   \
        pMat3: p_mat3_mul_v, \
        default: __hmm_invalid_generic \
    ), \
    pVec4: _Generic((a), \
        pVec4: p_vec4_mul,      \
        pMat4: p_mat4_mul_vec4, \
        default: __hmm_invalid_generic \
    ), \
    pMat2: p_mat2_mul, \
    pMat3: p_mat3_mul, \
    pMat4: p_mat4_mul, \
    pQuat: p_quat_mul  \
)(a, b)

#define p_div(a, b) _Generic((b), \
    float: _Generic((a), \
        pVec2: p_vec2_div_f, \
        pVec3: p_vec3_div_f, \
        pVec4: p_vec4_div_f, \
        pMat2: p_mat2_div_f, \
        pMat3: p_mat3_div_f, \
        pMat4: p_mat4_div_f, \
        pQuat: p_quat_div_f  \
    ), \
    pVec2: p_vec2_div, \
    pVec3: p_vec3_div, \
    pVec4: p_vec4_div  \
)(a, b)

#define p_len(a) _Generic((a), \
    pVec2: p_vec2_len, \
    pVec3: p_vec3_len, \
    pVec4: p_vec4_len  \
)(a)

#define p_len_sqr(a) _Generic((a), \
    pVec2: p_vec2_len_sqr, \
    pVec3: p_vec3_len_sqr, \
    pVec4: p_vec4_len_sqr  \
)(a)

#define p_norm(a) _Generic((a), \
    pVec2: p_vec2_norm, \
    pVec3: p_vec3_norm, \
    pVec4: p_vec4_norm, \
    pQuat: p_quat_norm  \
)(a)

#define p_dot(a, b) _Generic((a), \
    pVec2: p_vec2_dot, \
    pVec3: p_vec3_dot, \
    pVec4: p_vec4_dot, \
    pQuat: p_quat_dot  \
)(a, b)

#define p_lerp(a, T, b) _Generic((a), \
    float: p_lerp, \
    pVec2: p_lerp_vec2, \
    pVec3: p_lerp_vec3, \
    pVec4: p_lerp_vec4  \
)(a, T, b)

#define p_eq(a, b) _Generic((a), \
    pVec2: p_vec2_eq, \
    pVec3: p_vec3_eq, \
    pVec4: p_vec4_eq  \
)(a, b)

#define p_transpose(m) _Generic((m), \
    pMat2: p_mat2_transpose, \
    pMat3: p_mat3_transpose, \
    pMat4: p_mat4_transpose  \
)(m)

#define p_determinant(m) _Generic((m), \
    pMat2: p_mat2_determinant, \
    pMat3: p_mat3_determinant, \
    pMat4: p_mat4_determinant  \
)(m)

#define p_inv_general(m) _Generic((m), \
    pMat2: p_mat2_inv_general, \
    pMat3: p_mat3_inv_general, \
    pMat4: p_mat4_inv_general  \
)(m)

#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif /* P_MATH_H_HEADER_GUARD */