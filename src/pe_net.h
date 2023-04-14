#ifndef PE_NET_H
#define PE_NET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool pe_net_init(void);
void pe_net_update(void);
void pe_net_shutdown(void);

typedef enum peAddressType
{
    peAddress_Undefined,
    peAddress_IPv4,
    peAddress_IPv6,
} peAddressType;

typedef struct peAddress
{
    peAddressType type;

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
bool pe_address_is_valid(peAddress address);
bool pe_address_compare(peAddress a, peAddress b);

#if _WIN32
typedef uint64_t peSocketHandle;
#else
typedef int peSocketHandle;
#endif

typedef enum peSocketType {
    peSocket_Undefined,
    peSocket_IPv4,
    peSocket_IPv6
} peSocketType;

typedef enum peSocketCreateError {
    peSocketCreateError_None,
    peSocketCreateError_InvalidParameter,
    peSocketCreateError_CreateFailed,
    peSocketCreateError_SetNonBlockingFailed,
    peSocketCreateError_SockoptIPv6OnlyFailed,
    peSocketCreateError_BindIPv4Failed,
    peSocketCreateError_BindIPv6Failed,
    peSocketCreateError_GetSocknameIPv4Failed,
    peSocketCreateError_GetSocknameIPv6Failed
} peSocketCreateError;

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
    peSocketHandle handle;
    uint16_t port;
} peSocket;

peSocketCreateError pe_socket_create(peSocketType type, uint16_t port, peSocket *socket);
void pe_socket_destroy(peSocket *socket);
peSocketSendError pe_socket_send(peSocket socket, peAddress address, void *packet_data, size_t packet_bytes);
peSocketReceiveError pe_socket_receive(peSocket socket, void *packet_data, int max_packet_size, int *bytes_received, peAddress *from);

#endif // PE_NET_H