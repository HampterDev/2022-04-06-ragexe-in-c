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
#define LAG_MAX_MS 5000

static void lag_line_reset(struct LagLine *line)
{
    byte_queue_reset(&line->bytes, CONNECTION_QUEUE_SIZE);
    line->first = 0;
    line->used = 0;
}

static void lag_line_push(struct LagLine *line, DWORD due, int channel, int count, const void *bytes)
{
    struct LagChunk *chunk;

    byte_queue_write(&line->bytes, count, bytes);
    if (line->used == LAG_LINE_CHUNKS) {
        /* full: the newest chunk takes the bytes and waits for the later time */
        chunk = &line->chunks[(line->first + line->used - 1) % LAG_LINE_CHUNKS];
        chunk->due = due;
        chunk->count += count;
        return;
    }
    chunk = &line->chunks[(line->first + line->used) % LAG_LINE_CHUNKS];
    chunk->due = due;
    chunk->channel = channel;
    chunk->count = count;
    line->used++;
}

/* Moves every chunk whose time has come to its channel's queue of `out`. */
static void lag_line_release(struct LagLine *line, DWORD now, struct ByteQueue *out)
{
    while (line->used) {
        struct LagChunk *chunk = &line->chunks[line->first];

        if ((int)(now - chunk->due) < 0)
            break;
        byte_queue_write(&out[chunk->channel], chunk->count, line->bytes.data + line->bytes.read_pos);
        byte_queue_skip(&line->bytes, chunk->count);
        line->first = (line->first + 1) % LAG_LINE_CHUNKS;
        line->used--;
    }
}

static unsigned lag_setting(void)
{
    const char *text = getenv("RAGEXE_LAG_MS");
    long ms = text ? strtol(text, NULL, 10) : 0;

    if (ms < 0)
        return 0;
    return ms > LAG_MAX_MS ? LAG_MAX_MS : (unsigned)ms;
}

/* Queues whole packets for a channel of the socket, through the latency rig when it is on. */
void connection_queue_send(struct Connection *self, int channel, int count, const void *bytes)
{
    if (self->lag_ms)
        lag_line_push(&self->lag_out, platform_ticks_ms() + self->lag_ms, channel, count, bytes);
    else
        byte_queue_write(&self->send_queue[channel], count, bytes);
}

static void reset_channel_queues(struct ByteQueue *queues)
{
    int channel;

    for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++)
        byte_queue_reset(&queues[channel], CONNECTION_QUEUE_SIZE);
}

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
    int channel;

    memset(self, 0, sizeof *self);
    self->socket = NULL;
    self->hold_sends = 0;
    self->send_after = 0;
    for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++) {
        byte_queue_construct(&self->send_queue[channel]);
        byte_queue_construct(&self->recv_queue[channel]);
    }
    byte_queue_construct(&self->held);
    byte_queue_construct(&self->lag_out.bytes);
    byte_queue_construct(&self->lag_in.bytes);
    reset_channel_queues(self->recv_queue);
    reset_channel_queues(self->send_queue);
    byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
    lag_line_reset(&self->lag_out);
    lag_line_reset(&self->lag_in);
    self->lag_ms = lag_setting();
    return self;
}

struct RagConnection *rag_connection_construct(struct RagConnection *self)
{
    connection_construct(&self->base);
    return self;
}

void connection_destruct(struct Connection *self)
{
    int channel;

    if (self->kept)
        platform_socket_close(self->kept);
    byte_queue_destroy(&self->lag_in.bytes);
    byte_queue_destroy(&self->lag_out.bytes);
    byte_queue_destroy(&self->held);
    for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++) {
        byte_queue_destroy(&self->recv_queue[channel]);
        byte_queue_destroy(&self->send_queue[channel]);
    }
}

void connection_disconnect(struct Connection *self)
{
    struct IntInt64Pair *var;
    int key = 5;

    if (self->socket) {
        platform_socket_close(self->socket);
        self->socket = NULL;
        reset_channel_queues(self->send_queue);
        byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
        lag_line_reset(&self->lag_out);
    }
    self->failed = 0;
    var = tree_get(&g_session1.instantVars, &key, sizeof key, sizeof *var, tree_less_int);
    var->value = 0;
}

void connection_hop(struct Connection *self)
{
    if (self->socket && !self->kept) {
        self->kept = self->socket;
        self->socket = NULL;
        reset_channel_queues(self->send_queue);
        byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
        lag_line_reset(&self->lag_out);
    }
    connection_disconnect(self);
}

/* Keeps a kept socket's link alive and throws away what arrives on it: nothing is owed to a
   stage that has ended. Returns 0 when the server has closed it. */
static int service_kept(struct PlatformSocket *kept)
{
    char chunk[RECV_CHUNK];
    int channel, got;

    if (platform_socket_readable(kept) <= 0)
        return 1;
    for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++)
        while ((got = platform_socket_recv(kept, channel, chunk, RECV_CHUNK)) != PLATFORM_SOCKET_RETRY)
            if (got <= 0)
                return 0;
    return 1;
}

char connection_flush(struct Connection *self)
{
    lag_line_release(&self->lag_out, platform_ticks_ms(), self->send_queue);
    while (self->send_after < platform_ticks_ms()) {
        struct ByteQueue *queue;
        int ready, sent, channel;

        for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++)
            if (byte_queue_size(&self->send_queue[channel]) != 0)
                break;
        if (channel == PLATFORM_SOCKET_CHANNELS)
            break;
        queue = &self->send_queue[channel];
        ready = platform_socket_writable(self->socket);
        if (ready < 0)
            continue;
        if (ready == 0)
            break;
        sent = platform_socket_send(self->socket, channel, queue->data + queue->read_pos,
                                    queue->write_pos - queue->read_pos);
        if (sent >= 0) {
            byte_queue_skip(queue, sent);
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

    lag_line_release(&self->lag_in, platform_ticks_ms(), self->recv_queue);
    for (;;) {
        int ready, got, channel, took = 0;

        ready = platform_socket_readable(self->socket);
        if (ready <= 0)
            return 1;
        for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++) {
            got = platform_socket_recv(self->socket, channel, chunk, RECV_CHUNK);
            if (got == PLATFORM_SOCKET_RETRY)
                continue;
            if (got <= 0) {
                connection_disconnect(self);
                return 0;
            }
            took = 1;
            if (self->lag_ms)
                lag_line_push(&self->lag_in, platform_ticks_ms() + self->lag_ms, channel, got, chunk);
            else
                byte_queue_write(&self->recv_queue[channel], got, chunk);
        }
        if (!took)
            return 1;
    }
}

char connection_connect(struct Connection *self, const struct ServerAddress *address)
{
    char ip[sizeof *address];
    const char *colon;
    uint32_t resolved, link_address;
    int port;
    connection_set_hold_sends(self, 0);
    reset_channel_queues(self->recv_queue);
    reset_channel_queues(self->send_queue);
    byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
    lag_line_reset(&self->lag_out);
    lag_line_reset(&self->lag_in);

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

    link_address = platform_net_parse_ipv4(ip);
    if (self->kept) {
        struct PlatformSocket *kept = self->kept;

        self->kept = NULL;
        /* the next stage of a login, at the address the last one used: the same link */
        if (self->link_address == link_address && self->link_port == (uint16_t)port && service_kept(kept)) {
            self->socket = kept;
            self->failed = 0;
            return 1;
        }
        platform_socket_close(kept);
    }
    self->socket = platform_socket_open();
    if (!self->socket)
        return 0;
    if (!platform_socket_connect(self->socket, link_address, (uint16_t)port)) {
        connection_disconnect(self);
        return 0;
    }
    self->link_address = link_address;
    self->link_port = (uint16_t)port;
    self->failed = 0;
    return 1;
}

char connection_poll(struct Connection *self)
{
    if (self->kept && !service_kept(self->kept)) {
        platform_socket_close(self->kept);
        self->kept = NULL;
    }
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
    struct ByteQueue *queue = &self->recv_queue[PLATFORM_CHANNEL_WORLD];

    if (exact)
        return byte_queue_read(queue, count, dst) ? count : 0;
    if (count >= byte_queue_size(queue))
        count = byte_queue_size(queue);
    byte_queue_read(queue, count, dst);
    return count;
}

void connection_set_hold_sends(struct Connection *self, char hold)
{
    int count;
    self->hold_sends = hold;
    if (hold || byte_queue_size(&self->held) == 0)
        return;
    while ((count = self->held.write_pos - self->held.read_pos) != 0) {
        connection_queue_send(self, PLATFORM_CHANNEL_WORLD, count, self->held.data + self->held.read_pos);
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
    reset_channel_queues(self->recv_queue);
    reset_channel_queues(self->send_queue);
    byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
    lag_line_reset(&self->lag_out);
    lag_line_reset(&self->lag_in);
}

void connection_delay_sends(struct Connection *connection, unsigned milliseconds)
{
    connection->send_after = platform_ticks_ms() + milliseconds;
}

int connection_is_open(const struct Connection *connection)
{
    return connection->socket != NULL;
}
