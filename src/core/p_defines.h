#ifndef P_DEFINES_HEADER_GUARD
#define P_DEFINES_HEADER_GUARD

#include <stdint.h>

#define P_COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

#define P_KILOBYTES(x) (1024*(x))
#define P_MEGABYTES(x) (1024 * P_KILOBYTES(x))
#define P_GIGABYTES(x) (1024 * P_MEGABYTES(x))

#define P_MAX(a, b) ((a)>=(b)?(a):(b))
#define P_MIN(a, b) ((a)<=(b)?(a):(b))
#define P_CLAMP(x, a, b) ((x)<(a)?(a) : (x)>(b)?(b) : (x))

#define P_OFFSET_OF(struct_name, member_name) \
    ((size_t)((char *)&((struct_name *)0)->member_name - (char *)0))

#if !defined(P_INLINE)
	#if defined(_MSC_VER)
		#define P_INLINE __forceinline
	#else
		#define P_INLINE __attribute__ ((__always_inline__)) inline
	#endif
#endif

#ifndef P_DEFAULT_MEMORY_ALIGNMENT
    #if defined(__PSP__)
        #define P_DEFAULT_MEMORY_ALIGNMENT (sizeof(void*))
    #else
        #define P_DEFAULT_MEMORY_ALIGNMENT (2 * sizeof(void *))
    #endif
#endif

#ifndef P_ENDIAN_ORDER
#define P_ENDIAN_ORDER
    #define P_IS_BIG_ENDIAN (!*(uint8_t*)&(uint16_t){1})
    #define P_IS_LITTLE_ENDIAN (!P_IS_BIG_ENDIAN)
#endif

#endif // P_DEFINES_HEADER_GUARD
