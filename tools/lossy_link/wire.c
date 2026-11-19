#include "lossy_link/wire.h"

static uint32_t draw(struct LossyWire *wire)
{
    wire->seed ^= wire->seed << 13;
    wire->seed ^= wire->seed >> 17;
    wire->seed ^= wire->seed << 5;
    return wire->seed;
}

void lossy_wire_init(struct LossyWire *wire, uint32_t seed, const struct LossyWireConfig *config)
{
    int direction;

    wire->config = *config;
    wire->seed = seed ? seed : 1;
    wire->order = 0;
    wire->count = 0;
    for (direction = 0; direction < 2; direction++)
        wire->put[direction] = wire->lost[direction] = wire->repeated[direction] = 0;
}

/* Puts one copy of a datagram onto the wire, to arrive after the delay and its share of the
   jitter. */
static void fly(struct LossyWire *wire, uint32_t now, int direction, const unsigned char *bytes, size_t size)
{
    struct LossyFlight *flight;
    size_t i;

    if (wire->count == LOSSY_WIRE_FLIGHTS) {
        wire->lost[direction]++;
        return;
    }
    flight = &wire->flights[wire->count++];
    flight->due = now + wire->config.delay_ms + (wire->config.jitter_ms ? draw(wire) % (wire->config.jitter_ms + 1) : 0);
    flight->order = wire->order++;
    flight->size = (uint16_t)size;
    flight->direction = (uint8_t)direction;
    for (i = 0; i < size; i++)
        flight->bytes[i] = bytes[i];
}

void lossy_wire_put(struct LossyWire *wire, uint32_t now_ms, int direction, const void *bytes, size_t size)
{
    direction = direction != 0;
    wire->put[direction]++;
    if (size == 0 || size > LOSSY_WIRE_DATAGRAM_MAX || draw(wire) % 100 < wire->config.loss_pct) {
        wire->lost[direction]++;
        return;
    }
    fly(wire, now_ms, direction, bytes, size);
    if (draw(wire) % 100 < wire->config.repeat_pct) {
        wire->repeated[direction]++;
        fly(wire, now_ms, direction, bytes, size);
    }
}

size_t lossy_wire_take(struct LossyWire *wire, uint32_t now_ms, int *direction, void *out)
{
    const struct LossyFlight *first = NULL;
    unsigned char *bytes = out;
    size_t size, i;
    int at;

    for (at = 0; at < wire->count; at++) {
        const struct LossyFlight *flight = &wire->flights[at];

        if ((int32_t)(now_ms - flight->due) < 0)
            continue;
        if (!first || (int32_t)(flight->due - first->due) < 0
            || (flight->due == first->due && (int32_t)(flight->order - first->order) < 0))
            first = flight;
    }
    if (!first)
        return 0;
    size = first->size;
    *direction = first->direction;
    for (i = 0; i < size; i++)
        bytes[i] = first->bytes[i];
    wire->flights[first - wire->flights] = wire->flights[--wire->count];
    return size;
}
