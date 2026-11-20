#ifndef DELIVERY_DELIVERY_ROUTE_H
#define DELIVERY_DELIVERY_ROUTE_H

#include <stddef.h>
#include <stdint.h>

/* Which packets travel through the delivery piece, and how (the client repository's
 * plans/delivery.md, "What travels how"): the one table both ends read, the client for what it
 * sends and the gateway for what it sends. Every other packet travels on the channels it has
 * always used. Freestanding C: no allocation, no clock, no library call.
 *
 *   What                                                     Kind         Key               Priority
 *   CZ_INPUT_STAMP 0x0F01 and the requests it goes before:   ordered
 *     CZ_REQUEST_MOVE2 0x035F, CZ_REQUEST_ACT2 0x0437,
 *     CZ_CANCEL_LOCKON 0x0118, CZ_USE_SKILL2 0x0438,
 *     CZ_USE_SKILL_TOGROUND3 0x0AF4,
 *     CZ_USE_SKILL_TOGROUND_WITHTALKBOX2 0x0367, CZ_INTENT 0x0F02
 *   ZC_OWN_BODY 0x0F25                                       replaceable  the connection    0
 *   ZC_NOTICE_HIT 0x0F23                                     single                         1
 *   ZC_TRACK_MOVE 0x0F20, ZC_TRACK_ACT 0x0F21,               replaceable  unit and track,   2
 *     ZC_TRACK_VITALS 0x0F22                                              from the record's tick
 *
 * A replaceable message's key is the packet's id in the high half and the unit's GID in the
 * low half, so each track of each unit is a key of its own; its `from` is the record's tick.
 * A row without a unit would have the id alone as its key and 0 as its `from`, so that the
 * newest replaces every earlier one. The player's own body (ZC_OWN_BODY) is such a row: of
 * the bodies told to a client only the newest is sent until it arrives.
 *
 * What the server sends is data from outside to the client and the client's to the gateway, so
 * a packet goes through the piece only when it is exactly one packet of its id, as long as its
 * layout says. Anything else goes the way it always went. */

struct DeliveryRoute {
    int kind;      /* DELIVERY_REPLACEABLE, DELIVERY_SINGLE or DELIVERY_ORDERED */
    int priority;  /* 0 first */
    uint64_t key;  /* a replaceable message's */
    uint32_t from; /* a replaceable message's */
};

/* Returns 1 and fills `route` when the `size` bytes are one packet that travels through the
 * piece, 0 when they go the way they always went. */
int delivery_route(const unsigned char *packet, size_t size, struct DeliveryRoute *route);

#endif
