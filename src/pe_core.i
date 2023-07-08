#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

PE_INLINE void *pe_alloc_align(peAllocator a, size_t size, size_t alignment) {
    return a.proc(a.data, peAllocation_Alloc, size, alignment, NULL, 0);
}

PE_INLINE void *pe_alloc(peAllocator a, size_t size) {
    return pe_alloc_align(a, size, PE_DEFAULT_MEMORY_ALIGNMENT);
}

PE_INLINE void pe_free(peAllocator a, void *ptr) {
    if (ptr != NULL) {
        a.proc(a.data, peAllocation_Free, 0, 0, ptr, 0);
    }
}

PE_INLINE void pe_free_all(peAllocator a) {
    a.proc(a.data, peAllocation_FreeAll, 0, 0, NULL, 0);
}

PE_INLINE void *pe_resize_align(peAllocator a, void *ptr, size_t old_size, size_t new_size, size_t alignment) {
    return a.proc(a.data, peAllocation_Resize, new_size, alignment, ptr, old_size);
}

PE_INLINE void *pe_resize(peAllocator a, void *ptr, size_t old_size, size_t new_size) {
    return pe_resize_align(a, ptr, old_size, new_size, PE_DEFAULT_MEMORY_ALIGNMENT);
}

PE_INLINE void *pe_default_resize_align(peAllocator a, void *old_memory, size_t old_size, size_t new_size, size_t alignment) {
    if (!old_memory) {
        return pe_alloc_align(a, new_size, alignment);
    }

    if (new_size == 0) {
        pe_free(a, old_memory);
        return NULL;
    }

    if (new_size < old_size) {
        new_size = old_size;
    }

    if (new_size == old_size) {
        return old_memory;
    } else {
        void *new_memory = pe_alloc_align(a, new_size, alignment);
        if (!new_memory) {
            return NULL;
        }
        memcpy(new_memory, old_memory, PE_MIN(new_size, old_size));
        pe_free(a, old_memory);
        return new_memory;
    }
}

static void *pe_heap_allocator_proc(void *allocator_data, peAllocationType type,
size_t size, size_t alignment, void *old_memory, size_t old_size) {
    void *ptr = NULL;
    switch (type) {
        case peAllocation_Alloc: {
#if defined(_WIN32)
            ptr = _aligned_malloc(size, alignment);
#elif defined(PSP)
            ptr = memalign(alignment, size);
#else
            PE_UNIMPLEMENTED();
#endif
        } break;

        case peAllocation_Free: {
        #if defined(_WIN32)
            _aligned_free(old_memory);
        #else
            free(old_memory);
        #endif
        } break;

        case peAllocation_Resize: {
        #if defined(_WIN32)
            ptr = _aligned_realloc(old_memory, size, alignment);
        #else
            ptr = pe_default_resize_align(pe_heap_allocator(), old_memory, old_size, size, alignment);
        #endif
        } break;

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

static void *pe_measure_allocator_proc(void *allocator_data, peAllocationType type,
size_t size, size_t alignment, void *old_memory, size_t old_size) {
    peMeasureAllocatorData *measure_data = (peMeasureAllocatorData *)allocator_data;
    void *ptr = NULL;
    switch (type) {
        case peAllocation_Alloc: {
            size_t current_offset = measure_data->alignment + measure_data->total_allocated;
            size_t alignment_offset = (alignment - (current_offset % alignment)) % alignment;
            size_t allocation_size = size + alignment_offset;
            measure_data->total_allocated += allocation_size;
        } break;

        default: {
            PE_PANIC();
        } break;
    }
    return ptr;
}

PE_INLINE peAllocator pe_measure_allocator(peMeasureAllocatorData *data) {
    peAllocator a;
    a.proc = pe_measure_allocator_proc;
    a.data = data;
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
    arena->temp_count = 0;
}

PE_INLINE void pe_arena_init_from_allocator_align(peArena *arena, peAllocator backing, size_t size, size_t alignment) {
    arena->backing = backing;
    arena->physical_start = pe_alloc_align(backing, size, alignment);
    arena->total_size = size;
    arena->total_allocated = 0;
    arena->temp_count = 0;
}

PE_INLINE void pe_arena_init_from_allocator(peArena *arena, peAllocator backing, size_t size) {
    pe_arena_init_from_allocator_align(arena, backing, size, PE_DEFAULT_MEMORY_ALIGNMENT);
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

PE_INLINE void pe_arena_rewind_to_pointer(peArena *arena, void *pointer) {
    PE_ASSERT((uintptr_t)pointer >= (uintptr_t)arena->physical_start);
    PE_ASSERT((uintptr_t)pointer < (uintptr_t)arena->physical_start + arena->total_allocated);
    arena->total_allocated = (size_t)((uintptr_t)pointer - (uintptr_t)arena->physical_start);
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

static void *pe_arena_allocator_proc(void *allocator_data, peAllocationType type,
size_t size, size_t alignment, void *old_memory, size_t old_size) {
    peArena *arena = (peArena *)allocator_data;
    void *ptr = NULL;
    switch (type) {
        case peAllocation_Alloc: {
            size_t alignment_offset = pe_arena_alignment_offset(arena, alignment);
            size_t allocation_size = size + alignment_offset;
            if (arena->total_allocated + allocation_size > arena->total_size) {
                fprintf(stderr, "Arena out of memory\n");
                return NULL;
            }
            uintptr_t result_offset = (uintptr_t)arena->total_allocated + (uintptr_t)alignment_offset;
            ptr = (void *)((uintptr_t)arena->physical_start + result_offset);
            arena->total_allocated += allocation_size;
        } break;

        case peAllocation_FreeAll: {
            arena->total_allocated = 0;
        } break;

        case peAllocation_Resize: {
            peAllocator allocator = pe_arena_allocator(arena);
            ptr = pe_default_resize_align(allocator, old_memory, old_size, size, alignment);
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

PE_INLINE peTempArenaMemory pe_temp_arena_memory_begin(peArena *arena) {
    peTempArenaMemory temp_arena_memory;
    temp_arena_memory.arena = arena;
    temp_arena_memory.original_count = arena->total_allocated;
    arena->temp_count += 1;
    return temp_arena_memory;
}

PE_INLINE void pe_temp_arena_memory_end  (peTempArenaMemory tmp) {
    PE_ASSERT_MSG(tmp.arena->total_allocated >= tmp.original_count,
                  "%zu >= %zu", tmp.arena->total_allocated, tmp.original_count);
    PE_ASSERT(tmp.arena->temp_count > 0);
    tmp.arena->total_allocated = tmp.original_count;
    tmp.arena->temp_count -= 1;
}
