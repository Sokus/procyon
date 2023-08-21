#ifndef PE_GRAPHICS_H
#define PE_GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

typedef union peColor {
    struct { uint8_t r, g, b, a; };
    struct { uint32_t rgba; };
} peColor;

union HMM_Vec4;
union HMM_Vec4 pe_color_to_vec4(peColor color);

#define PE_COLOR_WHITE (peColor){ 255, 255, 255, 255 }

void pe_graphics_init(int window_width, int window_height, const char *window_name);

#endif // PE_GRAPHICS_H