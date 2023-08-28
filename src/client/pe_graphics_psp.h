#ifndef PE_GRAPHICS_PSP_H_HEADER_GUARD
#define PE_GRAPHICS_PSP_H_HEADER_GUARD

#define PSP_BUFF_W (512)
#define PSP_SCREEN_W (480)
#define PSP_SCREEN_H (272)

extern unsigned int __attribute__((aligned(16))) list[262144];

void pe_graphics_init_psp(void);

struct peAllocator;
struct peAllocator pe_edram_allocator(void);

unsigned int bytes_per_pixel(unsigned int psm);

#endif // PE_GRAPHICS_PSP_H_HEADER_GUARD