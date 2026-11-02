#include "platform/net.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
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
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef MSG_NOSIGNAL
#define SEND_FLAGS MSG_NOSIGNAL
#else
#define SEND_FLAGS 0
#endif

enum { SOCKET_CONNECTING, SOCKET_CONNECTED, SOCKET_REFUSED };

struct PlatformSocket {
    int fd;
    int state;
};

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

static void suppress_sigpipe(int fd)
{
#ifdef __APPLE__
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof on);
#else
    (void)fd;
#endif
}

struct PlatformSocket *platform_socket_open(void)
{
    struct PlatformSocket *sock;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return NULL;
    sock = malloc(sizeof *sock);
    if (!sock) {
        close(fd);
        return NULL;
    }
    suppress_sigpipe(fd);
    sock->fd = fd;
    sock->state = SOCKET_CONNECTING;
    return sock;
}

int platform_socket_set_nonblocking(struct PlatformSocket *sock)
{
    int flags = fcntl(sock->fd, F_GETFL, 0);
    return flags != -1 && fcntl(sock->fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int platform_socket_connect(struct PlatformSocket *sock, uint32_t address, uint16_t port)
{
    struct sockaddr_in target;
    memset(&target, 0, sizeof target);
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = address;
    target.sin_port = htons(port);
    if (connect(sock->fd, (struct sockaddr *)&target, sizeof target) == 0) {
        sock->state = SOCKET_CONNECTED;
        return 1;
    }
    return errno == EINPROGRESS || errno == EINTR;
}

static int poll_events(int fd, short events)
{
    struct pollfd entry;
    int ready;
    entry.fd = fd;
    entry.events = events;
    entry.revents = 0;
    ready = poll(&entry, 1, 0);
    if (ready <= 0)
        return ready;
    return (entry.revents & (events | POLLERR | POLLHUP | POLLNVAL)) != 0;
}

static int established(struct PlatformSocket *sock)
{
    int error = 0, ready;
    socklen_t size = sizeof error;
    if (sock->state != SOCKET_CONNECTING)
        return sock->state == SOCKET_CONNECTED;
    ready = poll_events(sock->fd, POLLOUT);
    if (ready <= 0)
        return ready;
    if (getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &error, &size) != 0 || error != 0) {
        sock->state = SOCKET_REFUSED;
        return 0;
    }
    sock->state = SOCKET_CONNECTED;
    return 1;
}

int platform_socket_readable(struct PlatformSocket *sock)
{
    int ready = established(sock);
    return ready <= 0 ? ready : poll_events(sock->fd, POLLIN);
}

int platform_socket_writable(struct PlatformSocket *sock)
{
    int ready = established(sock);
    return ready <= 0 ? ready : poll_events(sock->fd, POLLOUT);
}

static int transfer_error(void)
{
    return errno == EWOULDBLOCK || errno == EAGAIN || errno == ENOTCONN || errno == EINTR
        ? PLATFORM_SOCKET_RETRY : PLATFORM_SOCKET_FAILED;
}

int platform_socket_send(struct PlatformSocket *sock, const void *data, int size)
{
    ssize_t sent = send(sock->fd, data, (size_t)size, SEND_FLAGS);
    return sent >= 0 ? (int)sent : transfer_error();
}

int platform_socket_recv(struct PlatformSocket *sock, void *buffer, int size)
{
    ssize_t got = recv(sock->fd, buffer, (size_t)size, 0);
    return got >= 0 ? (int)got : transfer_error();
}

void platform_socket_close(struct PlatformSocket *sock)
{
    close(sock->fd);
    free(sock);
}
#endif
