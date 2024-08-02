#ifndef PE_GRAPHICS_PSP_H_HEADER_GUARD
#define PE_GRAPHICS_PSP_H_HEADER_GUARD

#include <stdbool.h>

typedef struct peArena peArena;
typedef struct peTexture peTexture;
peTexture pe_texture_create(peArena *temp_arena, void *data, int width, int height, int format);
void pe_texture_bind(peTexture texture);
void pe_texture_bind_default(void);

unsigned int bytes_per_pixel(unsigned int psm);
unsigned int closest_greater_power_of_two(unsigned int value);
void copy_texture_data(void *dest, const void *src, const int pW, const int width, const int height);
void swizzle_fast(u8 *out, const u8 *in, unsigned int width, unsigned int height);

#endif // PE_GRAPHICS_PSP_H_HEADER_GUARD