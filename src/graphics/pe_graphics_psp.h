#ifndef PE_GRAPHICS_PSP_H_HEADER_GUARD
#define PE_GRAPHICS_PSP_H_HEADER_GUARD

#include <stdbool.h>
#include <stdint.h>

int p_pixel_bytes(int count, int format);
unsigned int closest_greater_power_of_two(unsigned int value);
void p_swizzle(void *out, void *in, int byte_width, int pow2_byte_width, int height);
#endif // PE_GRAPHICS_PSP_H_HEADER_GUARD
