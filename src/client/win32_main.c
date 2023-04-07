#include <stdio.h>

#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"

int main(int argc, char *argv[]) {
    pe_time_init();
    pe_net_init();

    peSocket socket = pe_socket_create(peSocket_IPv4, 0);
    int data = 4;
    peAddress server = pe_address4(192, 168, 128, 238, 54727);
    pe_socket_send(socket, server, &data, sizeof(data));
    printf("port: %u\n", socket.port);

    pe_net_shutdown();

    return 0;
}