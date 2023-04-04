#include "pe_time.h"
#include "core.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#if defined(_WIN32)
    #include <windows.h>
    #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
    #endif
#elif defined(PSP)
    #include <psptypes.h>
    #include <time.h>
    #include <psprtc.h>
    #include <pspkernel.h> // sceKernelDelayThread();
#else
    #include <time.h>
    #include <unistd.h>
#endif

struct peTimeState {
    bool initialized;
    bool sleep_granular;
#if defined(_WIN32)
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
#else
    uint64_t start;
#endif
};
static struct peTimeState pe_time_state = {0};

void pe_time_init(void) {
    pe_time_state.initialized = true;
#if defined(_WIN32)
    QueryPerformanceCounter(&pe_time_state.start);
    QueryPerformanceFrequency(&pe_time_state.freq);
    UINT desired_scheduler_ms = 1;
    MMRESULT bprc = timeBeginPeriod(desired_scheduler_ms);
    pe_time_state.sleep_granular = (bprc == TIMERR_NOERROR);
#elif defined(PSP)
    uint32_t tick_resolution = sceRtcGetTickResolution(); // 14 858 377
    u64 current_tick;
    int get_current_tick_result = sceRtcGetCurrentTick(&current_tick);
    PE_ASSERT(get_current_tick_result == 0);
    //pe_time_state.start = (uint64_t)(1000000000/tick_resolution)*current_tick;
    pe_time_state.start = (uint64_t)(1000000000/tick_resolution)*current_tick;
    pe_time_state.sleep_granular = true;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    pe_time_state.start = (uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec;
    pe_time_state.sleep_granular = true;
#endif
}

#if defined(_WIN32)
static int64_t int64_muldiv(int64_t value, int64_t numer, int64_t denom) {
    int64_t q = value / denom;
    int64_t r = value % denom;
    return q * numer + r * numer / denom;
}
#endif

uint64_t pe_time_now(void) {
    PE_ASSERT(pe_time_state.initialized);
    uint64_t now;
#if defined(_WIN32)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    now = (uint64_t)int64_muldiv(counter.QuadPart - pe_time_state.start.QuadPart, 1000000000, pe_time_state.freq.QuadPart);
#elif defined(PSP)
    uint32_t tick_resolution = sceRtcGetTickResolution(); // TODO: Can we remove this?

    u64 current_tick;
    int get_current_tick_result = sceRtcGetCurrentTick(&current_tick);
    PE_ASSERT(get_current_tick_result == 0);
    now = (uint64_t)(1000000000/tick_resolution)*current_tick;
    //now = (uint64_t)current_tick;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = ((uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec) - pe_time_state.start;
#endif
    return now;
}

/*
uint64_t time_diff(uint64_t new_ticks, uint64_t old_ticks) {
    return (new_ticks > old_ticks ? new_ticks - old_ticks : 1);
}
*/
uint64_t pe_time_diff(uint64_t new_ticks, uint64_t old_ticks) {
    uint64_t result = 0;
    if (new_ticks >= old_ticks)
        result = new_ticks - old_ticks;
    else
        result = UINT64_MAX - old_ticks + new_ticks;
    return result;
}

uint64_t pe_time_since(uint64_t start_ticks) {
    return pe_time_diff(pe_time_now(), start_ticks);
}

uint64_t pe_time_laptime(uint64_t *last_time) {
    uint64_t dt = 0;
    uint64_t now = pe_time_now();
    if(*last_time != 0)
        dt = pe_time_diff(now, *last_time);
    *last_time = now;
    return dt;
}

double pe_time_sec(uint64_t ticks) {
    return (double)ticks / 1000000000.0;
}

uint64_t pe_time_sec_to_ticks(double sec) {
    return (uint64_t)sec * 1000000000;
}

double pe_time_ms(uint64_t ticks) {
    return (double)ticks / 1000000.0;
}

double pe_time_time_us(uint64_t ticks) {
    return (double)ticks / 1000.0;
}

double pe_time_ns(uint64_t ticks) {
    return (double)ticks;
}

void pe_time_sleep(unsigned long ms)
{
#if defined(_WIN32)
    Sleep(ms);
#elif defined(PSP)
    SceUInt delay_us = 1000 * ms;
    sceKernelDelayThread(delay_us);
#else
    struct timespec rem;
    struct timespec req;
    req.tv_sec = (int)(ms / 1000);
    req.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&req , &rem);
#endif
}