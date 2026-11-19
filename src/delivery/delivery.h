#ifndef DELIVERY_DELIVERY_H
#define DELIVERY_DELIVERY_H

#include <stddef.h>
#include <stdint.h>

/* Delivers small messages over datagrams that may be lost, repeated, late or out of order
 * (the client repository's plans/delivery.md). One end of a connection is one `struct
 * Delivery`; the other end is another, usually in another program. The caller carries the
 * datagrams: what delivery_make writes it puts on the wire, and what comes off the wire it
 * hands to delivery_arrived.
 *
 * Freestanding C: no allocation, no clock, no library call. The state lives in a block the
 * caller hands in, and the time is an argument: milliseconds that only go forward and may
 * wrap. The same source is compiled into the client and into the gateway.
 *
 * A message is the caller's bytes, 1 to DELIVERY_MESSAGE_MAX of them, and has one of three
 * kinds:
 *
 *   replaceable  arrives unless a newer one replaced it first; no order. It may arrive more
 *                than once, so what it says has to be safe to apply again.
 *   single       arrives once; no order.
 *   ordered      arrives once, in the order sent among the ordered ones. They leave together,
 *                the oldest first, at once when one is new and every 15 ms until acknowledged,
 *                so a lost datagram costs one of them 15 ms and not a wait for a timeout.
 *
 * Nothing is sent again as the datagram it was in. Every datagram has a number of its own,
 * the other end says which numbers arrived, and what a lost one carried and is still wanted
 * goes into a new one. A datagram counts as lost when three later ones were acknowledged, when
 * a later one was and nine eighths of the round trip have passed, or when nothing came back
 * for the round trip plus four times its variance (twice as long after each such wait, up to
 * eight times). An end remembers its last 128 datagrams with messages. While the oldest of
 * them is still on its way, a further one goes only every 15 ms and the oldest counts as
 * lost, so an end that hears nothing goes on asking and is back as soon as it is answered.
 *
 * A datagram, little-endian:
 *
 *   off size field
 *     0    1 flags     bit 0: the two fields after the number say something
 *     1    2 number    of this datagram
 *     3    2 ack       the highest datagram number received
 *     5    4 ack_bits  bit i: the datagram ack - 1 - i was received
 *     9      messages, one after another, each:
 *              1 kind    DELIVERY_REPLACEABLE, DELIVERY_SINGLE or DELIVERY_ORDERED
 *              2 number  single and ordered only: the message's number among its kind
 *              2 size    1 to DELIVERY_MESSAGE_MAX
 *              n bytes
 *
 * A datagram is data from outside. One that is not shaped like this changes nothing. */

/* The largest datagram delivery_make writes and delivery_arrived takes. */
#define DELIVERY_DATAGRAM_MAX 1200u
/* The largest message. */
#define DELIVERY_MESSAGE_MAX 1024u
/* Priorities are 0 to DELIVERY_PRIORITIES - 1, 0 first. */
#define DELIVERY_PRIORITIES 4

enum { DELIVERY_REPLACEABLE = 0, DELIVERY_SINGLE = 1, DELIVERY_ORDERED = 2 };

struct DeliveryConfig {
    uint32_t out_messages;     /* messages that may wait for their acknowledgement: 1 to 4096 */
    uint32_t out_bytes;        /* and their bytes together: DELIVERY_MESSAGE_MAX to 16 MB */
    uint32_t in_bytes;         /* bytes of arrived messages not taken yet: DELIVERY_DATAGRAM_MAX to 16 MB */
    uint32_t bytes_per_second; /* what may be sent, datagrams of acknowledgements alone aside; 0: no limit */
};

struct Delivery;

/* The bytes of the block an end with this config needs, 0 when the config is out of range. */
size_t delivery_size(const struct DeliveryConfig *config);

/* Makes an end in `memory`, which is aligned for a uint32_t and `size` bytes long. Returns it,
 * or NULL when the config is out of range or the block too small. */
struct Delivery *delivery_init(void *memory, size_t size, const struct DeliveryConfig *config, uint32_t now_ms);

/* Takes a message to deliver. `key` and `from` are a replaceable message's: it ends the sending
 * of every earlier replaceable message with the same key whose `from` is the same or later
 * (compared by signed difference, as ticks are). Returns 0 and takes nothing when the kind, the
 * priority or the size is out of range or the end is full; the caller then sends it some other
 * way. A replaceable message replaces the earlier ones even when it is refused itself. */
int delivery_send(struct Delivery *delivery, int kind, uint64_t key, uint32_t from, int priority,
                  const void *bytes, size_t size);

/* Writes the next datagram to put on the wire into `out`, which has room for
 * DELIVERY_DATAGRAM_MAX bytes, and returns its size. Returns 0 when nothing is due. To be
 * called until it returns 0, every few milliseconds while the connection lasts: the repeats of
 * ordered messages, the losses and the acknowledgements are found here. */
size_t delivery_make(struct Delivery *delivery, uint32_t now_ms, unsigned char *out, size_t room);

/* Takes a datagram that came off the wire. Returns 1 when it was read, 0 when it changed
 * nothing: not shaped like a datagram, read before, older than the 33 this end keeps track of,
 * or more than there is room for until delivery_take was called. What a datagram that changed
 * nothing carried is sent again by the other end. */
int delivery_arrived(struct Delivery *delivery, uint32_t now_ms, const unsigned char *bytes, size_t size);

/* The next message that arrived, or NULL. The bytes stay where they are until the next call
 * with this end. */
const unsigned char *delivery_take(struct Delivery *delivery, size_t *size);

/* The messages this end took that are neither acknowledged nor replaced yet. */
size_t delivery_waiting(const struct Delivery *delivery);

/* 1 when the block is an end in a state the functions above leave it in, for tests. */
int delivery_check(const struct Delivery *delivery);

#endif
