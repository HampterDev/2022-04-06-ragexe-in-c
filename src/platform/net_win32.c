#ifdef _WIN32
#include "platform/net.h"
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

/* Winsock's start and the address helpers. The sockets themselves are platform/net_enet.c. */

static WSADATA wsa_data;

int platform_net_startup(void)
{
    if (WSAStartup(0x101, &wsa_data) != 0) {
        WSACleanup();
        return 0;
    }
    return 1;
}

void platform_net_shutdown(void)
{
    WSACleanup();
}

int platform_net_resolve_ipv4(const char *host, uint32_t *address)
{
    struct hostent *found = gethostbyname(host);
    if (!found)
        return 0;
    memcpy(address, found->h_addr_list[0], sizeof *address);
    return 1;
}

uint32_t platform_net_parse_ipv4(const char *text)
{
    return inet_addr(text);
}

const char *platform_net_ipv4_text(uint32_t address)
{
    struct in_addr value;
    value.s_addr = address;
    return inet_ntoa(value);
}

uint32_t platform_net_to_host32(uint32_t value)
{
    return ntohl(value);
}
#endif
