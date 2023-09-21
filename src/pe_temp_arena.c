#include "pe_temp_arena.h"

#include "pe_core.h"

#include <stddef.h>

static void *temp_arena_memory = NULL;
peArena temp_arena = {0};

void pe_temp_arena_init(size_t size) {
    temp_arena_memory = pe_heap_alloc(size);
    pe_arena_init(&temp_arena, temp_arena_memory, size);
}

#if !defined(_MSC_VER)
#include "pe_temp_arena.i"
#endif