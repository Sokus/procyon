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
uint16_t pe_color_to_5650(peColor color);
uint32_t pe_color_to_8888(peColor color);

#define PE_COLOR_WHITE (peColor){ 255, 255, 255, 255 }
#define PE_COLOR_GRAY  (peColor){ 127, 127, 127, 255 }
#define PE_COLOR_RED   (peColor){ 255,   0,   0, 255 }
#define PE_COLOR_GREEN (peColor){   0, 255,   0, 255 }
#define PE_COLOR_BLUE  (peColor){   0,   0, 255, 255 }
#define PE_COLOR_BLACK (peColor){   0,   0,   0,   0 }

#if defined(PSP)
typedef struct peTexture {
	void *data;
	int width;
	int height;
	int power_of_two_width;
	int power_of_two_height;
	bool vram;
	int format;
} peTexture;

peTexture pe_texture_create(void *data, int width, int height, int format, bool vram);
void pe_texture_destroy(peTexture texture);
void pe_texture_bind(peTexture texture);
void pe_texture_unbind(void);
#endif

void pe_graphics_init(int window_width, int window_height, const char *window_name);
void pe_graphics_shutdown(void);
void pe_graphics_frame_begin(void);
void pe_graphics_frame_end(bool vsync);

int pe_screen_width(void);
int pe_screen_height(void);

void pe_clear_background(peColor color);

#endif // PE_GRAPHICS_H