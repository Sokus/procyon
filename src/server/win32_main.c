#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"

#include <stdio.h>

int main() {
    pe_net_init();
    pe_time_init();

    uint16_t port = 54727; // 49152 to 65535
    peSocket socket;
    pe_socket_create(peSocket_IPv4, port, &socket);

    printf("waiting for packets...\n");
    while(1) {
        uint8_t buf[PE_KILOBYTES(2)];
        int bytes_received;
        peAddress from;
        peSocketReceiveError error = pe_socket_receive(socket, buf, sizeof(buf), &bytes_received, &from);
        if (error == peSocketReceiveError_None && bytes_received > 0) {
            char address_string[256];
            printf("received %d bytes from %s\n", bytes_received,
            pe_address_to_string(from, address_string, sizeof(address_string)));
            break;
        }
    }

    pe_net_shutdown();

    return 0;
}