#ifndef P_HEAP_HEADER_GUARD
#define P_HEAP_HEADER_GUARD

#include "p_defines.h"

#include <stdlib.h>
#include <malloc.h>

static P_INLINE void *pe_heap_alloc_align(size_t size, size_t alignment) {
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

static P_INLINE void *pe_heap_alloc(size_t size) {
    return pe_heap_alloc_align(size, P_DEFAULT_MEMORY_ALIGNMENT);
}

static P_INLINE void pe_heap_free(void *ptr) {
#if defined(_WIN32)
    _aligned_free(ptr);
#elif defined(PSP) || defined(__linux__)
    free(ptr);
#endif
}

#endif // P_HEAP_HEADER_GUARD