#ifndef PE_GRAPHICS_PSP_H_HEADER_GUARD
#define PE_GRAPHICS_PSP_H_HEADER_GUARD

#include <stdbool.h>

#define PSP_BUFF_W (512)
#define PSP_SCREEN_W (480)
#define PSP_SCREEN_H (272)

extern unsigned int __attribute__((aligned(16))) list[262144];

void pe_graphics_init_psp(void);
void pe_graphics_shutdown_psp(void);

struct peTexture;
struct peArena;
struct peTexture pe_texture_create_psp(struct peArena *temp_arena, void *data, int width, int height, int format);
void pe_graphics_dynamic_draw_flush(void);
void pe_graphics_dynamic_draw_end_batch(void);

#endif // PE_GRAPHICS_PSP_H_HEADER_GUARD