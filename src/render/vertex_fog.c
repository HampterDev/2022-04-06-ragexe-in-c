#include "render/vertex_fog.h"
#include <stdint.h>

/* The rhw the table starts at: below the smallest normal float, 1 / rhw is beyond any fog's range
   (and a GPU may take such an rhw for 0), so the step there is the step at it. */
#define SMALLEST_NORMAL 0x00800000u
#define POSITIVE_INFINITY 0x7f800000u

union FloatBits {
    float value;
    uint32_t bits;
};

static float from_bits(uint32_t bits)
{
    union FloatBits f;
    f.bits = bits;
    return f.value;
}

int vertex_fog_step(float rhw, float start, float end, float origin, float span)
{
    float range = end - start;
    float t = (1.0f / rhw - origin) / span;

    if (range != 0.0f)
        t = (t - start) / range;
    t = 0.0f > t ? 0.0f : t;
    t = 1.0f < t ? 1.0f : t;
    t = (1.0f - t) * 255.0f;
    return (int)t;
}

/* The steps are monotonic in rhw, each operation being so: the first rhw from the smallest normal
   one, as bits, whose step is `levels` past the smallest's in the steps' direction. */
static uint32_t first_rhw(float start, float end, float origin, float span, int base, int direction, int levels)
{
    uint32_t low = SMALLEST_NORMAL, high = POSITIVE_INFINITY;
    while (low < high) {
        uint32_t middle = low + (high - low) / 2;
        int step = vertex_fog_step(from_bits(middle), start, end, origin, span);
        if ((step - base) * direction >= levels)
            high = middle;
        else
            low = middle + 1;
    }
    return low;
}

void vertex_fog_table(float start, float end, float origin, float span, struct VertexFogTable *out)
{
    int base = vertex_fog_step(from_bits(SMALLEST_NORMAL), start, end, origin, span);
    int last = vertex_fog_step(from_bits(POSITIVE_INFINITY), start, end, origin, span);
    int direction = last >= base ? 1 : -1, count = (last - base) * direction, i;
    out->base = (float)base;
    out->direction = (float)direction;
    out->count = (float)count;
    for (i = 0; i < VERTEX_FOG_ENTRIES; i++)
        out->rhw[i] = i < count ? from_bits(first_rhw(start, end, origin, span, base, direction, i + 1))
                                : from_bits(POSITIVE_INFINITY);
}

int vertex_fog_lookup(const struct VertexFogTable *table, float rhw)
{
    int at = 0, step;
    for (step = VERTEX_FOG_ENTRIES / 2; step > 0; step /= 2)
        if (table->rhw[at + step - 1] <= rhw)
            at += step;
    return (int)table->base + (int)table->direction * (at < (int)table->count ? at : (int)table->count);
}
