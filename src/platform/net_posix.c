#ifndef _WIN32
#include "platform/net.h"
#include <stdlib.h>
#include <string.h>
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
