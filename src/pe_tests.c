#include "pe_string.h"
#include "pe_core.h"

#include <stdio.h>

void test(int a, int b, char *c, int d, int e);

int main(int argc, char *argv[]) {

    peString string = PE_STRING_LITERAL("Hello, World!");
    peString prefix = pe_string_prefix(string, 5);
    peString chop   = pe_string_chop(string, 6);
    peString suffix = pe_string_suffix(string, 6);
    peString skip   = pe_string_skip(string, 5);
    peString range  = pe_string_range(string, 4, 7);

    peArena format_arena;
    char format_buffer[PE_KILOBYTES(128)];
    pe_arena_init(&format_arena, format_buffer, sizeof(format_buffer));
    peString format = pe_string_format(&format_arena, "%s, %s", "Hello", "World!");

    printf("string:    '%.*s'\n", PE_STRING_EXPAND(string));
    printf("prefix(5): '%.*s'\n", PE_STRING_EXPAND(prefix));
    printf("chop(6):   '%.*s'\n", PE_STRING_EXPAND(chop));
    printf("suffix(6): '%.*s'\n", PE_STRING_EXPAND(suffix));
    printf("skup(5):   '%.*s'\n", PE_STRING_EXPAND(skip));
    printf("range(4,7):'%.*s'\n", PE_STRING_EXPAND(range));
    printf("format:    '%.*s'\n", PE_STRING_EXPAND(format));

    return 0;
}