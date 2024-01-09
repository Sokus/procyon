#ifndef PE_CORE_H
#define PE_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#if !defined(PE_INLINE)
	#if defined(_MSC_VER)
		#if _MSC_VER < 1300
		#define PE_INLINE
		#else
		#define PE_INLINE __forceinline
		#endif
	#else
		#define PE_INLINE __attribute__ ((__always_inline__)) inline
	#endif
#endif

#if defined(__386__) || defined(i386)    || defined(__i386__)  || \
    defined(__X86)   || defined(_M_IX86)                       || \
    defined(_M_X64)  || defined(__x86_64__)                    || \
    defined(alpha)   || defined(__alpha) || defined(__alpha__) || \
    defined(_M_ALPHA)                                          || \
    defined(ARM)     || defined(_ARM)    || defined(__arm__)   || \
    defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) || \
    defined(_WIN32_WCE) || defined(__NT__)                     || \
    defined(__MIPSEL__)
  #define PE_ARCH_LITTLE_ENDIAN 1
#else
  #define PE_ARCH_BIG_ENDIAN 1
#endif

void pe_assert_handler(char *prefix, char *condition, char *file, int line, char *msg, ...);
void pe_debug_trap(void);

#ifndef PE_DEBUG_TRAP
	#if defined(PSP)
		#define PE_DEBUG_TRAP() pe_debug_trap()
	#elif defined(_MSC_VER)
	 	#if _MSC_VER < 1300
		#define PE_DEBUG_TRAP() __asm int 3 /* Trap to debugger! */
		#else
		#define PE_DEBUG_TRAP() __debugbreak()
		#endif
	#else
		#define PE_DEBUG_TRAP() __builtin_trap()
	#endif
#endif

#if !defined(PE_ASSERT_MSG) && !defined(NDEBUG)
#define PE_ASSERT_MSG(cond, msg, ...) \
do { \
	if (!(cond)) { \
		pe_assert_handler("Assertion Failure", #cond, __FILE__, (int)__LINE__, msg, ##__VA_ARGS__); \
		PE_DEBUG_TRAP(); \
	} \
} while (0)
#elif !defined(PE_ASSERT_MSG) && defined(NDEBUG)
#define PE_ASSERT_MSG(cond, msg, ...) do {} while (0)
#endif

#if !defined(PE_ASSERT)
#define PE_ASSERT(cond) PE_ASSERT_MSG(cond, NULL)
#endif

#if !defined(PE_ALWAYS) && !defined(NDEBUG)
#define PE_ALWAYS(cond) ((cond) ? 1 : (pe_assert_handler("Assertion Failure", #cond, __FILE__, (int)__LINE__, NULL), (void)PE_DEBUG_TRAP(), 0))
#elif !defined(PE_ALWAYS) && defined(NDEBUG)
#define PE_ALWAYS(cond) (cond)
#endif

#ifndef PE_PANIC_MSG
#define PE_PANIC_MSG(msg, ...) do { \
	pe_assert_handler("Panic", NULL, __FILE__, (int)__LINE__, msg, ##__VA_ARGS__); \
	PE_DEBUG_TRAP(); \
} while (0)
#endif

#ifndef PE_PANIC
#define PE_PANIC() do { \
	pe_assert_handler("Panic", NULL, __FILE__, (int)__LINE__, NULL); \
	PE_DEBUG_TRAP(); \
} while (0)
#endif

#ifndef PE_UNIMPLEMENTED
#define PE_UNIMPLEMENTED() do { \
	pe_assert_handler("Unimplemented", NULL, __FILE__, (int)__LINE__, NULL); \
	PE_DEBUG_TRAP(); \
} while(0)
#endif

////////////////////////////////////////////////////////////////////////////////

#define PE_COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

#define PE_KILOBYTES(x) (1024*(x))
#define PE_MEGABYTES(x) (1024 * PE_KILOBYTES(x))
#define PE_GIGABYTES(x) (1024 * PE_MEGABYTES(x))

#define PE_MAX(a, b) ((a)>=(b)?(a):(b))
#define PE_MIN(a, b) ((a)<=(b)?(a):(b))
#define PE_CLAMP(x, a, b) ((x)<(a)?(a) : (x)>(b)?(b) : (x))

#ifndef PE_DEFAULT_MEMORY_ALIGNMENT
#define PE_DEFAULT_MEMORY_ALIGNMENT (2 * sizeof(void *))
#endif

////////////////////////////////////////////////////////////////////////////////

void *pe_heap_alloc_align(size_t size, size_t alignment);
void *pe_heap_alloc      (size_t size);
void  pe_heap_free       (void *ptr);

void pe_zero_size(void *ptr, size_t size);
bool pe_is_power_of_two(uintptr_t x);

////////////////////////////////////////////////////////////////////////////////

typedef struct peArena {
    void * physical_start;
    size_t total_size;
    size_t total_allocated;
	size_t temp_count;
} peArena;

typedef struct peArenaTemp {
	peArena *arena;
	size_t original_count;
} peArenaTemp;

void pe_arena_init(peArena *arena, void *start, size_t size);
void pe_arena_init_sub_align(peArena *arena, peArena *parent_arena, size_t size, size_t alignment);
void pe_arena_init_sub(peArena *arena, peArena *parent_arena, size_t size);

size_t pe_arena_alignment_offset(peArena *arena, size_t alignment);
size_t pe_arena_size_remaining(peArena *arena, size_t alignment);

void *pe_arena_alloc_align(peArena *arena, size_t size, size_t alignment);
void *pe_arena_alloc(peArena *arena, size_t size);
void  pe_arena_clear(peArena *arena);

void pe_arena_rewind(peArena *arena, size_t size);
void pe_arena_rewind_to_pointer(peArena *arena, void *pointer);

peArenaTemp pe_arena_temp_begin(peArena *arena);
void pe_arena_temp_end(peArenaTemp temp_arena_memory);

////////////////////////////////////////////////////////////////////////////////

typedef struct peString {
    char *data;
    size_t size;
} peString;

#define PE_STRING_LITERAL(string_literal) (peString){ .data = (string_literal), .size = sizeof((string_literal))-1 }
#define PE_STRING_EXPAND(string) (int)((string).size), ((string).data)

peString pe_string(char *data, size_t size);
peString pe_string_from_range(char *first, char *one_past_last);
peString pe_string_from_cstring(char *cstring);

peString pe_string_prefix(peString string, size_t size);
peString pe_string_chop  (peString string, size_t size);
peString pe_string_suffix(peString string, size_t size);
peString pe_string_skip  (peString string, size_t size);
peString pe_string_range (peString string, size_t first, size_t one_past_last);

peString pe_string_format_variadic(peArena *arena, char *format, va_list argument_list);
peString pe_string_format         (peArena *arena, char *format, ...);

////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER)
#include "pe_core.i"
#endif

#endif // PE_CORE_H

