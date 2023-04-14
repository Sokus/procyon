#include "pe_core.h"

#include <stdio.h>
#include <stdarg.h>

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
    fprintf(stderr, buf);
}

#if !defined(_MSC_VER)
#include "pe_core.i"
#endif