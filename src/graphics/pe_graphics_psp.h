#ifndef PE_GRAPHICS_PSP_H_HEADER_GUARD
#define PE_GRAPHICS_PSP_H_HEADER_GUARD

#include <stdbool.h>
#include <stdint.h>

int p_pixel_bytes(int count, int format);
unsigned int bytes_per_pixel(unsigned int psm);
unsigned int closest_greater_power_of_two(unsigned int value);
void swizzle_fast(void *out, void *in, int width, int height);

#endif // PE_GRAPHICS_PSP_H_HEADER_GUARD