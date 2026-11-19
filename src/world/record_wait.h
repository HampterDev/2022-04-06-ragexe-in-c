#ifndef WORLD_RECORD_WAIT_H
#define WORLD_RECORD_WAIT_H

#include <stdint.h>
#include "world/act_track.h"
#include "world/move_track.h"

/* Records whose actor was not there when they arrived (plans/delivery.md, part 1). Through the
 * delivery piece a record can come before the packet that makes its actor known: it waits here
 * until the actor appears, RECORD_WAIT_MS at most, and is filed then as if it had just come. In
 * freestanding C: no allocation, no globals, no clock; the time is the caller's milliseconds,
 * 32 bits, and may wrap. */

#define RECORD_WAIT_ITEMS 64
#define RECORD_WAIT_MS 2000

enum { RECORD_WAIT_MOVE = 0, RECORD_WAIT_TIMED = 1 };

/* A move record, or an act or vitals record as the timed queue holds it, for the actor `gid`. */
struct RecordWaiting {
    uint32_t gid, since;
    unsigned char kind;
    union {
        struct MoveRecord move;
        struct TimedItem timed;
    } u;
};

struct RecordWait {
    struct RecordWaiting items[RECORD_WAIT_ITEMS]; /* in the order they arrived */
    int count;
};

/* Keeps a record that arrived at `now`. When RECORD_WAIT_ITEMS wait, the one that waited
 * longest goes. */
void record_wait_put(struct RecordWait *wait, uint32_t now, const struct RecordWaiting *item);

/* Takes the record that waited longest whose actor `present` says is there, and drops on the
 * way every one that waited longer than RECORD_WAIT_MS: its actor did not come, or came and
 * went. Returns 0 when none is to be filed. */
int record_wait_take(struct RecordWait *wait, uint32_t now, int (*present)(void *context, uint32_t gid),
                     void *context, struct RecordWaiting *item);

/* Forgets every record: the map was left. */
void record_wait_clear(struct RecordWait *wait);

#endif
