#ifndef PE_STRING_H_HEADER_GUARD
#define PE_STRING_H_HEADER_GUARD

#include <stddef.h>

typedef struct peString {
    char *data;
    size_t size;
} peString;

#define PE_STRING_LITERAL(string_literal) (peString){ .data = (string_literal), .size = sizeof((string_literal))-1 }

peString pe_string(char *data, size_t size);
peString pe_string_from_range(char *first, char *one_past_last);
peString pe_string_from_cstring(char *cstring);






#endif