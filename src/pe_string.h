#ifndef PE_STRING_H_HEADER_GUARD
#define PE_STRING_H_HEADER_GUARD

#include <stddef.h>

typedef struct peString {
    char *data;
    size_t size;
} peString;

// STRING CREATION:

#define PE_STRING_LITERAL(string_literal) (peString){ .data = (string_literal), .size = sizeof((string_literal))-1 }

peString pe_string(char *data, size_t size);
peString pe_string_from_range(char *first, char *one_past_last);
peString pe_string_from_cstring(char *cstring);

// SUBSTRINGS:

peString pe_string_prefix(peString string, size_t size);
peString pe_string_chop  (peString string, size_t size);
peString pe_string_suffix(peString string, size_t size);
peString pe_string_skip  (peString string, size_t size);
peString pe_string_range (peString string, size_t first, size_t one_past_last);

// STRING BUILDING:

struct peArena;
peString pe_string_format_variadic(struct peArena *arena, char *format, va_list argument_list);
peString pe_string_format         (struct peArena *arena, char *format, ...);

#endif // PE_STRING_H_HEADER_GUARD