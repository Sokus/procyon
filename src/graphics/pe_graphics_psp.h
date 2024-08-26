#ifndef PE_GRAPHICS_PSP_H_HEADER_GUARD
#define PE_GRAPHICS_PSP_H_HEADER_GUARD

#include <stdbool.h>

unsigned int bytes_per_pixel(unsigned int psm);
unsigned int closest_greater_power_of_two(unsigned int value);
void copy_texture_data(void *dest, const void *src, const int pW, const int width, const int height);
void swizzle_fast(u8 *out, const u8 *in, unsigned int width, unsigned int height);

#endif // PE_GRAPHICS_PSP_H_HEADER_GUARD