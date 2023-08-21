#ifndef PE_CORE_H
#define PE_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

// #define PE_INLINE
// TODO: Functions that are force inlined are not present
// in the .dlls causing link errors. (tested on windows at least)
// Maybe compile shared code into objs and link those instead of a dll.

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

#define PE_COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

#ifndef PE_DEBUG_TRAP
	#if defined(PSP)
		#define PE_DEBUG_TRAP() \
			asm(".set push\n" \
				".set noreorder\n" \
				"break\n" \
				"nop\n" \
				"jr $31\n" \
				"nop\n" \
				".set pop\n")
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

#ifndef PE_ASSERT_MSG
#define PE_ASSERT_MSG(cond, msg, ...) do { \
	if (!(cond)) { \
		pe_assert_handler("Assertion Failure", #cond, __FILE__, (int)__LINE__, msg, ##__VA_ARGS__); \
		PE_DEBUG_TRAP(); \
	} \
} while (0)
#endif

#ifndef PE_ASSERT
#define PE_ASSERT(cond) PE_ASSERT_MSG(cond, NULL)
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

#define PE_KILOBYTES(x) (1024*(x))
#define PE_MEGABYTES(x) (1024 * PE_KILOBYTES(x))
#define PE_GIGABYTES(x) (1024 * PE_MEGABYTES(x))

#define PE_MAX(a, b) ((a)>=(b)?(a):(b))
#define PE_MIN(a, b) ((a)<=(b)?(a):(b))

typedef enum peAllocationType {
	peAllocation_Alloc,
	peAllocation_Free,
	peAllocation_FreeAll,
	peAllocation_Resize,
} peAllocationType;

typedef void *peAllocatorProc(
    void *allocator_data, peAllocationType type,
    size_t size, size_t alignment,
    void *old_memory, size_t old_size
);

typedef struct peAllocator {
	peAllocatorProc *proc;
	void *data;
} peAllocator;

// NOTE: Temporary
#define PE_DEFAULT_MEMORY_ALIGNMENT (sizeof(void *)) // PSP

#ifndef PE_DEFAULT_MEMORY_ALIGNMENT
#define PE_DEFAULT_MEMORY_ALIGNMENT (2 * sizeof(void *))
#endif

void *pe_alloc_align (peAllocator a, size_t size, size_t aligmnent);
void *pe_alloc       (peAllocator a, size_t size);
void  pe_free        (peAllocator a, void *ptr);
void  pe_free_all    (peAllocator a);
void *pe_resize_align(peAllocator a, void *ptr, size_t old_size, size_t new_size, size_t alignment);
void *pe_resize      (peAllocator a, void *ptr, size_t old_size, size_t new_size);

#ifndef PE_ALLOC_ITEM
#define PE_ALLOC_ITEM(allocator, Type)         (Type *)pe_alloc(allocator, sizeof(Type))
#define PE_ALLOC_ARRAY(allocator, Type, count) (Type *)pe_alloc(allocator, sizeof(Type) * (count))
#endif

peAllocator pe_heap_allocator(void);

typedef struct peMeasureAllocatorData {
	size_t alignment;
	size_t total_allocated;
} peMeasureAllocatorData;
peAllocator pe_measure_allocator(peMeasureAllocatorData *data);

#ifndef PE_MALLOC
#define PE_MALLOC(sz) pe_alloc(pe_heap_allocator(), sz)
#define PE_FREE(ptr) pe_free(pe_heap_allocator(), ptr)
#endif

typedef struct peArena {
    peAllocator backing;
    void * physical_start;
    size_t total_size;
    size_t total_allocated;
	size_t temp_count;
} peArena;

void pe_arena_init_from_memory         (peArena *arena, void *start, size_t size);
void pe_arena_init_from_allocator_align(peArena *arena, peAllocator backing, size_t size, size_t alignment);
void pe_arena_init_from_allocator      (peArena *arena, peAllocator backing, size_t size);
void pe_arena_init_sub                 (peArena *arena, peArena *parent_arena, size_t size);
void pe_arena_free                     (peArena *arena);
void pe_arena_rewind_to_pointer        (peArena *arena, void *pointer);

size_t pe_arena_alignment_offset(peArena *arena, size_t alignment);
size_t pe_arena_size_remaining(peArena *arena, size_t alignment);

peAllocator pe_arena_allocator(peArena *arena);

typedef struct peTempArenaMemory {
	peArena *arena;
	size_t original_count;
} peTempArenaMemory;

peTempArenaMemory pe_temp_arena_memory_begin(peArena *arena);
void              pe_temp_arena_memory_end  (peTempArenaMemory temp_arena_memory);

void pe_zero_size(void *ptr, size_t size);
bool pe_is_power_of_two(uintptr_t x);

#if defined(_MSC_VER)
#include "pe_core.i"
#endif

#endif // PE_CORE_H

