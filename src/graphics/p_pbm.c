#include "p_pbm.h"
#include "core/p_assert.h"

#include <stdint.h>

static void *pbm_push_data(void **data, size_t size, size_t *contents_left) {
    P_ASSERT(size <= *contents_left);
    void *result = NULL;
    if (size <= *contents_left) {
        result = *data;
        *data = (uint8_t*)(*data) + size;
        *contents_left -= size;
    }
    return result;
}

static int pbm_color_format_bytes(pbmColorFormat color_format) {
    P_ASSERT(color_format >= 0);
    P_ASSERT(color_format < pbmColorFormat_Count);
    switch (color_format) {
        case pbmColorFormat_5650: return 2; break;
        case pbmColorFormat_5551: return 2; break;
        case pbmColorFormat_4444: return 2; break;
        case pbmColorFormat_8888: return 4; break;
        default: break;
    }
    return 0;
}

bool pbm_parse(void *data, size_t size, pbmFile *result) {
    void *ptr = data;
    size_t contents_left = size;

    pbmStaticInfo *static_info = pbm_push_data(&ptr, sizeof(pbmStaticInfo), &contents_left);
    if (!static_info) return false;

    void *palette;
    {
        int palette_length = (int)static_info->palette_length;
        int palette_format_bytes = pbm_color_format_bytes(static_info->palette_format);
        size_t palette_size = (size_t)(palette_length * palette_format_bytes);
        palette = pbm_push_data(&ptr, palette_size, &contents_left);
        if (!palette) return false;
    }

    void *index;
    {
        int width = static_info->width;
        int height = static_info->height;
        int bits_per_pixel = static_info->bits_per_pixel;
        size_t index_size = (size_t)((width * height * bits_per_pixel) / 8);
        index = pbm_push_data(&ptr, index_size, &contents_left);
        if (!index) return false;
    }

    *result = (pbmFile){
        .static_info = static_info,
        .palette = palette,
        .index = index,
    };
    return true;
}