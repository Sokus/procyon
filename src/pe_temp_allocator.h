#ifndef PE_TEMP_ALLOCATOR_H_HEADER_GUARD
#define PE_TEMP_ALLOCATOR_H_HEADER_GUARD

#include <stddef.h>

void pe_temp_allocator_init(size_t size);

struct peAllocator;
struct peAllocator pe_temp_allocator(void);

struct peArena;
struct peArena *pe_temp_arena(void);

#if defined(_MSC_VER)
#include "pe_temp_allocator.i"
#endif

#endif // PE_TEMP_ALLOCATOR_H_HEADER_GUARD