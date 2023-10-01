#ifndef PE_NET_H
#define PE_NET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool pe_net_init(void);
void pe_net_update(void);
void pe_net_shutdown(void);

typedef enum peAddressFamily {
    peAddressFamily_IPv4,
    peAddressFamily_IPv6,
} peAddressFamily;

typedef struct peAddress {
    peAddressFamily family;

    union {
        uint32_t ipv4;
        uint16_t ipv6[8];
    };

    uint16_t port;
} peAddress;

peAddress pe_address4(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port);
peAddress pe_address4_from_int(uint32_t address, uint16_t port);
peAddress pe_address6(uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                      uint16_t e, uint16_t f, uint16_t g, uint16_t h,
                      uint16_t port);
peAddress pe_address6_from_array(uint16_t address[], uint16_t port);
peAddress pe_address_parse(char *address_in);
peAddress pe_address_parse_ex(char *address_in, uint16_t port);
char *pe_address_to_string(peAddress address, char buffer[], int buffer_size);
bool pe_address_compare(peAddress a, peAddress b);

typedef enum peSocketCreateError {
    peSocketCreateError_None,
    peSocketCreateError_InvalidParameter,
    peSocketCreateError_CreateFailed,
    peSocketCreateError_SockoptIPv6OnlyFailed,
} peSocketCreateError;

typedef enum peSocketBindError {
    peSocketBindError_None,
    peSocketBindError_InvalidParameter,
    peSocketBindError_BindFailed,
} peSocketBindError;

typedef enum peSocketSendError {
    peSocketSendError_None,
} peSocketSendError;

typedef enum peSocketReceiveError {
    peSocketReceiveError_None,
    #if defined(_WIN32)
    peSocketReceiveError_WouldBlock = 10035, // WSAEWOULDBLOCK
    peSocketReceiveError_RemoteNotListening = 10054 // WSAECONNRESET,
    #else
    peSocketReceiveError_Timeout = 11 // EAGAIN, EWOULDBLOCK,
    #endif
} peSocketReceiveError;

typedef struct peSocket {
#if defined(_WIN32)
    uint64_t handle;
#else
    int handle;
#endif
} peSocket;


peSocketCreateError pe_socket_create(peAddressFamily address_family, peSocket *socket);
peSocketBindError pe_socket_bind(peSocket socket, peAddressFamily address_family, uint16_t port);
bool pe_socket_set_nonblocking(peSocket socket);

void pe_socket_destroy(peSocket socket);
peSocketSendError pe_socket_send(peSocket socket, peAddress address, void *packet_data, size_t packet_bytes);
peSocketReceiveError pe_socket_receive(peSocket socket, void *packet_data, int max_packet_size, int *bytes_received, peAddress *from);

#endif // PE_NET_H