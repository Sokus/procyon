#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "platform/pe_time.h"
#include "core/pe_core.h"
#include "pe_trace.h"
#include "core/pe_file_io.h"

struct peTraceState {
    peFileHandle output_file;
    uint8_t data[2][PE_TRACE_DATA_BATCH_SIZE];
    size_t data_used[2];
    int current_data_index;
};
#if defined(PE_TRACE_ENABLED)
    struct peTraceState pe_trace_state = {0};
#endif

void pe_trace_init(void) {
#if defined(PE_TRACE_ENABLED)
    pe_file_open("./trace.pt", &pe_trace_state.output_file);
    peTraceHeader trace_header = {
        .address_bytes = sizeof(void*),
        .address = (void*)PE_TRACE_REFERENCE_LITERAL
    };
    pe_file_write_async(pe_trace_state.output_file, &trace_header, sizeof(peTraceHeader));
    bool should_wait;
    bool poll_success = pe_file_poll(pe_trace_state.output_file, &should_wait);
    if (poll_success && should_wait) {
        pe_file_wait(pe_trace_state.output_file);
    }
#endif
}

static void pe_trace_data_flush(void);

void pe_trace_shutdown(void) {
#if defined(PE_TRACE_ENABLED)
    pe_trace_data_flush();
    bool should_wait;
    bool poll_success = pe_file_poll(pe_trace_state.output_file, &should_wait);
    if (poll_success && should_wait) {
        pe_file_wait(pe_trace_state.output_file);
    }
    pe_file_close(pe_trace_state.output_file);
#endif
}

#if defined(PE_TRACE_ENABLED)

static void pe_trace_event_add(const char *name, uint64_t timestamp, uint64_t duration) {
    size_t trace_event_space_needed = sizeof(peTraceEventData);
    if (pe_trace_state.data_used[pe_trace_state.current_data_index] + trace_event_space_needed > PE_TRACE_DATA_BATCH_SIZE) {
        pe_trace_data_flush();
    }

    uint8_t *current_trace_data = pe_trace_state.data[pe_trace_state.current_data_index];
    size_t *current_trace_data_used = &pe_trace_state.data_used[pe_trace_state.current_data_index];

    peTraceEventData trace_event_data = {
        .timestamp = timestamp,
        .duration = duration,
        .address = (void*)name,
    };

    memcpy(current_trace_data + *current_trace_data_used, &trace_event_data, sizeof(trace_event_data));
    *current_trace_data_used += sizeof(trace_event_data);
}

peTraceMark pe_trace_mark_begin_internal(const char *name) {
    peTraceMark result = {
        .name = name,
        .timestamp = pe_time_now()
    };
    return result;
}

void pe_trace_mark_end_internal(peTraceMark trace_mark) {
    uint64_t trace_mark_duration = pe_time_since(trace_mark.timestamp);
    pe_trace_event_add(
        trace_mark.name,
        trace_mark.timestamp,
        trace_mark_duration
    );
}

static void pe_trace_data_flush(void) {
    if (pe_trace_state.data_used[pe_trace_state.current_data_index] > 0) {
        bool wait_for_write_async;
        bool file_poll_success = pe_file_poll(pe_trace_state.output_file, &wait_for_write_async);
        uint64_t write_async_wait_start;
        uint64_t write_async_wait_duration;
        if (file_poll_success && wait_for_write_async) {
            write_async_wait_start = pe_time_now();
            pe_file_wait(pe_trace_state.output_file);
            write_async_wait_duration = pe_time_since(write_async_wait_start);
        }

        pe_file_write_async(
            pe_trace_state.output_file,
            pe_trace_state.data[pe_trace_state.current_data_index],
            pe_trace_state.data_used[pe_trace_state.current_data_index]
        );
        pe_trace_state.data_used[pe_trace_state.current_data_index] = 0;
        pe_trace_state.current_data_index = (pe_trace_state.current_data_index + 1) % 2;

        if (file_poll_success && wait_for_write_async) {
            const char wait_async_func_name[] = "sceIoWaitAsync";
            pe_trace_event_add(
                wait_async_func_name,
                write_async_wait_start,
                write_async_wait_duration
            );
        }
    }
}

#endif