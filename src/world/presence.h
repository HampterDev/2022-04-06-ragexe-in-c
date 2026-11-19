#ifndef WORLD_PRESENCE_H
#define WORLD_PRESENCE_H

#include <stdint.h>
#include "world/act_track.h"
#include "world/move_track.h"
#include "world/record_wait.h"
#include "world/view_floors.h"

/* Where a unit's records go, in whatever order they and its comings and goings arrive
 * (plans/presence.md, part 2). Every record is filed against the floors of its unit id
 * (world/view_floors.h), wherever it comes from: off the wire, out of the waiting list, or
 * handed back at a going. One that is the past is dropped; one whose unit is out of view or has
 * no actor waits by unit id (world/record_wait.h); the rest go to the actor's move track or to
 * the act and HP queue. In freestanding C: no allocation, no globals, no clock; the memory and
 * the time are the caller's. */

struct Presence {
    struct ViewFloors *view;
    struct TimedQueue *queue;
    struct RecordWait *wait;
    uint32_t own; /* the player's own character */
    /* The move track of the actor of `gid`, or NULL when there is no actor of it. */
    struct MoveTrack *(*track_of)(void *context, uint32_t gid);
    void *context;
};

/* A move record of `gid` that arrived at `now`. Returns 1 when it went to the actor's track. */
int presence_file_move(const struct Presence *presence, uint32_t now, uint32_t gid, const struct MoveRecord *record);

/* An act or vitals record or a hit that arrived at `now`. A hit has no revision: it is dropped
 * when it is dated in a stay of its source that is over (view_floors_past_notice) and filed
 * otherwise. Returns what timed_queue_file returns, and TIMED_DROPPED for one that waits; with
 * TIMED_FILED_EARLY the item in `early` is the caller's to play at once. */
int presence_file_timed(const struct Presence *presence, uint32_t now, const struct TimedItem *item,
                        struct TimedItem *early);

/* A ZC_VIEW_STAMP was read at `now`. At a going, what the unit holds goes, the actor's move
 * track and its items in the queue: what the stamp's revisions cover is dropped, what is newer
 * belongs to a later stay and waits, and a hit dated after the going stays. At a coming, its
 * hits dated before it go. Nothing of the actor itself is touched. Returns 0 when the stamp says
 * nothing (it is of the own character, or no unit could be kept). */
int presence_stamp(const struct Presence *presence, uint32_t now, const struct ViewStamp *stamp);

/* Files one record that waited and whose unit is in view with an actor now, as if it had just
 * come. Returns 0 when none is to be filed. `early` as presence_file_timed's, and *filed
 * TIMED_FILED_EARLY says it was written. */
int presence_file_waiting(const struct Presence *presence, uint32_t now, struct TimedItem *early, int *filed);

#endif
