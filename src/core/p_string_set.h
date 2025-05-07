#ifndef P_STRING_SET_HEADER_GUARD
#define P_STRING_SET_HEADER_GUARD

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define P_STRING_SET_LOAD_FACTOR 0.75f

typedef struct pStringSetEntry {
    uint32_t key;
    char *value;
} pStringSetEntry;

typedef struct pStringSet {
    pStringSetEntry *entry;
    int total_size;
    int total_allocated;
} pStringSet;

struct pArena;
void p_string_set_init(pStringSet *string_set, struct pArena *arena, int capacity);
bool p_string_set_add(pStringSet *string_set, struct pArena *arena, char *value);
char *p_string_set_get(pStringSet *string_set, uint32_t key);

#endif // P_STRING_SET_HEADER_GUARD
#if defined(P_CORE_IMPLEMENTATION) && !defined(P_STRING_SET_IMPLEMENTATION_GUARD)
#define P_STRING_SET_IMPLEMENTATION_GUARD

#include "p_arena.h"
#include "p_assert.h"

#include "p_data_structure_utility.h"

static pStringSetEntry *p_string_set_find_entry(pStringSet *string_set, uint32_t key) {
    int index = (int)(key % (uint32_t)string_set->total_size);
    pStringSetEntry *result = NULL;
    for (int checked = 0; checked < string_set->total_size; checked += 1) {
        pStringSetEntry *entry = &string_set->entry[index];
        if (entry->key == key || entry->value == NULL) {
            result = entry;
            break;
        }
        index = (index + 1) % string_set->total_size;
    }
    P_ASSERT(result != NULL);
    return result;
}

void p_string_set_init(pStringSet *string_set, pArena *arena, int capacity) {
    size_t string_set_entry_size = (size_t)capacity * sizeof(pStringSetEntry);
    string_set->entry = p_arena_alloc(arena, string_set_entry_size);
    memset(string_set->entry, 0x0, string_set_entry_size);
    string_set->total_size = capacity;
    string_set->total_allocated = 0;
}

bool p_string_set_add(pStringSet *string_set, pArena *arena, char *value) {
    if (value == NULL) return false;
    P_ASSERT(string_set->total_allocated < string_set->total_size);
    size_t value_length = strlen(value);
    uint32_t hash = p_hash_fnv_1a(value, value_length);
    pStringSetEntry *entry = p_string_set_find_entry(string_set, hash);
    bool result = false;
    if (entry->value == NULL) {
        entry->key = hash;
        entry->value = p_arena_alloc(arena, value_length+1);
        memcpy(entry->value, value, value_length+1);
        result = true;
    } else {
        bool values_match = (strcmp(entry->value, value) == 0);
        P_ASSERT_MSG(
            values_match,
            "Hash collision for entries '%s' and '%s'\n",
            entry->value, value
        );
    }
    return result;
}

char *p_string_set_get(pStringSet *string_set, uint32_t key) {
    pStringSetEntry *entry = p_string_set_find_entry(string_set, key);
    char *result = NULL;
    if (entry != NULL) {
        result = entry->value;
    }
    return result;
}


#endif // P_CORE_IMPLEMENTATION
