#ifndef P_ASSERT_HEADER_GUARD
#define P_ASSERT_HEADER_GUARD

#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>

#ifndef P_STATIC_ASSERT
#define P_STATIC_ASSERT3(cond, msg) typedef char p_static_assert_##msg[(cond)?1:-1]
#define P_STATIC_ASSERT2(cond, msg) P_STATIC_ASSERT3(cond, msg)
#define P_STATIC_ASSERT(cond) P_STATIC_ASSERT2(cond, __LINE__)
#endif

#ifndef P_DEBUG_TRAP
	#if defined(__PSP__)
		#define P_DEBUG_TRAP() p_debug_trap()
	#elif defined(_MSC_VER)
		#define P_DEBUG_TRAP() __debugbreak()
	#else
		#define P_DEBUG_TRAP() __builtin_trap()
	#endif
#endif

#if !defined(P_ASSERT_MSG) && !defined(NDEBUG)
#define P_ASSERT_MSG(cond, msg, ...) \
do { \
	if (!(cond)) { \
		p_assert_handler("Assertion Failure", #cond, __FILE__, (int)__LINE__, msg, ##__VA_ARGS__); \
		P_DEBUG_TRAP(); \
	} \
} while (0)
#elif !defined(P_ASSERT_MSG) && defined(NDEBUG)
#define P_ASSERT_MSG(cond, msg, ...) do {} while (0)
#endif

#if !defined(P_ASSERT)
#define P_ASSERT(cond) P_ASSERT_MSG(cond, NULL)
#endif

#if !defined(P_ALWAYS) && !defined(NDEBUG)
#define P_ALWAYS(cond) ((cond) ? 1 : (p_assert_handler("Assertion Failure", #cond, __FILE__, (int)__LINE__, NULL), (void)P_DEBUG_TRAP(), 0))
#elif !defined(P_ALWAYS) && defined(NDEBUG)
#define P_ALWAYS(cond) (cond)
#endif

#ifndef P_PANIC_MSG
#define P_PANIC_MSG(msg, ...) do { \
	p_assert_handler("Panic", NULL, __FILE__, (int)__LINE__, msg, ##__VA_ARGS__); \
	P_DEBUG_TRAP(); \
} while (0)
#endif

#ifndef P_PANIC
#define P_PANIC() P_PANIC_MSG(NULL)
#endif

#ifndef P_UNIMPLEMENTED
#define P_UNIMPLEMENTED() do { \
	p_assert_handler("Unimplemented", NULL, __FILE__, (int)__LINE__, NULL); \
	P_DEBUG_TRAP(); \
} while(0)
#endif

#if defined(__PSP__)
static __attribute__ ((__always_inline__)) inline void p_debug_trap(void) {
    asm(".set push\n"
		".set noreorder\n"
		"break\n"
		"nop\n"
		"jr $31\n" \
		"nop\n"
		".set pop\n");
}
#endif

void p_assert_handler(char *prefix, char *condition, char *file, int line, char *msg, ...);

#endif // P_ASSERT_HEADER_GUARD

#if defined(P_CORE_IMPLEMENTATION) && !defined(P_ASSERT_IMPLEMENTATION_GUARD)
#define P_ASSERT_IMPLEMENTATION_GUARD

#define P_ASSERT_BUF_SIZE 512

void p_assert_handler(char *prefix, char *condition, char *file, int line, char *msg, ...) {
    char buf[P_ASSERT_BUF_SIZE] = {0};
    int buf_off = 0;
    buf_off += snprintf(&buf[buf_off], P_ASSERT_BUF_SIZE-buf_off-1, "%s(%d): %s", file, line, prefix);
    if (condition || msg)
        buf_off += snprintf(&buf[buf_off], P_ASSERT_BUF_SIZE-buf_off-1, ": ");
	if (condition)
        buf_off += snprintf(&buf[buf_off], P_ASSERT_BUF_SIZE-buf_off-1, "`%s` ", condition);
	if (msg) {
		va_list va;
		va_start(va, msg);
        buf_off += vsnprintf(&buf[buf_off], P_ASSERT_BUF_SIZE-buf_off-1, msg, va);
		va_end(va);
	}
    buf_off += snprintf(&buf[buf_off], P_ASSERT_BUF_SIZE-buf_off-1, "\n");
    fprintf(stderr, "%s", buf);
}

#endif // P_CORE_IMPLEMENTATION