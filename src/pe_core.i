#include "pe_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

PE_INLINE void *pe_heap_alloc_align(size_t size, size_t alignment) {
    void *result = NULL;
#if defined(_WIN32)
    result = _aligned_malloc(size, alignment);
#elif defined(__linux__)
     posix_memalign(&result, alignment, size);
#elif defined(PSP)
    result = memalign(alignment, size);
#endif
    return result;
}

PE_INLINE void *pe_heap_alloc(size_t size) {
    return pe_heap_alloc_align(size, PE_DEFAULT_MEMORY_ALIGNMENT);
}

PE_INLINE void pe_heap_free(void *ptr) {
#if defined(_WIN32)
    _aligned_free(ptr);
#elif defined(PSP) || defined(__linux__)
    free(ptr);
#endif
}

PE_INLINE void pe_arena_init(peArena *arena, void *start, size_t size) {
    arena->physical_start = start;
    arena->total_size = size;
    arena->total_allocated = 0;
    arena->temp_count = 0;
}

PE_INLINE void pe_arena_init_sub_align(peArena *arena, peArena *parent_arena, size_t size, size_t alignment) {
    void *start = pe_arena_alloc_align(parent_arena, size, alignment);
    pe_arena_init(arena, start, size);
}

PE_INLINE void pe_arena_init_sub(peArena *arena, peArena *parent_arena, size_t size) {
    void *start = pe_arena_alloc_align(parent_arena, size, PE_DEFAULT_MEMORY_ALIGNMENT);
    pe_arena_init(arena, start, size);
}

PE_INLINE size_t pe_arena_alignment_offset(peArena *arena, size_t alignment) {
    PE_ASSERT(pe_is_power_of_two(alignment));
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

PE_INLINE void *pe_arena_alloc_align(peArena *arena, size_t size, size_t alignment) {
    size_t alignment_offset = pe_arena_alignment_offset(arena, alignment);
    size_t allocation_size = size + alignment_offset;
    if (arena->total_allocated + allocation_size > arena->total_size) {
        fprintf(stderr, "Arena out of memory\n");
        return NULL;
    }
    uintptr_t result_offset = (uintptr_t)arena->total_allocated + (uintptr_t)alignment_offset;
    void *result = (void *)((uintptr_t)arena->physical_start + result_offset);
    arena->total_allocated += allocation_size;
    return result;
}

PE_INLINE void *pe_arena_alloc(peArena *arena, size_t size) {
    void *result = pe_arena_alloc_align(arena, size, PE_DEFAULT_MEMORY_ALIGNMENT);
    return result;
}

PE_INLINE void pe_arena_clear(peArena *arena) {
    arena->total_allocated = 0;
}

PE_INLINE void pe_arena_rewind_to_pointer(peArena *arena, void *pointer) {
    PE_ASSERT((uintptr_t)pointer >= (uintptr_t)arena->physical_start);
    PE_ASSERT((uintptr_t)pointer < (uintptr_t)arena->physical_start + arena->total_allocated);
    arena->total_allocated = (size_t)((uintptr_t)pointer - (uintptr_t)arena->physical_start);
}

PE_INLINE peArenaTemp pe_arena_temp_begin(peArena *arena) {
    peArenaTemp temp_arena_memory;
    temp_arena_memory.arena = arena;
    temp_arena_memory.original_count = arena->total_allocated;
    arena->temp_count += 1;
    return temp_arena_memory;
}

PE_INLINE void pe_arena_temp_end  (peArenaTemp tmp) {
    PE_ASSERT_MSG(tmp.arena->total_allocated >= tmp.original_count,
                  "%zu >= %zu", tmp.arena->total_allocated, tmp.original_count);
    PE_ASSERT(tmp.arena->temp_count > 0);
    tmp.arena->total_allocated = tmp.original_count;
    tmp.arena->temp_count -= 1;
}

PE_INLINE void pe_zero_size(void *ptr, size_t size) {
    memset(ptr, 0, size);
}

PE_INLINE bool pe_is_power_of_two(uintptr_t x) {
	return (x & (x-1)) == 0;
}