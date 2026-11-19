#include "delivery/delivery.h"
#include "lossy_link/relay.h"
#include "platform/clock.h"
#include "platform/net.h"
#include "platform/thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <enet/enet.h>

/* measure_requests [loss=N] [delay=MS] [jitter=MS] [requests=N] [every=FRAMES] [frame=MS] [seed=N]
 *
 * How late a request reaches the server when datagrams are lost: through the delivery piece and,
 * for comparison, on the world channel, which is ENet's own resending (plans/delivery.md,
 * step 6). Two of the client's sockets (platform/net_enet.c) each send the same numbered
 * requests at the same moments, one on each channel, each over a lossy link of its own
 * (tools/lossy_link) to an ENet server in this process that holds the other end of the piece.
 * The client looks at its sockets once a frame, as the game does. The two run side by side in
 * one loop, so a busy machine holds both up alike. Nothing here is a pass or a fail but a
 * request that never arrives or arrives twice: it prints what it measured. */

#define REQUESTS_MOST 2000
#define WARM_UP 25

static struct {
    unsigned loss, delay, jitter, requests, every, frame, seed;
} g_set = { 10, 30, 0, 200, 3, 16, 1 };

static uint32_t g_sent_at[REQUESTS_MOST];

/* One way of sending: its socket, its link and its server. */
struct Way {
    const char *name;
    int channel;
    ENetHost *server;
    ENetPeer *peer;
    struct Delivery *delivery;
    struct LossyRelay *relay;
    struct PlatformSocket *sock;
    uint32_t heard_at[REQUESTS_MOST];
    unsigned heard, twice;
};

static struct Way g_ways[2] = { { "delivery piece", PLATFORM_CHANNEL_DELIVERY }, { "world channel (ENet)", PLATFORM_CHANNEL_WORLD } };

static void hear(struct Way *way, const unsigned char *bytes, size_t count)
{
    uint32_t number;

    for (; count >= sizeof number; bytes += sizeof number, count -= sizeof number) {
        memcpy(&number, bytes, sizeof number);
        if (number >= g_set.requests)
            continue; /* the warm-up's */
        if (way->heard_at[number]) {
            way->twice++;
            continue;
        }
        way->heard_at[number] = platform_ticks_ms();
        way->heard++;
    }
}

static void serve_events(struct Way *way)
{
    static const struct DeliveryConfig config = { 64, 16 * 1024, 4096, 0 };
    unsigned char datagram[DELIVERY_DATAGRAM_MAX];
    const unsigned char *message;
    ENetEvent event;
    size_t size;

    while (enet_host_service(way->server, &event, 0) > 0) {
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            size = delivery_size(&config);
            free(way->delivery);
            way->delivery = delivery_init(malloc(size), size, &config, enet_time_get());
            way->peer = event.peer;
        } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            if (event.channelID != PLATFORM_CHANNEL_DELIVERY) {
                hear(way, event.packet->data, event.packet->dataLength);
            } else if (way->delivery) {
                delivery_arrived(way->delivery, enet_time_get(), event.packet->data, event.packet->dataLength);
                while ((message = delivery_take(way->delivery, &size)) != NULL)
                    hear(way, message, size);
            }
            enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            way->peer = NULL;
        }
    }
    while (way->peer && way->delivery
           && (size = delivery_make(way->delivery, enet_time_get(), datagram, sizeof datagram)) != 0) {
        ENetPacket *packet = enet_packet_create(datagram, size, ENET_PACKET_FLAG_UNSEQUENCED);

        if (!packet || enet_peer_send(way->peer, PLATFORM_CHANNEL_DELIVERY, packet) != 0)
            break;
    }
    enet_host_flush(way->server);
}

static int open_way(struct Way *way)
{
    struct LossyWireConfig wire = { 0, 0, 0, 0 };
    ENetAddress address;

    wire.loss_pct = g_set.loss;
    wire.delay_ms = g_set.delay;
    wire.jitter_ms = g_set.jitter;
    enet_address_set_host_ip(&address, "127.0.0.1");
    address.port = ENET_PORT_ANY;
    way->server = enet_host_create(&address, 1, PLATFORM_SOCKET_CHANNELS, 0, 0);
    way->relay = way->server ? lossy_relay_open(0, way->server->address.port, g_set.seed, &wire) : NULL;
    way->sock = way->relay ? platform_socket_open() : NULL;
    return way->sock
        && platform_socket_connect(way->sock, platform_net_parse_ipv4("127.0.0.1"), lossy_relay_port(way->relay));
}

static int by_size(const void *a, const void *b)
{
    uint32_t left = *(const uint32_t *)a, right = *(const uint32_t *)b;

    return left < right ? -1 : left > right;
}

static void report(const struct Way *way, unsigned sent)
{
    static uint32_t took[REQUESTS_MOST];
    const struct LossyWire *counts = lossy_relay_wire(way->relay);
    /* on time: within the wire's delay and jitter and half a frame of slack */
    uint32_t on_time_most = g_set.delay + g_set.jitter + g_set.frame / 2, late_sum = 0, late_most = 0;
    unsigned late = 0, i;

    for (i = 0; i < sent; i++) {
        took[i] = way->heard_at[i] ? way->heard_at[i] - g_sent_at[i] : 0xffffffffu;
        if (way->heard_at[i] && took[i] > on_time_most) {
            late++;
            late_sum += took[i] - g_set.delay;
            if (took[i] - g_set.delay > late_most)
                late_most = took[i] - g_set.delay;
        }
    }
    qsort(took, sent, sizeof took[0], by_size);
    printf("%-22s %4u of %4u arrived", way->name, way->heard, sent);
    if (way->twice)
        printf(", %u TWICE", way->twice);
    printf("; lost on the way up %u of %u datagrams, down %u of %u\n", (unsigned)counts->lost[0],
           (unsigned)counts->put[0], (unsigned)counts->lost[1], (unsigned)counts->put[1]);
    if (sent)
        printf("%-22s took: half within %u ms, 95 of 100 within %u ms, the slowest %u ms\n", "",
               (unsigned)took[sent / 2], (unsigned)took[sent * 95 / 100], (unsigned)took[sent - 1]);
    printf("%-22s late (more than %u ms): %u requests, by %u ms on average and %u ms at most\n\n", "",
           (unsigned)on_time_most, late, late ? (unsigned)(late_sum / late) : 0, (unsigned)late_most);
}

static int setting(const char *argument, const char *name, unsigned *value)
{
    size_t length = strlen(name);

    if (strncmp(argument, name, length) != 0 || argument[length] != '=')
        return 0;
    *value = (unsigned)strtoul(argument + length + 1, NULL, 10);
    return 1;
}

int main(int argc, char **argv)
{
    uint32_t start, now, frame_at, last_sent = 0;
    unsigned sent = 0, warmed = 0, frames = 0;
    unsigned char drain[256];
    int i, c, w, up;

    for (i = 1; i < argc; i++) {
        if (!setting(argv[i], "loss", &g_set.loss) && !setting(argv[i], "delay", &g_set.delay)
            && !setting(argv[i], "jitter", &g_set.jitter) && !setting(argv[i], "requests", &g_set.requests)
            && !setting(argv[i], "every", &g_set.every) && !setting(argv[i], "frame", &g_set.frame)
            && !setting(argv[i], "seed", &g_set.seed)) {
            printf("measure_requests: cannot use \"%s\"\n", argv[i]);
            return 2;
        }
    }
    if (g_set.requests < 1 || g_set.requests > REQUESTS_MOST || g_set.every < 1 || g_set.frame < 1 || g_set.loss > 90) {
        printf("measure_requests: 1 to %d requests, a frame and a spacing of at least 1, a loss of at most 90\n",
               REQUESTS_MOST);
        return 2;
    }
    if (!platform_net_startup() || enet_initialize() != 0 || !open_way(&g_ways[0]) || !open_way(&g_ways[1])) {
        printf("measure_requests: no sockets\n");
        return 2;
    }
    printf("%u requests, one every %u ms, a client that looks at its sockets every %u ms;\n", g_set.requests,
           g_set.every * g_set.frame, g_set.frame);
    printf("each link loses %u of 100 datagrams each way and holds each %u ms and up to %u more (seed %u)\n\n", g_set.loss,
           g_set.delay, g_set.jitter, g_set.seed);
    start = frame_at = platform_ticks_ms();
    for (;;) {
        now = platform_ticks_ms();
        for (w = 0; w < 2; w++) {
            lossy_relay_pump(g_ways[w].relay, now);
            serve_events(&g_ways[w]);
        }
        if (now - frame_at >= g_set.frame) {
            /* a frame of the client: it looks at its sockets, and sends what the player asked for */
            frame_at += g_set.frame;
            frames++;
            for (up = 1, w = 0; w < 2; w++) {
                up &= platform_socket_writable(g_ways[w].sock) == 1 && platform_socket_readable(g_ways[w].sock) >= 0;
                for (c = 0; c < PLATFORM_SOCKET_CHANNELS; c++)
                    while (platform_socket_recv(g_ways[w].sock, c, drain, sizeof drain) > 0) {
                    }
            }
            if (up && warmed < WARM_UP) {
                /* so that both ways of resending know the round trip before they are measured */
                uint32_t number = 0xffff0000u + warmed++;

                for (w = 0; w < 2; w++)
                    platform_socket_send(g_ways[w].sock, g_ways[w].channel, &number, sizeof number);
            } else if (up && sent < g_set.requests && frames % g_set.every == 0) {
                uint32_t number = sent;

                g_sent_at[sent++] = last_sent = platform_ticks_ms();
                for (w = 0; w < 2; w++)
                    platform_socket_send(g_ways[w].sock, g_ways[w].channel, &number, sizeof number);
            }
        }
        if (sent == g_set.requests
            && ((g_ways[0].heard == sent && g_ways[1].heard == sent) || now - last_sent > 30000))
            break;
        if (now - start > 180000)
            break;
        platform_thread_sleep(1);
    }
    for (up = 1, w = 0; w < 2; w++) {
        report(&g_ways[w], sent);
        up &= g_ways[w].heard == g_set.requests && !g_ways[w].twice;
        platform_socket_close(g_ways[w].sock);
        lossy_relay_close(g_ways[w].relay);
        enet_host_destroy(g_ways[w].server);
        free(g_ways[w].delivery);
    }
    platform_net_shutdown();
    return up ? 0 : 1;
}
