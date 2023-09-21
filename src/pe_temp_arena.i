#include "pe_temp_arena.h"

#include "pe_core.h"

extern peArena temp_arena;

PE_INLINE peArena *pe_temp_arena(void) {
    return &temp_arena;
}