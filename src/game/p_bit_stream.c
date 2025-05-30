#include "p_bit_stream.h"
#include "core/p_assert.h"

#include <stdbool.h>
#include <stdint.h>

pBitStream p_create_write_stream(void *buffer, size_t buffer_size) {
    pBitStream write_stream = {0};
    write_stream.mode = pBitStream_Write;
    int num_words = (int)buffer_size / 4;
    write_stream.num_words = num_words;
    write_stream.num_bits = num_words * 32;
    write_stream.data = buffer;
    return write_stream;
}

pBitStream p_create_read_stream(void *buffer, size_t buffer_size, size_t data_size) {
    P_ASSERT(buffer_size >= 4*((data_size+3)/4));
    pBitStream read_stream = {0};
    read_stream.mode = pBitStream_Read;
    int num_words = (((int)data_size + 3)/4);
    read_stream.num_words = num_words;
    read_stream.num_bits = num_words * 32;
    read_stream.data = buffer;
    return read_stream;
}

pBitStream p_create_measure_stream(size_t buffer_size) {
    pBitStream measure_stream = {0};
    measure_stream.mode = pBitStream_Measure;
    int num_words = (int)buffer_size / 4;
    measure_stream.num_words = num_words;
    measure_stream.num_bits = num_words * 32;
    return measure_stream;
}

void p_bit_stream_flush_bits(pBitStream *bit_stream) {
    P_ASSERT(bit_stream->mode == pBitStream_Write);
    P_ASSERT(bit_stream->scratch_bits >= 0);
    if (bit_stream->scratch_bits > 0) {
        P_ASSERT(bit_stream->word_index < bit_stream->num_words);
        bit_stream->data[bit_stream->word_index] = (uint32_t)(bit_stream->scratch & 0xFFFFFFFF);
        bit_stream->word_index += 1;
        bit_stream->scratch >>= 32;
        bit_stream->scratch_bits = P_MAX(bit_stream->scratch_bits - 32, 0);
    }
}

//
// SERIALIZATION
//

static pSerializationError p_serialize_bits_write_stream(pBitStream *bs, uint32_t *value, int bits);
static pSerializationError p_serialize_bits_read_stream(pBitStream *bs, uint32_t *value, int bits);
static pSerializationError p_serialize_bits_measure_stream(pBitStream *bs, uint32_t *value, int bits);

pSerializationError p_serialize_bits(pBitStream *bs, uint32_t *value, int bits) {
    switch (bs->mode) {
        case pBitStream_Write: return p_serialize_bits_write_stream(bs, value, bits);
        case pBitStream_Read: return p_serialize_bits_read_stream(bs, value, bits);
        case pBitStream_Measure: return p_serialize_bits_measure_stream(bs, value, bits);
    }
    return pSerializationError_InvalidBitStreamMode;
}

pSerializationError p_serialize_bytes(pBitStream *bs, void *bytes, size_t num_bytes) {
    P_ASSERT(num_bytes > 0);
    pSerializationError error = pSerializationError_None;
    error = p_serialize_align(bs);
    if (error) return error;

    if (bs->mode == pBitStream_Measure) {
        bs->bits_processed += (int)num_bytes * 8;
        return pSerializationError_None;
    }
    P_ASSERT(bs->mode == pBitStream_Write || bs->mode == pBitStream_Read);
    P_ASSERT(p_bit_stream_align_offset(bs) == 0);
    P_ASSERT_MSG(p_bit_stream_bits_remaining(bs) >= (num_bytes * 8), "(%d >= %d)\n", p_bit_stream_bits_remaining(bs), num_bytes * 8);

    int num_head_bytes = (4 - (bs->bits_processed % 32) / 8) % 4;
    num_head_bytes = P_MIN(num_head_bytes, (int)num_bytes);
    for (int i = 0; i < num_head_bytes; i += 1) {
        error = p_serialize_u8(bs, (uint8_t*)bytes + i);
        if (error) return error;
    }
    if (num_head_bytes == num_bytes) {
        return pSerializationError_None;
    }

    if (bs->mode == pBitStream_Write) {
        p_bit_stream_flush_bits(bs);
    }

    int num_middle_bytes = (((int)num_bytes - num_head_bytes) / 4) * 4;
    if (num_middle_bytes > 0) {
        P_ASSERT(bs->bits_processed % 32 == 0);
        uint32_t *middle_words = (uint32_t*)((uint8_t*)bytes + num_head_bytes);
        int num_middle_words = num_middle_bytes / 4;
        switch (bs->mode) {
            case pBitStream_Write: {
                for (int i = 0; i < num_middle_words; i += 1) {
                    bs->data[bs->word_index + i] = p_host_to_network_u32(middle_words[i]);
                }
            } break;
            case pBitStream_Read: {
                for (int i = 0; i < num_middle_words; i += 1) {
                    middle_words[i] = p_network_to_host_u32(bs->data[bs->word_index + i]);
                }
            } break;
        }
        bs->bits_processed += num_middle_bytes * 8;
        bs->word_index += num_middle_bytes / 4;
        bs->scratch = 0;
    }

    P_ASSERT(p_bit_stream_align_offset(bs) == 0);

    int num_tail_bytes = (int)num_bytes - num_head_bytes - num_middle_bytes;
    P_ASSERT(num_tail_bytes >= 0 && num_tail_bytes < 4);
    int tail_bytes_start = num_head_bytes + num_middle_bytes;
    for (int i = 0; i < num_tail_bytes; i += 1) {
        error = p_serialize_u8(bs, (uint8_t*)bytes + tail_bytes_start + i);
        if (error) return error;
    }
    P_ASSERT(num_head_bytes + num_middle_bytes + num_tail_bytes == num_bytes);
    return pSerializationError_None;
}

pSerializationError p_serialize_align(pBitStream *bs) {
    pSerializationError error = pSerializationError_None;
    int align_offset = p_bit_stream_align_offset(bs);
    if (align_offset > 0) {
        uint32_t value = 0;
        if (error = p_serialize_bits(bs, &value, align_offset)) return error;
        if (bs->mode == pBitStream_Write || bs->mode == pBitStream_Read) {
            P_ASSERT(bs->bits_processed % 8 == 0);
        }
    }
    return error;
}

pSerializationError p_serialize_check(pBitStream *bs, char *str) {
    P_UNIMPLEMENTED();
    return pSerializationError_InvalidBitStreamMode;
}

unsigned int p_most_significant_bit(unsigned int value) {
    unsigned int bit = 0;
    while (value >>= 1) {
        bit += 1;
    }
    return bit;
}

int p_bits_required_for_range_int(int min_value, int max_value) {
    P_ASSERT(min_value < max_value);
    uint32_t max_relative_value = (uint32_t)((int64_t)max_value - (int64_t)min_value);
    unsigned int bits = p_most_significant_bit(max_relative_value) + 1;
    return bits;
}

int p_bits_required_for_range_uint(unsigned min_value, unsigned max_value) {
    P_ASSERT(min_value < max_value);
    unsigned int bits = p_most_significant_bit(max_value - min_value) + 1;
    return bits;
}

pSerializationError p_serialize_range_int(pBitStream *bs, int *value, int min_value, int max_value) {
    P_ASSERT(min_value < max_value);
    int bits = p_bits_required_for_range_int(min_value, max_value);

    switch (bs->mode) {
        case pBitStream_Write: {
            P_ASSERT(*value >= min_value);
            P_ASSERT(*value <= max_value);
            uint32_t relative_value = (uint32_t)(*value - min_value);
            return p_serialize_bits(bs, &relative_value, bits);
        } break;
        case pBitStream_Read: {
            uint32_t relative_value;
            pSerializationError error = p_serialize_bits(bs, &relative_value, bits);
            if (error) return error;
            *value = (int)((int64_t)min_value + (int64_t)relative_value);
            if (*value < min_value || *value > max_value) {
                return pSerializationError_ValueOutOfRange;
            }
            return error;
        } break;
        case pBitStream_Measure: {
            return p_serialize_bits(bs, NULL, bits);
        } break;
    }
    return pSerializationError_InvalidBitStreamMode;
}

pSerializationError p_serialize_range_uint(pBitStream *bs, unsigned *value, unsigned min_value, unsigned max_value) {
    P_ASSERT(min_value < max_value);
    int bits = p_bits_required_for_range_uint(min_value, max_value);

    switch (bs->mode) {
        case pBitStream_Write: {
            P_ASSERT(*value >= min_value);
            P_ASSERT(*value <= max_value);
            uint32_t relative_value = *value - min_value;
            return p_serialize_bits(bs, &relative_value, bits);
        } break;
        case pBitStream_Read: {
            uint32_t relative_value;
            pSerializationError error = p_serialize_bits(bs, &relative_value, bits);
            if (error) return error;
            *value = (unsigned)((uint32_t)min_value + relative_value);
            if (*value < min_value || *value > max_value) {
                return pSerializationError_ValueOutOfRange;
            }
            return error;
        } break;
        case pBitStream_Measure: {
            return p_serialize_bits(bs, NULL, bits);
        } break;
    }
    return pSerializationError_InvalidBitStreamMode;
}

// TODO: p_serialize_bit_set
// ...

pSerializationError p_serialize_u32(pBitStream *bs, uint32_t *value) {
    return p_serialize_bits(bs, value, 32);
}

pSerializationError p_serialize_u16(pBitStream *bs, uint16_t *value) {
    uint32_t raw_value;
    if (bs->mode == pBitStream_Write) raw_value = (uint32_t)(*value);
    pSerializationError error = p_serialize_bits(bs, &raw_value, 16);
    if (error) return error;
    if (bs->mode == pBitStream_Read) *value = (uint16_t)raw_value;
    return error;
}

pSerializationError p_serialize_u8(pBitStream *bs, uint8_t *value) {
    uint32_t raw_value;
    if (bs->mode == pBitStream_Write) raw_value = (uint32_t)(*value);
    pSerializationError error = p_serialize_bits(bs, &raw_value, 8);
    if (error) return error;
    if (bs->mode == pBitStream_Read) *value = (uint8_t)raw_value;
    return error;
}

pSerializationError p_serialize_bool(pBitStream *bs, bool *value) {
    uint32_t raw_value;
    if (bs->mode == pBitStream_Write) raw_value = *value ? 0x1 : 0x0;
    pSerializationError error = p_serialize_bits(bs, &raw_value, 1);
    if (error) return error;
    if (bs->mode == pBitStream_Read) *value = raw_value == 0x1 ? true : false;
    return pSerializationError_None;
}

pSerializationError p_serialize_float(pBitStream *bs, float *value) {
    return p_serialize_bits(bs, (void*)value, 32);
}

pSerializationError p_serialize_enum(pBitStream *bs, void *value, int enum_value_count) {
    return p_serialize_range_int(bs, (int *)value, 0,  enum_value_count - 1);
}

//
// SERIALIZATION INTERNALS
//

static pSerializationError p_serialize_bits_write_stream(pBitStream *bs, uint32_t *value, int bits) {
    P_ASSERT(bs->mode == pBitStream_Write);
    P_ASSERT(bits > 0);
    P_ASSERT(bits <= 32);
    P_ASSERT(!p_bit_stream_would_overflow(bs, bits));

    if (p_bit_stream_would_overflow(bs, bits)) {
        return pSerializationError_Overflow;
    }

    uint64_t write_value = (uint64_t)(*value) & ((1ULL << bits) - 1);
    bs->scratch |= write_value << bs->scratch_bits;
    bs->scratch_bits += bits;

    if (bs->scratch_bits >= 32) {
        P_ASSERT(bs->word_index < bs->num_words);
        bs->data[bs->word_index] = p_host_to_network_u32((uint32_t)(bs->scratch & 0xFFFFFFFF));
        bs->scratch >>= 32;
        bs->scratch_bits -= 32;
        bs->word_index += 1;
    }

    bs->bits_processed += bits;

    return pSerializationError_None;
}

static pSerializationError p_serialize_bits_read_stream(pBitStream *bs, uint32_t *value, int bits) {
    P_ASSERT(bs->mode == pBitStream_Read);
    P_ASSERT(bits > 0);
    P_ASSERT(bits <= 32);
    P_ASSERT(bs->scratch_bits >= 0);
    P_ASSERT(bs->scratch_bits <= 64);

    if (p_bit_stream_would_overflow(bs, bits)) {
        return pSerializationError_Overflow;
    }

    if (bs->scratch_bits < bits) {
        P_ASSERT(bs->word_index < bs->num_words);
        bs->scratch |= (uint64_t)(p_network_to_host_u32(bs->data[bs->word_index])) << bs->scratch_bits;
        bs->scratch_bits += 32;
        bs->word_index += 1;
    }

    P_ASSERT(bs->scratch_bits >= bits);
    *value = (uint32_t)(bs->scratch & ((1ULL << bits) - 1));
    bs->bits_processed += bits;
    bs->scratch >>= bits;
    bs->scratch_bits -= bits;

    return pSerializationError_None;
}

static pSerializationError p_serialize_bits_measure_stream(pBitStream *bs, uint32_t *value, int bits) {
    P_ASSERT(bits > 0);
    P_ASSERT(bits <= 32);
    P_ASSERT(bs->mode == pBitStream_Measure);
    bs->bits_processed += bits;
    return pSerializationError_None;
}
