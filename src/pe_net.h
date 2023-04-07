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

typedef enum peSocketType
{
    peSocket_Undefined,
    peSocket_IPv4,
    peSocket_IPv6
} peSocketType;

typedef enum peSocketError
{
    peSocketError_None,
    peSocketError_CreateFailed,
    peSocketError_SetNonBlockingFailed,
    peSocketError_SockoptIPv6OnlyFailed,
    peSocketError_BindIPv4Failed,
    peSocketError_BindIPv6Failed,
    peSocketError_GetSocknameIPv4Failed,
    peSocketError_GetSocknameIPv6Failed
} peSocketError;

typedef struct peSocket
{
    peSocketHandle handle;
    uint16_t port;
    int error;
} peSocket;

peSocket pe_socket_create(peSocketType type, uint16_t port);
void pe_socket_destroy(peSocket *socket);
bool pe_socket_send(peSocket socket, peAddress address, void *packet_data, size_t packet_bytes);
int pe_socket_receive(peSocket socket, peAddress *from, void *packet_data, int max_packet_size);
bool pe_socket_is_error(peSocket socket);

#endif // PE_NET_H