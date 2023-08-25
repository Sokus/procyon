#ifndef PE_PSP_GU_H
#define PE_PSP_GU_H

#include "pe_core.h"

#include <stdbool.h>

#define PSP_BUFF_W (512)
#define PSP_SCREEN_W (480)
#define PSP_SCREEN_H (272)

extern unsigned int __attribute__((aligned(16))) list[262144];

void pe_gu_init(void);

peAllocator pe_edram_allocator(void);

unsigned int bytes_per_pixel(unsigned int psm);

#endif // PE_PSP_GU_H