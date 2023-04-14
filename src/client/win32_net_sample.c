#include "pe_core.h"
#include "pe_net.h"

#include "pe_protocol.h"

int main(int argc, char *argv[]) {
    pe_net_init();

    peSocket socket;
    pe_socket_create(peSocket_IPv4, 0, &socket);

    peAddress server = pe_address4(127, 0, 0, 1, 54727);

    int data;
    pe_socket_send(socket, server, &data, sizeof(data));

    return 0;
}