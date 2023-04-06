#include <stdio.h>

#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"

int main(int argc, char *argv[]) {
    pe_time_init();
    pe_net_init();

    peSocket socket = pe_socket_create(peSocket_IPv6, 0);
    int data = 4;
    peAddress server = pe_address6(0, 0, 0, 0, 0, 0, 0, 1, 54727);
    pe_socket_send(socket, server, &data, sizeof(data));


    pe_net_shutdown();

    return 0;
}