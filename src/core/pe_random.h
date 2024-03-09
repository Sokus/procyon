#ifndef PE_RANDOM_H_HEADER_GUARD
#define PE_RANDOM_H_HEADER_GUARD

#include <stdint.h>

typedef struct peRandom {
	uint32_t state;
} peRandom;

peRandom pe_random_from_seed(uint32_t seed);
peRandom pe_random_from_time(void);
uint32_t pe_random_uint32(peRandom *random);

#endif // PE_RANDOM_H_HEADER_GUARD