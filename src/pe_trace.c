#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "pe_time.h"
#include "pe_core.h"
#include "pe_trace.h"
#include "pe_file_io.h"

peFileHandle trace_file;

uint8_t trace_data_0[PE_TRACE_DATA_BATCH_SIZE];
uint8_t trace_data_1[PE_TRACE_DATA_BATCH_SIZE];
void *trace_data[2] = { trace_data_0, trace_data_1 };
size_t trace_data_used[2] = {0};
bool trace_data_batch_busy[2] = {0};

int current_trace_index = 0;

void pe_trace_init(void) {
    pe_file_open("./trace.pt", &trace_file);
}

static void pe_trace_data_flush(void);

void pe_trace_shutdown(void) {
    pe_trace_data_flush();
    bool should_wait;
    bool poll_success = pe_file_poll(trace_file, &should_wait);
    if (poll_success && should_wait) {
        pe_file_wait(trace_file);
    }
    pe_file_close(trace_file);
}

void pe_trace_event_add(const char *name, size_t name_length, peTraceEventType type, uint64_t timestamp, uint64_t duration) {
    PE_ASSERT(type >= 0 && type < peTraceEvent_Count);

    size_t trace_event_space_needed = sizeof(peTraceEventData) + name_length*sizeof(char);

    if (trace_data_used[current_trace_index] + trace_event_space_needed > PE_TRACE_DATA_BATCH_SIZE) {
        pe_trace_data_flush();
    }

    peTraceEventData trace_event_data = {
        .type = (int16_t)type,
        .name_length = (uint16_t)name_length,
        .timestamp = timestamp,
        .duration = duration
    };
    memcpy((uint8_t*)trace_data[current_trace_index] + trace_data_used[current_trace_index], &trace_event_data, sizeof(peTraceEventData));
    trace_data_used[current_trace_index] += sizeof(peTraceEventData);

    if (name_length > 0) {
        memcpy((uint8_t*)trace_data[current_trace_index] + trace_data_used[current_trace_index], name, name_length*sizeof(char));
        trace_data_used[current_trace_index] += name_length*sizeof(char);
    }
}

peTraceMark pe_trace_mark_begin(const char *name, size_t name_length) {
    peTraceMark result = {
        .name = name,
        .name_length = name_length,
        .timestamp = pe_time_now()
    };
    return result;
}

void pe_trace_mark_end(peTraceMark trace_mark) {
    pe_trace_event_add(trace_mark.name, trace_mark.name_length, peTraceEvent_Complete, trace_mark.timestamp, pe_time_since(trace_mark.timestamp));
}

static void pe_trace_data_flush(void) {
    if (trace_data_used[current_trace_index] > 0) {
        bool wait_for_write_async;
        bool file_poll_success = pe_file_poll(trace_file, &wait_for_write_async);
        uint64_t write_async_wait_start;
        uint64_t write_async_wait_duration;
        if (file_poll_success && wait_for_write_async) {
            write_async_wait_start = pe_time_now();
            pe_file_wait(trace_file);
            write_async_wait_duration = pe_time_since(write_async_wait_start);
        }

        pe_file_write(trace_file, trace_data[current_trace_index], trace_data_used[current_trace_index]);
        trace_data_used[current_trace_index] = 0;
        current_trace_index = (current_trace_index + 1) % 2;

        if (file_poll_success && wait_for_write_async) {
            const char wait_async_func_name[] = "sceIoWaitAsync";
            pe_trace_event_add(
                wait_async_func_name,
                sizeof(wait_async_func_name),
                peTraceEvent_Complete,
                write_async_wait_start,
                write_async_wait_duration
            );
        }
    }
}