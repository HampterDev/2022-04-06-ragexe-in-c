#ifndef WORLD_ACT_TRACK_H
#define WORLD_ACT_TRACK_H

#include <stdint.h>
#include "world/view_floors.h"

/* What an actor does, a hit, and an actor's HP, as the server sends them
 * (plans/core-loop.md), and the queue they wait in until the clock they are played on reaches
 * their tick. In freestanding C: no allocation, no globals, integer math only. Ticks are the
 * server's clock in milliseconds, 32 bits, and may wrap. The packets are little-endian and
 * packed, and of fixed length, which the packet table holds them to.
 *
 * ZC_TRACK_ACT 0x0F21, 37 bytes: what an actor does from at_tick on.
 *
 *   off size field
 *     0    2 id
 *     2    4 gid
 *     6    4 at_tick
 *    10    1 kind       ACT_*; the top bit (ACT_RECORD_OLD) marks a record sent later than it
 *                       was made, as the one that comes with an entry
 *    11    4 end_tick   a swing's animation or a cast ends; 0 for idle, which holds until the
 *                       next record
 *    15    4 mark_tick  a swing lands
 *    19    4 target     gid of a swing's or a cast's target; 0 for a cast on the ground
 *    23    2 x          the cell of a cast on the ground
 *    25    2 y
 *    27    2 skill_id   of a cast
 *    29    2 skill_lv
 *    31    2 motion_ms  length of the swing's animation, which sets its speed; a cast's is
 *                       ZC_USESKILL_ACK's attack motion
 *    33    1 element    of a cast, signed, as in ZC_USESKILL_ACK
 *    34    1 disposable 1 for a cast that packet marks so
 *    35    2 rev        the record's revision: the server counts it up for each record it makes
 *                       of the actor's act track, and of two records the one with the later
 *                       revision is the newer (compared by signed difference, as ticks are)
 *
 * ZC_TRACK_VITALS 0x0F22, 20 bytes: an actor's HP from at_tick on.
 *
 *     2    4 gid
 *     6    4 at_tick
 *    10    4 hp
 *    14    4 max_hp
 *    18    2 rev        as an act record's, of the actor's vitals track
 *
 * ZC_NOTICE_HIT 0x0F23, 32 bytes: a hit to show at at_tick, the moment it lands. It changes
 * nothing that lasts; the HP it took comes as a vitals record with the same tick.
 *
 *     2    4 source
 *     6    4 target
 *    10    4 at_tick
 *    14    4 damage       as in ZC_NOTIFY_ACT2, a heal negative
 *    18    4 left_damage
 *    22    2 count
 *    24    1 kind         ZC_NOTIFY_ACT2's action
 *    25    1 sp_damage    1 when the damage is to SP
 *    26    2 flinch_ms    the target's damage motion
 *    28    4              skill of the hits still to come; 0 */

enum { ACT_IDLE = 0, ACT_SWING = 1, ACT_CAST = 2 };
/* The top bit of the kind byte; the kind is the other seven. */
#define ACT_RECORD_OLD 0x80

struct ActRecord {
    uint32_t gid, at_tick, end_tick, mark_tick, target;
    int x, y;
    unsigned skill_id, skill_lv, motion_ms;
    int element;
    uint16_t rev;
    unsigned char kind, disposable;
    unsigned char old; /* sent later than it was made */
};

struct VitalsRecord {
    uint32_t gid, at_tick;
    int32_t hp, max_hp;
    uint16_t rev;
};

struct HitNotice {
    uint32_t source, target, at_tick;
    int32_t damage, left_damage;
    int count;
    unsigned flinch_ms;
    unsigned char kind, sp_damage;
};

/* Reads a ZC_TRACK_ACT. Returns 0 and writes nothing for a kind this client does not know, its
 * top bit taken off. */
int act_record_read(const unsigned char *packet, struct ActRecord *record);
void vitals_record_read(const unsigned char *packet, struct VitalsRecord *record);
void hit_notice_read(const unsigned char *packet, struct HitNotice *notice);

enum { TIMED_ACT, TIMED_HIT, TIMED_VITALS };

/* One of the three, with the tick it is played at and the actor it is of: the actor of an act
 * record, the source of a hit, the actor of a vitals record. */
struct TimedItem {
    uint32_t tick, actor;
    unsigned char kind;
    union {
        struct ActRecord act;
        struct HitNotice hit;
        struct VitalsRecord vitals;
    } u;
};

/* What a siege shows in the render delay, with room: two hundred actors swinging once a second
 * are forty items in 200 ms. */
#define TIMED_QUEUE_ITEMS 128
/* The actors that can be in the middle of something aimed at the player's own character at
 * once and have its end told on the own clock; with one more, the end of the one that began
 * longest ago is told on the render clock. */
#define TIMED_AIMED_ACTORS 16

struct TimedQueue {
    struct TimedItem items[TIMED_QUEUE_ITEMS]; /* by tick, earliest first; equal ticks in arrival order */
    int count;
    /* the actors whose act record played last was aimed at the own character, the one that
       began longest ago first */
    uint32_t aimed[TIMED_AIMED_ACTORS];
    int aimed_count;
};

/* Empties the queue. A zeroed queue is an empty one. */
void timed_queue_clear(struct TimedQueue *queue);

/* The clock an item is played on. There are two: the own clock, which the player's own
 * character is drawn on and which runs ahead of the server's by the time a request takes to be
 * applied (plans/netcode.md), and the render clock, which everyone else is drawn on and which
 * runs behind the server's. An item is played on the own clock when it is the own character's,
 * the actor `own`'s, or aimed at it: a swing or a cast of another actor whose target the own
 * character is, another actor's hit on it, and the idle record that ends such a swing or cast.
 * So a swing at the own character is shown landing when the own clock reaches the tick it
 * lands at, which is the last moment a request sent now is still applied before it; shown on
 * the render clock it would land on the screen after that moment had passed. Whoever plays a
 * swing paces its animation to that tick (std/swing_pace.h). Everything else is
 * played on the render clock, and everything while there is no own character (`own` 0). Returns
 * 1 for the own clock. */
int timed_queue_on_own_clock(const struct TimedQueue *queue, const struct TimedItem *item, uint32_t own);


enum { TIMED_DROPPED = 0, TIMED_FILED = 1, TIMED_FILED_EARLY = 2 };

/* Files an item that arrived, in whatever order and however often records arrive
 * (plans/delivery.md, part 1). An act or vitals record is a track's, of the unit whose floors are `floors`:
 * it is dropped when it is not newer than its track's floor (world/view_floors.h), when one of its
 * revision is held (it came twice), or when a newer held one of its track has the same or an
 * earlier tick (that one was made knowing it). Otherwise every older held record of its track
 * with the same or a later tick goes: the server made it knowing them, and it is how a swing is
 * cancelled. A hit is filed as it is, and `floors` may be NULL for one. Equal ticks come out in
 * the order they were filed. When the queue is full its earliest item is written to `early`:
 * the caller plays it at once, and it has left the queue as a taken item does. `own` is the
 * player's own character. Returns TIMED_DROPPED, TIMED_FILED, or TIMED_FILED_EARLY when `early`
 * was written. */
int timed_queue_file(struct TimedQueue *queue, const struct TimedItem *item, const struct RecordFloors *floors,
                     uint32_t own, struct TimedItem *early);

/* An item was played: its record is its track's floor, unless the floor is newer. */
void record_floors_played(struct RecordFloors *floors, const struct TimedItem *item);

/* Takes the earliest item that is due, and writes the clock it was taken on to `now`. An item
 * is due when the clock it is played on (timed_queue_on_own_clock) has reached its tick:
 * `own_now` or `render_now`. What an actor does is played in the order of its ticks whichever
 * clock each is on: an act or a hit that is due on the own clock brings every earlier act and
 * hit of its actor with it, on the own clock too. Returns 0 when none is due. */
int timed_queue_take(struct TimedQueue *queue, uint32_t own, uint32_t own_now, uint32_t render_now,
                     struct TimedItem *item, uint32_t *now);

/* Takes the earliest item of `actor`, due or not, and forgets that the actor is in the middle of
 * something aimed at the own character: the unit went (plans/presence.md, part 2). Returns 0
 * when it has none. */
int timed_queue_take_actor(struct TimedQueue *queue, uint32_t actor, struct TimedItem *item);

/* Takes the earliest vitals record of `gid` whose tick is not after `tick`, due or not: a hit
 * that is shown ahead of the render clock brings the HP it took with it. */
int timed_queue_take_vitals(struct TimedQueue *queue, uint32_t gid, uint32_t tick, struct TimedItem *item);

#endif
