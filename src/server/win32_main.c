#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"

#include "pe_protocol.h"

#include <stdio.h>

int main() {
    pe_net_init();
    pe_time_init();

    uint16_t port = 54727; // 49152 to 65535
    peSocket socket;
    pe_socket_create(peSocket_IPv4, port, &socket);

    printf("waiting for packets...\n");
    while(1) {
        peAddress from;
        pePacket packet = {0};
        if (pe_receive_packet(socket, &from, &packet)) {
            printf("packet received! messages:\n");
            for (int i = 0; i < packet.message_count; i += 1) {
                printf(" %d\n", (int)packet.message_types[i]);
            }
            pe_free_packet(pe_heap_allocator(), &packet);
        }

    }

    pe_net_shutdown();

    return 0;
}