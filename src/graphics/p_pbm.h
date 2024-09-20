#ifndef PROCYON_BITMAP_FORMAT_HEADER_GUARD
#define PROCYON_BITMAP_FORMAT_HEADER_GUARD

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum pbmColorFormat {
    pbmColorFormat_5650,
    pbmColorFormat_5551,
    pbmColorFormat_4444,
    pbmColorFormat_8888,
    pbmColorFormat_Count,
} pbmColorFormat;

// 1. static info header
typedef struct pbmStaticInfo {
    char extension_magic[4]; // " PBM"
    int8_t palette_format; // pbmColorFormat
    uint8_t palette_length;
    int16_t width;
    int16_t height;
    int8_t bits_per_pixel; // 4 or 8
    uint8_t alignment;
} pbmStaticInfo;
// pbmStaticInfo static_info;

typedef struct pbmFile {
    pbmStaticInfo *static_info;
    void *palette;
    void *index;
} pbmFile;

bool pbm_parse(void *data, size_t size, pbmFile *result);

#endif // PROCYON_BITMAP_FORMAT_HEADER_GUARD