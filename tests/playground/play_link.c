/* The playground's line to its runner (tests/playground/README.md): one blocking TCP connection
   to this machine, over which the driver and the runner take turns, a frame at a time. Kept
   apart from the driver so that the socket headers meet none of the client's. */
#include "play.h"
#include <winsock2.h>
#include <ws2tcpip.h>

static SOCKET g_link = INVALID_SOCKET;

int play_link_open(int port)
{
    WSADATA data;
    struct sockaddr_in to;
    int on = 1;

    if (WSAStartup(MAKEWORD(2, 2), &data))
        return 0;
    g_link = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_link == INVALID_SOCKET)
        return 0;
    memset(&to, 0, sizeof to);
    to.sin_family = AF_INET;
    to.sin_port = htons((unsigned short)port);
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(g_link, (const struct sockaddr *)&to, sizeof to))
        return 0;
    setsockopt(g_link, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof on);
    return 1;
}

int play_link_send(const void *data, size_t size)
{
    const char *at = data;

    while (size) {
        int sent = send(g_link, at, size > 0x10000 ? 0x10000 : (int)size, 0);

        if (sent <= 0)
            return 0;
        at += sent;
        size -= (size_t)sent;
    }
    return 1;
}

int play_link_receive(void *data, size_t size)
{
    char *at = data;

    while (size) {
        int got = recv(g_link, at, size > 0x10000 ? 0x10000 : (int)size, 0);

        if (got <= 0)
            return 0;
        at += got;
        size -= (size_t)got;
    }
    return 1;
}
