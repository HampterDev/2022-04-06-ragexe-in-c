#ifdef _WIN32
#include "platform/net.h"
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

typedef int (WINAPI *Ws2SendFn)(SOCKET s, const char *buf, int len, int flags);
typedef int (WINAPI *Ws2RecvFn)(SOCKET s, char *buf, int len, int flags);

struct PlatformSocket {
    SOCKET handle;
};

static HMODULE ws2_32;
static Ws2SendFn ws2_send;
static Ws2RecvFn ws2_recv;
static WSADATA wsa_data;

int platform_net_startup(void)
{
    if (WSAStartup(0x101, &wsa_data) != 0) {
        WSACleanup();
        return 0;
    }
    ws2_32 = LoadLibraryA("ws2_32.dll");
    ws2_send = (Ws2SendFn)GetProcAddress(ws2_32, "send");
    ws2_recv = (Ws2RecvFn)GetProcAddress(ws2_32, "recv");
    if (!ws2_send) {
        ws2_send = send;
        MessageBoxA(0, "GetProcAddress(\"send\") Failed.", "Module Hooking Error", 0);
    }
    if (!ws2_recv) {
        ws2_recv = recv;
        MessageBoxA(0, "GetProcAddress(\"recv\") Failed.", "Module Hooking Error", 0);
    }
    return 1;
}

void platform_net_shutdown(void)
{
    WSACleanup();
    FreeLibrary(ws2_32);
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

struct PlatformSocket *platform_socket_open(void)
{
    struct PlatformSocket *sock;
    SOCKET handle = socket(AF_INET, SOCK_STREAM, 0);
    if (handle == INVALID_SOCKET)
        return NULL;
    sock = malloc(sizeof *sock);
    if (!sock) {
        closesocket(handle);
        return NULL;
    }
    sock->handle = handle;
    return sock;
}

int platform_socket_set_nonblocking(struct PlatformSocket *sock)
{
    u_long non_blocking = 1;
    return ioctlsocket(sock->handle, FIONBIO, &non_blocking) != SOCKET_ERROR;
}

int platform_socket_connect(struct PlatformSocket *sock, uint32_t address, uint16_t port)
{
    struct sockaddr_in target;
    memset(&target, 0, sizeof target);
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = address;
    target.sin_port = htons(port);
    return connect(sock->handle, (struct sockaddr *)&target, sizeof target) != SOCKET_ERROR
        || WSAGetLastError() == WSAEWOULDBLOCK;
}

static int ready(struct PlatformSocket *sock, int writing)
{
    fd_set set;
    struct timeval no_wait = { 0, 0 };
    set.fd_count = 1;
    set.fd_array[0] = sock->handle;
    return select(0, writing ? 0 : &set, writing ? &set : 0, 0, &no_wait);
}

int platform_socket_readable(struct PlatformSocket *sock)
{
    return ready(sock, 0);
}

int platform_socket_writable(struct PlatformSocket *sock)
{
    return ready(sock, 1);
}

static int transfer_error(void)
{
    int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAENOTCONN ? PLATFORM_SOCKET_RETRY : PLATFORM_SOCKET_FAILED;
}

int platform_socket_send(struct PlatformSocket *sock, const void *data, int size)
{
    int sent = ws2_send(sock->handle, data, size, 0);
    return sent != SOCKET_ERROR ? sent : transfer_error();
}

int platform_socket_recv(struct PlatformSocket *sock, void *buffer, int size)
{
    int got = ws2_recv(sock->handle, buffer, size, 0);
    return got != SOCKET_ERROR ? got : transfer_error();
}

void platform_socket_close(struct PlatformSocket *sock)
{
    closesocket(sock->handle);
    free(sock);
}
#endif
