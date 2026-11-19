#ifndef PLATFORM_NET_H
#define PLATFORM_NET_H

#include <stdint.h>

enum {
    PLATFORM_SOCKET_FAILED = -1,
    PLATFORM_SOCKET_RETRY = -2
};

/* A socket's channels. A packet lost on one holds back only what follows it on the same one. */
enum {
    PLATFORM_CHANNEL_WORLD = 0,  /* in order, resent until it arrives: everything not named below */
    PLATFORM_CHANNEL_LISTS = 1,  /* in order, resent: text that names no actor, windows' contents */
    PLATFORM_CHANNEL_NEWEST = 2, /* never resent, and dropped when a newer one arrived first */
    /* The delivery piece's (delivery/delivery.h, plans/delivery.md). What is sent on it arrives
       once and in the order sent, and is sent again every 15 ms until the server has it: what
       the player asks for. Together with what is sent on the world channel it is one stream in
       the order of the sends: a request goes on the world channel while world packets sent
       before it are on their way, and a world packet through the piece while requests sent
       before it are in it. What arrives on it is in no order. A send that is more than the
       piece takes waits, or goes on the world channel when nothing is in the piece, and so does
       everything on a link without this channel. */
    PLATFORM_CHANNEL_DELIVERY = 3,
    PLATFORM_SOCKET_CHANNELS = 4
};

struct PlatformSocket;

int platform_net_startup(void);
void platform_net_shutdown(void);
int platform_net_resolve_ipv4(const char *host, uint32_t *address);
uint32_t platform_net_parse_ipv4(const char *text);
const char *platform_net_ipv4_text(uint32_t address);
uint32_t platform_net_to_host32(uint32_t value);

/* A socket is a link to a server carried by ENet over UDP (platform/net_enet.c), with one
   byte stream per channel. What is sent in one call is whole packets and arrives together or,
   on the newest channel, not at all. What arrives on the delivery channel is whole packets
   too, one message of the piece after another. Only the close waits, and briefly: a socket is not
   writable until the server has answered the connect, and one whose server never answers or
   went away reads as closed once everything that arrived was taken. */
struct PlatformSocket *platform_socket_open(void);
int platform_socket_connect(struct PlatformSocket *sock, uint32_t address, uint16_t port);
int platform_socket_readable(struct PlatformSocket *sock);
int platform_socket_writable(struct PlatformSocket *sock);
int platform_socket_send(struct PlatformSocket *sock, int channel, const void *data, int size);
int platform_socket_recv(struct PlatformSocket *sock, int channel, void *buffer, int size);
void platform_socket_close(struct PlatformSocket *sock);

#endif
