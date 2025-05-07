#ifndef P_TEST_HEADER_GUARD
#define P_TEST_HEADER_GUARD

// based on: https://github.com/siu/minunit/blob/master/minunit.h

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>

#include "core/p_time.h"

#define P_TEST_MESSAGE_BUFFER_SIZE 1024
#define P_TEST_EPSILON 1E-12

//      P_TEST_FORMAT_BASE "  <test_case> failed: \n    <file>:<line> "
#define P_TEST_FORMAT_BASE "  %s failed: \n    %s:%d: "

typedef void pTestSetupProc(void);
typedef void pTestTeardownProc(void);

typedef struct pTestState {
    uint64_t start_tick;
    int run_count;
    int assert_count;
    int fail_count;
    int status;
    char message[P_TEST_MESSAGE_BUFFER_SIZE];
    pTestSetupProc *setup_proc;
    pTestTeardownProc *teardown_proc;
} pTestState;
static pTestState p_test = {0};

static inline void p_test_setup(void) {
    if (p_test.start_tick == 0) {
        p_test.start_tick = p_time_now();
    }
    if (p_test.setup_proc) {
        p_test.setup_proc();
    }
    p_test.status = 0;
}

static inline void p_test_teardown(void) {
    p_test.run_count += 1;
    if (p_test.status != 0) {
        p_test.fail_count += 1;
        printf("F\n%s\n", p_test.message);
    }
    (void)fflush(stdout);
    if (p_test.teardown_proc) {
        p_test.teardown_proc();
    }
}

static inline void p_test_report(void) {
    printf(\
        "%d tests, %d assertions, %d failures\n",\
        p_test.run_count,\
        p_test.assert_count,\
        p_test.fail_count\
    );\
    uint64_t duration_ticks = p_time_since(p_test.start_tick);\
    double duration_sec = p_time_sec(duration_ticks);\
    printf("finished in %.8f seconds\n", duration_sec);\
}

static inline void p_test_print_variadic(bool test_value, const char *format, va_list argument_list) {
    p_test.assert_count += 1;
    if (!test_value) {
        (void)vsnprintf(p_test.message, P_TEST_MESSAGE_BUFFER_SIZE, format, argument_list);
        p_test.status = 1;
    } else {
        printf(".");
    }
}

static inline void p_test_print(bool test, const char *format, ...) {
    va_list argument_list;
    va_start(argument_list, format);
    p_test_print_variadic(test, format, argument_list);
    va_end(argument_list);
}

#define P_TEST(proc_name) static void proc_name(void)
#define P_TEST_SUITE(suite_name) static void suite_name(void)

#define P_TEST_SUITE_RUN(suite_name) do {\
    printf("%s:", #suite_name);\
    suite_name();\
    printf("\n");\
    p_test.setup_proc = NULL;\
    p_test.teardown_proc = NULL;\
} while(0)

#define P_TEST_SUITE_CONFIGURE(new_setup_proc, new_teardown_proc) do {\
    p_test.setup_proc = new_setup_proc;\
    p_test.teardown_proc = new_teardown_proc;\
} while(0)

#define P_TEST_RUN(proc_name) do {\
    p_test_setup();\
    proc_name();\
    p_test_teardown();\
} while(0)

#define P_TEST_REPORT() p_test_report()

#define P_TEST_CHECK(test_value) do {\
    bool p_tmp_value = (test_value);\
    p_test_print(p_tmp_value, P_TEST_FORMAT_BASE "%s", __func__, __FILE__, __LINE__, #test_value);\
    if (!p_tmp_value) return;\
} while(0)

#define P_TEST_FAIL(message) do {\
    p_test_print(false, P_TEST_FORMAT_BASE "%s", __func__, __FILE__, __LINE__, message);\
    return;\
} while(0)

#define P_TEST_ASSERT(test_value, message) do {\
    bool p_tmp_value = (test_value);\
    p_test_print((test_value), P_TEST_FORMAT_BASE "%s", __func__, __FILE__, __LINE__, message)\
    if (!p_tmp_value) return;\
} while(0)

#define P_TEST_EQ_HELPER(expected, result, type, format) do {\
    type p_tmp_expected = (expected);\
    type p_tmp_result = (result);\
    bool p_tmp_value = (p_tmp_expected == p_tmp_result);\
    p_test_print(p_tmp_value, P_TEST_FORMAT_BASE format " expected but was "format,\
        __func__, __FILE__, __LINE__, p_tmp_expected, p_tmp_result);\
    if (!p_tmp_value) return;\
} while(0)

#define P_TEST_EQ_INT(expected, result) P_TEST_EQ_HELPER(expected, result, int, "%d")
#define P_TEST_EQ_SIZE(expected, result) P_TEST_EQ_HELPER(expected, result, size_t, "%zu")

#define P_TEST_NEQ_HELPER(expected, result, type, format) do {\
    type p_tmp_expected = (expected);\
    type p_tmp_result = (result);\
    bool p_tmp_value = (p_tmp_expected != p_tmp_result);\
    p_test_print(p_tmp_value, P_TEST_FORMAT_BASE "value shouldn't be "format,\
        __func__, __FILE__, __LINE__, p_tmp_expected);\
    if (!p_tmp_value) return;\
} while(0)


#define P_TEST_EQ_DOUBLE(expected, result) do {\
    p_test.assert_count += 1;\
    double p_test_tmp_expected = (expected);\
    double p_test_tmp_result = (result);\
    if (fabs(p_test_tmp_expected-p_test_tmp_result) > P_TEST_EPSILON) {\
        int p_test_significant_figures = 1 - log10(P_TEST_EPSILON);\
        (void)snprintf(\
            p_test.message, P_TEST_MESSAGE_BUFFER_SIZE,\
            "  %s failed:\n    %s:%d: %.*g expected but was %.*g",\
            __func__, __FILE__, __LINE__,\
            p_test_significant_figures, p_test_tmp_expected,\
            p_test_significant_figures, p_test_tmp_result\
        );\
        p_test.status = 1;\
        return;\
    } else {\
        printf(".");\
    }\
} while(0)

#define P_TEST_EQ_STRING(expected, result) do {\
    p_test.assert_count += 1;\
    char *p_test_tmp_expected = (expected);\
    char *p_test_tmp_result = (result);\
    if (!p_test_tmp_expected) {\
        p_test_tmp_expected = "<null pointer>";\
    }\
    if (!p_test_tmp_result) {\
        p_test_tmp_result = "<null pointer>";\
    }\
    if (strcmp(p_test_tmp_expected, p_test_tmp_result) != 0) {\
        (void)snprintf(\
            p_test.message, P_TEST_MESSAGE_BUFFER_SIZE,\
            "  %s failed:\n    %s:%d: '%s' expected but was '%s'",\
            __func__, __FILE__, __LINE__,\
            p_test_tmp_expected, p_test_tmp_result\
        );\
        p_test.status = 1;\
        return;\
    } else {\
        printf(".");\
    }\
} while(0)



#endif // P_TEST_HEADER_GUARD
