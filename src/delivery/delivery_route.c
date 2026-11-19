#include "delivery/delivery_route.h"
#include "delivery/delivery.h"

/* A route by packet id, and where its fields are. */
struct Route {
    uint16_t id;
    uint8_t kind, priority;
    uint8_t length;   /* the packet's size; 0: the size is at offset 2, and at least `head` */
    uint8_t head;     /* the bytes before what a variable packet carries */
    int8_t gid_at;    /* the unit's GID; -1: the key is the id alone */
    int8_t tick_at;   /* the record's tick; -1: from 0 */
};

static const struct Route g_routes[] = {
    /* what the player asks for, each with its stamp; the client's own */
    { 0x0f01, DELIVERY_ORDERED, 0, 0, 0, -1, -1 },     /* CZ_INPUT_STAMP */
    { 0x035f, DELIVERY_ORDERED, 0, 0, 0, -1, -1 },     /* CZ_REQUEST_MOVE2 */
    { 0x0437, DELIVERY_ORDERED, 0, 0, 0, -1, -1 },     /* CZ_REQUEST_ACT2 */
    { 0x0118, DELIVERY_ORDERED, 0, 0, 0, -1, -1 },     /* CZ_CANCEL_LOCKON */
    { 0x0438, DELIVERY_ORDERED, 0, 0, 0, -1, -1 },     /* CZ_USE_SKILL2 */
    { 0x0af4, DELIVERY_ORDERED, 0, 0, 0, -1, -1 },     /* CZ_USE_SKILL_TOGROUND3 */
    { 0x0367, DELIVERY_ORDERED, 0, 0, 0, -1, -1 },     /* CZ_USE_SKILL_TOGROUND_WITHTALKBOX2 */
    { 0x0f02, DELIVERY_ORDERED, 0, 0, 0, -1, -1 },     /* CZ_INTENT */
    /* what the server sends */
    { 0x0f23, DELIVERY_SINGLE, 1, 32, 0, -1, -1 },     /* ZC_NOTICE_HIT */
    { 0x0f20, DELIVERY_REPLACEABLE, 2, 0, 25, 4, 8 },  /* ZC_TRACK_MOVE */
    { 0x0f21, DELIVERY_REPLACEABLE, 2, 37, 0, 2, 6 },  /* ZC_TRACK_ACT */
    { 0x0f22, DELIVERY_REPLACEABLE, 2, 20, 0, 2, 6 },  /* ZC_TRACK_VITALS */
};

static uint32_t get16(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

static uint32_t get32(const unsigned char *p)
{
    return get16(p) | get16(p + 2) << 16;
}

int delivery_route(const unsigned char *packet, size_t size, struct DeliveryRoute *route)
{
    const struct Route *found = 0;
    uint32_t id;
    size_t i;

    if (!packet || size < 2)
        return 0;
    id = get16(packet);
    for (i = 0; i < sizeof g_routes / sizeof g_routes[0]; i++)
        if (g_routes[i].id == id)
            found = &g_routes[i];
    if (!found)
        return 0;
    /* what the server sends is one packet, as long as its layout says; a request is the client's */
    if (found->kind != DELIVERY_ORDERED) {
        if (found->length ? size != found->length : size < found->head || size < 4 || get16(packet + 2) != size)
            return 0;
    }
    route->kind = found->kind;
    route->priority = found->priority;
    route->key = (uint64_t)id << 32 | (found->gid_at >= 0 ? get32(packet + found->gid_at) : 0);
    route->from = found->tick_at >= 0 ? get32(packet + found->tick_at) : 0;
    return 1;
}
