#ifndef P_NET_H
#define P_NET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool p_net_init(void);
void p_net_update(void);
void p_net_shutdown(void);

typedef enum pAddressFamily {
    pAddressFamily_IPv4,
    pAddressFamily_IPv6,
} pAddressFamily;

typedef struct pAddress {
    pAddressFamily family;
    union {
        uint32_t ipv4;
        uint16_t ipv6[8];
    };
    uint16_t port;
} pAddress;

pAddress p_address4(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port);
pAddress p_address4_from_int(uint32_t address, uint16_t port);
pAddress p_address6(uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                    uint16_t e, uint16_t f, uint16_t g, uint16_t h,
                    uint16_t port);
pAddress p_address6_from_array(uint16_t address[], uint16_t port);
pAddress p_address_parse(char *address_in);
pAddress p_address_parse_ex(char *address_in, uint16_t port);
char *p_address_to_string(pAddress address, char buffer[], int buffer_size);
bool p_address_compare(pAddress a, pAddress b);

typedef enum pSocketCreateError {
    pSocketCreateError_None,
    pSocketCreateError_InvalidParameter,
    pSocketCreateError_CreateFailed,
    pSocketCreateError_SockoptIPv6OnlyFailed,
} pSocketCreateError;

typedef enum pSocketBindError {
    pSocketBindError_None,
    pSocketBindError_InvalidParameter,
    pSocketBindError_BindFailed,
} pSocketBindError;

typedef enum pSocketSendError {
    pSocketSendError_None,
} pSocketSendError;

typedef enum pSocketReceiveError {
    pSocketReceiveError_None,
    #if defined(_WIN32)
    pSocketReceiveError_WouldBlock = 10035, // WSAEWOULDBLOCK
    pSocketReceiveError_RemoteNotListening = 10054 // WSAECONNRESET,
    #else
    pSocketReceiveError_Timeout = 11 // EAGAIN, EWOULDBLOCK,
    #endif
} pSocketReceiveError;

typedef struct pSocket {
#if defined(_WIN32)
    uint64_t handle;
#else
    int handle;
#endif
} pSocket;

pSocketCreateError p_socket_create(pAddressFamily address_family, pSocket *socket);
pSocketBindError p_socket_bind(pSocket socket, pAddressFamily address_family, uint16_t port);
bool p_socket_set_nonblocking(pSocket socket);

void p_socket_destroy(pSocket socket);
pSocketSendError p_socket_send(pSocket socket, pAddress address, void *packet_data, size_t packet_bytes);
pSocketReceiveError p_socket_receive(pSocket socket, void *packet_data, int max_packet_size, int *bytes_received, pAddress *from);

#endif // P_NET_H
