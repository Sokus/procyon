#include "pe_core.h"
#include "pe_time.h"
#include "pe_random.h"


#include <stdio.h>

int main(int argc, char *argv[]) {
    pe_time_init();

    peRandom random = pe_random_from_time();

    fprintf(stdout, "%u\n", pe_random_uint32(&random));

    return 0;
}