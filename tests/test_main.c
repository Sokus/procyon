#include <stdio.h>

#include "utility/p_test.h"

#include "test_free_list.c"
#include "test_string_set.c"

int main(int argc, char *argv[]) {
    test_free_list_main();
    test_string_set_main();
    P_TEST_REPORT();
    return 0;
}

