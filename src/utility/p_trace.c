#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "core/p_defines.h"
#include "core/p_time.h"
#include "p_trace.h"
#include "platform/p_file.h"

#if defined(__PSP__)
#include <pspkerneltypes.h>
#include <pspiofilemgr.h>
#endif

#ifndef P_TRACE_ENABLED
void p_trace_init(void) {}
void p_trace_shutdown(void) {}
#endif

#ifdef P_TRACE_ENABLED

static struct {
    pFileHandle output_file;
    uint8_t data[2][P_TRACE_DATA_BATCH_SIZE];
    size_t data_used[2];
    int current_data_index;
} p_trace_state = {0};

static void p_trace_event_add(const char *name, uint64_t timestamp, uint64_t duration);
static void p_trace_data_flush(void);

pTraceMark p_trace_mark_begin_internal(const char *name) {
    pTraceMark result = {
        .name = name,
        .timestamp = p_time_now()
    };
    return result;
}

void p_trace_mark_end_internal(pTraceMark trace_mark) {
    uint64_t trace_mark_duration = p_time_since(trace_mark.timestamp);
    p_trace_event_add(
        trace_mark.name,
        trace_mark.timestamp,
        trace_mark_duration
    );
}

static void p_trace_event_add(const char *name, uint64_t timestamp, uint64_t duration) {
    size_t trace_event_space_needed = sizeof(pTraceEventData);
    if (p_trace_state.data_used[p_trace_state.current_data_index] + trace_event_space_needed > P_TRACE_DATA_BATCH_SIZE) {
        p_trace_data_flush();
    }

    uint8_t *current_trace_data = p_trace_state.data[p_trace_state.current_data_index];
    size_t *current_trace_data_used = &p_trace_state.data_used[p_trace_state.current_data_index];

    pTraceEventData trace_event_data = {
        .timestamp = timestamp,
        .duration = duration,
        .address = (void*)name,
    };

    memcpy(current_trace_data + *current_trace_data_used, &trace_event_data, sizeof(trace_event_data));
    *current_trace_data_used += sizeof(trace_event_data);
}

#if defined(__PSP__) // PSP SPECIFIC

static bool p_trace_file_poll(pFileHandle file, bool *result);
static bool p_trace_file_wait(pFileHandle file);

void p_trace_init(void) {
    int mode = pFO_NONBLOCK|pFO_WRONLY|pFO_CREATE|pFO_TRUNC;
    p_file_open(P_TRACE_FILE_PATH, mode, 0777, &p_trace_state.output_file);
    sceIoChangeAsyncPriority((SceUID)p_trace_state.output_file, 16);

    pTraceHeader trace_header = {
        .address_bytes = sizeof(void*),
        .address = (void*)P_TRACE_REFERENCE_LITERAL
    };

    sceIoWriteAsync((SceUID)p_trace_state.output_file, &trace_header, (SceSize)sizeof(pTraceHeader));

    bool should_wait;
    bool poll_success = p_trace_file_poll(p_trace_state.output_file, &should_wait);
    if (poll_success && should_wait) {
        p_trace_file_wait(p_trace_state.output_file);
    }
}

void p_trace_shutdown(void) {
    p_trace_data_flush();
    bool should_wait;
    bool poll_success = p_trace_file_poll(p_trace_state.output_file, &should_wait);
    if (poll_success && should_wait) {
        p_trace_file_wait(p_trace_state.output_file);
    }
    p_file_close(p_trace_state.output_file);
}

static void p_trace_data_flush(void) {
    if (p_trace_state.data_used[p_trace_state.current_data_index] > 0) {
        bool wait_for_write_async;
        bool file_poll_success = p_trace_file_poll(p_trace_state.output_file, &wait_for_write_async);
        uint64_t write_async_wait_start;
        uint64_t write_async_wait_duration;
        if (file_poll_success && wait_for_write_async) {
            write_async_wait_start = p_time_now();
            p_trace_file_wait(p_trace_state.output_file);
            write_async_wait_duration = p_time_since(write_async_wait_start);
        }


        sceIoWriteAsync(
            (SceUID)p_trace_state.output_file,
            p_trace_state.data[p_trace_state.current_data_index],
            (SceSize)p_trace_state.data_used[p_trace_state.current_data_index]
        );
        p_trace_state.data_used[p_trace_state.current_data_index] = 0;
        p_trace_state.current_data_index = (p_trace_state.current_data_index + 1) % 2;

        if (file_poll_success && wait_for_write_async) {
            const char wait_async_func_name[] = "sceIoWaitAsync";
            p_trace_event_add(
                wait_async_func_name,
                write_async_wait_start,
                write_async_wait_duration
            );
        }
    }
}

static bool p_trace_file_poll(pFileHandle file, bool *result) {
    SceInt64 sce_result;
    int sce_rc = sceIoPollAsync(file, &sce_result);
    bool success = false;
    switch (sce_rc) {
        case 0: success = true; *result = false; break;
        case 1: success = true; *result = true; break;
        default: break;
    }
    return success;
}

static bool p_trace_file_wait(pFileHandle file) {
    SceInt64 sce_result;
    int sce_rc = sceIoWaitAsync(file, &sce_result);
    bool success = false;
    if (sce_rc >= 0) {
        success = true;
    }
    return success;
}

#else // PC SPECIFIC

void p_trace_init(void) {
    int trace_file_mode = (pFO_RDWR|pFO_CREATE|pFO_TRUNC);
    p_file_open(P_TRACE_FILE_PATH, trace_file_mode, 0644, &p_trace_state.output_file);
    pTraceHeader trace_header = {
        .address_bytes = sizeof(void*),
        .address = (void*)P_TRACE_REFERENCE_LITERAL
    };
    p_file_write(p_trace_state.output_file, &trace_header, sizeof(pTraceHeader));
}

void p_trace_shutdown(void) {
    p_trace_data_flush();
    p_file_close(p_trace_state.output_file);
}

static void p_trace_data_flush(void) {
    if (p_trace_state.data_used[p_trace_state.current_data_index] > 0) {
        p_file_write(
            p_trace_state.output_file,
            p_trace_state.data[p_trace_state.current_data_index],
            p_trace_state.data_used[p_trace_state.current_data_index]
        );
        p_trace_state.data_used[p_trace_state.current_data_index] = 0;
        p_trace_state.current_data_index = (p_trace_state.current_data_index + 1) % 2;
    }
}

#endif // PSP / PC SPECIFIC IMPLEMENTATION
#endif // P_TRACE_ENABLED
