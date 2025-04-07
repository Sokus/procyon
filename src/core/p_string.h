#ifndef P_STRING_HEADER_GUARD
#define P_STRING_HEADER_GUARD

#include "p_defines.h"
#include "p_arena.h"

#include <stdlib.h>

typedef struct pString {
    char *data;
    size_t size;
} pString;

#define P_STRING_LITERAL(string_literal) (pString){ .data = (string_literal), .size = sizeof((string_literal))-1 }
#define P_STRING_EXPAND(string) (int)((string).size), ((string).data)

pString p_string(char *data, size_t size);
pString p_string_from_range(char *first, char *one_past_last);
pString p_string_from_cstring(char *cstring);

pString p_string_prefix(pString string, size_t size);
pString p_string_chop  (pString string, size_t size);
pString p_string_suffix(pString string, size_t size);
pString p_string_skip  (pString string, size_t size);
pString p_string_range (pString string, size_t first, size_t one_past_last);

pString p_string_format_variadic(pArena *arena, char *format, va_list argument_list);
pString p_string_format         (pArena *arena, char *format, ...);

#endif // P_STRING_HEADER_GUARD

#if defined(P_CORE_IMPLEMENTATION) && !defined(P_STRING_IMPLEMENTATION_GUARD)

pString p_string(char *data, size_t size) {
    P_ASSERT(data != NULL);
    pString result = { .data = data, .size = size };
    return result;
}

pString p_string_from_range(char *first, char *one_past_last) {
    P_ASSERT(first != NULL);
    P_ASSERT(one_past_last != NULL);
    P_ASSERT((uintptr_t)one_past_last >= (uintptr_t)first);
    pString result = {
        .data = first,
        .size = (size_t)((uintptr_t)one_past_last - (uintptr_t)first)
    };
    return result;
}

pString p_string_from_cstring(char *cstring) {
    size_t length = 0;
    const size_t max_length = 2048;
    while (cstring[length] != '\0' && length < max_length) {
        length += 1;
    }
    pString result = {
        .data = cstring,
        .size = length,
    };
    return result;
}

pString p_string_prefix(pString string, size_t size) {
    size_t size_clamped = P_MIN(size, string.size);
    pString result = { .data = string.data, .size = size_clamped };
    return result;
}

pString p_string_chop(pString string, size_t size) {
    size_t size_clamped = P_MIN(size, string.size);
    size_t size_remaining = string.size - size_clamped;
    pString result = { .data = string.data, .size = size_remaining };
    return result;
}

pString p_string_suffix(pString string, size_t size) {
    size_t size_clamped = P_MIN(size, string.size);
    size_t skip_size = string.size - size_clamped;
    char *data = (char *)((uintptr_t)string.data + (uintptr_t)skip_size);
    pString result = { .data = data, .size = size_clamped };
    return result;
}

pString p_string_skip(pString string, size_t size) {
    size_t size_clamped = P_MIN(size, string.size);
    size_t size_remaining = string.size - size_clamped;
    char *data = (char *)((uintptr_t)string.data + (uintptr_t)size_clamped);
    pString result = { .data = data, .size = size_remaining };
    return result;
}

pString p_string_range(pString string, size_t first, size_t one_past_last) {
    P_ASSERT(one_past_last >= first);
    size_t one_past_last_clamped = P_MIN(one_past_last, string.size);
    size_t first_clamped = P_MIN(first, string.size);
    size_t size_remaining = one_past_last_clamped - first_clamped;
    char *data = (char *)((uintptr_t)string.data + (uintptr_t)first_clamped);
    pString result = { .data = data, .size = size_remaining };
    return result;
}

pString p_string_format_variadic(pArena *arena, char *format, va_list argument_list) {
    va_list argument_list_backup;
    va_copy(argument_list_backup, argument_list);

    size_t buffer_size = 1024;
    char *buffer = p_arena_alloc(arena, buffer_size);
    size_t actual_size = vsnprintf(buffer, buffer_size, format, argument_list);

    pString result;
    if (actual_size < buffer_size) {
        p_arena_rewind(arena, buffer_size - actual_size - 1);
        result = p_string(buffer, actual_size);
    } else {
        p_arena_rewind(arena, buffer_size);
        char *fixed_buffer = p_arena_alloc(arena, actual_size + 1);
        size_t final_size = vsnprintf(fixed_buffer, actual_size + 1, format, argument_list_backup);
        P_ASSERT(actual_size == final_size);
        result = p_string(fixed_buffer, final_size);
    }

    va_end(argument_list_backup);
    return result;
}

pString p_string_format(pArena *arena, char *format, ...) {
    va_list argument_list;
    va_start(argument_list, format);
    pString result = p_string_format_variadic(arena, format, argument_list);
    va_end(argument_list);
    return result;
}

#endif // P_CORE_IMPLEMENTATION
