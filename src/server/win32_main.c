#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"

#include <stdio.h>

int main() {
    pe_net_init();
    pe_time_init();

    uint16_t port = 54727; // 49152 to 65535
    peSocket socket = pe_socket_create(peSocket_IPv6, port);

    printf("waiting for packets...\n");
    while(1) {
        uint8_t buf[PE_KILOBYTES(2)];
        peAddress from;
        int bytes_received;
        if ((bytes_received = pe_socket_receive(socket, &from, buf, sizeof(buf))) > 0) {
            char address_string[256];
            printf("received %d bytes from %s\n", bytes_received,
            pe_address_to_string(from, address_string, sizeof(address_string)));
            break;
        }
    }

    pe_net_shutdown();

    return 0;
}