#ifndef P_ASSET_LOADER_HEADER_GUARD
#define P_ASSET_LOADER_HEADER_GUARD

#include <stdint.h>

#define P_MAX_ASSET_COUNT 32

typedef enum pAssetType {
    pAssetType_Model,
    pAssetType_Texture,
    PAssetType_Count,
} pAssetType;

typedef struct pAsset {
    void *_;
} pAsset;

typedef struct pAssetLoader {
    pAsset *assets;
    uint16_t *free_indices;
    int free_indices_count;
} pAssetLoader;

void p_asset_loader_init(void);



#endif // P_ASSET_LOADER_HEADER_GUARD
