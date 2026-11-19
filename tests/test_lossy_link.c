#include "lossy_link/relay.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <enet/enet.h>

/* The lossy link (tools/lossy_link): the wire alone, by its seed, and the relay between two
   sockets on this machine. */

static struct LossyWire g_wire, g_twin;

static void put_number(struct LossyWire *wire, uint32_t now, int direction, uint32_t number)
{
    lossy_wire_put(wire, now, direction, &number, sizeof number);
}

static void the_wire(void)
{
    static const struct LossyWireConfig plain = { 0, 0, 0, 0 }, late = { 0, 0, 30, 10 }, rough = { 20, 10, 5, 0 };
    unsigned char bytes[LOSSY_WIRE_DATAGRAM_MAX + 1] = { 0 }, out[LOSSY_WIRE_DATAGRAM_MAX], twin[LOSSY_WIRE_DATAGRAM_MAX];
    uint32_t number, now, start = 0xfffffe00u, taken = 0, turned = 0, last = 0;
    int direction, twin_direction, i;
    size_t size;

    /* nothing lost, nothing held: out as put, each in its direction, at once */
    lossy_wire_init(&g_wire, 1, &plain);
    assert(lossy_wire_take(&g_wire, start, &direction, out) == 0);
    for (i = 0; i < 6; i++)
        put_number(&g_wire, start, i & 1, (uint32_t)i);
    for (i = 0; i < 6; i++) {
        assert(lossy_wire_take(&g_wire, start, &direction, out) == sizeof number);
        memcpy(&number, out, sizeof number);
        assert(number == (uint32_t)i && direction == (i & 1));
    }
    assert(lossy_wire_take(&g_wire, start + 1000, &direction, out) == 0);
    /* an empty datagram and one too long are lost; the longest passes whole */
    lossy_wire_put(&g_wire, start, 0, bytes, 0);
    lossy_wire_put(&g_wire, start, 0, bytes, sizeof bytes);
    assert(g_wire.lost[0] == 2 && lossy_wire_take(&g_wire, start, &direction, out) == 0);
    bytes[LOSSY_WIRE_DATAGRAM_MAX - 1] = 7;
    lossy_wire_put(&g_wire, start, 1, bytes, LOSSY_WIRE_DATAGRAM_MAX);
    assert(lossy_wire_take(&g_wire, start, &direction, out) == LOSSY_WIRE_DATAGRAM_MAX && direction == 1);
    assert(out[LOSSY_WIRE_DATAGRAM_MAX - 1] == 7);
    /* no room for more than it holds */
    for (i = 0; i < LOSSY_WIRE_FLIGHTS + 5; i++)
        put_number(&g_wire, start, 0, (uint32_t)i);
    assert(g_wire.lost[0] == 2 + 5);

    /* a delay and a jitter: none before 30 ms, all by 40, some turned around, and the clock
       wraps on the way */
    lossy_wire_init(&g_wire, 7, &late);
    for (i = 0; i < 1100; i++) {
        now = start + (uint32_t)i;
        if (i < 1000)
            put_number(&g_wire, now, 0, now);
        while (lossy_wire_take(&g_wire, now, &direction, out) != 0) {
            memcpy(&number, out, sizeof number);
            assert(now - number >= 30 && now - number <= 40);
            turned += taken && (int32_t)(number - last) < 0;
            last = number;
            taken++;
        }
    }
    assert(taken == 1000 && turned > 50 && g_wire.count == 0);

    /* loss and repeats near their share, and the same seed gives the same wire */
    lossy_wire_init(&g_wire, 99, &rough);
    lossy_wire_init(&g_twin, 99, &rough);
    for (now = 0, taken = 0; now < 10100; now++) {
        if (now < 10000) {
            put_number(&g_wire, now, now & 1, now);
            put_number(&g_twin, now, now & 1, now);
        }
        while ((size = lossy_wire_take(&g_wire, now, &direction, out)) != 0) {
            assert(lossy_wire_take(&g_twin, now, &twin_direction, twin) == size && twin_direction == direction);
            assert(memcmp(out, twin, size) == 0);
            taken++;
        }
    }
    assert(g_wire.put[0] == 5000 && g_wire.put[1] == 5000);
    assert(g_wire.lost[0] + g_wire.lost[1] > 1800 && g_wire.lost[0] + g_wire.lost[1] < 2200);
    assert(g_wire.repeated[0] + g_wire.repeated[1] > 650 && g_wire.repeated[0] + g_wire.repeated[1] < 950);
    assert(taken == 10000 - g_wire.lost[0] - g_wire.lost[1] + g_wire.repeated[0] + g_wire.repeated[1]);
}

/* A socket on this machine, and the port it has. */
static ENetSocket a_socket(ENetAddress *address)
{
    ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);

    assert(sock != ENET_SOCKET_NULL && enet_address_set_host_ip(address, "127.0.0.1") == 0);
    address->port = ENET_PORT_ANY;
    assert(enet_socket_bind(sock, address) == 0 && enet_socket_get_address(sock, address) == 0);
    assert(enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1) == 0);
    return sock;
}

static void send_number(ENetSocket sock, const ENetAddress *to, uint32_t number)
{
    ENetBuffer buffer;

    buffer.data = &number;
    buffer.dataLength = sizeof number;
    assert(enet_socket_send(sock, to, &buffer, 1) == (int)sizeof number);
}

static int receive_number(ENetSocket sock, ENetAddress *from, uint32_t *number)
{
    ENetBuffer buffer;

    buffer.data = number;
    buffer.dataLength = sizeof *number;
    return enet_socket_receive(sock, from, &buffer, 1) == (int)sizeof *number;
}

static void the_relay(void)
{
    static const struct LossyWireConfig held = { 0, 0, 20, 0 }, dead = { 100, 0, 0, 0 };
    ENetAddress client_address, server_address, relay_address, from;
    ENetSocket client = a_socket(&client_address), server = a_socket(&server_address);
    struct LossyRelay *relay = lossy_relay_open(0, server_address.port, 3, &held);
    uint32_t number, start, sent_at[20], up = 0, down = 0;
    int i;

    assert(relay && lossy_relay_port(relay) != 0);
    assert(enet_address_set_host_ip(&relay_address, "127.0.0.1") == 0);
    relay_address.port = lossy_relay_port(relay);
    /* twenty numbers up; the server answers each with the number and a thousand; all arrive in
       order, none sooner than the wire holds it each way */
    start = enet_time_get();
    for (i = 0; i < 20; i++) {
        sent_at[i] = enet_time_get();
        send_number(client, &relay_address, (uint32_t)i);
        lossy_relay_pump(relay, enet_time_get());
    }
    while ((up < 20 || down < 20) && enet_time_get() - start < 2000) {
        lossy_relay_pump(relay, enet_time_get());
        while (receive_number(server, &from, &number)) {
            assert(number == up && enet_time_get() - sent_at[number] >= 19);
            send_number(server, &from, number + 1000);
            up++;
        }
        while (receive_number(client, &from, &number)) {
            assert(number == down + 1000 && enet_time_get() - sent_at[down] >= 39);
            assert(from.port == relay_address.port);
            down++;
        }
        lossy_relay_wait(relay, 1);
    }
    assert(up == 20 && down == 20);
    assert(lossy_relay_wire(relay)->put[0] == 20 && lossy_relay_wire(relay)->put[1] == 20);
    lossy_relay_close(relay);

    /* a wire that loses everything passes nothing */
    relay = lossy_relay_open(0, server_address.port, 3, &dead);
    assert(relay);
    relay_address.port = lossy_relay_port(relay);
    send_number(client, &relay_address, 5);
    for (start = enet_time_get(); enet_time_get() - start < 100;) {
        lossy_relay_pump(relay, enet_time_get());
        assert(!receive_number(server, &from, &number));
        lossy_relay_wait(relay, 1);
    }
    assert(lossy_relay_wire(relay)->put[0] == 1 && lossy_relay_wire(relay)->lost[0] == 1);
    lossy_relay_close(relay);
    enet_socket_destroy(client);
    enet_socket_destroy(server);
}

int main(void)
{
    the_wire();
    assert(enet_initialize() == 0);
    the_relay();
    enet_deinitialize();
    printf("Lossy link: the wire's order, limits, delay and jitter, loss and repeats by their share, the same wire from the same seed, and a relay that holds what it passes both ways and one that loses all passed.\n");
    return 0;
}
