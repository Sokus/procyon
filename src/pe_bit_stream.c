#include "pe_bit_stream.h"
#include "pe_core.h"

#include <stdbool.h>
#include <stdint.h>

peBitStream pe_create_write_stream(void *buffer, size_t buffer_size) {
    peBitStream write_stream = {0};
    write_stream.mode = peBitStream_Write;
    int num_words = (int)buffer_size / 4;
    write_stream.num_words = num_words;
    write_stream.num_bits = num_words * 32;
    write_stream.data = buffer;
    return write_stream;
}

peBitStream pe_create_read_stream(void *buffer, size_t buffer_size, size_t data_size) {
    PE_ASSERT(buffer_size >= 4*((data_size+3)/4));
    peBitStream read_stream = {0};
    read_stream.mode = peBitStream_Read;
    int num_words = (((int)data_size + 3)/4);
    read_stream.num_words = num_words;
    read_stream.num_bits = num_words * 32;
    read_stream.data = buffer;
    return read_stream;
}

peBitStream pe_create_measure_stream(size_t buffer_size) {
    peBitStream measure_stream = {0};
    measure_stream.mode = peBitStream_Measure;
    int num_words = (int)buffer_size / 4;
    measure_stream.num_words = num_words;
    measure_stream.num_bits = num_words * 32;
    return measure_stream;
}

void pe_bit_stream_flush_bits(peBitStream *bit_stream) {
    PE_ASSERT(bit_stream->mode == peBitStream_Write);
    PE_ASSERT(bit_stream->scratch_bits >= 0);
    if (bit_stream->scratch_bits > 0) {
        PE_ASSERT(bit_stream->word_index < bit_stream->num_words);
        bit_stream->data[bit_stream->word_index] = (uint32_t)(bit_stream->scratch & 0xFFFFFFFF);
        bit_stream->word_index += 1;
        bit_stream->scratch >>= 32;
        bit_stream->scratch_bits = PE_MAX(bit_stream->scratch_bits - 32, 0);
    }
}

//
// SERIALIZATION
//

static peSerializationError pe_serialize_bits_write_stream(peBitStream *bs, uint32_t *value, int bits);
static peSerializationError pe_serialize_bits_read_stream(peBitStream *bs, uint32_t *value, int bits);
static peSerializationError pe_serialize_bits_measure_stream(peBitStream *bs, uint32_t *value, int bits);

peSerializationError pe_serialize_bits(peBitStream *bs, uint32_t *value, int bits) {
    switch (bs->mode) {
        case peBitStream_Write: return pe_serialize_bits_write_stream(bs, value, bits);
        case peBitStream_Read: return pe_serialize_bits_read_stream(bs, value, bits);
        case peBitStream_Measure: return pe_serialize_bits_measure_stream(bs, value, bits);
    }
    return peSerializationError_InvalidBitStreamMode;
}

peSerializationError pe_serialize_bytes(peBitStream *bs, void *bytes, size_t num_bytes) {
    PE_ASSERT(num_bytes > 0);
    peSerializationError error = peSerializationError_None;
    error = pe_serialize_align(bs);
    if (error) return error;

    if (bs->mode == peBitStream_Measure) {
        bs->bits_processed += (int)num_bytes * 8;
        return peSerializationError_None;
    }
    PE_ASSERT(bs->mode == peBitStream_Write || bs->mode == peBitStream_Read);
    PE_ASSERT(pe_bit_stream_align_offset(bs) == 0);
    PE_ASSERT_MSG(pe_bit_stream_bits_remaining(bs) >= (num_bytes * 8), "(%d >= %d)\n", pe_bit_stream_bits_remaining(bs), num_bytes * 8);

    int num_head_bytes = (4 - (bs->bits_processed % 32) / 8) % 4;
    num_head_bytes = PE_MIN(num_head_bytes, (int)num_bytes);
    for (int i = 0; i < num_head_bytes; i += 1) {
        error = pe_serialize_u8(bs, (uint8_t*)bytes + i);
        if (error) return error;
    }
    if (num_head_bytes == num_bytes) {
        return peSerializationError_None;
    }

    if (bs->mode == peBitStream_Write) {
        pe_bit_stream_flush_bits(bs);
    }

    int num_middle_bytes = (((int)num_bytes - num_head_bytes) / 4) * 4;
    if (num_middle_bytes > 0) {
        PE_ASSERT(bs->bits_processed % 32 == 0);
        uint32_t *middle_words = (uint32_t*)((uint8_t*)bytes + num_head_bytes);
        int num_middle_words = num_middle_bytes / 4;
        switch (bs->mode) {
            case peBitStream_Write: {
                for (int i = 0; i < num_middle_words; i += 1) {
                    bs->data[bs->word_index + i] = pe_host_to_network_u32(middle_words[i]);
                }
            } break;
            case peBitStream_Read: {
                for (int i = 0; i < num_middle_words; i += 1) {
                    middle_words[i] = pe_network_to_host_u32(bs->data[bs->word_index + i]);
                }
            } break;
        }
        bs->bits_processed += num_middle_bytes * 8;
        bs->word_index += num_middle_bytes / 4;
        bs->scratch = 0;
    }

    PE_ASSERT(pe_bit_stream_align_offset(bs) == 0);

    int num_tail_bytes = (int)num_bytes - num_head_bytes - num_middle_bytes;
    PE_ASSERT(num_tail_bytes >= 0 && num_tail_bytes < 4);
    int tail_bytes_start = num_head_bytes + num_middle_bytes;
    for (int i = 0; i < num_tail_bytes; i += 1) {
        error = pe_serialize_u8(bs, (uint8_t*)bytes + tail_bytes_start + i);
        if (error) return error;
    }
    PE_ASSERT(num_head_bytes + num_middle_bytes + num_tail_bytes == num_bytes);
    return peSerializationError_None;
}

peSerializationError pe_serialize_align(peBitStream *bs) {
    peSerializationError error = peSerializationError_None;
    int align_offset = pe_bit_stream_align_offset(bs);
    if (align_offset > 0) {
        uint32_t value = 0;
        if (error = pe_serialize_bits(bs, &value, align_offset)) return error;
        if (bs->mode == peBitStream_Write || bs->mode == peBitStream_Read) {
            PE_ASSERT(bs->bits_processed % 8 == 0);
        }
    }
    return error;
}

peSerializationError pe_serialize_check(peBitStream *bs, char *str) {
    PE_UNIMPLEMENTED();
    return peSerializationError_InvalidBitStreamMode;
}

unsigned int pe_most_significant_bit(unsigned int value) {
    unsigned int bit = 0;
    while (value >>= 1) {
        bit += 1;
    }
    return bit;
}

int pe_bits_required_for_range_int(int min_value, int max_value) {
    PE_ASSERT(min_value < max_value);
    uint32_t max_relative_value = (uint32_t)((int64_t)max_value - (int64_t)min_value);
    unsigned int bits = pe_most_significant_bit(max_relative_value) + 1;
    return bits;
}

int pe_bits_required_for_range_uint(unsigned min_value, unsigned max_value) {
    PE_ASSERT(min_value < max_value);
    unsigned int bits = pe_most_significant_bit(max_value - min_value) + 1;
    return bits;
}

peSerializationError pe_serialize_range_int(peBitStream *bs, int *value, int min_value, int max_value) {
    PE_ASSERT(min_value < max_value);
    int bits = pe_bits_required_for_range_int(min_value, max_value);

    switch (bs->mode) {
        case peBitStream_Write: {
            PE_ASSERT(*value >= min_value);
            PE_ASSERT(*value <= max_value);
            uint32_t relative_value = (uint32_t)(*value - min_value);
            return pe_serialize_bits(bs, &relative_value, bits);
        } break;
        case peBitStream_Read: {
            uint32_t relative_value;
            peSerializationError error = pe_serialize_bits(bs, &relative_value, bits);
            if (error) return error;
            *value = (int)((int64_t)min_value + (int64_t)relative_value);
            if (*value < min_value || *value > max_value) {
                return peSerializationError_ValueOutOfRange;
            }
            return error;
        } break;
        case peBitStream_Measure: {
            return pe_serialize_bits(bs, NULL, bits);
        } break;
    }
    return peSerializationError_InvalidBitStreamMode;
}

peSerializationError pe_serialize_range_uint(peBitStream *bs, unsigned *value, unsigned min_value, unsigned max_value) {
    PE_ASSERT(min_value < max_value);
    int bits = pe_bits_required_for_range_uint(min_value, max_value);

    switch (bs->mode) {
        case peBitStream_Write: {
            PE_ASSERT(*value >= min_value);
            PE_ASSERT(*value <= max_value);
            uint32_t relative_value = *value - min_value;
            return pe_serialize_bits(bs, &relative_value, bits);
        } break;
        case peBitStream_Read: {
            uint32_t relative_value;
            peSerializationError error = pe_serialize_bits(bs, &relative_value, bits);
            if (error) return error;
            *value = (unsigned)((uint32_t)min_value + relative_value);
            if (*value < min_value || *value > max_value) {
                return peSerializationError_ValueOutOfRange;
            }
            return error;
        } break;
        case peBitStream_Measure: {
            return pe_serialize_bits(bs, NULL, bits);
        } break;
    }
    return peSerializationError_InvalidBitStreamMode;
}

// TODO: pe_serialize_bit_set
// ...

peSerializationError pe_serialize_u32(peBitStream *bs, uint32_t *value) {
    return pe_serialize_bits(bs, value, 32);
}

peSerializationError pe_serialize_u16(peBitStream *bs, uint16_t *value) {
    uint32_t raw_value;
    if (bs->mode == peBitStream_Write) raw_value = (uint32_t)(*value);
    peSerializationError error = pe_serialize_bits(bs, &raw_value, 16);
    if (error) return error;
    if (bs->mode == peBitStream_Read) *value = (uint16_t)raw_value;
    return error;
}

peSerializationError pe_serialize_u8(peBitStream *bs, uint8_t *value) {
    uint32_t raw_value;
    if (bs->mode == peBitStream_Write) raw_value = (uint32_t)(*value);
    peSerializationError error = pe_serialize_bits(bs, &raw_value, 8);
    if (error) return error;
    if (bs->mode == peBitStream_Read) *value = (uint8_t)raw_value;
    return error;
}

peSerializationError pe_serialize_bool(peBitStream *bs, bool *value) {
    uint32_t raw_value;
    if (bs->mode == peBitStream_Write) raw_value = *value ? 0x1 : 0x0;
    peSerializationError error = pe_serialize_bits(bs, &raw_value, 1);
    if (error) return error;
    if (bs->mode == peBitStream_Read) *value = raw_value == 0x1 ? true : false;
    return peSerializationError_None;
}

peSerializationError pe_serialize_float(peBitStream *bs, float *value) {
    return pe_serialize_bits(bs, (void*)value, 32);
}

//
// SERIALIZATION INTERNALS
//

static peSerializationError pe_serialize_bits_write_stream(peBitStream *bs, uint32_t *value, int bits) {
    PE_ASSERT(bs->mode == peBitStream_Write);
    PE_ASSERT(bits > 0);
    PE_ASSERT(bits <= 32);
    PE_ASSERT(!pe_bit_stream_would_overflow(bs, bits));

    if (pe_bit_stream_would_overflow(bs, bits)) {
        return peSerializationError_Overflow;
    }

    uint64_t write_value = (uint64_t)(*value) & ((1ULL << bits) - 1);
    bs->scratch |= write_value << bs->scratch_bits;
    bs->scratch_bits += bits;

    if (bs->scratch_bits >= 32) {
        PE_ASSERT(bs->word_index < bs->num_words);
        bs->data[bs->word_index] = pe_host_to_network_u32((uint32_t)(bs->scratch & 0xFFFFFFFF));
        bs->scratch >>= 32;
        bs->scratch_bits -= 32;
        bs->word_index += 1;
    }

    bs->bits_processed += bits;

    return peSerializationError_None;
}

static peSerializationError pe_serialize_bits_read_stream(peBitStream *bs, uint32_t *value, int bits) {
    PE_ASSERT(bs->mode == peBitStream_Read);
    PE_ASSERT(bits > 0);
    PE_ASSERT(bits <= 32);
    PE_ASSERT(bs->scratch_bits >= 0);
    PE_ASSERT(bs->scratch_bits <= 64);

    if (pe_bit_stream_would_overflow(bs, bits)) {
        return peSerializationError_Overflow;
    }

    if (bs->scratch_bits < bits) {
        PE_ASSERT(bs->word_index < bs->num_words);
        bs->scratch |= (uint64_t)(pe_network_to_host_u32(bs->data[bs->word_index])) << bs->scratch_bits;
        bs->scratch_bits += 32;
        bs->word_index += 1;
    }

    PE_ASSERT(bs->scratch_bits >= bits);
    *value = (uint32_t)(bs->scratch & ((1ULL << bits) - 1));
    bs->bits_processed += bits;
    bs->scratch >>= bits;
    bs->scratch_bits -= bits;

    return peSerializationError_None;
}

static peSerializationError pe_serialize_bits_measure_stream(peBitStream *bs, uint32_t *value, int bits) {
    PE_ASSERT(bits > 0);
    PE_ASSERT(bits <= 32);
    PE_ASSERT(bs->mode == peBitStream_Measure);
    bs->bits_processed += bits;
    return peSerializationError_None;
}

