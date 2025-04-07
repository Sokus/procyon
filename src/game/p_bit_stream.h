#ifndef P_BIT_STREAM_H
#define P_BIT_STREAM_H

#include "core/p_defines.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum pBitStreamMode {
    pBitStream_Write,
    pBitStream_Read,
    pBitStream_Measure,
} pBitStreamMode;

typedef struct pBitStream {
    pBitStreamMode mode;
    uint32_t *data;
    int num_bits;
    int num_words;
    uint64_t scratch;
    int scratch_bits;
    int word_index;
    int bits_processed;
} pBitStream;

typedef enum pSerializationError {
    pSerializationError_None,
    pSerializationError_InvalidBitStreamMode,
    pSerializationError_Overflow,
    pSerializationError_InvalidCheckValue,
    pSerializationError_ValueOutOfRange,
} pSerializationError;

pBitStream p_create_write_stream(void *buffer, size_t buffer_size);
pBitStream p_create_read_stream(void *buffer, size_t buffer_size, size_t data_size);
pBitStream p_create_measure_stream(size_t buffer_size);

static P_INLINE bool p_bit_stream_would_overflow(pBitStream *bit_stream, int bits);
static P_INLINE int  p_bit_stream_bits_total(pBitStream *bit_stream);
static P_INLINE int  p_bit_stream_bytes_total(pBitStream *bit_stream);
static P_INLINE int  p_bit_stream_bits_processed(pBitStream *bit_stream);
static P_INLINE int  p_bit_stream_bytes_processed(pBitStream *bit_stream);
static P_INLINE int  p_bit_stream_bits_remaining(pBitStream *bit_stream);
static P_INLINE int  p_bit_stream_bytes_remaining(pBitStream *bit_stream);
static P_INLINE int  p_bit_stream_align_offset(pBitStream *bit_stream);
void p_bit_stream_flush_bits(pBitStream *bit_stream);


//
// SERIALIZATION
//

static P_INLINE uint32_t p_reverse_bytes_u32(uint32_t value);
static P_INLINE uint32_t p_reverse_bytes_u16(uint16_t value);
static P_INLINE uint32_t p_host_to_network_u32(uint32_t value);
static P_INLINE uint32_t p_network_to_host_u32(uint32_t value);
static P_INLINE uint16_t p_host_to_network_u16(uint16_t value);
static P_INLINE uint16_t p_network_to_host_u16(uint16_t value);
pSerializationError p_serialize_bits(pBitStream *bs, uint32_t *value, int bits);
pSerializationError p_serialize_bytes(pBitStream *bs, void *bytes, size_t num_bytes);
pSerializationError p_serialize_align(pBitStream *bs);
pSerializationError p_serialize_check(pBitStream *bs, char *str);
unsigned int p_most_significant_bit(unsigned int value);
int p_bits_required_for_range_int(int min_value, int max_value);
int p_bits_required_for_range_uint(unsigned min_value, unsigned max_value);
pSerializationError p_serialize_range_int(pBitStream *bs, int *value, int min_value, int max_value);
pSerializationError p_serialize_range_uint(pBitStream *bs, unsigned *value, unsigned min_value, unsigned max_value);
// TODO: p_serialize_bit_set?
pSerializationError p_serialize_u32(pBitStream *bs, uint32_t *value);
pSerializationError p_serialize_u16(pBitStream *bs, uint16_t *value);
pSerializationError p_serialize_u8(pBitStream *bs, uint8_t *value);
pSerializationError p_serialize_bool(pBitStream *bs, bool *value);
pSerializationError p_serialize_float(pBitStream *bs, float *value);
pSerializationError p_serialize_enum(pBitStream *bs, void *value, int enum_value_count);

// IMPLEMENTATIONS

static P_INLINE bool p_bit_stream_would_overflow(pBitStream *bit_stream, int bits) {
    return bit_stream->bits_processed + bits > bit_stream->num_bits;
}

static P_INLINE int  p_bit_stream_bits_total(pBitStream *bit_stream) {
    return bit_stream->num_bits;
}

static P_INLINE int p_bit_stream_bytes_total(pBitStream *bit_stream) {
    return bit_stream->num_bits / 8;
}

static P_INLINE int p_bit_stream_bits_processed(pBitStream *bit_stream) {
    return bit_stream->bits_processed;
}

static P_INLINE int p_bit_stream_bytes_processed(pBitStream *bit_stream) {
    return (bit_stream->bits_processed + 7) / 8;
}

static P_INLINE int p_bit_stream_bits_remaining(pBitStream *bit_stream) {
    return bit_stream->num_bits - bit_stream->bits_processed;
}

static P_INLINE int p_bit_stream_bytes_remaining(pBitStream *bit_stream) {
    return p_bit_stream_bytes_total(bit_stream) - p_bit_stream_bytes_processed(bit_stream);
}

static P_INLINE int p_bit_stream_align_offset(pBitStream *bit_stream) {
    return (8 - bit_stream->bits_processed % 8) % 8;
}

//
// SERIALIZATION
//

P_INLINE uint32_t p_reverse_bytes_u32(uint32_t value) {
    return ((value & 0x000000FF) << 24 | (value & 0x0000FF00) <<  8 |
            (value & 0x00FF0000) >>  8 | (value & 0xFF000000) >> 24);
}

P_INLINE uint32_t p_reverse_bytes_u16(uint16_t value) {
    return ((value & 0x00FF) << 8 | (value & 0xFF00) >> 8);
}

// IMPORTANT: These functions consider network order to be little endian
// because most modern processors are little endian. Least amount of work!

static P_INLINE uint32_t p_host_to_network_u32(uint32_t value) {
    if (P_IS_LITTLE_ENDIAN) return value;
    else return p_reverse_bytes_u32(value);
}

static P_INLINE uint32_t p_network_to_host_u32(uint32_t value) {
    if (P_IS_LITTLE_ENDIAN) return value;
    else return p_reverse_bytes_u32(value);
}

static P_INLINE uint16_t p_host_to_network_u16(uint16_t value) {
    if (P_IS_LITTLE_ENDIAN) return value;
    else return p_reverse_bytes_u16(value);
}

static P_INLINE uint16_t p_network_to_host_u16(uint16_t value) {
    if (P_IS_LITTLE_ENDIAN) return value;
    else return p_reverse_bytes_u16(value);
}

#endif // P_BIT_STREAM_H
