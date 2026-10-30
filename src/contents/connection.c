#include "std/tree.h"
#include "session/state.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "contents/connection.h"
#include "std/std.h"
#include "contents/ui_window_mgr.h"
#include <mmsystem.h>

typedef int (__stdcall *Ws2TransferFn)(SOCKET s, char *buf, int len, int flags);

HMODULE g_ws2_32;
Ws2TransferFn g_ws2_send;
Ws2TransferFn g_ws2_recv;
WSADATA g_wsa_data;
#define RECV_CHUNK 0x800

static const char *const ws2_checked[4] = { "connect", "closesocket", "send", "select" };

char connection_ws2_unhooked(void)
{
    HMODULE module = LoadLibraryA("ws2_32.dll");
    unsigned i;

    if (!module)
        return 1;
    for (i = 0; i < 4; i++) {
        if (GetProcAddress(module, ws2_checked[i]) != GetProcAddress(g_ws2_32, ws2_checked[i])) {
            FreeLibrary(module);
            return 0;
        }
    }
    FreeLibrary(module);
    return 1;
}

char connection_startup(void)
{
    if (WSAStartup(0x101, &g_wsa_data) != 0) {
        error_msg("Failed to load Winsock library!", 0);
        WSACleanup();
        return 0;
    }
    g_ws2_32 = LoadLibraryA("ws2_32.dll");
    g_ws2_send = (Ws2TransferFn)GetProcAddress(g_ws2_32, "send");
    g_ws2_recv = (Ws2TransferFn)GetProcAddress(g_ws2_32, "recv");
    if (!g_ws2_send) {
        g_ws2_send = (Ws2TransferFn)send;
        MessageBoxA(0, "GetProcAddress(\"send\") Failed.", "Module Hooking Error", 0);
    }
    if (!g_ws2_recv) {
        g_ws2_recv = (Ws2TransferFn)recv;
        MessageBoxA(0, "GetProcAddress(\"recv\") Failed.", "Module Hooking Error", 0);
    }
    rag_connection_instance();
    connection_ws2_unhooked();
    return 1;
}

void connection_shutdown(void)
{
    WSACleanup();
    FreeLibrary(g_ws2_32);
}

struct Connection *connection_construct(struct Connection *self)
{
    memset(self, 0, sizeof *self);
    self->socket = INVALID_SOCKET;
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
    self->drop_sends = 0;
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

    if (self->socket != INVALID_SOCKET) {
        closesocket(self->socket);
        self->socket = INVALID_SOCKET;
        byte_queue_reset(&self->send_queue, CONNECTION_QUEUE_SIZE);
        byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);
    }
    self->failed = 0;
    var = tree_get(&g_session1.instantVars, &key, sizeof key, sizeof *var, tree_less_int);
    var->value = 0;
}

char connection_flush(struct Connection *self)
{
    while (self->send_after < timeGetTime()) {
        fd_set writable;
        struct timeval no_wait = { 0, 0 };
        int ready, sent, err;

        if (byte_queue_size(&self->send_queue) == 0)
            break;
        writable.fd_count = 1;
        writable.fd_array[0] = self->socket;
        ready = select(0, 0, &writable, 0, &no_wait);
        if (ready == SOCKET_ERROR)
            continue;
        if (ready <= 0)
            break;
        sent = g_ws2_send(self->socket, self->send_queue.data + self->send_queue.read_pos,
                          self->send_queue.write_pos - self->send_queue.read_pos, 0);
        if (sent != SOCKET_ERROR) {
            byte_queue_skip(&self->send_queue, sent);
            continue;
        }
        err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAENOTCONN) {
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
        fd_set readable;
        struct timeval no_wait = { 0, 0 };
        int ready, got, err;

        readable.fd_count = 1;
        readable.fd_array[0] = self->socket;
        ready = select(0, &readable, 0, 0, &no_wait);
        if (ready == SOCKET_ERROR || ready <= 0)
            return 1;
        got = g_ws2_recv(self->socket, chunk, RECV_CHUNK, 0);
        if (got == SOCKET_ERROR) {
            err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAENOTCONN)
                continue;
            connection_disconnect(self);
            return 0;
        }
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
    u_long non_blocking = 1;
    int port, err;
    connection_set_hold_sends(self, 0);
    byte_queue_reset(&self->recv_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&self->send_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&self->held, CONNECTION_QUEUE_SIZE);

    self->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (self->socket == INVALID_SOCKET)
        return 0;
    if (ioctlsocket(self->socket, FIONBIO, &non_blocking) == SOCKET_ERROR) {
        error_msg("Failed to setup select mode", 0);
        return 0;
    }
    memset(&self->addr, 0, sizeof self->addr);

    strcpy(ip, address->ip);
    colon = strrchr(address->host, ':');
    if (colon) {
        char name[sizeof address->host];
        struct hostent *found;

        memcpy(name, address->host, colon - address->host);
        name[colon - address->host] = 0;
        found = gethostbyname(name);
        if (found)
            strcpy(ip, inet_ntoa(*(struct in_addr *)found->h_addr_list[0]));
    }
    self->addr.sin_family = AF_INET;
    self->addr.sin_addr.s_addr = inet_addr(ip);

    port = address->port;
    if (colon)
        port = atoi(colon + 1);
    self->addr.sin_port = htons((u_short)port);

    if (connect(self->socket, (struct sockaddr *)&self->addr, sizeof self->addr) == SOCKET_ERROR
        && WSAGetLastError() != WSAEWOULDBLOCK) {
        err = WSAGetLastError();
        (void)err;
        connection_disconnect(self);
        return 0;
    }
    self->failed = 0;
    return 1;
}

char connection_poll(struct Connection *self)
{
    if (self->socket == INVALID_SOCKET)
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

void connection_unread(struct Connection *self, int count, const void *src)
{
    byte_queue_prepend(&self->recv_queue, count, src);
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
    connection->send_after = timeGetTime() + milliseconds;
}

int connection_is_open(const struct Connection *connection)
{
    return connection->socket != INVALID_SOCKET;
}
