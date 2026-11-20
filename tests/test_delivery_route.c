#include "delivery/delivery_route.h"
#include "delivery/delivery.h"
#include <stdio.h>
#include <string.h>

/* The route table (delivery_route.h): which packets travel through the delivery piece, as
   what, and the key and tick a record's message carries. */

static int g_checks, g_failed;

#define CHECK(condition)                                                        \
    do {                                                                        \
        g_checks++;                                                             \
        if (!(condition)) {                                                     \
            g_failed++;                                                         \
            printf("FAILED line %d: %s\n", __LINE__, #condition);               \
        }                                                                       \
    } while (0)

static void put16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

static void put32(unsigned char *p, uint32_t v)
{
    put16(p, v & 0xffffu);
    put16(p + 2, v >> 16);
}

/* A packet of `id` and `size` bytes, its length field set for a variable one, with a GID and a
   tick where a record has them. */
static size_t packet_of(unsigned char *p, unsigned id, size_t size, int variable, size_t gid_at, size_t tick_at)
{
    memset(p, 0x5a, size);
    put16(p, id);
    if (variable)
        put16(p + 2, (unsigned)size);
    if (gid_at)
        put32(p + gid_at, 0x89abcdefu);
    if (tick_at)
        put32(p + tick_at, 0xfffffff0u);
    return size;
}

static void test_requests(void)
{
    static const unsigned ids[] = { 0x0f01, 0x035f, 0x0437, 0x0118, 0x0438, 0x0af4, 0x0367, 0x0f02 };
    unsigned char p[64];
    struct DeliveryRoute route;
    size_t i;

    for (i = 0; i < sizeof ids / sizeof ids[0]; i++) {
        put16(p, ids[i]);
        memset(&route, 0x77, sizeof route);
        CHECK(delivery_route(p, 2, &route) == 1 && route.kind == DELIVERY_ORDERED && route.priority == 0);
        CHECK(delivery_route(p, 30, &route) == 1 && route.kind == DELIVERY_ORDERED);
        CHECK(delivery_route(p, 1, &route) == 0);
    }
    CHECK(delivery_route(NULL, 12, &route) == 0);
    /* a turn, an item used, chat and the timed request go as they always went */
    put16(p, 0x0361);
    CHECK(delivery_route(p, 5, &route) == 0);
    put16(p, 0x0439);
    CHECK(delivery_route(p, 8, &route) == 0);
    put16(p, 0x00f3);
    CHECK(delivery_route(p, 8, &route) == 0);
    put16(p, 0x0360);
    CHECK(delivery_route(p, 6, &route) == 0);
}

static void test_records(void)
{
    unsigned char p[128];
    struct DeliveryRoute route;
    size_t size;

    /* a move record: its size is its length field and at least its head */
    size = packet_of(p, 0x0f20, 25 + 3, 1, 4, 8);
    CHECK(delivery_route(p, size, &route) == 1 && route.kind == DELIVERY_REPLACEABLE && route.priority == 2);
    CHECK(route.key == ((uint64_t)0x0f20 << 32 | 0x89abcdefu) && route.from == 0xfffffff0u);
    CHECK(delivery_route(p, size - 1, &route) == 0 && delivery_route(p, size + 1, &route) == 0);
    size = packet_of(p, 0x0f20, 25, 1, 4, 8);
    CHECK(delivery_route(p, size, &route) == 1);
    size = packet_of(p, 0x0f20, 24, 1, 4, 8);
    CHECK(delivery_route(p, size, &route) == 0);
    /* an act record: 37 bytes and no other size, the layout before the revision included */
    size = packet_of(p, 0x0f21, 37, 0, 2, 6);
    CHECK(delivery_route(p, size, &route) == 1 && route.kind == DELIVERY_REPLACEABLE && route.priority == 2);
    CHECK(route.key == ((uint64_t)0x0f21 << 32 | 0x89abcdefu) && route.from == 0xfffffff0u);
    CHECK(delivery_route(p, 35, &route) == 0 && delivery_route(p, 38, &route) == 0);
    /* a vitals record: 20 bytes */
    size = packet_of(p, 0x0f22, 20, 0, 2, 6);
    CHECK(delivery_route(p, size, &route) == 1 && route.key == ((uint64_t)0x0f22 << 32 | 0x89abcdefu));
    CHECK(delivery_route(p, 18, &route) == 0);
}

static void test_body_and_notice(void)
{
    unsigned char p[300];
    struct DeliveryRoute route;
    size_t size;

    /* the own body: the newest replaces every earlier one, before notices and records; its
       size is its length field and at least its head */
    size = packet_of(p, 0x0f25, 36 + 224, 1, 0, 0);
    CHECK(delivery_route(p, size, &route) == 1 && route.kind == DELIVERY_REPLACEABLE && route.priority == 0);
    CHECK(route.key == (uint64_t)0x0f25 << 32 && route.from == 0);
    CHECK(delivery_route(p, size - 1, &route) == 0 && delivery_route(p, size + 1, &route) == 0);
    size = packet_of(p, 0x0f25, 36, 1, 0, 0);
    CHECK(delivery_route(p, size, &route) == 1);
    size = packet_of(p, 0x0f25, 35, 1, 0, 0);
    CHECK(delivery_route(p, size, &route) == 0);
    /* a hit: once, in no order */
    size = packet_of(p, 0x0f23, 32, 0, 0, 0);
    CHECK(delivery_route(p, size, &route) == 1 && route.kind == DELIVERY_SINGLE && route.priority == 1);
    CHECK(delivery_route(p, 31, &route) == 0 && delivery_route(p, 33, &route) == 0);
    /* the own intent stays in order with the world */
    size = packet_of(p, 0x0f24, 14, 0, 0, 0);
    CHECK(delivery_route(p, size, &route) == 0);
}

static void test_every_id(void)
{
    unsigned char p[300];
    struct DeliveryRoute route;
    unsigned id, routed = 0, sizes[] = { 2, 20, 32, 37, 64, 256 }, s;

    for (id = 0; id <= 0xffff; id++) {
        int any = 0;

        for (s = 0; s < sizeof sizes / sizeof sizes[0]; s++) {
            packet_of(p, id, sizes[s], 1, 0, 0);
            any |= delivery_route(p, sizes[s], &route);
        }
        routed += any;
    }
    /* eight requests and five packets of the server's */
    CHECK(routed == 13);
}

int main(void)
{
    test_requests();
    test_records();
    test_body_and_notice();
    test_every_id();
    if (g_failed) {
        printf("delivery route: %d of %d checks FAILED\n", g_failed, g_checks);
        return 1;
    }
    printf("delivery route: %d checks passed\n", g_checks);
    return 0;
}
