#ifndef PE_BIT_STREAM_H
#define PE_BIT_STREAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum peBitStreamMode {
    peBitStream_Write,
    peBitStream_Read,
    peBitStream_Measure,
} peBitStreamMode;

typedef struct peBitStream {
    peBitStreamMode mode;
    uint32_t *data;
    int num_bits;
    int num_words;
    uint64_t scratch;
    int scratch_bits;
    int word_index;
    int bits_processed;
} peBitStream;

typedef enum peSerializationError {
    peSerializationError_None,
    peSerializationError_InvalidBitStreamMode,
    peSerializationError_Overflow,
    peSerializationError_InvalidCheckValue,
    peSerializationError_ValueOutOfRange,
} peSerializationError;

peBitStream pe_create_write_stream(void *buffer, size_t buffer_size);
peBitStream pe_create_read_stream(void *buffer, size_t buffer_size, size_t data_size);
peBitStream pe_create_measure_stream(size_t buffer_size);

bool pe_bit_stream_would_overflow(peBitStream *bit_stream, int bits);
int  pe_bit_stream_bits_total(peBitStream *bit_stream);
int  pe_bit_stream_bytes_total(peBitStream *bit_stream);
int  pe_bit_stream_bits_processed(peBitStream *bit_stream);
int  pe_bit_stream_bytes_processed(peBitStream *bit_stream);
int  pe_bit_stream_bits_remaining(peBitStream *bit_stream);
int  pe_bit_stream_bytes_remaining(peBitStream *bit_stream);
int  pe_bit_stream_align_offset(peBitStream *bit_stream);
void pe_bit_stream_flush_bits(peBitStream *bit_stream);


//
// SERIALIZATION
//

uint32_t pe_reverse_bytes_u32(uint32_t value);
uint32_t pe_reverse_bytes_u16(uint16_t value);
uint32_t pe_host_to_network_u32(uint32_t value);
uint32_t pe_network_to_host_u32(uint32_t value);
uint16_t pe_host_to_network_u16(uint16_t value);
uint16_t pe_network_to_host_u16(uint16_t value);
peSerializationError pe_serialize_bits(peBitStream *bs, uint32_t *value, int bits);
peSerializationError pe_serialize_bytes(peBitStream *bs, void *bytes, size_t num_bytes);
peSerializationError pe_serialize_align(peBitStream *bs);
peSerializationError pe_serialize_check(peBitStream *bs, char *str);
unsigned int pe_most_significant_bit(unsigned int value);
int pe_bits_required_for_range_int(int min_value, int max_value);
int pe_bits_required_for_range_uint(unsigned min_value, unsigned max_value);
peSerializationError pe_serialize_range_int(peBitStream *bs, int *value, int min_value, int max_value);
peSerializationError pe_serialize_range_uint(peBitStream *bs, unsigned *value, unsigned min_value, unsigned max_value);
// TODO: pe_serialize_bit_set?
peSerializationError pe_serialize_u32(peBitStream *bs, uint32_t *value);
peSerializationError pe_serialize_u16(peBitStream *bs, uint16_t *value);
peSerializationError pe_serialize_u8(peBitStream *bs, uint8_t *value);
peSerializationError pe_serialize_bool(peBitStream *bs, bool *value);
peSerializationError pe_serialize_float(peBitStream *bs, float *value);
peSerializationError pe_serialize_enum(peBitStream *bs, void *value, int enum_value_count);


#if defined(_MSC_VER)
#include "pe_bit_stream.i"
#endif

#endif // PE_BIT_STREAM_H