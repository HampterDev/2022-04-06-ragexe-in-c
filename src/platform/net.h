#ifndef PLATFORM_NET_H
#define PLATFORM_NET_H

#include <stdint.h>

enum {
    PLATFORM_SOCKET_FAILED = -1,
    PLATFORM_SOCKET_RETRY = -2
};

struct PlatformSocket;

int platform_net_startup(void);
void platform_net_shutdown(void);
int platform_net_resolve_ipv4(const char *host, uint32_t *address);
uint32_t platform_net_parse_ipv4(const char *text);
const char *platform_net_ipv4_text(uint32_t address);
uint32_t platform_net_to_host32(uint32_t value);

struct PlatformSocket *platform_socket_open(void);
int platform_socket_set_nonblocking(struct PlatformSocket *sock);
int platform_socket_connect(struct PlatformSocket *sock, uint32_t address, uint16_t port);
int platform_socket_readable(struct PlatformSocket *sock);
int platform_socket_writable(struct PlatformSocket *sock);
int platform_socket_send(struct PlatformSocket *sock, const void *data, int size);
int platform_socket_recv(struct PlatformSocket *sock, void *buffer, int size);
void platform_socket_close(struct PlatformSocket *sock);

#endif
