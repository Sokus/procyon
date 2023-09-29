#include "pe_core.h"

#include "pe_time.h"

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

void pe_assert_handler(char *prefix, char *condition, char *file, int line, char *msg, ...) {
    char buf[512] = {0};
    int buf_off = 0;
    buf_off += snprintf(&buf[buf_off], PE_COUNT_OF(buf)-buf_off-1, "%s(%d): %s: ", file, line, prefix);
	if (condition)
        buf_off += snprintf(&buf[buf_off], PE_COUNT_OF(buf)-buf_off-1, "`%s` ", condition);
	if (msg) {
		va_list va;
		va_start(va, msg);
        buf_off += vsnprintf(&buf[buf_off], PE_COUNT_OF(buf)-buf_off-1, msg, va);
		va_end(va);
	}
    buf_off += snprintf(&buf[buf_off], PE_COUNT_OF(buf)-buf_off-1, "\n");
    fprintf(stderr, "%s", buf);
}

////////////////////////////////////////////////////////////////////////////////

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
    const size_t max_length = 2048;
    while (cstring[length] != '\0' && length < max_length) {
        length += 1;
    }
    peString result = {
        .data = cstring,
        .size = length,
    };
    return result;
}

peString pe_string_prefix(peString string, size_t size) {
    size_t size_clamped = PE_MIN(size, string.size);
    peString result = { .data = string.data, .size = size_clamped };
    return result;
}

peString pe_string_chop(peString string, size_t size) {
    size_t size_clamped = PE_MIN(size, string.size);
    size_t size_remaining = string.size - size_clamped;
    peString result = { .data = string.data, .size = size_remaining };
    return result;
}

peString pe_string_suffix(peString string, size_t size) {
    size_t size_clamped = PE_MIN(size, string.size);
    size_t skip_size = string.size - size_clamped;
    char *data = (char *)((uintptr_t)string.data + (uintptr_t)skip_size);
    peString result = { .data = data, .size = size_clamped };
    return result;
}

peString pe_string_skip(peString string, size_t size) {
    size_t size_clamped = PE_MIN(size, string.size);
    size_t size_remaining = string.size - size_clamped;
    char *data = (char *)((uintptr_t)string.data + (uintptr_t)size_clamped);
    peString result = { .data = data, .size = size_remaining };
    return result;
}

peString pe_string_range(peString string, size_t first, size_t one_past_last) {
    PE_ASSERT(one_past_last >= first);
    size_t one_past_last_clamped = PE_MIN(one_past_last, string.size);
    size_t first_clamped = PE_MIN(first, string.size);
    size_t size_remaining = one_past_last_clamped - first_clamped;
    char *data = (char *)((uintptr_t)string.data + (uintptr_t)first_clamped);
    peString result = { .data = data, .size = size_remaining };
    return result;
}

peString pe_string_format_variadic(peArena *arena, char *format, va_list argument_list) {
    va_list argument_list_backup;
    va_copy(argument_list_backup, argument_list);

    size_t buffer_size = 1024;
    char *buffer = pe_arena_alloc(arena, buffer_size);
    size_t actual_size = vsnprintf(buffer, buffer_size, format, argument_list);

    peString result;
    if (actual_size < buffer_size) {
        pe_arena_rewind(arena, buffer_size - actual_size - 1);
        result = pe_string(buffer, actual_size);
    } else {
        pe_arena_rewind(arena, buffer_size);
        char *fixed_buffer = pe_arena_alloc(arena, actual_size + 1);
        size_t final_size = vsnprintf(fixed_buffer, actual_size + 1, format, argument_list_backup);
        PE_ASSERT(actual_size == final_size);
        result = pe_string(fixed_buffer, final_size);
    }

    va_end(argument_list_backup);
    return result;
}

peString pe_string_format(peArena *arena, char *format, ...) {
    va_list argument_list;
    va_start(argument_list, format);
    peString result = pe_string_format_variadic(arena, format, argument_list);
    va_end(argument_list);
    return result;
}

////////////////////////////////////////////////////////////////////////////////

#if !defined(_MSC_VER)
#include "pe_core.i"
#endif