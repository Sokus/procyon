#include "graphics/p_asset_loader.h"

#include <string.h>
#include <stdlib.h>

#include "core/p_assert.h"
#include "core/p_heap.h"

pAssetLoader p_asset_loader = {0};

void p_asset_loader_init(void) {
    pAssetLoader *p_al = &p_asset_loader;
    p_al->assets = p_heap_alloc(P_ASSET_LOADER_MAX_ASSET_COUNT * sizeof(pAsset));
    p_al->free_indices = p_heap_alloc(P_ASSET_LOADER_MAX_ASSET_COUNT * sizeof(uint16_t));
    p_al->free_indices_count = 0;
    for (int index = 0; index < MAX_ASSET_COUNT; index += 1) {
        int reverse_index = MAX_ASET_COUNT - index - 1;
        p_al->free_indices[p_al->free_indices_count] = (uint16_t)reverse_index;
        p_al->free_indices_count += 1;
    }
}

pAsset *p_asset_loader_make_asset(void) {
    pAssetLoader *p_al = &p_asset_loader;
    pAsset *result;
    P_ALWAYS(p_al->free_indices_count > 0) {
        p_al->free_indices_count -= 1;
        uint16_t slot = p_al->free_indices[p_al->free_indices_count];
        result = &p_al->assets[slot];
    }
}

