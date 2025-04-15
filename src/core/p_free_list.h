#ifndef P_FREE_LIST_HEADER_GUARD
#define P_FREE_LIST_HEADER_GUARD

// based on: https://www.gingerbill.org/article/2021/11/30/memory-allocation-strategies-005/#linked-list-free-list-implementation

#include <stdint.h>
#include <stdlib.h>

typedef struct pFreeListAllocationHeader {
    size_t block_size;
    size_t padding;
} pFreeListAllocationHeader;

struct pFreeListNode;
typedef struct pFreeListNode {
    struct pFreeListNode *next;
    size_t block_size;
} pFreeListNode;

typedef enum pFreeListPlacementPolicy {
    pFreeListPlacementPolicy_First,
    pFreeListPlacementPolicy_Best,
    pFreeListPlacementPolicy_Count,
} pFreeListPlacementPolicy;

typedef struct pFreeList {
    void *physical_start;
    size_t total_size;
    size_t total_allocated;

    pFreeListNode *head;
    pFreeListPlacementPolicy placement_policy;
} pFreeList;

typedef struct pFreeListFindResult {
    pFreeListNode *node;
    size_t padding;
    pFreeListNode *prev_node;
} pFreeListFindResult;

void p_free_list_init(pFreeList *free_list, void *start, size_t size);
void p_free_list_clear(pFreeList *free_list);
void *p_free_list_alloc_align(pFreeList *free_list, size_t size, size_t alignment);
void *p_free_list_alloc(pFreeList *free_list, size_t size);
void p_free_list_free(pFreeList *free_list, void *ptr);

pFreeListFindResult p_free_list_find_first(pFreeList *free_list, size_t size, size_t alignment);
pFreeListFindResult p_free_list_find_best(pFreeList *free_list, size_t size, size_t alignment);
void p_free_list_coalescence(pFreeList *free_list, pFreeListNode *prev_node, pFreeListNode *free_node);

void p_free_list_node_insert(pFreeListNode **prev_head, pFreeListNode *prev_node, pFreeListNode *new_node);
void p_free_list_node_remove(pFreeListNode **prev_head, pFreeListNode *prev_node, pFreeListNode *rem_node);


#endif // P_FREE_LIST_HEADER_GUARD
#if defined(P_CORE_IMPLEMENTATION) && !defined(P_FREE_LIST_IMPLEMENTATION_GUARD)
#define P_FREE_LIST_IMPLEMENTATION_GUARD

#include "p_data_structure_utility.h"

void p_free_list_init(pFreeList *free_list, void *start, size_t size) {
    free_list->physical_start = start;
    free_list->total_size = size;
    p_free_list_clear(free_list);
}

void p_free_list_clear(pFreeList *free_list) {
    free_list->total_allocated = 0;
    pFreeListNode *first_node = (pFreeListNode *)free_list->physical_start;
    first_node->block_size = free_list->total_size;
    first_node->next = NULL;
    free_list->head = first_node;
}

void *p_free_list_alloc_align(pFreeList *free_list, size_t size, size_t alignment) {
    if (size < sizeof(pFreeListNode)) {
        size = sizeof(pFreeListNode);
    }
    if (alignment < 8) {
        alignment = 8;
    }

    pFreeListFindResult find_result;
    switch (free_list->placement_policy) {
        case pFreeListPlacementPolicy_First:
            find_result = p_free_list_find_first(free_list, size, alignment);
            break;
        case pFreeListPlacementPolicy_Best:
            find_result = p_free_list_find_best(free_list, size, alignment);
            break;
        default:
            P_PANIC_MSG("Invalid pFreeListPlacementPolicy: %d\n", free_list->placement_policy);
            find_result = (pFreeListFindResult){0};
            break;
    }
    if (find_result.node == NULL) {
        fprintf(stderr, "Free list out of memory\n");
        return NULL;
    }

    size_t alignment_padding = find_result.padding - sizeof(pFreeListAllocationHeader);
    size_t required_space = size + find_result.padding;
    size_t remaining = find_result.node->block_size - required_space;

    if (remaining > 0) {
        pFreeListNode *new_node = (pFreeListNode *)((char *)find_result.node + required_space);
        new_node->block_size = remaining;
        p_free_list_node_insert(&free_list->head, find_result.node, new_node);
    }

    p_free_list_node_remove(&free_list->head, find_result.prev_node, find_result.node);

    pFreeListAllocationHeader *allocation_header = (pFreeListAllocationHeader *)(
        (char *)find_result.node + alignment_padding
    );
    allocation_header->block_size = required_space;
    allocation_header->padding = alignment_padding;

    free_list->total_allocated += required_space;

    void *result = (void *)((char *)allocation_header + sizeof(pFreeListAllocationHeader));
    return result;
}

void *p_free_list_alloc(pFreeList *free_list, size_t size) {
    void *result = p_free_list_alloc_align(free_list, size, P_DEFAULT_MEMORY_ALIGNMENT);
    return result;
}

void p_free_list_free(pFreeList *free_list, void *ptr) {
    if (ptr == NULL) {
        return;
    }

    pFreeListAllocationHeader *allocation_header = (pFreeListAllocationHeader *)(
        (char *)ptr - sizeof(pFreeListAllocationHeader)
    );
    pFreeListNode *free_node = (pFreeListNode *)allocation_header;
    free_node->block_size = allocation_header->block_size + allocation_header->padding;
    free_node->next = NULL;

    pFreeListNode *node = free_list->head;
    pFreeListNode *prev_node = NULL;
    while (node != NULL) {
        if (ptr < (void*)node) {
            p_free_list_node_insert(&free_list->head, prev_node, free_node);
            break;
        }
        prev_node = node;
        node = node->next;
    }

    free_list->total_allocated -= free_node->block_size;

    p_free_list_coalescence(free_list, prev_node, free_node);
}

pFreeListFindResult p_free_list_find_first(pFreeList *free_list, size_t size, size_t alignment) {
    pFreeListNode *node = free_list->head;
    pFreeListNode *prev_node = NULL;
    size_t padding = 0;
    while (node != NULL) {
        padding = p_calc_padding_with_header(
            (uintptr_t)node,
            (uintptr_t)alignment,
            sizeof(pFreeListAllocationHeader)
        );
        size_t required_space = size + padding;
        if (node->block_size >= required_space) {
            break;
        }
        prev_node = node;
        node = node->next;
    }

    pFreeListFindResult result = {0};
    if (node != NULL) {
        result.node = node;
        result.padding = padding;
        result.prev_node = prev_node;
    }

    return result;
}

pFreeListFindResult p_free_list_find_best(pFreeList *free_list, size_t size, size_t alignment) {
    pFreeListNode *node = free_list->head;
    pFreeListNode *prev_node = NULL;
    pFreeListNode *best_node = NULL;
    pFreeListNode *prev_to_best_node = NULL;
    size_t smallest_diff = ~(size_t)0;
    size_t padding = 0;
    while (node != NULL) {
        padding = p_calc_padding_with_header(
            (uintptr_t)node,
            (uintptr_t)alignment,
            sizeof(pFreeListAllocationHeader)
        );
        size_t required_space = size + padding;
        bool block_big_enough = (node->block_size >= required_space);
        bool new_best_node = (node->block_size - required_space < smallest_diff);
        if (block_big_enough && new_best_node) {
            best_node = node;
            prev_to_best_node = prev_node;
        }
        prev_node = node;
        node = node->next;
    }
    pFreeListFindResult result = {0};
    if (best_node != NULL) {
        result.node = best_node;
        result.padding = padding;
        result.prev_node = prev_to_best_node;
    }
    return result;
}

void p_free_list_coalescence(pFreeList *free_list, pFreeListNode *prev_node, pFreeListNode *free_node) {
    if (free_node->next != NULL && (void *)((char *)free_node + free_node->block_size) == free_node->next) {
        free_node->block_size += free_node->next->block_size;
        p_free_list_node_remove(&free_list->head, free_node, free_node->next);
    }

    if (prev_node != NULL && prev_node->next != NULL && (void *)((char *)prev_node + prev_node->block_size) == free_node) {
        prev_node->block_size += free_node->block_size;
        p_free_list_node_remove(&free_list->head, prev_node, free_node);
    }
}

void p_free_list_node_insert(pFreeListNode **prev_head, pFreeListNode *prev_node, pFreeListNode *new_node) {
    if (prev_node == NULL) {
        if (*prev_head != NULL) {
            new_node->next = *prev_head;
        } else {
            *prev_head = new_node;
        }
    } else {
        if (prev_node->next == NULL) {
            prev_node->next = new_node;
            new_node->next = NULL;
        } else {
            new_node->next = prev_node->next;
            prev_node->next = new_node;
        }
    }
}

void p_free_list_node_remove(pFreeListNode **prev_head, pFreeListNode *prev_node, pFreeListNode *rem_node) {
    if (prev_node == NULL) {
        *prev_head = rem_node->next;
    } else {
        prev_node = rem_node->next;
    }
}



#endif // P_CORE_IMPLEMENTATION
