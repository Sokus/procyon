#ifndef P_SCRATCH_HEADER_GUARD
#define P_SCRATCH_HEADER_GUARD

#include "p_arena.h"

pArenaTemp p_scratch_begin(pArena **conflicts, int conflict_count);
void p_scratch_end(pArenaTemp arena_temp);
void p_scratch_clear(void);

#endif // P_SCRATCH_HEADER_GUARD

#if defined(P_CORE_IMPLEMENTATION) && !defined(P_SCRATCH_IMPLEMENTATION_GUARD)
#define P_SCRATCH_IMPLEMENTATION_GUARD

#include "p_defines.h"
#include "p_arena.h"
#include "p_heap.h"

#include <stdint.h>
#include <stddef.h>

#define P_SCRATCH_ARENA_COUNT 2
#define P_SCRATCH_ARENA_SIZE P_MEGABYTES(4)

static struct pScratchState {
    bool initialized;
    void *backbuffer;
    pArena arenas[P_SCRATCH_ARENA_COUNT];
} p_scratch = {0};

static void p_scratch_init(void) {
    P_ASSERT(!p_scratch.initialized);
    size_t scratch_size_sum = P_SCRATCH_ARENA_COUNT * P_SCRATCH_ARENA_SIZE;
    p_scratch.backbuffer = p_heap_alloc(scratch_size_sum);
    for (int i = 0; i < P_SCRATCH_ARENA_COUNT; i += 1) {
        void *arena_start = (uint8_t*)p_scratch.backbuffer + (i * P_SCRATCH_ARENA_SIZE);
        p_arena_init(&p_scratch.arenas[i], arena_start, P_SCRATCH_ARENA_SIZE);
    }
    p_scratch.initialized = true;
}

pArenaTemp p_scratch_begin(pArena **conflicts, int conflict_count) {
    if (!p_scratch.initialized) {
        p_scratch_init();
    }
    P_ASSERT(conflict_count < P_SCRATCH_ARENA_COUNT);
    pArena *scratch_arena = NULL;
    for (int s = 0; s < P_SCRATCH_ARENA_COUNT; s += 1) {
        bool conflict = false;
        for (int c = 0; c < conflict_count; c += 1) {
            if (&p_scratch.arenas[s] == conflicts[c]) {
                conflict = true;
                break;
            }
        }
        if (!conflict) {
            scratch_arena = &p_scratch.arenas[s];
        }
    }
    P_ASSERT(scratch_arena != NULL);
    pArenaTemp result = p_arena_temp_begin(scratch_arena);
    return result;
}

void p_scratch_end(pArenaTemp arena_temp) {
    P_ASSERT(p_scratch.initialized);
    P_ASSERT(arena_temp.arena >= &p_scratch.arenas[0]);
    P_ASSERT(arena_temp.arena <= &p_scratch.arenas[P_SCRATCH_ARENA_COUNT-1]);
    p_arena_temp_end(arena_temp);
}

void p_scratch_clear(void) {
    if (!p_scratch.initialized) {
        p_scratch_init();
    }
    for (int s = 0; s < P_SCRATCH_ARENA_COUNT; s += 1) {
        p_arena_clear(&p_scratch.arenas[s]);
    }
}

#endif // P_SCRATCH_IMPLEMENTATION
