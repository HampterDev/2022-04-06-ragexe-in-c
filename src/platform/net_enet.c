#include "platform/net.h"
#include "delivery/delivery.h"
#include <stdlib.h>
#include <string.h>
#include <enet/enet.h>
#include <enet/time.h>

/* The client's sockets on every platform: a link to a server over UDP, carried by ENet
   (vendor/enet-1.3.18), with one byte stream per channel of platform/net.h. On the two
   reliable channels a lost packet is resent after about the measured round trip; on the
   newest channel it is not resent. The delivery channel carries the datagrams of the delivery
   piece, which ENet neither resends nor orders: the piece does what resending there is. */

/* A server that has not answered the connect by now is taken for unreachable. */
#define LINK_CONNECT_MS 5000
/* How long a closing socket waits for the server to acknowledge what was sent last. */
#define LINK_LINGER_MS 100
#define LINK_LINGER_STEP_MS 10
#define RECEIVED_START 0x4000

/* This end of the delivery piece. The player's requests go out through it, a few small ones at
   a time; nothing comes in through it yet but the acknowledgements of those. */
static const struct DeliveryConfig g_deliveryConfig = { 64, 16 * 1024, 4096, 0 };

struct PlatformSocket {
    ENetHost *host;
    ENetPeer *peer;
    int up;     /* the server answered the connect */
    int closed; /* the server closed the link, stopped answering or never answered */
    enet_uint32 connect_started;
    struct Delivery *delivery; /* this end of the piece, once the server answered with its channel */
    size_t world_waiting;      /* packets sent on the world channel and not acknowledged yet */
    /* Per channel, the bytes read off the link and not taken yet: [taken, held) of `bytes`. */
    struct Received {
        unsigned char *bytes;
        size_t taken, held, room;
    } received[PLATFORM_SOCKET_CHANNELS];
};

static int g_started;

static int start_enet(void)
{
    if (g_started)
        return 1;
    if (enet_initialize() != 0)
        return 0;
    atexit(enet_deinitialize);
    g_started = 1;
    return 1;
}

struct PlatformSocket *platform_socket_open(void)
{
    struct PlatformSocket *sock;

    if (!start_enet())
        return NULL;
    sock = calloc(1, sizeof *sock);
    if (!sock)
        return NULL;
    sock->host = enet_host_create(NULL, 1, PLATFORM_SOCKET_CHANNELS, 0, 0);
    if (!sock->host) {
        free(sock);
        return NULL;
    }
    return sock;
}

int platform_socket_connect(struct PlatformSocket *sock, uint32_t address, uint16_t port)
{
    ENetAddress target;

    target.host = address;
    target.port = port;
    sock->peer = enet_host_connect(sock->host, &target, PLATFORM_SOCKET_CHANNELS, 0);
    if (!sock->peer)
        return 0;
    sock->connect_started = enet_time_get();
    enet_host_flush(sock->host);
    return 1;
}

/* Keeps what the server sent until it is taken. The server decides how much that is, so a
   failure to hold it closes the socket and nothing else. */
static void keep(struct PlatformSocket *sock, struct Received *in, const unsigned char *bytes, size_t count)
{
    if (in->taken == in->held)
        in->taken = in->held = 0;
    if (in->room - in->held < count) {
        size_t room = in->room ? in->room : RECEIVED_START;
        unsigned char *grown;

        while (room - in->held < count) {
            if (room > (size_t)-1 / 2) {
                sock->closed = 1;
                return;
            }
            room *= 2;
        }
        grown = realloc(in->bytes, room);
        if (!grown) {
            sock->closed = 1;
            return;
        }
        in->bytes = grown;
        in->room = room;
    }
    memcpy(in->bytes + in->held, bytes, count);
    in->held += count;
}

static int anything_held(const struct PlatformSocket *sock)
{
    int channel;

    for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++)
        if (sock->received[channel].held > sock->received[channel].taken)
            return 1;
    return 0;
}

/* The server answered. When its link has the delivery channel, this end of the piece is made.
   ENet drops packets that are not reliable at the sender when it finds the round trip rising;
   the piece's datagrams are its own to send again, so that is turned off. */
static void link_up(struct PlatformSocket *sock)
{
    size_t size = delivery_size(&g_deliveryConfig);

    sock->up = 1;
    if (sock->peer->channelCount <= PLATFORM_CHANNEL_DELIVERY)
        return;
    sock->delivery = delivery_init(malloc(size), size, &g_deliveryConfig, enet_time_get());
    if (!sock->delivery) {
        sock->closed = 1;
        return;
    }
    enet_peer_throttle_configure(sock->peer, ENET_PEER_PACKET_THROTTLE_INTERVAL,
                                 ENET_PEER_PACKET_THROTTLE_ACCELERATION, 0);
}

/* A datagram of the delivery piece arrived: what it hands on is kept as the channel's bytes. */
static void take_datagram(struct PlatformSocket *sock, const unsigned char *bytes, size_t count)
{
    const unsigned char *message;
    size_t size;

    if (!sock->delivery)
        return;
    delivery_arrived(sock->delivery, enet_time_get(), bytes, count);
    while ((message = delivery_take(sock->delivery, &size)) != NULL)
        keep(sock, &sock->received[PLATFORM_CHANNEL_DELIVERY], message, size);
}

/* Puts on the wire what the delivery piece has due: the requests the server has not
   acknowledged, again every 15 ms, and the acknowledgements of what it sent. A datagram ENet
   does not take is one more that was lost. */
static void send_due(struct PlatformSocket *sock)
{
    unsigned char datagram[DELIVERY_DATAGRAM_MAX];
    int sent = 0;
    size_t size;

    if (!sock->delivery || sock->closed)
        return;
    while ((size = delivery_make(sock->delivery, enet_time_get(), datagram, sizeof datagram)) != 0) {
        ENetPacket *packet = enet_packet_create(datagram, size, ENET_PACKET_FLAG_UNSEQUENCED);

        if (!packet)
            break;
        if (enet_peer_send(sock->peer, PLATFORM_CHANNEL_DELIVERY, packet) != 0) {
            enet_packet_destroy(packet);
            break;
        }
        sent = 1;
    }
    if (sent)
        enet_host_flush(sock->host);
}

/* Sends what is due and takes what arrived, waiting up to `wait` ms for the first event. */
static void service(struct PlatformSocket *sock, unsigned wait)
{
    ENetEvent event;
    int result;

    while (!sock->closed && (result = enet_host_service(sock->host, &event, wait)) != 0) {
        wait = 0;
        if (result < 0) {
            sock->closed = 1;
            return;
        }
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            link_up(sock);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            /* the channel is the server's to name; one this client does not have is not kept */
            if (event.channelID == PLATFORM_CHANNEL_DELIVERY)
                take_datagram(sock, event.packet->data, event.packet->dataLength);
            else if (event.channelID < PLATFORM_SOCKET_CHANNELS)
                keep(sock, &sock->received[event.channelID], event.packet->data, event.packet->dataLength);
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            sock->closed = 1;
            break;
        default:
            break;
        }
    }
    if (!sock->up && !sock->closed && sock->peer
        && ENET_TIME_DIFFERENCE(enet_time_get(), sock->connect_started) >= LINK_CONNECT_MS)
        sock->closed = 1;
    send_due(sock);
}

/* 1 with bytes to take or the socket closed (the read then says so), 0 with nothing. */
int platform_socket_readable(struct PlatformSocket *sock)
{
    service(sock, 0);
    return anything_held(sock) || sock->closed;
}

/* 1 once the server has answered the connect, 0 before. A closed socket says 1 too, and the
   send that follows fails. */
int platform_socket_writable(struct PlatformSocket *sock)
{
    service(sock, 0);
    return sock->up || sock->closed;
}

/* ENet lets a reliable packet go once the server has acknowledged it. */
static void ENET_CALLBACK world_acknowledged(ENetPacket *packet)
{
    struct PlatformSocket *sock = packet->userData;

    if (sock->world_waiting)
        sock->world_waiting--;
}

int platform_socket_send(struct PlatformSocket *sock, int channel, const void *data, int size)
{
    ENetPacket *packet;

    if (sock->closed || size < 0 || channel < 0 || channel >= PLATFORM_SOCKET_CHANNELS)
        return PLATFORM_SOCKET_FAILED;
    if (!sock->up)
        return PLATFORM_SOCKET_RETRY;
    if (sock->delivery && (channel == PLATFORM_CHANNEL_DELIVERY || channel == PLATFORM_CHANNEL_WORLD)) {
        /* What leaves on these two is one stream to the server, in the order of these sends: each
           send goes where the other has nothing the server has not acknowledged, so neither
           overtakes what was sent before it on the other. A request goes through the piece as one
           message unless world packets sent before it are still on their way; a world packet goes
           on the world channel unless requests sent before it are still in the piece. */
        int in_piece = delivery_waiting(sock->delivery) > 0;

        if (!sock->world_waiting && (channel == PLATFORM_CHANNEL_DELIVERY || in_piece)) {
            if (delivery_send(sock->delivery, DELIVERY_ORDERED, 0, 0, 0, data, (size_t)size)) {
                send_due(sock);
                return size;
            }
            /* more than the piece takes as one message, or the piece is full: it waits behind
               what the piece carries, and goes on the world channel when that has arrived */
            if (in_piece)
                return PLATFORM_SOCKET_RETRY;
        }
        channel = PLATFORM_CHANNEL_WORLD;
    } else if (channel == PLATFORM_CHANNEL_DELIVERY) {
        /* a link without the piece */
        channel = PLATFORM_CHANNEL_WORLD;
    }
    /* a server that answered with fewer channels gets everything on the world's */
    if ((size_t)channel >= sock->peer->channelCount)
        channel = PLATFORM_CHANNEL_WORLD;
    packet = enet_packet_create(data, (size_t)size, channel == PLATFORM_CHANNEL_NEWEST ? 0 : ENET_PACKET_FLAG_RELIABLE);
    if (!packet)
        return PLATFORM_SOCKET_FAILED;
    if (channel == PLATFORM_CHANNEL_WORLD) {
        packet->userData = sock;
        packet->freeCallback = world_acknowledged;
    }
    if (enet_peer_send(sock->peer, (enet_uint8)channel, packet) != 0) {
        enet_packet_destroy(packet);
        return PLATFORM_SOCKET_FAILED;
    }
    if (channel == PLATFORM_CHANNEL_WORLD)
        sock->world_waiting++;
    enet_host_flush(sock->host);
    return size;
}

/* What arrived on a channel, 0 once the socket is closed and everything on every channel was
   taken. */
int platform_socket_recv(struct PlatformSocket *sock, int channel, void *buffer, int size)
{
    struct Received *in;
    size_t count;

    if (size < 0 || channel < 0 || channel >= PLATFORM_SOCKET_CHANNELS)
        return PLATFORM_SOCKET_FAILED;
    in = &sock->received[channel];
    count = in->held - in->taken;
    if (count == 0)
        return sock->closed && !anything_held(sock) ? 0 : PLATFORM_SOCKET_RETRY;
    if (count > (size_t)size)
        count = (size_t)size;
    memcpy(buffer, in->bytes + in->taken, count);
    in->taken += count;
    return (int)count;
}

/* What was sent and not yet acknowledged gets a short time to arrive before the socket goes:
   what the delivery piece still carries first, sent again until the server has it, then what
   ENet carries, until the server acknowledges the close. */
void platform_socket_close(struct PlatformSocket *sock)
{
    if (sock->up && !sock->closed) {
        unsigned waited = 0;

        for (; sock->delivery && delivery_waiting(sock->delivery) && !sock->closed && waited < LINK_LINGER_MS;
             waited += LINK_LINGER_STEP_MS)
            service(sock, LINK_LINGER_STEP_MS);
        enet_peer_disconnect_later(sock->peer, 0);
        for (; !sock->closed && waited < LINK_LINGER_MS; waited += LINK_LINGER_STEP_MS)
            service(sock, LINK_LINGER_STEP_MS);
        /* no acknowledgement in time: one last notice, so the server need not wait for silence */
        if (!sock->closed)
            enet_peer_disconnect_now(sock->peer, 0);
    }
    enet_host_destroy(sock->host);
    {
        int channel;

        for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++)
            free(sock->received[channel].bytes);
    }
    free(sock->delivery);
    free(sock);
}
