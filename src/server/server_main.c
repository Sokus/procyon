#include <stdio.h>

#include "pe_core.h"
#include "pe_time.h"
#include "pe_net.h"
#include "pe_temp_arena.h"

#include "pe_config.h"
#include "client/pe_client.h"

int main(int argc, char *argv[]) {
	pe_temp_arena_init(PE_MEGABYTES(4));
    pe_time_init();
    pe_net_init();

    peSocket socket;
    pe_socket_create(peAddressFamily_IPv4, &socket);
    pe_socket_set_nonblocking(socket);

    for (uint16_t port = SERVER_PORT_MIN; port < SERVER_PORT_MAX; port += 1) {
        peSocketBindError socket_bind_error = pe_socket_bind(socket, peAddressFamily_IPv4, port);
        if (socket_bind_error == peSocketBindError_None) break;
    }

#if !defined(PE_SERVER_STANDALONE)
    pe_client_init(&socket);
#endif

    while(true) {
        pe_client_update();

        if (pe_client_should_quit()) break;
    }

#if !defined(PE_SERVER_STANDALONE)
    pe_client_shutdown();
#endif

    pe_net_shutdown();

    return 0;
}