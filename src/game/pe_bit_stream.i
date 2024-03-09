#include "core/pe_core.h"

#include <stdbool.h>
#include <stdint.h>

PE_INLINE bool pe_bit_stream_would_overflow(peBitStream *bit_stream, int bits) {
    return bit_stream->bits_processed + bits > bit_stream->num_bits;
}

PE_INLINE int  pe_bit_stream_bits_total(peBitStream *bit_stream) {
    return bit_stream->num_bits;
}

PE_INLINE int pe_bit_stream_bytes_total(peBitStream *bit_stream) {
    return bit_stream->num_bits / 8;
}

PE_INLINE int pe_bit_stream_bits_processed(peBitStream *bit_stream) {
    return bit_stream->bits_processed;
}

PE_INLINE int pe_bit_stream_bytes_processed(peBitStream *bit_stream) {
    return (bit_stream->bits_processed + 7) / 8;
}

PE_INLINE int pe_bit_stream_bits_remaining(peBitStream *bit_stream) {
    return bit_stream->num_bits - bit_stream->bits_processed;
}

PE_INLINE int pe_bit_stream_bytes_remaining(peBitStream *bit_stream) {
    return pe_bit_stream_bytes_total(bit_stream) - pe_bit_stream_bytes_processed(bit_stream);
}

PE_INLINE int pe_bit_stream_align_offset(peBitStream *bit_stream) {
    return (8 - bit_stream->bits_processed % 8) % 8;
}

//
// SERIALIZATION
//

PE_INLINE uint32_t pe_reverse_bytes_u32(uint32_t value) {
    return ((value & 0x000000FF) << 24 | (value & 0x0000FF00) <<  8 |
            (value & 0x00FF0000) >>  8 | (value & 0xFF000000) >> 24);
}

PE_INLINE uint32_t pe_reverse_bytes_u16(uint16_t value) {
    return ((value & 0x00FF) << 8 | (value & 0xFF00) >> 8);
}

// IMPORTANT: These functions consider network order to be little endian
// because most modern processors are little endian. Least amount of work!

PE_INLINE uint32_t pe_host_to_network_u32(uint32_t value) {
#if PE_ARCH_BIG_ENDIAN
    return pe_reverse_bytes_u32(value);
#else
    return value;
#endif
}

PE_INLINE uint32_t pe_network_to_host_u32(uint32_t value) {
#if PE_ARCH_BIG_ENDIAN
    return pe_reverse_bytes_u32(value);
#else
    return value;
#endif
}

PE_INLINE uint16_t pe_host_to_network_u16(uint16_t value) {
#if PE_ARCH_BIG_ENDIAN
    return pe_reverse_bytes_u16(value);
#else
    return value;
#endif
}

PE_INLINE uint16_t pe_network_to_host_u16(uint16_t value) {
#if PE_ARCH_BIG_ENDIAN
    return pe_reverse_bytes_u16(value);
#else
    return value;
#endif
}