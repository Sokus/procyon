#ifndef PE_CORE_H
#define PE_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifndef PE_INLINE
	#ifdef _MSC_VER
		#if _MSC_VER < 1300
		#define PE_INLINE
		#else
		#define PE_INLINE __forceinline
		#endif
	#else
		#define PE_INLINE __attribute__ ((__always_inline__)) inline
	#endif
#endif


#define PE_DEBUG_BREAK()  \
	asm(".set noreorder\n"\
		"break \n"        \
		"jr    $31\n"     \
		"nop\n");
#define PE_ASSERT(check, ...)\
	if (!(check)) {          \
		/* TODO: Logging */  \
		PE_DEBUG_BREAK();    \
	}


typedef enum peAllocationType {
	peAllocation_Alloc,
	peAllocation_Free,
	peAllocation_FreeAll,
	peAllocation_Resize,
} peAllocationType;

typedef enum peAllocatorFlag {
	peAllocatorFlag_ClearToZero = 1 << 0,
} peAllocatorFlag;

typedef void *peAllocatorProc(
    void *allocator_data, peAllocationType type,
    size_t size, size_t alignment,
    void *old_memory, size_t old_size,
    peAllocatorFlag flags);

typedef struct peAllocator {
	peAllocatorProc *proc;
	void *         data;
} peAllocator;

// NOTE: Temporary
#define PE_DEFAULT_MEMORY_ALIGNMENT (sizeof(void *)) // PSP

#ifndef PE_DEFAULT_MEMORY_ALIGNMENT
#define PE_DEFAULT_MEMORY_ALIGNMENT (2 * sizeof(void *))
#endif

#ifndef PE_DEFAULT_ALLOCATOR_FLAGS
#define PE_DEFAULT_ALLOCATOR_FLAGS (peAllocatorFlag_ClearToZero)
#endif

void *pe_alloc_align (peAllocator a, size_t size, size_t aligmnent);
void *pe_alloc       (peAllocator a, size_t size);
void  pe_free        (peAllocator a, void *ptr);
void  pe_free_all    (peAllocator a);
//void *pe_resize_align(peAllocator a, void *ptr, size_t old_size, size_t new_size, size_t alignment);
//void *pe_resize      (peAllocator a, void *ptr, size_t old_size, size_t new_size);

#ifndef PE_ALLOC_ITEM
#define PE_ALLOC_ITEM(allocator, Type)         (Type *)pe_alloc(allocator, sizeof(Type))
#define PE_ALLOC_ARRAY(allocator, Type, count) (Type *)pe_alloc(allocator, sizeof(Type) * (count))
#endif

peAllocator pe_heap_allocator(void);

#ifndef PE_MALLOC
#define PE_MALLOC(sz) pe_alloc(pe_heap_allocator(), sz)
#define PE_FREE(ptr) pe_free(pe_heap_allocator(), ptr)
#endif

typedef struct peArena {
    peAllocator backing;
    void * physical_start;
    size_t total_size;
    size_t total_allocated;
} peArena;

void pe_arena_init_from_memory   (peArena *arena, void *start, size_t size);
void pe_arena_init_from_allocator(peArena *arena, peAllocator backing, size_t size);
void pe_arena_init_sub           (peArena *arena, peArena *parent_arena, size_t size);
void pe_arena_free               (peArena *arena);

size_t pe_arena_alignment_offset(peArena *arena, size_t alignment);
size_t pe_arena_size_remaining(peArena *arena, size_t alignment);

peAllocator pe_arena_allocator(peArena *arena);

void pe_zero_size(void *ptr, size_t size);
bool pe_is_power_of_two(uintptr_t x);

#endif // PE_CORE_H

