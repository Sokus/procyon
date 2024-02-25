#ifndef PE_TRACE_H_HEADER_GUARD
#define PE_TRACE_H_HEADER_GUARD

#include <stdint.h>
#include <stddef.h>

#define PE_TRACE_DATA_BATCH_SIZE PE_KILOBYTES(48)
#define PE_TRACE_EVENT_NAME_SIZE 256

void pe_trace_init(void);
void pe_trace_shutdown(void);

typedef enum peTraceEventType {
    peTraceEvent_Begin,
    peTraceEvent_End,
    peTraceEvent_Complete,
    peTraceEvent_Count
} peTraceEventType;

typedef struct peTraceEventData {
    int16_t type;
    uint16_t name_length;
    uint32_t _alignment;
    uint64_t timestamp;
    uint64_t duration; // type == peTraceEvent_Complete
} peTraceEventData;

typedef struct peTraceMark {
    const char *name;
    size_t name_length;
    uint64_t timestamp;
} peTraceMark;

#if defined(PE_TRACE_ENABLED)
    peTraceMark pe_trace_mark_begin_internal(const char *name, size_t name_length);
    void pe_trace_mark_end_internal(peTraceMark trace_mark);

    #define PE_TRACE_MARK_BEGIN(name, name_length) pe_trace_mark_begin_internal(name, name_length)
    #define PE_TRACE_MARK_BEGIN_LITERAL(name) pe_trace_mark_begin_internal(name, sizeof(name))
    #define PE_TRACE_MARK_END(trace_mark) pe_trace_mark_end_internal(trace_mark)
    #define PE_TRACE_FUNCTION_BEGIN() peTraceMark _##__func__##_trace_mark = pe_trace_mark_begin_internal(__func__, sizeof(__func__))
    #define PE_TRACE_FUNCTION_END() pe_trace_mark_end_internal(_##__func__##_trace_mark)
#else
    #define PE_TRACE_MARK_BEGIN(name, name_length) (peTraceMark){0}
    #define PE_TRACE_MARK_BEGIN_LITERAL(name) (peTraceMark){0}
    #define PE_TRACE_MARK_END(trace_mark) (void)(trace_mark)
    #define PE_TRACE_FUNCTION_BEGIN() (void)0
    #define PE_TRACE_FUNCTION_END() (void)0
#endif


#endif // PE_TRACE_H_HEADER_GUARD