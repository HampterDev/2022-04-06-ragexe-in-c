#include "delivery/delivery.h"
#include "platform/clock.h"
#include "platform/net.h"
#include "platform/thread.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <enet/enet.h>

/* The sockets of platform/net.h against an ENet server in this process, on loopback, and the
   address helpers. The server keeps what it receives in `heard`. Its link has the delivery
   channel, and it holds the other end of the delivery piece for the client that is connected:
   what the piece hands on is heard like any channel's bytes, and the test can have the wire
   lose the datagrams the client sends. */
static ENetHost *g_server;
static ENetPeer *g_peer;
static unsigned char g_heard[0x800];
static size_t g_heard_size;
static int g_heard_channel;
static int g_heard_channels[64]; /* of each thing heard since g_hears was set to 0 */
static int g_hears;
static int g_connects, g_disconnects;
static struct Delivery *g_delivery;
static int g_lose_datagrams; /* this many of the client's next datagrams never arrive */

static void serve(uint16_t *port)
{
    ENetAddress address;

    assert(enet_address_set_host_ip(&address, "127.0.0.1") == 0);
    address.port = ENET_PORT_ANY;
    g_server = enet_host_create(&address, 4, PLATFORM_SOCKET_CHANNELS, 0, 0);
    assert(g_server);
    *port = g_server->address.port;
    assert(*port != 0);
}

static void hear(const unsigned char *bytes, size_t count, int channel)
{
    assert(g_heard_size + count <= sizeof g_heard);
    memcpy(g_heard + g_heard_size, bytes, count);
    g_heard_size += count;
    g_heard_channel = channel;
    g_heard_channels[g_hears++ % 64] = channel;
}

static void serve_events(void)
{
    unsigned char datagram[DELIVERY_DATAGRAM_MAX];
    ENetEvent event;
    size_t due;

    while (enet_host_service(g_server, &event, 1) > 0) {
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            static const struct DeliveryConfig config = { 16, 4096, 4096, 0 };
            size_t size = delivery_size(&config);

            assert(event.peer->channelCount == PLATFORM_SOCKET_CHANNELS);
            free(g_delivery);
            g_delivery = delivery_init(malloc(size), size, &config, enet_time_get());
            assert(g_delivery);
            g_peer = event.peer;
            g_connects++;
        } else if (event.type == ENET_EVENT_TYPE_RECEIVE && event.channelID == PLATFORM_CHANNEL_DELIVERY) {
            const unsigned char *message;
            size_t size;

            if (g_lose_datagrams)
                g_lose_datagrams--;
            else
                delivery_arrived(g_delivery, enet_time_get(), event.packet->data, event.packet->dataLength);
            while ((message = delivery_take(g_delivery, &size)) != NULL)
                hear(message, size, PLATFORM_CHANNEL_DELIVERY);
            enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            hear(event.packet->data, event.packet->dataLength, event.channelID);
            enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            g_peer = NULL;
            g_disconnects++;
        }
    }
    /* what the server's end of the piece has due: its acknowledgements, and what the test sent */
    while (g_peer && (due = delivery_make(g_delivery, enet_time_get(), datagram, sizeof datagram)) != 0) {
        ENetPacket *packet = enet_packet_create(datagram, due, ENET_PACKET_FLAG_UNSEQUENCED);

        assert(packet && enet_peer_send(g_peer, PLATFORM_CHANNEL_DELIVERY, packet) == 0);
    }
}

static void serve_send_on(int channel, const void *bytes, size_t count)
{
    ENetPacket *packet = enet_packet_create(bytes, count, channel == PLATFORM_CHANNEL_NEWEST ? 0 : ENET_PACKET_FLAG_RELIABLE);

    assert(packet && enet_peer_send(g_peer, (enet_uint8)channel, packet) == 0);
}

static void serve_send(const void *bytes, size_t count)
{
    serve_send_on(PLATFORM_CHANNEL_WORLD, bytes, count);
}

enum { WRITABLE, READABLE, CONNECTED, HEARD, DISCONNECTED };

/* Runs both ends until the condition holds, for two seconds at most. */
static int run_until(struct PlatformSocket *sock, int what, size_t count)
{
    uint32_t start = platform_ticks_ms();

    while (platform_ticks_ms() - start < 2000) {
        serve_events();
        if (what == WRITABLE && platform_socket_writable(sock) == 1)
            return 1;
        if (what == READABLE && platform_socket_readable(sock) == 1)
            return 1;
        if (what == CONNECTED && g_connects >= (int)count)
            return 1;
        if (what == HEARD && g_heard_size >= count)
            return 1;
        if (what == DISCONNECTED && g_disconnects >= (int)count)
            return 1;
        if (sock && what != READABLE)
            platform_socket_readable(sock);
    }
    return 0;
}

static struct PlatformSocket *connect_to(uint16_t port)
{
    struct PlatformSocket *sock = platform_socket_open();

    assert(sock);
    assert(platform_socket_connect(sock, platform_net_parse_ipv4("127.0.0.1"), port));
    return sock;
}

/* Takes exactly `count` bytes off one channel of the socket, in pieces as the connection does. */
static void take_on(struct PlatformSocket *sock, int channel, unsigned char *out, size_t count)
{
    uint32_t start = platform_ticks_ms();
    size_t got = 0;

    while (got < count) {
        int piece;

        assert(platform_ticks_ms() - start < 4000);
        serve_events();
        platform_socket_readable(sock);
        piece = platform_socket_recv(sock, channel, out + got, count - got > 0x800 ? 0x800 : (int)(count - got));
        if (piece == PLATFORM_SOCKET_RETRY)
            continue;
        assert(piece > 0);
        got += (size_t)piece;
    }
}

static void take(struct PlatformSocket *sock, unsigned char *out, size_t count)
{
    take_on(sock, PLATFORM_CHANNEL_WORLD, out, count);
}

static void a_stream_both_ways(uint16_t port)
{
    static unsigned char large[100000], back[100000];
    struct PlatformSocket *sock = connect_to(port);
    unsigned char few[8];
    size_t i;

    /* not writable, not readable and nothing sent before the server has answered */
    assert(platform_socket_writable(sock) == 0 && platform_socket_readable(sock) == 0);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "x", 1) == PLATFORM_SOCKET_RETRY);
    /* the client has its answer one packet before the server has the client's */
    assert(run_until(sock, WRITABLE, 0) && run_until(sock, CONNECTED, 1));
    assert(platform_socket_recv(sock, PLATFORM_CHANNEL_WORLD, few, sizeof few) == PLATFORM_SOCKET_RETRY);

    /* two sends arrive as one stream, in order */
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "abc", 3) == 3);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "defgh", 5) == 5);
    assert(run_until(sock, HEARD, 8) && g_heard_size == 8 && memcmp(g_heard, "abcdefgh", 8) == 0);

    /* a packet far above one datagram, then small ones, read back in pieces */
    for (i = 0; i < sizeof large; i++)
        large[i] = (unsigned char)(i * 31 + i / 251);
    serve_send(large, sizeof large);
    serve_send("12", 2);
    serve_send("345", 3);
    take(sock, back, sizeof back);
    assert(memcmp(large, back, sizeof large) == 0);
    take(sock, few, 5);
    assert(memcmp(few, "12345", 5) == 0);

    /* each channel is its own stream, both ways, and the server hears which one was used */
    serve_send_on(PLATFORM_CHANNEL_LISTS, "li", 2);
    serve_send_on(PLATFORM_CHANNEL_NEWEST, "new", 3);
    serve_send_on(PLATFORM_CHANNEL_LISTS, "st", 2);
    serve_send("w", 1);
    take(sock, few, 1);
    assert(few[0] == 'w');
    take_on(sock, PLATFORM_CHANNEL_LISTS, few, 4);
    assert(memcmp(few, "list", 4) == 0);
    take_on(sock, PLATFORM_CHANNEL_NEWEST, few, 3);
    assert(memcmp(few, "new", 3) == 0);
    assert(platform_socket_recv(sock, PLATFORM_CHANNEL_WORLD, few, 8) == PLATFORM_SOCKET_RETRY);
    assert(platform_socket_recv(sock, PLATFORM_SOCKET_CHANNELS, few, 8) == PLATFORM_SOCKET_FAILED);
    assert(platform_socket_send(sock, PLATFORM_SOCKET_CHANNELS, "x", 1) == PLATFORM_SOCKET_FAILED);
    g_heard_size = 0;
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_NEWEST, "t", 1) == 1);
    assert(run_until(sock, HEARD, 1) && g_heard[0] == 't' && g_heard_channel == PLATFORM_CHANNEL_NEWEST);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_LISTS, "m", 1) == 1);
    assert(run_until(sock, HEARD, 2) && g_heard[1] == 'm' && g_heard_channel == PLATFORM_CHANNEL_LISTS);

    /* the server's last bytes land before its close does, and the close reads as an end of the stream */
    serve_send("bye", 3);
    enet_peer_disconnect_later(g_peer, 0);
    take(sock, few, 3);
    assert(memcmp(few, "bye", 3) == 0);
    assert(run_until(sock, READABLE, 0));
    assert(platform_socket_recv(sock, PLATFORM_CHANNEL_WORLD, few, sizeof few) == 0);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "x", 1) == PLATFORM_SOCKET_FAILED);
    assert(run_until(sock, DISCONNECTED, 1));
    platform_socket_close(sock);
}

/* What is sent on the delivery channel: the player's requests, through the delivery piece. */
static void requests_through_the_piece(uint16_t port)
{
    static unsigned char much[DELIVERY_MESSAGE_MAX + 1];
    struct PlatformSocket *sock = connect_to(port);
    int connects = g_connects;
    unsigned char got[8];
    uint32_t start;

    g_heard_size = 0;
    assert(run_until(sock, WRITABLE, 0) && run_until(sock, CONNECTED, (size_t)connects + 1));
    /* two requests arrive once each, in order, and through the piece */
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, "walk", 4) == 4);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, "hit", 3) == 3);
    assert(run_until(sock, HEARD, 7) && g_heard_size == 7 && memcmp(g_heard, "walkhit", 7) == 0);
    assert(g_heard_channel == PLATFORM_CHANNEL_DELIVERY);

    /* the wire loses three datagrams in a row: the request is sent again every 15 ms and is
       there after the fourth, long before a lost datagram's timeout would have sent it */
    g_lose_datagrams = 3;
    start = platform_ticks_ms();
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, "skill", 5) == 5);
    assert(run_until(sock, HEARD, 12) && g_lose_datagrams == 0 && memcmp(g_heard + 7, "skill", 5) == 0);
    assert(platform_ticks_ms() - start >= 40 && platform_ticks_ms() - start < 200);
    /* and nothing of it arrives again */
    start = platform_ticks_ms();
    while (platform_ticks_ms() - start < 150) {
        serve_events();
        platform_socket_readable(sock);
    }
    assert(g_heard_size == 12);

    /* what the server sends through the piece is read off the delivery channel */
    assert(delivery_send(g_delivery, DELIVERY_SINGLE, 0, 0, 0, "note", 4));
    take_on(sock, PLATFORM_CHANNEL_DELIVERY, got, 4);
    assert(memcmp(got, "note", 4) == 0);
    assert(platform_socket_recv(sock, PLATFORM_CHANNEL_DELIVERY, got, sizeof got) == PLATFORM_SOCKET_RETRY);

    /* more than the piece takes as one message goes on the world channel */
    g_heard_size = 0;
    memset(much, 'm', sizeof much);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, much, sizeof much) == (int)sizeof much);
    assert(run_until(sock, HEARD, sizeof much) && g_heard_size == sizeof much && g_heard_channel == PLATFORM_CHANNEL_WORLD);
    assert(g_heard[0] == 'm' && g_heard[sizeof much - 1] == 'm');
    platform_socket_close(sock);
    assert(run_until(NULL, DISCONNECTED, (size_t)g_disconnects + 1));
}

/* A server whose link has no delivery channel: a request goes on the world channel. */
static void a_server_without_the_piece(void)
{
    ENetAddress address;
    ENetHost *plain;
    struct PlatformSocket *sock;
    unsigned char heard[2] = { 0 };
    int channel = -1, sent = 0;
    uint32_t start = platform_ticks_ms();

    assert(enet_address_set_host_ip(&address, "127.0.0.1") == 0);
    address.port = ENET_PORT_ANY;
    plain = enet_host_create(&address, 1, PLATFORM_CHANNEL_DELIVERY, 0, 0);
    assert(plain);
    sock = connect_to(plain->address.port);
    while (channel < 0 && platform_ticks_ms() - start < 2000) {
        ENetEvent event;

        while (enet_host_service(plain, &event, 1) > 0) {
            if (event.type != ENET_EVENT_TYPE_RECEIVE)
                continue;
            assert(event.packet->dataLength == 2);
            memcpy(heard, event.packet->data, 2);
            channel = event.channelID;
            enet_packet_destroy(event.packet);
        }
        if (!sent && platform_socket_writable(sock) == 1) {
            assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, "rq", 2) == 2);
            sent = 1;
        }
    }
    assert(channel == PLATFORM_CHANNEL_WORLD && memcmp(heard, "rq", 2) == 0);
    platform_socket_close(sock);
    enet_host_destroy(plain);
}

/* The server's side on a thread of its own, as a real server answers while the client closes. */
static int g_disconnects_awaited;

static void serve_until_the_awaited_disconnect(void *context)
{
    uint32_t start = platform_ticks_ms();

    (void)context;
    while (g_disconnects < g_disconnects_awaited && platform_ticks_ms() - start < 2000)
        serve_events();
}

static void a_close_delivers_what_was_sent(uint16_t port)
{
    struct PlatformSocket *sock = connect_to(port);
    struct PlatformThread *server;
    uint32_t start;

    int connects = g_connects, disconnects = g_disconnects;

    g_heard_size = 0;
    assert(run_until(sock, WRITABLE, 0) && run_until(sock, CONNECTED, (size_t)connects + 1));
    g_disconnects_awaited = disconnects + 1;
    server = platform_thread_create(serve_until_the_awaited_disconnect, NULL);
    assert(server);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "last", 4) == 4);
    start = platform_ticks_ms();
    platform_socket_close(sock);
    /* the close waited for the server's acknowledgement, not for its whole patience */
    assert(platform_ticks_ms() - start < 80);
    platform_thread_join(server);
    assert(g_heard_size == 4 && memcmp(g_heard, "last", 4) == 0 && g_disconnects == disconnects + 1);
}

/* Lets both ends run for `milliseconds`. */
static void run_for(struct PlatformSocket *sock, uint32_t milliseconds)
{
    uint32_t start = platform_ticks_ms();

    while (platform_ticks_ms() - start < milliseconds) {
        serve_events();
        platform_socket_readable(sock);
    }
}

/* Requests and world packets are one stream: neither overtakes what was sent before it on the
   other, whatever the piece's wire loses. */
static void requests_and_the_world_keep_their_order(uint16_t port)
{
    static unsigned char much[DELIVERY_MESSAGE_MAX + 1];
    struct PlatformSocket *sock = connect_to(port);
    int connects = g_connects;

    assert(run_until(sock, WRITABLE, 0) && run_until(sock, CONNECTED, (size_t)connects + 1));
    g_heard_size = 0;
    g_hears = 0;
    /* a request right behind a world packet ENet still carries goes behind it on the world channel */
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "w1", 2) == 2);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, "r1", 2) == 2);
    assert(run_until(sock, HEARD, 4) && memcmp(g_heard, "w1r1", 4) == 0);
    assert(g_heard_channels[0] == PLATFORM_CHANNEL_WORLD && g_heard_channels[1] == PLATFORM_CHANNEL_WORLD);
    run_for(sock, 60);
    /* once that was acknowledged, a request takes the piece; the wire loses it twice, and a world
       packet sent behind it goes through the piece behind it */
    g_lose_datagrams = 2;
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, "r2", 2) == 2);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "w2", 2) == 2);
    assert(run_until(sock, HEARD, 8) && memcmp(g_heard + 4, "r2w2", 4) == 0);
    assert(g_heard_channels[2] == PLATFORM_CHANNEL_DELIVERY && g_heard_channels[3] == PLATFORM_CHANNEL_DELIVERY);
    run_for(sock, 60);
    /* with nothing in the piece a world packet takes the world channel */
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "w3", 2) == 2);
    assert(run_until(sock, HEARD, 10) && memcmp(g_heard + 8, "w3", 2) == 0 && g_heard_channels[4] == PLATFORM_CHANNEL_WORLD);
    run_for(sock, 60);
    /* a world send more than the piece takes waits while a request is in the piece, then goes */
    g_lose_datagrams = 2;
    memset(much, 'm', sizeof much);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, "r3", 2) == 2);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, much, sizeof much) == PLATFORM_SOCKET_RETRY);
    assert(run_until(sock, HEARD, 12) && memcmp(g_heard + 10, "r3", 2) == 0);
    run_for(sock, 60);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, much, sizeof much) == (int)sizeof much);
    assert(run_until(sock, HEARD, 12 + sizeof much) && g_heard_channel == PLATFORM_CHANNEL_WORLD);
    run_for(sock, 60);

    /* a request the wire lost just before the close is there before the close is */
    g_lose_datagrams = 2;
    g_heard_size = 0;
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_DELIVERY, "last", 4) == 4);
    {
        int disconnects = g_disconnects;
        struct PlatformThread *server;

        g_disconnects_awaited = disconnects + 1;
        server = platform_thread_create(serve_until_the_awaited_disconnect, NULL);
        assert(server);
        platform_socket_close(sock);
        platform_thread_join(server);
        assert(g_heard_size == 4 && memcmp(g_heard, "last", 4) == 0 && g_disconnects == disconnects + 1);
    }
}

/* A port nobody answers on: the socket waits, then reads as closed. */
static void a_server_that_never_answers(void)
{
    ENetAddress address;
    ENetSocket silent = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    struct PlatformSocket *sock;
    uint32_t start;
    unsigned char byte;

    /* a UDP port that is bound and says nothing */
    assert(silent != ENET_SOCKET_NULL && enet_address_set_host_ip(&address, "127.0.0.1") == 0);
    address.port = ENET_PORT_ANY;
    assert(enet_socket_bind(silent, &address) == 0 && enet_socket_get_address(silent, &address) == 0);

    sock = connect_to(address.port);
    start = platform_ticks_ms();
    while (platform_ticks_ms() - start < 300)
        assert(platform_socket_writable(sock) == 0 && platform_socket_readable(sock) == 0);
    assert(platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "x", 1) == PLATFORM_SOCKET_RETRY);
    assert(platform_socket_recv(sock, PLATFORM_CHANNEL_WORLD, &byte, 1) == PLATFORM_SOCKET_RETRY);

    /* after five seconds without an answer it is closed: readable, and the read says so */
    while (platform_socket_readable(sock) == 0) {
        assert(platform_ticks_ms() - start < 7000);
        platform_thread_sleep(20);
    }
    assert(platform_ticks_ms() - start >= 4900);
    assert(platform_socket_recv(sock, PLATFORM_CHANNEL_WORLD, &byte, 1) == 0);
    assert(platform_socket_writable(sock) == 1 && platform_socket_send(sock, PLATFORM_CHANNEL_WORLD, "x", 1) == PLATFORM_SOCKET_FAILED);
    start = platform_ticks_ms();
    platform_socket_close(sock);
    assert(platform_ticks_ms() - start < 50);
    enet_socket_destroy(silent);
}

static void addresses(void)
{
    uint32_t address = 0;
    assert(platform_net_to_host32(platform_net_parse_ipv4("1.2.3.4")) == 0x01020304u);
    assert(strcmp(platform_net_ipv4_text(platform_net_parse_ipv4("10.20.30.40")), "10.20.30.40") == 0);
    assert(platform_net_parse_ipv4("300.1.1.1") == 0xffffffffu);
    assert(platform_net_resolve_ipv4("127.0.0.1", &address));
    assert(address == platform_net_parse_ipv4("127.0.0.1"));
    assert(platform_net_resolve_ipv4("localhost", &address));
    assert(platform_net_to_host32(address) >> 24 == 127);
}

int main(void)
{
    uint16_t port;

    assert(platform_net_startup());
    addresses();
    assert(enet_initialize() == 0);
    serve(&port);
    a_stream_both_ways(port);
    requests_through_the_piece(port);
    requests_and_the_world_keep_their_order(port);
    a_close_delivers_what_was_sent(port);
    a_server_that_never_answers();
    a_server_without_the_piece();
    enet_host_destroy(g_server);
    free(g_delivery);
    platform_net_shutdown();
    printf("Sockets: addresses, connect, a stream both ways, a 100000-byte packet in pieces, three channels, the server's close after its last bytes, requests through the delivery piece with three datagrams lost, requests and world packets in one order and a request lost before a close, a close that delivers what was sent, a server that never answers and one without the delivery channel passed.\n");
    return 0;
}
