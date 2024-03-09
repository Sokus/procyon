#ifndef PE_TIME_H
#define PE_TIME_H

#include <stdint.h>

void pe_time_init(void);
uint64_t pe_time_now(void);
uint64_t pe_time_diff(uint64_t new_ticks, uint64_t old_ticks);
uint64_t pe_time_since(uint64_t start_ticks);
uint64_t pe_time_laptime(uint64_t *last_time);
double pe_time_sec(uint64_t ticks);
uint64_t pe_time_sec_to_ticks(double sec);
double pe_time_ms(uint64_t ticks);
double pe_time_us(uint64_t ticks);
double pe_time_ns(uint64_t ticks);
void pe_time_sleep(unsigned long ms);

typedef struct peTime {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;
} peTime;

peTime pe_time_local(void);


#endif // PE_TIME_H