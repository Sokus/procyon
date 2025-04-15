#ifndef P_TEST_HEADER_GUARD
#define P_TEST_HEADER_GUARD

// based on: https://github.com/siu/minunit/blob/master/minunit.h

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "core/p_time.h"

#define P_TEST_MESSAGE_BUFFER_SIZE 1024
#define P_TEST_EPSILON 1E-12

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
    if (p_test.start_tick == 0) {\
        p_test.start_tick = p_time_now();\
    }\
    if (p_test.setup_proc) { p_test.setup_proc(); }\
    p_test.status = 0;\
    proc_name();\
    p_test.run_count += 1;\
    if (p_test.status != 0) {\
        p_test.fail_count += 1;\
        printf("F\n%s\n", p_test.message);\
    }\
    (void)fflush(stdout);\
    if (p_test.teardown_proc) { p_test.teardown_proc(); }\
} while(0)

#define P_TEST_REPORT() do {\
    printf(\
        "%d tests, %d assertions, %d failures\n",\
        p_test.run_count,\
        p_test.assert_count,\
        p_test.fail_count\
    );\
    uint64_t duration_ticks = p_time_since(p_test.start_tick);\
    double duration_sec = p_time_sec(duration_ticks);\
    printf("finished in %.8f seconds\n", duration_sec);\
} while(0)

#define P_TEST_CHECK(test) do {\
    p_test.assert_count += 1;\
    if (!(test)) {\
        (void)snprintf(\
            p_test.message, P_TEST_MESSAGE_BUFFER_SIZE,\
            "  %s failed:\n    %s:%d: %s", __func__, __FILE__, __LINE__, #test\
        );\
        p_test.status = 1;\
        return;\
    } else {\
        printf(".");\
    }\
} while(0)

#define P_TEST_FAIL(message) do {\
    p_test.assert_count += 1;\
    (void)snprintf(\
        p_test.message, P_TEST_MESSAGE_BUFFER_SIZE,\
        "  %s failed:\n    %s:%d: %s", __func__, __FILE__, __LINE__, message\
    );\
    p_test.status = 1;\
    return;\
} while(0)

#define P_TEST_ASSERT(test, message) do {\
    p_test.assert_count += 1;\
    if (!(test)) {\
        (void)snprintf(\
            p_test.message, P_TEST_MESSAGE_BUFFER_SIZE,\
            "  %s failed:\n    %s:%d: %s", __func__, __FILE__, __LINE__, message\
        );\
        p_test.status = 1;\
        return;\
    } else {\
        printf(".");\
    }\
} while(0)

#define P_TEST_ASSERT_INT_EQ(expected, result) do {\
    p_test.assert_count += 1;\
    int p_test_tmp_expected = (expected);\
    int p_test_tmp_result = (result);\
    if (p_test_tmp_expected != p_test_tmp_result) {\
        (void)snprintf(\
            p_test.message, P_TEST_MESSAGE_BUFFER_SIZE,\
            "  %s failed:\n    %s:%d: %d expected but was %d",\
            __func__, __FILE__, __LINE__, p_test_tmp_expected, p_test_tmp_result\
        );\
        p_test.status = 1;\
        return;\
    } else {\
        printf(".");\
    }\
} while(0)

#define P_TEST_ASSERT_DOUBLE_EQ(expected, result) do {\
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

#define P_TEST_ASSERT_STRING_EQ(expected, result) do {\
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
        int p_test_significant_figures = 1 - log10(P_TEST_EPSILON);\
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
