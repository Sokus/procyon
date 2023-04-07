#include "pe_net.h"
#include "pe_core.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)
	#define NOMINMAX
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <ws2ipdef.h>
	#pragma comment( lib, "WS2_32.lib" )

	#ifdef SetPort
	#undef SetPort
	#endif // #ifdef SetPort
#elif defined(PSP)
    #include <unistd.h>
    #include <pspnet.h>
    #include <pspnet_inet.h>
    #include <pspnet_apctl.h>
    #include <pspkernel.h>
    #include <pspnet_resolver.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <psputility.h>
    #include <psprtc.h>
#else
	#include <netdb.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
#endif

// TODO:
// - clean up the includes, especially on PSP
// - change 'printf' for 'fprintf(stdout...' on PSP

static bool network_initialized = false;
#if defined(PSP)
static int apctl_state = PSP_NET_APCTL_STATE_DISCONNECTED;
static u64 next_apctl_get_state_tick;
static u64 next_apctl_connect_tick;
#endif

bool pe_net_init(void) {
    PE_ASSERT(!network_initialized);
    bool result = true;
#if defined(_WIN32)
    WSADATA WsaData;
    result = WSAStartup(MAKEWORD(2, 2), &WsaData) == NO_ERROR;
#elif defined(PSP)
    result = result && (sceUtilityLoadNetModule(PSP_NET_MODULE_INET) == 0);
    result = result && (sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON) == 0);
#if 0 /* NOTE: We don't need those modules (yet?) */
    result = result && (sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEURI) == 0);
    result = result && (sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEHTTP) == 0);
    result = result && (sceUtilityLoadNetModule(PSP_NET_MODULE_HTTP) == 0);
#endif
	result = result && (sceNetInit(0x20000, 0x20, 0x1000, 0x20, 0x1000) == 0);
	result = result && (sceNetInetInit() == 0);
	result = result && (sceNetApctlInit(0x1600, 42) == 0);

    if (result) {
        sceNetApctlConnect(1);
        u64 current_tick;
        sceRtcGetCurrentTick(&current_tick);
        next_apctl_get_state_tick = current_tick;
        next_apctl_connect_tick = current_tick;
    }
#endif
    if (result) {
        network_initialized = result;
    }
    PE_ASSERT(network_initialized);
    return result;
}

// NOTE: For now the only use for `pe_net_update` is to reconnect
// to an Access Point on PSP in case the connection was lost
void pe_net_update(void) {
#if defined(PSP)
	u64 apclt_get_state_cooldown_ms = 100;
	u64 apctl_connect_cooldown_ms = 5000;

	u64 current_tick;
	sceRtcGetCurrentTick(&current_tick);

	if (sceRtcCompareTick(&current_tick, &next_apctl_get_state_tick) >= 0) {
		int new_apctl_state;
		if (sceNetApctlGetState(&new_apctl_state) == 0) {
			if (new_apctl_state != apctl_state) {
				if (new_apctl_state == PSP_NET_APCTL_STATE_DISCONNECTED) {
					sceRtcTickAddMicroseconds(&next_apctl_connect_tick, &current_tick, 1000ULL*apctl_connect_cooldown_ms);
				}
				apctl_state = new_apctl_state;
			}
		}
		sceRtcTickAddMicroseconds(&next_apctl_get_state_tick, &current_tick, 1000ULL*apclt_get_state_cooldown_ms);
	}

	if (apctl_state == PSP_NET_APCTL_STATE_DISCONNECTED) {
		if (sceRtcCompareTick(&current_tick, &next_apctl_connect_tick) >= 0) {
			sceNetApctlConnect(1);
			sceRtcTickAddMicroseconds(&next_apctl_connect_tick, &current_tick, 1000ULL*apctl_connect_cooldown_ms);
		}
	}
#endif
}

void pe_net_shutdown(void) {
    PE_ASSERT(network_initialized);
#if defined(_WIN32)
    WSACleanup();
#elif defined(PSP)
	sceNetApctlTerm();
    //sceNetResolverTerm();
    sceNetInetTerm();
    sceNetTerm();

    sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);
    sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
#endif
    network_initialized = false;
}

peAddress pe_address4(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port) {
    peAddress result = {0};
    result.type = peAddress_IPv4;
    result.ipv4 = (uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24);
    result.port = port;
    return result;
}

peAddress pe_address4_from_int(uint32_t address, uint16_t port) {
    peAddress result = {0};
    result.type = peAddress_IPv4;
    result.ipv4 = htonl(address);
    result.port = port;
    return result;
}

peAddress pe_address6(uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                 uint16_t e, uint16_t f, uint16_t g, uint16_t h,
                 uint16_t port) {
    peAddress result = {0};
    result.type = peAddress_IPv6;
    result.ipv6[0] = htons(a);
    result.ipv6[1] = htons(b);
    result.ipv6[2] = htons(c);
    result.ipv6[3] = htons(d);
    result.ipv6[4] = htons(e);
    result.ipv6[5] = htons(f);
    result.ipv6[6] = htons(g);
    result.ipv6[7] = htons(h);
    result.port = port;
    return result;
}

peAddress pe_address6_from_array(uint16_t address[], uint16_t port) {
    peAddress result = {0};
    result.type = peAddress_IPv6;
    for (int i = 0; i < 8; i++) {
        result.ipv6[i] = htons(address[i]);
    }
    result.port = port;
    return result;
}

#if defined(PSP)
static peAddress pe_address_from_sockaddr(struct sockaddr *addr) {
    struct peSockaddrDataLayout {
        uint8_t port_msb;
        uint8_t port_lsb;
        uint8_t a, b, c, d;
    };
    struct peSockaddrDataLayout *data = (void *)addr->sa_data;
    uint16_t port = (uint16_t)data->port_msb << 8 | data->port_lsb;
    peAddress result = pe_address4(data->a, data->b, data->c, data->d, port);
    return result;
}
#else
static peAddress pe_address_from_sockaddr_storage(struct sockaddr_storage *addr) {
    PE_ASSERT(addr->ss_family == AF_INET || addr->ss_family == AF_INET6);
    peAddress result = {0};
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *addr_ipv4 = (struct sockaddr_in *)addr;
        result.type = peAddress_IPv4;
        result.ipv4 = addr_ipv4->sin_addr.s_addr;
        result.port = ntohs(addr_ipv4->sin_port);
    } else if (addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *addr_ipv6 = (struct sockaddr_in6 *)addr;
        result.type = peAddress_IPv6;
        memcpy(result.ipv6, &addr_ipv6->sin6_addr, 16);
        result.port = ntohs(addr_ipv6->sin6_port);
    }
    return result;
}
#endif

peAddress pe_address_parse(char *address_in) {
    PE_ASSERT(address_in != NULL);
    peAddress result = {0};

    char buffer[256] = {0};
    char *address = buffer;
    int address_length = 0;
    while (address_length < PE_COUNT_OF(buffer) - 1 && address_in[address_length] != '\0') {
        buffer[address_length] = address_in[address_length];
        address_length += 1;
    }

    // TODO: Fix that. Although PSP has no IPv6 support
    // we can still let the user parse IPv6 addreesses.
#if !defined(PSP)
    // first try to parse as an IPv6 address:
    // 1. if the first character is '[' then it's probably an ipv6 in form "[addr6]:portnum"
    // 2. otherwise try to parse as raw IPv6 address, parse using inet_pton
    if (address[0] == '[') {
        const int base_index = address_length - 1;
        // note: no need to search past 6 characters as ":65535" is longest port value
        for (int i = 0; i < 6; ++i) {
            const int index = base_index - i;
            if (index < 3)
                break;
            if (address[index] == ':') {
                result.port = (uint16_t)atoi(&address[index + 1]);
                address[index - 1] = '\0';
            }
        }
        address += 1;
    }
    struct in6_addr sockaddr6;
    if (inet_pton(AF_INET6, address, &sockaddr6) == 1) {
        result.type = peAddress_IPv6;
        memcpy(result.ipv6, &sockaddr6, 16);
        return result;
    }
#endif

    // otherwise it's probably an IPv4 address:
    // 1. look for ":portnum", if found save the portnum and strip it out
    // 2. parse remaining ipv4 address via inet_pton
    address_length = (int)strlen(address);
    const int base_index = address_length - 1;
    // note: no need to search past 6 characters as ":65535" is longest port value
    for (int i = 0; i < 6; ++i) {
        const int index = base_index - i;
        if (index < 0)
            break;
        if (address[index] == ':') {
            result.port = (uint16_t)atoi(&address[index + 1]);
            address[index] = '\0';
        }
    }

    struct sockaddr_in sockaddr4;
    if (inet_pton(AF_INET, address, &sockaddr4.sin_addr) == 1) {
        result.type  = peAddress_IPv4;
        result.ipv4 = sockaddr4.sin_addr.s_addr;
    }
    else {
        // nope: it's not an IPv4 address. maybe it's a hostname? set address as undefined.
        result = (peAddress){0};
    }

    return result;
}

peAddress pe_address_parse_ex(char *address_in, uint16_t port) {
    peAddress result = pe_address_parse(address_in);
    result.port = port;
    return result;
}

#if defined(PSP) && !defined(INET6_ADDRSTRLEN)
#define INET6_ADDRSTRLEN 46
#endif
char *pe_address_to_string(peAddress address, char buffer[], int buffer_size) {
    if (address.type == peAddress_IPv4) {
        uint8_t a = address.ipv4 & 0xFF;
        uint8_t b = (address.ipv4 >> 8) & 0xFF;
        uint8_t c = (address.ipv4 >> 16) & 0xFF;
        uint8_t d = (address.ipv4 >> 24) & 0xFF;
        if (address.port != 0)
            snprintf(buffer, buffer_size, "%u.%u.%u.%u:%u", a, b, c, d, address.port);
        else
            snprintf(buffer, buffer_size, "%u.%u.%u.%u", a, b, c, d);
        return buffer;
    } else if (address.type == peAddress_IPv6) {
        if (address.port == 0) {
            inet_ntop(AF_INET6, (void *)&address.ipv6, buffer, buffer_size);
            return buffer;
        } else {
            char address_string[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, (void *)&address.ipv6, address_string, INET6_ADDRSTRLEN);
            snprintf(buffer, buffer_size, "[%s]:%u", address_string, address.port);
            return buffer;
        }
    } else {
        return "undefined";
    }
}

bool pe_address_is_valid(peAddress address) {
    return address.type != peAddress_Undefined;
}

bool pe_address_compare(peAddress a, peAddress b) {
    if (a.port != b.port)
        return false;
    if (a.type == peAddress_IPv4 && b.type == peAddress_IPv4) {
        if (a.ipv4 == b.ipv4)
            return true;
    }
    if (a.type == peAddress_IPv6 && b.type == peAddress_IPv6) {
        if (memcmp(a.ipv6, b.ipv6, sizeof(a.ipv6)) == 0)
            return true;
    }
    return false;
}

peSocket pe_socket_create(peSocketType type, uint16_t port) {
    //PE_ASSERT(network_initialized);
#if defined(PSP)
    PE_ASSERT(type != peSocket_IPv6);
#endif

    peSocket result = {0};

    // Create socket
    result.handle = socket((type == peSocket_IPv6) ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (result.handle <= 0) {
        result.error = peSocketError_CreateFailed;
        return result;
    }

    // Force IPv6 if necessary
    if (type == peSocket_IPv6) {
#if !defined(PSP)
        char optval = 1;
        if (setsockopt(result.handle, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) != 0)
        {
            printf("failed to set ipv6 only sockopt\n");
            result.error = peSocketError_SockoptIPv6OnlyFailed;
            return result;
        }
#endif
    }

    // Bind to port
    if (type == peSocket_IPv6) {
#if !defined(PSP)
        struct sockaddr_in6 sock_address;
        sock_address.sin6_family = AF_INET6;
        sock_address.sin6_addr = in6addr_any;
        sock_address.sin6_port = htons(port);

        if (bind(result.handle, (const struct sockaddr *)&sock_address, sizeof(sock_address)) < 0) {
            result.error = peSocketError_BindIPv6Failed;
            return result;
        }
#endif
    } else {
        struct sockaddr_in sock_address;
        sock_address.sin_family = AF_INET;
        sock_address.sin_addr.s_addr = INADDR_ANY;
        sock_address.sin_port = htons(port);

        if (bind(result.handle, (const struct sockaddr *)&sock_address, sizeof(sock_address)) < 0) {
            result.error = peSocketError_BindIPv4Failed;
            return result;
        }
    }

    // If bound to port 0 find the actual port we got
    result.port = port;
    if (result.port == 0) {
        if (type == peSocket_IPv6) {
#if !defined(PSP)
            struct sockaddr_in6 sin;
            socklen_t len = sizeof(sin);
            if (getsockname(result.handle, (struct sockaddr *)&sin, &len) == -1) {
                result.error = peSocketError_GetSocknameIPv6Failed;
                return result;
            }
            result.port = ntohs(sin.sin6_port);
#endif
        } else if (type == peSocket_IPv4) {
            struct sockaddr_in sin;
            socklen_t len = sizeof(sin);
            if (getsockname(result.handle, (struct sockaddr *)&sin, &len) == -1)
            {
                result.error = peSocketError_GetSocknameIPv4Failed;
                return result;
            }
            result.port = ntohs(sin.sin_port);
        }
    }

    // Set non-blocking IO
#if _WIN32
    DWORD non_blocking = 1;
    if (ioctlsocket(result.handle, FIONBIO, &non_blocking) != 0) {
        printf("failed to make socket non-blocking\n");
        result.error = peSocketError_SetNonBlockingFailed;
        return result;
    }
#else
    int non_blocking = 1;
    if (fcntl(result.handle, F_SETFL, O_NONBLOCK, non_blocking) == -1) {
        printf("failed to make socket non-blocking\n");
        result.error = peSocketError_SetNonBlockingFailed;
        return result;
    }
#endif
    return result;
}

void pe_socket_destroy(peSocket *socket) {
    if (socket->handle != 0) {
#if _WIN32
        closesocket(socket->handle);
#else
        close(socket->handle);
#endif
        socket->handle = 0;
    }
}

bool pe_socket_send(peSocket socket, peAddress address, void *packet_data, size_t packet_bytes) {
    PE_ASSERT(packet_data != NULL);
    PE_ASSERT(packet_bytes > 0);
    PE_ASSERT(pe_address_is_valid(address));
    PE_ASSERT(socket.handle != 0);
    PE_ASSERT_MSG(!pe_socket_is_error(socket), "socket error: %d\n", socket.error);

    bool result = false;
    if (address.type == peAddress_IPv6) {
#if !defined(PSP)
        struct sockaddr_in6 socket_address = {0};
        socket_address.sin6_family = AF_INET6;
        socket_address.sin6_port = htons(address.port);
        memcpy(&socket_address.sin6_addr, address.ipv6, sizeof(socket_address.sin6_addr));
        size_t sent_bytes = sendto(socket.handle, packet_data, (int)packet_bytes, 0, (struct sockaddr *)&socket_address, sizeof(socket_address));
        result = (sent_bytes == packet_bytes);
#endif
    } else if (address.type == peAddress_IPv4) {
        struct sockaddr_in socket_address = {0};
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = address.ipv4;
        socket_address.sin_port = htons(address.port);
        size_t sent_bytes = sendto(socket.handle, packet_data, (int)packet_bytes, 0, (struct sockaddr *)&socket_address, sizeof(socket_address));
        result = (sent_bytes == packet_bytes);
    }
    return result;
}

int pe_socket_receive(peSocket socket, peAddress *opt_from, void *packet_data, int max_packet_size) {
    PE_ASSERT(socket.handle != 0);
    PE_ASSERT(packet_data != NULL);
    PE_ASSERT(max_packet_size > 0);

#if defined(_WIN32)
    typedef int socklen_t;
#endif
#if !defined(PSP)
    struct sockaddr_storage sockaddr_from;
#else
    struct sockaddr sockaddr_from;
#endif
    socklen_t from_length = sizeof(sockaddr_from);
    int result = recvfrom(socket.handle, packet_data, max_packet_size, 0, (struct sockaddr *)&sockaddr_from, &from_length);

#if defined(_WIN32)
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        // TODO: Check if it is safe to ignore WSAECONNRESET
        if (error == WSAEWOULDBLOCK || error == WSAECONNRESET)
            return 0;
        printf("recvfrom failed: %d\n", error);
        printf("max packet size: %d\n", max_packet_size);
        return 0;
    }
#else
    if (result <= 0) {
        if (errno == EAGAIN)
            return 0;
        printf("recvfrom failed: %s\n", strerror(errno));
        return 0;
    }
#endif
    PE_ASSERT(result >= 0);
    if (opt_from != NULL) {
#if defined(PSP)
        *opt_from = pe_address_from_sockaddr(&sockaddr_from);
#else
        *opt_from = pe_address_from_sockaddr_storage(&sockaddr_from);
#endif
    }
    return result;
}

bool pe_socket_is_error(peSocket socket) {
    return socket.error != peSocketError_None;
}