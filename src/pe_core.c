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

peRandom pe_random_from_seed(uint32_t seed) {
    peRandom result = {
        .state = seed
    };
    return result;
}

peRandom pe_random_from_time(void) {
    peRandom result = {
        .state = (uint32_t)pe_time_now()
    };
    return result;
}

uint32_t pe_random_uint32(peRandom *random) {
    // https://en.wikipedia.org/w/index.php?title=Linear_congruential_generator&oldid=1169975619
    uint64_t a = 1664525;
    uint64_t c = 1013904223;
    uint64_t m = 4294967296; // 2^32
    uint32_t result = (uint32_t)((a * random->state + c) % m);
    random->state = result;
    return result;
}

#if !defined(_MSC_VER)
#include "pe_core.i"
#endif