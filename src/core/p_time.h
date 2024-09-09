#ifndef P_TIME_HEADER_GUARD
#define P_TIME_HEADER_GUARD

#include <stdint.h>

typedef struct pTime {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;
} pTime;

void ptime_init(void);
void ptime_sleep(unsigned long ms);
uint64_t ptime_now(void);
uint64_t ptime_diff(uint64_t new_ticks, uint64_t old_ticks);
uint64_t ptime_since(uint64_t start_ticks);
uint64_t ptime_laptime(uint64_t *last_time);
pTime ptime_local(void);

static inline double ptime_sec(uint64_t ticks) {
    return (double)ticks / 1000000000.0;
}

static inline uint64_t ptime_sec_to_ticks(double sec) {
    return (uint64_t)sec * 1000000000;
}

static inline double ptime_ms(uint64_t ticks) {
    return (double)ticks / 1000000.0;
}

static inline double ptime_us(uint64_t ticks) {
    return (double)ticks / 1000.0;
}

static inline double ptime_ns(uint64_t ticks) {
    return (double)ticks;
}

#endif // P_TIME_HEADER_GUARD

#if defined(P_CORE_IMPLEMENTATION) && !defined(P_TIME_IMPLEMENTATION_GUARD)
#define P_TIME_IMPLEMENTATION_GUARD

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef P_ASSERT
#include <assert.h>
#define P_ASSERT assert
#endif

// GENERAL

uint64_t ptime_diff(uint64_t new_ticks, uint64_t old_ticks) {
    uint64_t result = 0;
    if (new_ticks >= old_ticks) {
        result = new_ticks - old_ticks;
    } else {
        result = UINT64_MAX - old_ticks + new_ticks;
    }
    return result;
}

uint64_t ptime_since(uint64_t start_ticks) {
    uint64_t time_now = ptime_now();
    uint64_t time_diff = ptime_diff(time_now, start_ticks);
    return time_diff;
}

uint64_t ptime_laptime(uint64_t *last_time) {
    uint64_t dt = 0;
    uint64_t now = ptime_now();
    if (*last_time != 0) {
        dt = ptime_diff(now, *last_time);
    }
    *last_time = now;
    return dt;
}

// PLATFORM SPECIFIC (WIN32)
#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "Winmm.lib")

static struct pTimeStateWin32 {
    bool initialized;
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    bool sleep_granular;
} ptime_state_win32 = {0};

void ptime_init(void) {
    ptime_state_win32.initialized = true;
    QueryPerformanceCounter(&ptime_state_win32.start);
    QueryPerformanceFrequency(&ptime_state_win32.freq);
    UINT desired_scheduler_ms = 1;
    MMRESULT bprc = timeBeginPeriod(desired_scheduler_ms);
    ptime_state_win32.sleep_granular = (bprc == TIMERR_NOERROR);
}

static inline LONGLONG longlong_muldiv(LONGLONG value, LONGLONG numer, LONGLONG denom) {
    LONGLONG q = value / denom;
    LONGLONG r = value % denom;
    return q * numer + r * numer / denom;
}

uint64_t ptime_now(void) {
    P_ASSERT(ptime_state_win32.initialized);
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    LONGLONG value = counter.QuadPart - ptime_state_win32.start.QuadPart;
    LONGLONG numer = 1000000000;
    LONGLONG denom = ptime_state_win32.freq.QuadPart;
    uint64_t now = (uint64_t)longlong_muldiv(value, numer, denom);
    return now;
}

void ptime_sleep(unsigned long ms) {
    Sleep(ms);
}

pTime ptime_local(void) {
    SYSTEMTIME systemtime;
    GetLocalTime(&systemtime);
    pTime time = (pTime){
        .year = systemtime.wYear,
        .month = systemtime.wMonth,
        .day = systemtime.wDay,
        .hour = systemtime.wHour,
        .minutes = systemtime.wMinute,
        .seconds = systemtime.wSecond,
        .milliseconds = systemtime.wMilliseconds,
    };
    return time;
}
#endif // PLATFORM SPECIFIC (WIN32)

// PLATFORM SPECIFIC (LINUX)
#if defined(__linux__)
#include <time.h>
#include <unistd.h>

static struct pTimeStateLinux {
    bool initialized;
    bool sleep_granular; // TODO
    uint64_t start;
} ptime_state_linux = {0};

void ptime_init(void) {
    ptime_state_linux.initialized = true;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ptime_state_linux.start = (uint64_t)ts.tv_sec*1000000000 + (uint64_t)ts.tv_nsec;
    ptime_state_linux.sleep_granular = true;
}

uint64_t ptime_now(void) {
    P_ASSERT(ptime_state_linux.initialized);
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = ((uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec) - ptime_state_linux.start;
    return now;
}

void ptime_sleep(unsigned long ms) {
    struct timespec rem;
    struct timespec req;
    req.tv_sec = (int)(ms / 1000);
    req.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&req, &rem);
}

pTime ptime_local(void) {
    struct timespec ts;
    int clock_gettime_result = clock_gettime(CLOCK_REALTIME, &ts);
    P_ASSERT(clock_gettime_result == 0); // TODO
    time_t tv_sec = ts.tv_sec;
    struct tm *tm = localtime(&tv_sec);
    P_ASSERT(tm != NULL); // TODO
    pTime time = (pTime){
        .year = 1900+tm->tm_year,
        .month = 1+tm->tm_mon,
        .day = tm->tm_mday,
        .hour = tm->tm_hour,
        .minutes = tm->tm_min,
        .seconds = tm->tm_sec,
        .milliseconds = ts.tv_nsec/1000000,
    };
    return time;
}
#endif // PLATFORM SPECIFIC (LINUX)

// PLATFORM SPECIFIC (PSP)
#if defined(__PSP__)
#include <psptypes.h>
#include <psprtc.h>
#include <pspkernel.h> // sceKernelDelayThread();

struct pTimeStatePSP {
    bool initialized;
    bool sleep_granular; // TODO
    uint64_t start;
} ptime_state_psp = {0};

void ptime_init(void) {
    ptime_state_psp.initialized = true;
    uint32_t tick_resolution = sceRtcGetTickResolution(); // 14 858 377
    u64 current_tick;
    int get_current_tick_result = sceRtcGetCurrentTick(&current_tick);
    P_ASSERT(get_current_tick_result == 0);
    ptime_state_psp.start = (uint64_t)(1000000000/tick_resolution)*current_tick;
    ptime_state_psp.sleep_granular = true;
}

uint64_t ptime_now(void) {
    P_ASSERT(ptime_state_psp.initialized);
    uint32_t tick_resolution = sceRtcGetTickResolution(); // TODO: Can we remove this?
    u64 current_tick;
    int get_current_tick_result = sceRtcGetCurrentTick(&current_tick);
    P_ASSERT(get_current_tick_result == 0);
    uint64_t now = (uint64_t)(1000000000/tick_resolution)*current_tick;
    return now;
}

void ptime_sleep(unsigned long ms) {
    SceUInt delay_us = 1000 * ms;
    sceKernelDelayThread(delay_us);
}

pTime ptime_local(void) {
    ScePspDateTime psp_time;
    int rtc_get_local_time_result = sceRtcGetCurrentClockLocalTime(&psp_time);
    P_ASSERT(rtc_get_local_time_result == 0);
    pTime time = (pTime){
        .year = psp_time.year,
        .month = psp_time.month,
        .day = psp_time.day,
        .hour = psp_time.hour,
        .minutes = psp_time.minute,
        .seconds = psp_time.second,
        .milliseconds = psp_time.microsecond / 1000,
    };
    return time;
}
#endif // PLATFORM SPECIFIC (PSP)

#endif // P_TIME_IMPLEMENTATION