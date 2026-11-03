/* Sockets that never leave the process: connects succeed at once, what the client sends is cut
   into packets and handed to the scene driver, and what the driver delivers is read back by the
   client as if it came from a server. */
#include "platform/net.h"
#include "contents/connection_api.h"
#include "scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PlatformSocket {
    int port;
    unsigned char *in;
    size_t in_size, in_read, in_capacity;
    unsigned char out[0x10000];
    size_t out_size;
    struct PlatformSocket *next;
};

static struct PlatformSocket *g_sockets;
static SceneServerPacket g_handler;

void scene_net_set_handler(SceneServerPacket handler)
{
    g_handler = handler;
}

static struct PlatformSocket *find(int port)
{
    struct PlatformSocket *s;
    for (s = g_sockets; s; s = s->next)
        if (s->port == port)
            return s;
    return NULL;
}

int scene_net_connected(int port)
{
    return find(port) != NULL;
}

int scene_net_deliver(int port, const void *data, int size)
{
    struct PlatformSocket *s = find(port);
    if (!s || size < 0)
        return 0;
    if (s->in_size + (size_t)size > s->in_capacity) {
        size_t capacity = s->in_capacity ? s->in_capacity : 0x1000;
        unsigned char *grown;
        while (capacity < s->in_size + (size_t)size)
            capacity *= 2;
        grown = realloc(s->in, capacity);
        if (!grown)
            return 0;
        s->in = grown;
        s->in_capacity = capacity;
    }
    memcpy(s->in + s->in_size, data, (size_t)size);
    s->in_size += (size_t)size;
    return 1;
}

static int packet_length(const unsigned char *data, size_t size)
{
    int id, length;
    if (size < 2)
        return 0;
    id = data[0] | data[1] << 8;
    length = rag_connection_get_packet_length(rag_connection_instance(), id);
    if (length == -1) {
        if (size < 4)
            return 0;
        length = data[2] | data[3] << 8;
    }
    if (id == 0xf3) {
        /* CZ_REQUEST_CHAT: the table holds what the client reads, and 0xf3 reads as another packet. */
        if (size < 4)
            return 0;
        length = data[2] | data[3] << 8;
    }
    if (length < 2)
        length = id == 0x436 ? 19 : (int)size;
    return (size_t)length <= size ? length : 0;
}

static void dispatch(struct PlatformSocket *s)
{
    size_t done = 0;
    int length;
    while ((length = packet_length(s->out + done, s->out_size - done)) > 0) {
        if (g_handler)
            g_handler(s->port, s->out + done, length);
        done += (size_t)length;
    }
    memmove(s->out, s->out + done, s->out_size - done);
    s->out_size -= done;
}

int platform_net_startup(void)
{
    return 1;
}

void platform_net_shutdown(void)
{
}

uint32_t platform_net_parse_ipv4(const char *text)
{
    unsigned a = 127, b = 0, c = 0, d = 1;
    if (text)
        sscanf(text, "%u.%u.%u.%u", &a, &b, &c, &d);
    return (a & 0xff) | (b & 0xff) << 8 | (c & 0xff) << 16 | (uint32_t)(d & 0xff) << 24;
}

int platform_net_resolve_ipv4(const char *host, uint32_t *address)
{
    *address = platform_net_parse_ipv4(host);
    return 1;
}

const char *platform_net_ipv4_text(uint32_t address)
{
    static char text[16];
    snprintf(text, sizeof text, "%u.%u.%u.%u", address & 0xff, (address >> 8) & 0xff, (address >> 16) & 0xff,
             address >> 24);
    return text;
}

uint32_t platform_net_to_host32(uint32_t value)
{
    return (value >> 24) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | (value << 24);
}

struct PlatformSocket *platform_socket_open(void)
{
    return calloc(1, sizeof(struct PlatformSocket));
}

int platform_socket_set_nonblocking(struct PlatformSocket *sock)
{
    (void)sock;
    return 1;
}

int platform_socket_connect(struct PlatformSocket *sock, uint32_t address, uint16_t port)
{
    (void)address;
    sock->port = port;
    sock->next = g_sockets;
    g_sockets = sock;
    return 1;
}

int platform_socket_readable(struct PlatformSocket *sock)
{
    return sock->in_read < sock->in_size;
}

int platform_socket_writable(struct PlatformSocket *sock)
{
    (void)sock;
    return 1;
}

int platform_socket_send(struct PlatformSocket *sock, const void *data, int size)
{
    size_t room = sizeof sock->out - sock->out_size;
    if (size <= 0)
        return 0;
    if ((size_t)size > room)
        size = (int)room;
    memcpy(sock->out + sock->out_size, data, (size_t)size);
    sock->out_size += (size_t)size;
    dispatch(sock);
    return size;
}

int platform_socket_recv(struct PlatformSocket *sock, void *buffer, int size)
{
    size_t left = sock->in_size - sock->in_read;
    if (!left)
        return PLATFORM_SOCKET_RETRY;
    if ((size_t)size < left)
        left = (size_t)size;
    memcpy(buffer, sock->in + sock->in_read, left);
    sock->in_read += left;
    if (sock->in_read == sock->in_size)
        sock->in_read = sock->in_size = 0;
    return (int)left;
}

void platform_socket_close(struct PlatformSocket *sock)
{
    struct PlatformSocket **link;
    if (getenv("RAGEXE_SCENE_TRACE"))
        fprintf(stderr, "scene: client closed port %d with %zu bytes unread\n", sock->port, sock->in_size - sock->in_read);
    for (link = &g_sockets; *link; link = &(*link)->next)
        if (*link == sock) {
            *link = sock->next;
            break;
        }
    free(sock->in);
    free(sock);
}
