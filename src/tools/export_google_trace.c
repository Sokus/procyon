#include <stdio.h>
#include <stdlib.h>

#include "pe_core.h"
#include "pe_file_io.h"
#include "pe_time.h"

#include "pe_trace.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("usage: export_google_trace input.pt output.json\n");
        return 1;
    }

    peArena temp_arena;
    {
        size_t temp_arena_size = PE_MEGABYTES(256);
        pe_arena_init(&temp_arena, pe_heap_alloc(temp_arena_size), temp_arena_size);
    }

    char *input_path = argv[1];
    peFileContents input_file_contents = pe_file_read_contents(&temp_arena, input_path, false);
    if (input_file_contents.size == 0) {
        return 1;
    }

    char *output_path = argv[2];
    FILE *output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        return 1;
    }
    fprintf(output_file, "[\n");

    size_t input_file_contents_offset = 0;
    size_t input_file_contents_left = input_file_contents.size;

    while (input_file_contents_left > 0) {
        peTraceEventData *trace_event_data;
        char trace_event_name[PE_TRACE_EVENT_NAME_SIZE+1];

        if (sizeof(peTraceEventData) > input_file_contents_left) {
            printf("not enough content left for event data\n");
            break;
        }
        trace_event_data = (void*)((uint8_t*)input_file_contents.data + input_file_contents_offset);
        input_file_contents_offset += sizeof(peTraceEventData);
        input_file_contents_left -= sizeof(peTraceEventData);

        if (trace_event_data->name_length > input_file_contents_left) {
            printf("not enough content left for event name\n");
            break;
        }
        if (trace_event_data->name_length > PE_TRACE_EVENT_NAME_SIZE) {
            printf("name too long (%u)\n", trace_event_data->name_length);
            break;
        }
        void *trace_event_name_source = (uint8_t*)input_file_contents.data + input_file_contents_offset;
        memcpy(trace_event_name, trace_event_name_source, trace_event_data->name_length);
        trace_event_name[trace_event_data->name_length] = '\0';
        input_file_contents_offset += trace_event_data->name_length;
        input_file_contents_left -= trace_event_data->name_length;

        char trace_event_type_character;
        switch (trace_event_data->type) {
            case peTraceEvent_Begin: trace_event_type_character = 'B'; break;
            case peTraceEvent_End: trace_event_type_character = 'E'; break;
            case peTraceEvent_Complete: trace_event_type_character = 'X'; break;
            default: trace_event_type_character = '?'; break;
        }
        if (trace_event_type_character == '?') {
            printf("%llu: unknown event type: %d\n", input_file_contents_offset, trace_event_data->type);
        }

        fprintf(
            output_file,
            "\t{\"pid\":0,\"name\":\"%s\",\"ph\":\"%c\",\"ts\":%f,\"dur\":%f},\n",
            trace_event_name,
            trace_event_type_character,
            pe_time_us(trace_event_data->timestamp),
            pe_time_us(trace_event_data->duration)
        );
    }

    fprintf(output_file, "]");

    fclose(output_file);

    return 0;
}