#include "pe_temp_allocator.h"

peArena temp_arena;
peAllocator temp_allocator;

void pe_temp_allocator_init(size_t size) {
    pe_arena_init_from_allocator(&temp_arena, pe_heap_allocator(), size);
    temp_allocator = pe_arena_allocator(&temp_arena);
}

#if !defined(_MSC_VER)
#include "pe_temp_allocator.i"
#endif