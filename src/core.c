#include "core.h"



PE_INLINE void *pe_alloc_align(peAllocator a, size_t size, size_t alignment) {
    return a.proc(a.data, peAllocation_Alloc, size, alignment, NULL, 0, PE_DEFAULT_ALLOCATOR_FLAGS);
}

PE_INLINE void *pe_alloc(peAllocator a, size_t size) {
    return pe_alloc_align(a, size, PE_DEFAULT_MEMORY_ALIGNMENT);
}

PE_INLINE void pe_free(peAllocator a, void *ptr) {
    if (ptr != NULL) {
        a.proc(a.data, peAllocation_Free, 0, 0, ptr, 0, PE_DEFAULT_ALLOCATOR_FLAGS);
    }
}

PE_INLINE void pe_free_all(peAllocator a) {
    a.proc(a.data, peAllocation_FreeAll, 0, 0, NULL, 0, PE_DEFAULT_ALLOCATOR_FLAGS);
}

#include <malloc.h> // PSP

#include <string.h>

static void *pe_heap_allocator_proc(void *allocator_data, peAllocationType type,
size_t size, size_t alignment, void *old_memory, size_t old_size, peAllocatorFlag flags) {
    void *ptr = NULL;
    switch (type) {
        case peAllocation_Alloc: {
            ptr = memalign(alignment, size);
            if (flags & peAllocatorFlag_ClearToZero) {
                pe_zero_size(ptr, size);
            }
        } break;

        case peAllocation_Free: {
            free(old_memory);
        } break;

        case peAllocation_Resize: {
            // TODO: Implementation
        }

        default: break;
    }
    return ptr;
}

PE_INLINE peAllocator pe_heap_allocator(void) {
    peAllocator a;
    a.proc = pe_heap_allocator_proc;
    a.data = NULL;
    return a;
}

PE_INLINE void pe_zero_size(void *ptr, size_t size) {
    memset(ptr, 0, size);
}

PE_INLINE bool pe_is_power_of_two(uintptr_t x) {
	return (x & (x-1)) == 0;
}

PE_INLINE void pe_arena_init_from_memory(peArena *arena, void *start, size_t size) {
    arena->backing.proc = NULL;
    arena->backing.data = NULL;
    arena->physical_start = start;
    arena->total_size = size;
    arena->total_allocated = 0;
}

PE_INLINE void pe_arena_init_from_allocator(peArena *arena, peAllocator backing, size_t size) {
    arena->backing = backing;
    arena->physical_start = pe_alloc(backing, size);
    arena->total_size = size;
    arena->total_allocated = 0;
}

PE_INLINE void pe_arena_init_sub(peArena *arena, peArena *parent_arena, size_t size) {
    pe_arena_init_from_allocator(arena, pe_arena_allocator(parent_arena), size);
}

PE_INLINE void pe_arena_free(peArena *arena) {
    if (arena->backing.proc) {
        pe_free(arena->backing, arena->physical_start);
        arena->physical_start = NULL;
    }
}

PE_INLINE size_t pe_arena_alignment_offset(peArena *arena, size_t alignment) {
    PE_ASSERT(pe_is_power_of_two(alignment))
    size_t alignment_offset = 0;
    uintptr_t result_pointer = (uintptr_t)arena->physical_start + arena->total_allocated;
    uintptr_t mask = alignment - 1;
    if (result_pointer & mask) {
        alignment_offset = alignment - (size_t)(result_pointer & mask);
    }
    return alignment_offset;
}

PE_INLINE size_t pe_arena_size_remaining(peArena *arena, size_t alignment) {
    size_t unusable_size = arena->total_allocated + pe_arena_alignment_offset(arena, alignment);
    size_t result = arena->total_size - unusable_size;
    return result;
}

static void *pe_arena_allocator_proc(void *allocator_data, peAllocationType type,
size_t size, size_t alignment, void *old_memory, size_t old_size, peAllocatorFlag flags) {
    peArena *arena = (peArena *)allocator_data;
    void *ptr = NULL;
    switch (type) {
        case peAllocation_Alloc: {
            size_t alignment_offset = pe_arena_alignment_offset(arena, alignment);
            size_t allocation_size = size + alignment_offset;
            if (arena->total_allocated + allocation_size > arena->total_size) {
                // TODO: Print error
                return NULL;
            }
            uintptr_t result_offset = (uintptr_t)arena->total_allocated + (uintptr_t)alignment_offset;
            ptr = arena->physical_start + result_offset;
            arena->total_allocated += allocation_size;
            if (flags & peAllocatorFlag_ClearToZero) {
                pe_zero_size(ptr, size);
            }
        } break;

        case peAllocation_FreeAll: {
            arena->total_allocated = 0;
        } break;

        case peAllocation_Resize: {
            // TODO: Implementation
        } break;

        default: break;
    }
    return ptr;
}

PE_INLINE peAllocator pe_arena_allocator(peArena *arena) {
    peAllocator allocator;
    allocator.proc = pe_arena_allocator_proc;
    allocator.data = arena;
    return allocator;
}