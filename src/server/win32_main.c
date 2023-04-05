#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"

#include <stdio.h>

int main(void) {
    pe_net_init();
    pe_time_init();

    uint16_t port = 54727; // 49152 to 65535
    peSocket socket = pe_socket_create(peSocket_IPv4, port);

    printf("waiting for packets...\n");
    while(1) {
        uint8_t buf[256];
        peAddress from;
        if (pe_socket_receive(socket, &from, buf, sizeof(buf))) {
            printf("something got received!\n");
            break;
        }
    }

    pe_net_shutdown();

    return 0;
}