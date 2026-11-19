#include "session/state.h"
#include "contents/connection.h"
#include "platform/clock.h"
#include "platform/thread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <enet/enet.h>

/* The connection (contents/connection.c) over real sockets against an ENet server in this
   process: its three channels in order each way, what it sends on the delivery channel to a
   server whose link has none, and a login's hops kept on one link. */

struct Session g_session1;
static struct RagConnection g_connection;

struct RagConnection *rag_connection_instance(void)
{
    return &g_connection;
}

void *mem_alloc(size_t bytes)
{
    void *memory = malloc(bytes);

    assert(memory);
    return memory;
}

void mem_free(void *memory)
{
    free(memory);
}

void error_msg(const char *text, const char *caption)
{
    (void)text;
    (void)caption;
    assert(!"no error is expected");
}

static ENetHost *g_server;
static ENetPeer *g_peer;
static int g_connects, g_disconnects;
static unsigned char g_heard[PLATFORM_SOCKET_CHANNELS][64];
static size_t g_heard_size[PLATFORM_SOCKET_CHANNELS];

static void serve_events(void)
{
    ENetEvent event;

    while (enet_host_service(g_server, &event, 1) > 0) {
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            g_peer = event.peer;
            g_connects++;
        } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            assert(event.channelID < PLATFORM_SOCKET_CHANNELS);
            assert(g_heard_size[event.channelID] + event.packet->dataLength <= sizeof g_heard[0]);
            memcpy(g_heard[event.channelID] + g_heard_size[event.channelID], event.packet->data, event.packet->dataLength);
            g_heard_size[event.channelID] += event.packet->dataLength;
            enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            g_peer = NULL;
            g_disconnects++;
        }
    }
}

static void serve_send(int channel, const char *text)
{
    ENetPacket *packet = enet_packet_create(text, strlen(text), channel == PLATFORM_CHANNEL_NEWEST ? 0 : ENET_PACKET_FLAG_RELIABLE);

    assert(packet && enet_peer_send(g_peer, (enet_uint8)channel, packet) == 0);
}

/* Runs both ends for a moment. */
static void run(struct Connection *connection, unsigned milliseconds)
{
    uint32_t start = platform_ticks_ms();

    while (platform_ticks_ms() - start < milliseconds) {
        serve_events();
        assert(connection_poll(connection));
    }
}

static void expect_received(struct Connection *connection, int channel, const char *text)
{
    char got[32] = {0};
    struct ByteQueue *queue = &connection->recv_queue[channel];

    assert(byte_queue_size(queue) == (int)strlen(text));
    assert(byte_queue_read(queue, (int)strlen(text), got) && strcmp(got, text) == 0);
}

int main(void)
{
    struct Connection *connection = &g_connection.base;
    struct ServerAddress address = {0}, elsewhere = {0};
    ENetAddress listen;
    ENetSocket silent;

    tree_init(&g_session1.instantVars);
    assert(connection_startup() && enet_initialize() == 0);
    assert(enet_address_set_host_ip(&listen, "127.0.0.1") == 0);
    listen.port = ENET_PORT_ANY;
    /* a server whose link has no delivery channel */
    g_server = enet_host_create(&listen, 4, PLATFORM_CHANNEL_DELIVERY, 0, 0);
    assert(g_server);
    strcpy(address.ip, "127.0.0.1");
    address.port = g_server->address.port;

    connection_construct(connection);
    assert(!connection_is_open(connection) && connection_poll(connection));
    assert(connection_connect(connection, &address) && connection_is_open(connection));

    /* what is queued before the server answered leaves once it has, each channel on its own */
    connection_queue_send(connection, PLATFORM_CHANNEL_WORLD, 2, "w1");
    connection_queue_send(connection, PLATFORM_CHANNEL_LISTS, 2, "l1");
    connection_queue_send(connection, PLATFORM_CHANNEL_NEWEST, 2, "n1");
    connection_queue_send(connection, PLATFORM_CHANNEL_WORLD, 2, "w2");
    assert(connection_flush(connection));
    run(connection, 300);
    assert(g_connects == 1);
    assert(g_heard_size[PLATFORM_CHANNEL_WORLD] == 4 && memcmp(g_heard[PLATFORM_CHANNEL_WORLD], "w1w2", 4) == 0);
    assert(g_heard_size[PLATFORM_CHANNEL_LISTS] == 2 && memcmp(g_heard[PLATFORM_CHANNEL_LISTS], "l1", 2) == 0);
    assert(g_heard_size[PLATFORM_CHANNEL_NEWEST] == 2 && memcmp(g_heard[PLATFORM_CHANNEL_NEWEST], "n1", 2) == 0);

    /* a request to a server whose link has no delivery channel goes on the world's, behind what
       was sent there before it */
    connection_queue_send(connection, PLATFORM_CHANNEL_DELIVERY, 2, "rq");
    assert(connection_flush(connection));
    run(connection, 100);
    assert(g_heard_size[PLATFORM_CHANNEL_WORLD] == 6 && memcmp(g_heard[PLATFORM_CHANNEL_WORLD], "w1w2rq", 6) == 0);

    /* what the server sends is read into its channel's queue; raw reads take the world's */
    serve_send(PLATFORM_CHANNEL_LISTS, "list");
    serve_send(PLATFORM_CHANNEL_WORLD, "raw!world");
    serve_send(PLATFORM_CHANNEL_NEWEST, "new");
    run(connection, 200);
    {
        char raw[5] = {0};

        assert(connection_recv_bytes(connection, raw, 4, 1) == 4 && strcmp(raw, "raw!") == 0);
    }
    expect_received(connection, PLATFORM_CHANNEL_WORLD, "world");
    expect_received(connection, PLATFORM_CHANNEL_LISTS, "list");
    expect_received(connection, PLATFORM_CHANNEL_NEWEST, "new");

    /* A hop ends the stage for the client and keeps the link. What arrives meanwhile is thrown
       away, and a connect to the same address goes on over the same link. */
    connection_queue_send(connection, PLATFORM_CHANNEL_WORLD, 4, "lost");
    connection_hop(connection);
    assert(!connection_is_open(connection));
    assert(byte_queue_size(&connection->send_queue[PLATFORM_CHANNEL_WORLD]) == 0);
    serve_send(PLATFORM_CHANNEL_WORLD, "late");
    run(connection, 200);
    assert(g_disconnects == 0);
    assert(connection_connect(connection, &address) && connection_is_open(connection));
    assert(byte_queue_size(&connection->recv_queue[PLATFORM_CHANNEL_WORLD]) == 0);
    connection_queue_send(connection, PLATFORM_CHANNEL_WORLD, 2, "w3");
    assert(connection_flush(connection));
    serve_send(PLATFORM_CHANNEL_WORLD, "next");
    run(connection, 200);
    assert(g_connects == 1 && g_disconnects == 0);
    assert(g_heard_size[PLATFORM_CHANNEL_WORLD] == 8 && memcmp(g_heard[PLATFORM_CHANNEL_WORLD], "w1w2rqw3", 8) == 0);
    expect_received(connection, PLATFORM_CHANNEL_WORLD, "next");

    /* A hop followed by a connect elsewhere closes the kept link. */
    silent = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    assert(silent != ENET_SOCKET_NULL && enet_address_set_host_ip(&listen, "127.0.0.1") == 0);
    listen.port = ENET_PORT_ANY;
    assert(enet_socket_bind(silent, &listen) == 0 && enet_socket_get_address(silent, &listen) == 0);
    strcpy(elsewhere.ip, "127.0.0.1");
    elsewhere.port = listen.port;
    connection_hop(connection);
    assert(connection_connect(connection, &elsewhere));
    {
        uint32_t start = platform_ticks_ms();

        while (g_disconnects == 0 && platform_ticks_ms() - start < 2000)
            serve_events();
    }
    assert(g_disconnects == 1);

    /* A disconnect closes the link at once, and a kept link the server closes is dropped. */
    connection_disconnect(connection);
    assert(!connection_is_open(connection));
    assert(connection_connect(connection, &address));
    run(connection, 300);
    assert(g_connects == 2);
    connection_hop(connection);
    enet_peer_disconnect(g_peer, 0);
    run(connection, 300);
    assert(connection->kept == NULL);
    assert(connection_connect(connection, &address));
    run(connection, 300);
    assert(g_connects == 3);
    connection_hop(connection);
    connection_destruct(connection);
    {
        uint32_t start = platform_ticks_ms();

        while (g_disconnects < 3 && platform_ticks_ms() - start < 2000)
            serve_events();
    }
    assert(g_disconnects == 3);

    enet_socket_destroy(silent);
    enet_host_destroy(g_server);
    connection_shutdown();
    printf("Connection: three channels each way, a request to a server without the delivery channel, raw reads, a hop that keeps the link, a hop to another address, a kept link the server closed and a kept link at shutdown passed.\n");
    return 0;
}
