#ifndef P_DATA_STRUCTURE_UTILITY_HEADER_GUARD
#define P_DATA_STRUCTURE_UTILITY_HEADER_GUARD

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "p_defines.h"
#include "p_assert.h"

static P_INLINE bool p_is_power_of_two(uintptr_t x);

size_t p_calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size);

static P_INLINE bool p_is_power_of_two(uintptr_t x) {
	return (x & (x-1)) == 0;
}

#endif // P_DATA_STRUCTURE_UTILITY_HEADER_GUARD
#if defined(P_CORE_IMPLEMENTATION) && !defined(P_DATA_STRUCTURE_UTILITY_IMPLEMENTATION_GUARD)
#define P_DATA_STRUCTURE_UTILITY_IMPLEMENTATION_GUARD

size_t p_calc_padding_with_header(uintptr_t ptr, uintptr_t alignment, size_t header_size) {
    // based on: https://www.gingerbill.org/article/2019/02/15/memory-allocation-strategies-003/

    P_ASSERT(p_is_power_of_two(alignment));
    uintptr_t modulo = ptr & (alignment-1);

    uintptr_t padding = 0;
    if (modulo != 0) {
        padding = alignment - modulo;
    }

    uintptr_t needed_space = (uintptr_t)header_size;
    if (padding < needed_space) {
        needed_space -= padding;
        if ((needed_space & (alignment-1)) != 0) {
            padding += alignment * ((needed_space / alignment) + 1);
        } else {
            padding += alignment * (needed_space / alignment);
        }
    }

    return (size_t)padding;
}

#endif // P_CORE_IMPLEMENTATION
