#ifndef P_ARENA_HEADER_GUARD
#define P_ARENA_HEADER_GUARD

#include "p_defines.h"
#include "p_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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

static P_INLINE void pe_arena_init(peArena *arena, void *start, size_t size);
static P_INLINE void pe_arena_init_sub_align(peArena *arena, peArena *parent_arena, size_t size, size_t alignment);
static P_INLINE void pe_arena_init_sub(peArena *arena, peArena *parent_arena, size_t size);

static P_INLINE size_t pe_arena_alignment_offset(peArena *arena, size_t alignment);
static P_INLINE size_t pe_arena_size_remaining(peArena *arena, size_t alignment);

static P_INLINE void *pe_arena_alloc_align(peArena *arena, size_t size, size_t alignment);
static P_INLINE void *pe_arena_alloc(peArena *arena, size_t size);
static P_INLINE void  pe_arena_clear(peArena *arena);

static P_INLINE void pe_arena_rewind(peArena *arena, size_t size);
static P_INLINE void pe_arena_rewind_to_pointer(peArena *arena, void *pointer);

static P_INLINE peArenaTemp pe_arena_temp_begin(peArena *arena);
static P_INLINE void pe_arena_temp_end(peArenaTemp temp_arena_memory);

static P_INLINE bool pe_is_power_of_two(uintptr_t x);

static P_INLINE void pe_arena_init(peArena *arena, void *start, size_t size) {
    arena->physical_start = start;
    arena->total_size = size;
    arena->total_allocated = 0;
    arena->temp_count = 0;
}

static P_INLINE void pe_arena_init_sub_align(peArena *arena, peArena *parent_arena, size_t size, size_t alignment) {
    void *start = pe_arena_alloc_align(parent_arena, size, alignment);
    pe_arena_init(arena, start, size);
}

static P_INLINE void pe_arena_init_sub(peArena *arena, peArena *parent_arena, size_t size) {
    void *start = pe_arena_alloc_align(parent_arena, size, P_DEFAULT_MEMORY_ALIGNMENT);
    pe_arena_init(arena, start, size);
}

static P_INLINE size_t pe_arena_alignment_offset(peArena *arena, size_t alignment) {
    P_ASSERT(pe_is_power_of_two(alignment));
    size_t alignment_offset = 0;
    uintptr_t result_pointer = (uintptr_t)arena->physical_start + arena->total_allocated;
    uintptr_t mask = alignment - 1;
    if (result_pointer & mask) {
        alignment_offset = alignment - (size_t)(result_pointer & mask);
    }
    return alignment_offset;
}

static P_INLINE size_t pe_arena_size_remaining(peArena *arena, size_t alignment) {
    size_t unusable_size = arena->total_allocated + pe_arena_alignment_offset(arena, alignment);
    size_t result = arena->total_size - unusable_size;
    return result;
}

static P_INLINE void *pe_arena_alloc_align(peArena *arena, size_t size, size_t alignment) {
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

static P_INLINE void *pe_arena_alloc(peArena *arena, size_t size) {
    void *result = pe_arena_alloc_align(arena, size, P_DEFAULT_MEMORY_ALIGNMENT);
    return result;
}

static P_INLINE void pe_arena_clear(peArena *arena) {
    arena->total_allocated = 0;
}

static P_INLINE void pe_arena_rewind(peArena *arena, size_t size) {
    P_ASSERT(size <= arena->total_allocated);
    arena->total_allocated -= size;
}

static P_INLINE void pe_arena_rewind_to_pointer(peArena *arena, void *pointer) {
    P_ASSERT((uintptr_t)pointer >= (uintptr_t)arena->physical_start);
    P_ASSERT((uintptr_t)pointer < (uintptr_t)arena->physical_start + arena->total_allocated);
    arena->total_allocated = (size_t)((uintptr_t)pointer - (uintptr_t)arena->physical_start);
}

static P_INLINE peArenaTemp pe_arena_temp_begin(peArena *arena) {
    peArenaTemp temp_arena_memory;
    temp_arena_memory.arena = arena;
    temp_arena_memory.original_count = arena->total_allocated;
    arena->temp_count += 1;
    return temp_arena_memory;
}

static P_INLINE void pe_arena_temp_end  (peArenaTemp tmp) {
    P_ASSERT_MSG(tmp.arena->total_allocated >= tmp.original_count,
                  "%zu >= %zu", tmp.arena->total_allocated, tmp.original_count);
    P_ASSERT(tmp.arena->temp_count > 0);
    tmp.arena->total_allocated = tmp.original_count;
    tmp.arena->temp_count -= 1;
}

static P_INLINE bool pe_is_power_of_two(uintptr_t x) {
	return (x & (x-1)) == 0;
}

#endif // P_ARENA_HEADER_GUARD