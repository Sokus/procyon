#ifndef PE_TRACE_H_HEADER_GUARD
#define PE_TRACE_H_HEADER_GUARD

#include <stdint.h>
#include <stddef.h>

#define PE_TRACE_DATA_BATCH_SIZE PE_KILOBYTES(48)

#define PE_TRACE_REFERENCE_LITERAL "fB8PgEXDfgO5i2a5HfQ0hvelQ07mP4ce"

void pe_trace_init(void);
void pe_trace_shutdown(void);

typedef struct peTraceHeader {
    int32_t address_bytes;
    union {
        void *address;
        uint64_t address_64;
        uint32_t address_32;
    };
} peTraceHeader;

typedef struct peTraceEventData {
    uint64_t timestamp;
    uint64_t duration;
    union {
        void *address;
        uint64_t address_64;
        uint32_t address_32;
    };
} peTraceEventData;

typedef struct peTraceMark {
    const char *name;
    uint64_t timestamp;
} peTraceMark;

#if defined(PE_TRACE_ENABLED)
    peTraceMark pe_trace_mark_begin_internal(const char *name);
    void pe_trace_mark_end_internal(peTraceMark trace_mark);

    #define PE_TRACE_MARK_BEGIN(name) pe_trace_mark_begin_internal(name)
    #define PE_TRACE_MARK_END(trace_mark) pe_trace_mark_end_internal(trace_mark)
    #define PE_TRACE_FUNCTION_BEGIN() peTraceMark _##__func__##_trace_mark = pe_trace_mark_begin_internal(__func__)
    #define PE_TRACE_FUNCTION_END() pe_trace_mark_end_internal(_##__func__##_trace_mark)
#else
    #define PE_TRACE_MARK_BEGIN(name) (peTraceMark){0}
    #define PE_TRACE_MARK_END(trace_mark) (void)(trace_mark)
    #define PE_TRACE_FUNCTION_BEGIN() (void)0
    #define PE_TRACE_FUNCTION_END() (void)0
#endif


#endif // PE_TRACE_H_HEADER_GUARD