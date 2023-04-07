#include "pe_bit_stream.h"
#include "pe_core.h"

#include <stdio.h>
#include <string.h>

typedef struct Foo {
    uint8_t values[8];
} Foo;

int main(int argc, char *arv[]) {

    Foo value;
    for (int i = 0; i < (int)PE_COUNT_OF(value.values); i += 1) {
        value.values[i] = (i * 147) % 53;
    }

    int bytes_written;
    uint8_t buffer[1024];
    {
        peBitStream bs = pe_create_write_stream(&buffer, sizeof(buffer));
        uint32_t garbage = 666;
        pe_serialize_bits(&bs, &garbage, 17);
        peSerializationError error = pe_serialize_bytes(&bs, &value, sizeof(Foo));
        if (error) printf("erorr: %d\n", error);
        pe_bit_stream_flush_bits(&bs);
        bytes_written = pe_bit_stream_bytes_processed(&bs);
    }

    printf("%d\n", bytes_written);

    Foo read_value = {0};
    {
        peBitStream bs = pe_create_read_stream(&buffer, sizeof(buffer), bytes_written);
        uint32_t garbage;
        pe_serialize_bits(&bs, &garbage, 17);
        peSerializationError error = pe_serialize_bytes(&bs, &read_value, sizeof(Foo));
        if (error) printf("erorr: %d\n", error);
    }

    PE_ASSERT(memcmp(&value, &read_value, sizeof(Foo)) == 0);




    return 0;
}