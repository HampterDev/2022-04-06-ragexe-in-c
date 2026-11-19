#include "std/way_up.h"

void way_up_reset(struct WayUp *way)
{
    way->count = 0;
    way->next = 0;
    way->lead = 0;
    way->slewed_at = 0;
}

/* The lead the samples ask for: the second longest (the longest while fewer than three are held)
   and half its height above the shortest. The samples are compared as distances from the first,
   so that ticks that wrap between them order right. */
static uint32_t target(const struct WayUp *way)
{
    uint32_t base = way->took[0];
    int32_t least = 0, first = 0, second = 0, d;
    int i;

    for (i = 0; i < way->count; i++) {
        d = (int32_t)(way->took[i] - base);
        if (d < least)
            least = d;
        if (i == 0 || d > first) {
            second = i == 0 ? d : first;
            first = d;
        } else if (i == 1 || d > second) {
            second = d;
        }
    }
    if (way->count < 3)
        second = first;
    return base + (uint32_t)second + (uint32_t)(second - least) / 2;
}

void way_up_sample(struct WayUp *way, uint32_t client_send, uint32_t server_receive)
{
    int first = way->count == 0;

    way->took[way->next] = server_receive - client_send;
    way->next = (way->next + 1) % WAY_UP_SAMPLES;
    if (way->count < WAY_UP_SAMPLES)
        way->count++;
    if (first) {
        way->lead = target(way);
        way->slewed_at = client_send;
    }
}

uint32_t way_up_arrival(struct WayUp *way, uint32_t now)
{
    uint32_t want;
    int32_t off, step;

    if (!way->count)
        return now;
    want = target(way);
    off = (int32_t)(want - way->lead);
    step = (int32_t)(now - way->slewed_at) / WAY_UP_SLEW;
    if (step > 0)
        way->slewed_at += (uint32_t)step * WAY_UP_SLEW;
    if (off > 0)
        way->lead = want;
    else if (off < 0 && step > 0)
        way->lead -= (uint32_t)(step < -off ? step : -off);
    return now + way->lead;
}
