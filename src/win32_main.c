#include <stdio.h>

#include "pe_core.h"
#include "pe_time.h"

int main() {
    pe_time_init();

    uint64_t last_time = pe_time_now();
    while(1) {
        printf("laptime: %f\n", pe_time_sec(pe_time_laptime(&last_time)));

        pe_time_sleep(1000);
    }

    return 0;
}