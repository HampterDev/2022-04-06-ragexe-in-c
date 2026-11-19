#ifndef _WIN32
#include "platform/net.h"
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

/* The address helpers. The sockets themselves are platform/net_enet.c. */

int platform_net_startup(void)
{
    return 1;
}

void platform_net_shutdown(void)
{
}

int platform_net_resolve_ipv4(const char *host, uint32_t *address)
{
    struct addrinfo hints, *found;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &found) != 0)
        return 0;
    *address = ((const struct sockaddr_in *)found->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(found);
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
