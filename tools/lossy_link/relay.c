#include "lossy_link/relay.h"
#include <stdlib.h>
#include <enet/enet.h>

struct LossyRelay {
    ENetSocket near_side; /* the client sends here */
    ENetSocket far_side;  /* the server sees this one as the client */
    ENetAddress client, server;
    int client_known;
    uint16_t port;
    struct LossyWire wire;
};

static ENetSocket open_socket(uint16_t port, ENetAddress *bound)
{
    ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    ENetAddress address;

    if (sock == ENET_SOCKET_NULL)
        return ENET_SOCKET_NULL;
    address.port = port;
    if (enet_address_set_host_ip(&address, "127.0.0.1") != 0 || enet_socket_bind(sock, &address) != 0
        || enet_socket_get_address(sock, bound) != 0 || enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1) != 0) {
        enet_socket_destroy(sock);
        return ENET_SOCKET_NULL;
    }
    return sock;
}

struct LossyRelay *lossy_relay_open(uint16_t listen_port, uint16_t server_port, uint32_t seed,
                                    const struct LossyWireConfig *config)
{
    struct LossyRelay *relay = calloc(1, sizeof *relay);
    ENetAddress bound;

    if (!relay)
        return NULL;
    relay->near_side = open_socket(listen_port, &bound);
    relay->port = bound.port;
    relay->far_side = relay->near_side == ENET_SOCKET_NULL ? ENET_SOCKET_NULL : open_socket(0, &bound);
    if (relay->far_side == ENET_SOCKET_NULL || enet_address_set_host_ip(&relay->server, "127.0.0.1") != 0) {
        lossy_relay_close(relay);
        return NULL;
    }
    relay->server.port = server_port;
    lossy_wire_init(&relay->wire, seed, config);
    return relay;
}

uint16_t lossy_relay_port(const struct LossyRelay *relay)
{
    return relay->port;
}

const struct LossyWire *lossy_relay_wire(const struct LossyRelay *relay)
{
    return &relay->wire;
}

/* Puts every datagram that waits on a socket onto the wire. What comes to the far side from
   anyone but the server is not the link's. */
static void take_in(struct LossyRelay *relay, uint32_t now_ms, int direction)
{
    unsigned char bytes[LOSSY_WIRE_DATAGRAM_MAX];
    ENetAddress from;
    ENetBuffer buffer;
    int got;

    for (;;) {
        buffer.data = bytes;
        buffer.dataLength = sizeof bytes;
        got = enet_socket_receive(direction ? relay->far_side : relay->near_side, &from, &buffer, 1);
        if (got <= 0)
            return;
        if (direction == 0) {
            relay->client = from;
            relay->client_known = 1;
        } else if (from.host != relay->server.host || from.port != relay->server.port) {
            continue;
        }
        lossy_wire_put(&relay->wire, now_ms, direction, bytes, (size_t)got);
    }
}

void lossy_relay_pump(struct LossyRelay *relay, uint32_t now_ms)
{
    unsigned char bytes[LOSSY_WIRE_DATAGRAM_MAX];
    ENetBuffer buffer;
    int direction;
    size_t size;

    take_in(relay, now_ms, 0);
    take_in(relay, now_ms, 1);
    while ((size = lossy_wire_take(&relay->wire, now_ms, &direction, bytes)) != 0) {
        buffer.data = bytes;
        buffer.dataLength = size;
        if (direction == 0)
            enet_socket_send(relay->far_side, &relay->server, &buffer, 1);
        else if (relay->client_known)
            enet_socket_send(relay->near_side, &relay->client, &buffer, 1);
    }
}

void lossy_relay_wait(struct LossyRelay *relay, uint32_t milliseconds)
{
    enet_uint32 condition = ENET_SOCKET_WAIT_RECEIVE;

    enet_socket_wait(relay->near_side, &condition, milliseconds);
}

void lossy_relay_close(struct LossyRelay *relay)
{
    if (relay->near_side != ENET_SOCKET_NULL)
        enet_socket_destroy(relay->near_side);
    if (relay->far_side != ENET_SOCKET_NULL)
        enet_socket_destroy(relay->far_side);
    free(relay);
}
