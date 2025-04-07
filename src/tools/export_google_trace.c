#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/p_defines.h"
#include "core/p_assert.h"
#include "core/p_heap.h"
#include "core/p_arena.h"
#include "core/p_file.h"
#include "core/p_time.h"

#include "utility/pe_trace.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("usage: export_google_trace traced_binary input.pt output.json\n");
        return 1;
    }

    pArena temp_arena;
    {
        size_t temp_arena_size = P_MEGABYTES(256);
        p_arena_init(&temp_arena, p_heap_alloc(temp_arena_size), temp_arena_size);
    }

    char *binary_path = argv[1];
    pFileContents binary_file_contents = p_file_read_contents(&temp_arena, binary_path, false);
    if (binary_file_contents.size == 0) {
        return 1;
    }

    char *input_path = argv[2];
    pFileContents input_file_contents = p_file_read_contents(&temp_arena, input_path, false);
    if (input_file_contents.size == 0) {
        return 1;
    }

    bool trace_reference_literal_found = false;
    size_t trace_reference_literal_offset;
    if (binary_file_contents.size >= sizeof(P_TRACE_REFERENCE_LITERAL)) {
        for (
            size_t binary_offset = 0;
            binary_offset <= binary_file_contents.size - sizeof(P_TRACE_REFERENCE_LITERAL);
            binary_offset += 1
        ) {
            int memcmp_result = memcmp(
                (uint8_t*)binary_file_contents.data + binary_offset,
                P_TRACE_REFERENCE_LITERAL,
                sizeof(P_TRACE_REFERENCE_LITERAL)
            );
            if (memcmp_result == 0) {
                printf("trace reference literal found at 0x%zx\n", binary_offset);
                trace_reference_literal_found = true;
                trace_reference_literal_offset = binary_offset;
            }
        }
    }

    char *output_path = argv[3];
    FILE *output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        return 1;
    }
    fprintf(output_file, "[\n");

    size_t input_file_contents_offset = 0;
    size_t input_file_contents_left = input_file_contents.size;

    pTraceHeader *trace_header;
    if (sizeof(pTraceHeader) > input_file_contents_left) {
        printf("not enough content left for trace header\n");
        input_file_contents_left = 0;
    }
    trace_header = (void*)((uint8_t*)input_file_contents.data + input_file_contents_offset);
    input_file_contents_offset += sizeof(pTraceHeader);
    input_file_contents_left -= sizeof(pTraceHeader);
    size_t reference_literal_address = (
        trace_header->address_bytes == 8
        ? trace_header->address_64
        : trace_header->address_32
    );

    printf("address_bytes: %d, address: %zx\n", trace_header->address_bytes, reference_literal_address);

    ptrdiff_t trace_address_correction = (ptrdiff_t)trace_reference_literal_offset - (ptrdiff_t)reference_literal_address;
    printf("address correction: %lld\n", trace_address_correction);

    while (input_file_contents_left > 0) {
        pTraceEventData *trace_event_data;

        if (sizeof(pTraceEventData) > input_file_contents_left) {
            printf("not enough content left for event data\n");
            break;
        }
        trace_event_data = (void*)((uint8_t*)input_file_contents.data + input_file_contents_offset);
        input_file_contents_offset += sizeof(pTraceEventData);
        input_file_contents_left -= sizeof(pTraceEventData);

        void *trace_event_name_address = (void*)(
            trace_header->address_bytes == 8
            ? trace_event_data->address_64
            : trace_event_data->address_32
        );
        size_t trace_event_name_offset = (size_t)((ptrdiff_t)trace_event_name_address + trace_address_correction);
        char *trace_event_name = (char*)binary_file_contents.data + trace_event_name_offset;

        fprintf(
            output_file,
            "\t{\"pid\":0,\"name\":\"%s\",\"ph\":\"X\",\"ts\":%f,\"dur\":%f},\n",
            trace_event_name,
            p_time_us(trace_event_data->timestamp),
            p_time_us(trace_event_data->duration)
        );
    }

    fprintf(output_file, "]");

    fclose(output_file);

    return 0;
}
