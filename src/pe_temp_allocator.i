#include "pe_temp_allocator.h"

#include "pe_core.h"

extern peAllocator temp_allocator;
extern peArena temp_arena;

PE_INLINE peAllocator pe_temp_allocator(void) {
    return temp_allocator;
}

PE_INLINE peArena *pe_temp_arena(void) {
    return &temp_arena;
}