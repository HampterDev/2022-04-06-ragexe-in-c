#include "std/tree.h"
#include "session/state.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "contents/connection.h"
#include "std/std.h"
#include "contents/ui_window_mgr.h"
#include "platform/clock.h"
#include "platform/window.h"

#define RECV_CHUNK 0x800

char connection_startup(void)
{
    if (!platform_net_startup()) {
        error_msg("Failed to load Winsock library!", 0);
        return 0;
    }
    rag_connection_instance();
    return 1;
}

void connection_shutdown(void)
{
    platform_net_shutdown();
}

struct Connection *connection_construct(struct Connection *self)
{
    memset(self, 0, sizeof *self);
    self->socket = NULL;
    self->hold_sends = 0;
    self->send_after = 0;
    byte_queue_construct(&self->send_queue);
    byte_queue_construct(&self->recv_queue);
    byte_queue_construct(&self->held);
    byte_queue_reset(&self->recv_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&self->send_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
    return self;
}

struct RagConnection *rag_connection_construct(struct RagConnection *self)
{
    connection_construct(&self->base);
    return self;
}

void connection_destruct(struct Connection *self)
{
    byte_queue_destroy(&self->held);
    byte_queue_destroy(&self->recv_queue);
    byte_queue_destroy(&self->send_queue);
}

void connection_disconnect(struct Connection *self)
{
    struct IntInt64Pair *var;
    int key = 5;

    if (self->socket) {
        platform_socket_close(self->socket);
        self->socket = NULL;
        byte_queue_reset(&self->send_queue, CONNECTION_QUEUE_SIZE);
        byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
    }
    self->failed = 0;
    var = tree_get(&g_session1.instantVars, &key, sizeof key, sizeof *var, tree_less_int);
    var->value = 0;
}

char connection_flush(struct Connection *self)
{
    while (self->send_after < platform_ticks_ms()) {
        int ready, sent;

        if (byte_queue_size(&self->send_queue) == 0)
            break;
        ready = platform_socket_writable(self->socket);
        if (ready < 0)
            continue;
        if (ready == 0)
            break;
        sent = platform_socket_send(self->socket, self->send_queue.data + self->send_queue.read_pos,
                                    self->send_queue.write_pos - self->send_queue.read_pos);
        if (sent >= 0) {
            byte_queue_skip(&self->send_queue, sent);
            continue;
        }
        if (sent != PLATFORM_SOCKET_RETRY) {
            connection_disconnect(self);
            return 0;
        }
    }
    return 1;
}

static char connection_receive(struct Connection *self)
{
    char chunk[RECV_CHUNK];

    for (;;) {
        int ready, got;

        ready = platform_socket_readable(self->socket);
        if (ready <= 0)
            return 1;
        got = platform_socket_recv(self->socket, chunk, RECV_CHUNK);
        if (got == PLATFORM_SOCKET_RETRY)
            continue;
        if (got <= 0) {
            connection_disconnect(self);
            return 0;
        }
        byte_queue_write(&self->recv_queue, got, chunk);
    }
}

char connection_connect(struct Connection *self, const struct ServerAddress *address)
{
    char ip[sizeof *address];
    const char *colon;
    uint32_t resolved;
    int port;
    connection_set_hold_sends(self, 0);
    byte_queue_reset(&self->recv_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&self->send_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);

    self->socket = platform_socket_open();
    if (!self->socket)
        return 0;
    if (!platform_socket_set_nonblocking(self->socket)) {
        error_msg("Failed to setup select mode", 0);
        return 0;
    }

    strcpy(ip, address->ip);
    colon = strrchr(address->host, ':');
    if (colon) {
        char name[sizeof address->host];

        memcpy(name, address->host, colon - address->host);
        name[colon - address->host] = 0;
        if (platform_net_resolve_ipv4(name, &resolved))
            strcpy(ip, platform_net_ipv4_text(resolved));
    }

    port = address->port;
    if (colon)
        port = atoi(colon + 1);

    if (!platform_socket_connect(self->socket, platform_net_parse_ipv4(ip), (uint16_t)port)) {
        connection_disconnect(self);
        return 0;
    }
    self->failed = 0;
    return 1;
}

char connection_poll(struct Connection *self)
{
    if (!self->socket)
        return 1;
    if (!connection_receive(self))
        return 0;
    if (!connection_flush(self))
        return 0;
    return 1;
}

int connection_recv_bytes(struct Connection *self, void *dst, int count, char exact)
{
    if (exact)
        return byte_queue_read(&self->recv_queue, count, dst) ? count : 0;
    if (count >= byte_queue_size(&self->recv_queue))
        count = byte_queue_size(&self->recv_queue);
    byte_queue_read(&self->recv_queue, count, dst);
    return count;
}

void connection_set_hold_sends(struct Connection *self, char hold)
{
    int count;
    self->hold_sends = hold;
    if (hold || byte_queue_size(&self->held) == 0)
        return;
    while ((count = self->held.write_pos - self->held.read_pos) != 0) {
        byte_queue_write(&self->send_queue, count, self->held.data + self->held.read_pos);
        byte_queue_skip(&self->held, count);
        connection_flush(self);
    }
}

void connection_clear_held(struct Connection *self)
{
    byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
}

void connection_reset_queues(struct Connection *self)
{
    byte_queue_reset(&self->recv_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&self->send_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
}

void connection_delay_sends(struct Connection *connection, unsigned milliseconds)
{
    connection->send_after = platform_ticks_ms() + milliseconds;
}

int connection_is_open(const struct Connection *connection)
{
    return connection->socket != NULL;
}
