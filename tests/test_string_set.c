#include "core/p_string_set.h"
#include "core/p_arena.h"
#include "core/p_data_structure_utility.h"

#include <stdint.h>

#define P_TEST_STRING_SET_CAPACITY 8
#define P_TEST_STRING_SET_BUFFER_SIZE 1024

struct {
    pStringSet string_set;
    uint8_t buffer[P_TEST_STRING_SET_BUFFER_SIZE];
    pArena arena;
} test_string_set_state = {0};

static void test_string_set_setup(void) {
    pStringSet *string_set = &test_string_set_state.string_set;
    uint8_t *buffer = test_string_set_state.buffer;
    pArena *arena = &test_string_set_state.arena;
    p_arena_init(arena, buffer, P_TEST_STRING_SET_BUFFER_SIZE);
    p_string_set_init(string_set, arena, P_TEST_STRING_SET_CAPACITY);
}

static void test_string_set_teardown(void) {
}

static char *strings[6] = {
    "the first string",
    "the second string",
    "the third string",
    "the fourth string",
    "the fifth string",
    "the sixth string"
};

P_TEST(test_string_set_add) {
    pStringSet *string_set = &test_string_set_state.string_set;
    pArena *arena = &test_string_set_state.arena;
    for (int i = 0; i < 6; i += 1) {
        bool add_result = p_string_set_add(string_set, arena, strings[i]);
        P_TEST_CHECK(add_result);
    }
}

P_TEST(test_string_set_get) {
    pStringSet *string_set = &test_string_set_state.string_set;
    pArena *arena = &test_string_set_state.arena;
    for (int i = 0; i < 6; i += 1) {
        p_string_set_add(string_set, arena, strings[i]);
    }
    for (int i = 0; i < 6; i += 1) {
        uint32_t hash = p_hash_fnv_1a(strings[i], strlen(strings[i]));
        char *value = p_string_set_get(string_set, hash);
        P_TEST_EQ_STRING(strings[i], value);
    }
}

P_TEST_SUITE(test_string_set) {
    P_TEST_RUN(test_string_set_add);
    P_TEST_RUN(test_string_set_get);
}

void test_string_set_main(void) {
    P_TEST_SUITE_CONFIGURE(test_string_set_setup, test_string_set_teardown);
    P_TEST_SUITE_RUN(test_string_set);
}
