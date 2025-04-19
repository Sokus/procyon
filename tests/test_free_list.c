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
    int allocations[4] = { 25, 45, 102, 67 };
    int alignments[4] = { 8, 16, 8, 32 };
    int total_allocated = 0;
    for (int a = 0; a < 4; a += 1) {
        void *result = p_free_list_alloc_align(free_list, allocations[a], alignments[a]);
        P_TEST_CHECK(result != NULL);
        P_TEST_EQ_SIZE(0, (size_t)result % alignments[a]);
        total_allocated += allocations[a] + sizeof(pFreeListAllocationHeader);
    }
    P_TEST_CHECK(free_list->total_allocated >= total_allocated);
}

P_TEST(test_free_list_free) {
    pFreeList *free_list = &test_free_list_state.free_list;
    void *pointers[4] = {0};
    int allocations[4] = { 25, 45, 102, 67 };
    for (int a = 0; a < 4; a += 1) {
        pointers[a] = p_free_list_alloc(free_list, allocations[a]);
        memset(pointers[a], 0x0, allocations[a]);
    }
    for (int a = 0; a < 4; a += 1) {
        p_free_list_free(free_list, pointers[a]);
    }
    P_TEST_EQ_SIZE(0, free_list->total_allocated);
    P_TEST_CHECK(free_list->head == free_list->physical_start);
    P_TEST_CHECK(free_list->head->block_size == free_list->total_size);
}

P_TEST(test_free_list_same_ptr) {
    pFreeList *free_list = &test_free_list_state.free_list;
    size_t allocation_size = 32;
    (void)p_free_list_alloc(free_list, allocation_size);
    (void)p_free_list_alloc(free_list, allocation_size);
    void *old_ptr = p_free_list_alloc(free_list, allocation_size);
    (void)p_free_list_alloc(free_list, allocation_size);
    (void)p_free_list_alloc(free_list, allocation_size);
    p_free_list_free(free_list, old_ptr);
    void *new_ptr = p_free_list_alloc(free_list, allocation_size);
    P_TEST_CHECK(old_ptr == new_ptr);
}

P_TEST(test_free_list_node_order_backward) {
    pFreeList *free_list = &test_free_list_state.free_list;
    size_t allocation_size = 32;
    void *pointers[7] = {0};
    bool free_pointer[7] = { false, true, false, true, true, false, true };
    const int expected_free_blocks = 3;
    for (int i = 0; i < 7; i += 1) {
        pointers[i] = p_free_list_alloc(free_list, allocation_size);
    }
    for (int i = 6; i >= 0; i -= 1) {
        if (free_pointer[i]) {
            p_free_list_free(free_list, pointers[i]);
        }
    }
    pFreeListNode *node = free_list->head;
    int free_blocks = 0;
    while (node != NULL) {
        free_blocks += 1;
        if (node->next != NULL) {
            P_TEST_CHECK(node < node->next);
        }
        node = node->next;
    }
    P_TEST_EQ_INT(expected_free_blocks, free_blocks);
}

P_TEST(test_free_list_unordered_free) {
    pFreeList *free_list = &test_free_list_state.free_list;
    size_t allocation_size = 32;
    void *pointers[7] = {0};
    int free_order[7] = { 3, 1, 6, 2, 5, 4, 0 };
    for (int i = 0; i < 7; i += 1) {
        pointers[i] = p_free_list_alloc(free_list, allocation_size);
    }
    int total_frees = 0;
    for (int free_turn = 0; free_turn < 7; free_turn += 1) {
        for (int i = 0; i < 7; i += 1) {
            if (free_order[i] == free_turn) {
                p_free_list_free(free_list, pointers[free_order[i]]);
                total_frees += 1;
            }
        }
    }
    P_TEST_EQ_INT(7, total_frees);
    P_TEST_CHECK(free_list->head == free_list->physical_start);
    P_TEST_CHECK(free_list->head->next == NULL);
    P_TEST_CHECK(free_list->head->block_size == free_list->total_size);
}

P_TEST_SUITE(test_free_list) {
    P_TEST_RUN(test_free_list_alloc);
    P_TEST_RUN(test_free_list_free);
    P_TEST_RUN(test_free_list_same_ptr);
    P_TEST_RUN(test_free_list_node_order_backward);
    P_TEST_RUN(test_free_list_unordered_free);
}

void test_free_list_main() {
    P_TEST_SUITE_CONFIGURE(test_free_list_setup, test_free_list_teardown);
    P_TEST_SUITE_RUN(test_free_list);
}
