#ifndef P_RANDOM_HEADER_GUARD
#define P_RANDOM_HEADER_GUARD

#include <stdint.h>

typedef struct pRandom {
	uint32_t state;
} pRandom;

pRandom p_random_from_seed(uint32_t seed);
pRandom p_random_from_time(void);
uint32_t p_random_uint32(pRandom *random);

#endif // P_RANDOM_HEADER_GUARD
#if defined(P_CORE_IMPLEMENTATION) && !defined(P_RANDOM_IMPLEMENTATION_GUARD)
#define P_RANDOM_IMPLEMENTATION_GUARD

pRandom p_random_from_seed(uint32_t seed) {
    pRandom result = {
        .state = seed
    };
    return result;
}

static uint32_t p_jenkins_one_at_a_time_hash(const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = 0;

    for (size_t i = 0; i < size; i++) {
        hash += bytes[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

// TODO: the user should hash the time and pass it as seed
#if 0
pRandom p_random_from_time(void) {
    peTime time = p_time_local();
    uint32_t seed = p_jenkins_one_at_a_time_hash(&time, sizeof(time));
    return p_random_from_seed(seed);
}
#endif

uint32_t p_random_uint32(pRandom *random) {
    // https://en.wikipedia.org/w/index.php?title=Linear_congruential_generator&oldid=1169975619
    uint64_t a = 1664525;
    uint64_t c = 1013904223;
    uint64_t m = 4294967296; // 2^32
    uint32_t result = (uint32_t)((a * random->state + c) % m);
    random->state = result;
    return result;
}

#endif // P_CORE_IMPLEMENTATION
