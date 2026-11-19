#ifndef LOSSY_LINK_WIRE_H
#define LOSSY_LINK_WIRE_H

#include <stddef.h>
#include <stdint.h>

/* A wire between two ends that loses, repeats and delays datagrams by a seed (plans/delivery.md,
 * step 6): the model inside the lossy relay (lossy_link/relay.h). Freestanding C: the state is
 * the caller's and the time is an argument, milliseconds that only go forward and may wrap. The
 * same seed and the same datagrams at the same times give the same wire. */

#define LOSSY_WIRE_DATAGRAM_MAX 1500
/* Datagrams that can be on the wire at a time; one that finds no room is lost. */
#define LOSSY_WIRE_FLIGHTS 512

struct LossyWireConfig {
    unsigned loss_pct;   /* datagrams of a hundred that never arrive */
    unsigned repeat_pct; /* datagrams of a hundred that arrive twice */
    uint32_t delay_ms;   /* every datagram is on the wire this long */
    uint32_t jitter_ms;  /* and up to this much longer, which is what turns two around */
};

struct LossyWire {
    struct LossyWireConfig config;
    uint32_t seed;
    uint32_t order; /* counts the datagrams that went onto the wire */
    /* by direction: datagrams put in, lost, repeated */
    uint32_t put[2], lost[2], repeated[2];
    int count;
    struct LossyFlight {
        uint32_t due, order;
        uint16_t size;
        uint8_t direction;
        unsigned char bytes[LOSSY_WIRE_DATAGRAM_MAX];
    } flights[LOSSY_WIRE_FLIGHTS];
};

void lossy_wire_init(struct LossyWire *wire, uint32_t seed, const struct LossyWireConfig *config);

/* A datagram enters the wire in `direction`, 0 or 1. An empty one, one longer than
 * LOSSY_WIRE_DATAGRAM_MAX and one the wire has no room for are lost. */
void lossy_wire_put(struct LossyWire *wire, uint32_t now_ms, int direction, const void *bytes, size_t size);

/* Takes the datagram that is due and was due first, of two due at the same time the one put in
 * first. Returns its size and writes its direction and its bytes, for which `out` has
 * LOSSY_WIRE_DATAGRAM_MAX; returns 0 when none is due. */
size_t lossy_wire_take(struct LossyWire *wire, uint32_t now_ms, int *direction, void *out);

#endif
