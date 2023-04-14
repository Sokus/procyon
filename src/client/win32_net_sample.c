#include "pe_core.h"
#include "pe_net.h"

#include "pe_protocol.h"

int main(int argc, char *argv[]) {
    pe_net_init();

    peSocket socket;
    pe_socket_create(peSocket_IPv4, 0, &socket);

    peAddress server = pe_address4(127, 0, 0, 1, 54727);

    peMessageType type = peMessage_ConnectionClosed;
    peConnectionRequestMessage *msg = pe_alloc_message(pe_heap_allocator(), type);
    pePacket packet = {0};
    pe_append_message(&packet, msg, type);

    pe_send_packet(socket, server, &packet);

    return 0;
}