#ifndef P_TRACE_H_HEADER_GUARD
#define P_TRACE_H_HEADER_GUARD

#include <stdint.h>
#include <stddef.h>

#define P_TRACE_DATA_BATCH_SIZE P_KILOBYTES(48)

#define P_TRACE_REFERENCE_LITERAL "fB8PgEXDfgO5i2a5HfQ0hvelQ07mP4ce"

void p_trace_init(void);
void p_trace_shutdown(void);

typedef struct pTraceHeader {
    int32_t address_bytes;
    union {
        void *address;
        uint64_t address_64;
        uint32_t address_32;
    };
} pTraceHeader;

typedef struct pTraceEventData {
    uint64_t timestamp;
    uint64_t duration;
    union {
        void *address;
        uint64_t address_64;
        uint32_t address_32;
    };
} pTraceEventData;

typedef struct pTraceMark {
    const char *name;
    uint64_t timestamp;
} pTraceMark;

#if defined(P_TRACE_ENABLED)
    pTraceMark p_trace_mark_begin_internal(const char *name);
    void p_trace_mark_end_internal(pTraceMark trace_mark);

    #define P_TRACE_MARK_BEGIN(name) p_trace_mark_begin_internal(name)
    #define P_TRACE_MARK_END(trace_mark) p_trace_mark_end_internal(trace_mark)
    #define P_TRACE_FUNCTION_BEGIN() pTraceMark _##__func__##_trace_mark = p_trace_mark_begin_internal(__func__)
    #define P_TRACE_FUNCTION_END() p_trace_mark_end_internal(_##__func__##_trace_mark)
#else
    #define P_TRACE_MARK_BEGIN(name) (pTraceMark){0}
    #define P_TRACE_MARK_END(trace_mark) (void)(trace_mark)
    #define P_TRACE_FUNCTION_BEGIN() (void)0
    #define P_TRACE_FUNCTION_END() (void)0
#endif

#endif // P_TRACE_H_HEADER_GUARD
