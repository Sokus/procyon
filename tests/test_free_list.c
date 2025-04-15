#include "core/p_free_list.h"

#include <stdint.h>
#include <stdlib.h>

#define P_TEST_FREE_LIST_BUFFER_SIZE 1024

struct {
    pFreeList free_list;
    uint8_t buffer[P_TEST_FREE_LIST_BUFFER_SIZE];
} test_free_list_state = {0};

static void test_free_list_setup(void) {
    pFreeList *free_list = &test_free_list_state.free_list;
    void *buffer = test_free_list_state.buffer;
    p_free_list_init(free_list, buffer, P_TEST_FREE_LIST_BUFFER_SIZE);
}

static void test_free_list_teardown(void) {
    test_free_list_state.free_list = (pFreeList){0};
    memset(test_free_list_state.buffer, 0x0, P_TEST_FREE_LIST_BUFFER_SIZE);
}

P_TEST(test_free_list_alloc) {
    pFreeList *free_list = &test_free_list_state.free_list;
    int alignment = 16;
    int allocations[] = { 25, 45, 102, 67 };
    int total_allocated = 0;
    for (int a = 0; a < sizeof(allocations)/sizeof(int); a += 1) {
        void *result = p_free_list_alloc_align(free_list, allocations[a], alignment);
        P_TEST_CHECK(result != NULL);
        P_TEST_CHECK((uintptr_t)result % alignment == 0);
        total_allocated += allocations[a];
    }
    P_TEST_CHECK(free_list->total_allocated >= total_allocated);
}

P_TEST(test_free_list_free) {
    pFreeList *free_list = &test_free_list_state.free_list;
    void *pointers[4] = {0};
    int allocations[4] = { 25, 45, 102, 67 };
    for (int a = 0; a < 4; a += 1) {
        pointers[a] = p_free_list_alloc(free_list, allocations[a]);
    }
    for (int a = 0; a < 4; a += 1) {
        p_free_list_free(free_list, pointers[a]);
    }
    P_TEST_ASSERT_INT_EQ(0, (int)free_list->total_allocated);
}

P_TEST_SUITE(test_free_list) {
    P_TEST_RUN(test_free_list_alloc);
    P_TEST_RUN(test_free_list_free);
}

void test_free_list_main() {
    P_TEST_SUITE_CONFIGURE(test_free_list_setup, test_free_list_teardown);
    P_TEST_SUITE_RUN(test_free_list);
}
