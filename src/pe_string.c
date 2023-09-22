#include "pe_string.h"
#include "pe_core.h"

#include <stddef.h>
#include <stdint.h>

peString pe_string(char *data, size_t size) {
    PE_ASSERT(data != NULL);
    peString result = { .data = data, .size = size };
    return result;
}

peString pe_string_from_range(char *first, char *one_past_last) {
    PE_ASSERT(first != NULL);
    PE_ASSERT(one_past_last != NULL);
    PE_ASSERT((uintptr_t)one_past_last >= (uintptr_t)first);
    peString result = {
        .data = first,
        .size = (size_t)((uintptr_t)one_past_last - (uintptr_t)first)
    };
    return result;
}

peString pe_string_from_cstring(char *cstring) {
    size_t length = 0;
    const size_t max_length = 65536;
    while (cstring[length] != '\0' && length < max_length) {
        length += 1;
    }
    peString result = {
        .data = cstring,
        .size = length,
    };
    return result;
}