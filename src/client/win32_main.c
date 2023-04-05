#include <stdio.h>

#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"

int main() {
    pe_time_init();
    pe_net_init();

    peSocket socket = pe_socket_create(peSocket_IPv4, 0);
    int data = 4;
    pe_socket_send(socket, pe_address4(127, 0, 0, 1, 54727), &data, sizeof(data));


    pe_net_shutdown();

    return 0;
}