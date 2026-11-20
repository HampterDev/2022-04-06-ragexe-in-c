#ifndef WORLD_VIEW_FLOORS_H
#define WORLD_VIEW_FLOORS_H

#include <stdint.h>

/* What the client has of each unit by its id, whichever actor shows it (plans/presence.md,
 * part 2): the floor of each of its tracks, below which a record is the past, and the ticks of
 * its last coming into view and its last going. They outlive the actor, so a record made before
 * a unit went that arrives after it came back is known for the past. In freestanding C: no
 * allocation, no globals, no clock, no library; the memory is the caller's.
 *
 * ZC_VIEW_STAMP 0x0F26, 17 bytes, the project's own, sent directly before the entry or vanish
 * packet it stamps:
 *
 *   off size field
 *     0    2 id
 *     2    4 gid
 *     6    4 tick       the server tick the unit is in this viewer's world from (comes), or no
 *                       longer in it from (goes)
 *    10    1 kind       VIEW_COMES or VIEW_GOES
 *    11    2 rev_move   the unit's revision of each track at that moment
 *    13    2 rev_act
 *    15    2 rev_vitals */

enum { VIEW_COMES = 0, VIEW_GOES = 1 };
enum { VIEW_STAMP_SIZE = 17 };

struct ViewStamp {
    uint32_t gid, tick;
    uint16_t rev_move, rev_act, rev_vitals;
    unsigned char kind;
};

/* Reads a ZC_VIEW_STAMP of `size` bytes. Returns 0 and writes nothing when it is shorter than
 * its fields or of an unknown kind. */
int view_stamp_read(const unsigned char *packet, unsigned size, struct ViewStamp *stamp);

/* Whether the packet that came directly after `stamp` is the one it stamps: the entry (0x09fd,
 * 0x09fe, 0x09ff) of its unit for a coming, the vanish (0x0080) of its unit for a going. A stamp
 * belongs to the very next packet only; one before any other packet is dropped, and a vanish
 * without a stamp, as of a player who died and lies where she fell, is no going. The packet is
 * read bounded by `size`. */
int view_stamp_stamps(const struct ViewStamp *stamp, int packet_id, const unsigned char *packet, unsigned size);

/* The floors of a unit's tracks: what of each was played last, or what its going or coming
 * said is the past. A record of a track that is not newer than its floor is the past. All zero
 * is nothing played. */
struct RecordFloors {
    uint16_t move, act, vitals;
    unsigned char has_move, has_act, has_vitals;
};

/* Whether revision `a` is newer than `b`, however the counter has wrapped. */
int record_rev_newer(uint16_t a, uint16_t b);

/* How a record of a track that arrives, of `tick` and revision `rev`, stands to one of the same
 * track that is held, of `held_tick` and `held_rev` (plans/core-loop.md, rule 3; plans/delivery.md,
 * part 1). Every track files by this one rule, so records may arrive in any order and more than
 * once:
 *   RECORD_REFUSED   the arriving one is dropped: it came twice (the same revision), or the held
 *                    one is newer and its tick is the same or earlier (it was made knowing it);
 *   RECORD_REPLACES  the held one goes: the arriving one is newer and its tick the same or
 *                    earlier, so the server made it knowing the held one (a cancelled swing);
 *   RECORD_BESIDE    both stay. */
enum { RECORD_BESIDE = 0, RECORD_REFUSED = 1, RECORD_REPLACES = 2 };
int record_against(uint32_t held_tick, uint16_t held_rev, uint32_t tick, uint16_t rev);

struct ViewUnit {
    uint32_t gid;
    uint32_t came, went;  /* the ticks of its last coming and its last going */
    uint32_t used;        /* when it was last asked for, by the count of `clock` */
    struct RecordFloors floors;
    unsigned char in_view;
    unsigned char pinned; /* the player's own character: in view without a coming */
    unsigned char has_came, has_went;
};

/* More units than are seen at once in a siege, so that a unit that went is still known when its
   late records arrive. */
#define VIEW_FLOORS_UNITS 1024

struct ViewFloors {
    struct ViewUnit units[VIEW_FLOORS_UNITS];
    int count;
    uint32_t clock;
};

/* Forgets every unit: the map was left. A zeroed one is empty. */
void view_floors_clear(struct ViewFloors *view);

/* The unit `gid`, made when it is not known. When all VIEW_FLOORS_UNITS are known, the one out
 * of view that was asked for longest ago gives its place; units in view never do. Returns NULL
 * when every one is in view, or for gid 0. */
struct ViewUnit *view_floors_unit(struct ViewFloors *view, uint32_t gid);

/* The unit `gid` when it is known, without making it. */
const struct ViewUnit *view_floors_find(const struct ViewFloors *view, uint32_t gid);

/* Whether the unit is in view: it came and has not gone since, or it is pinned. */
int view_floors_in_view(const struct ViewFloors *view, uint32_t gid);

/* The player's own character is in view without a coming, and a stamp of it changes nothing. */
void view_floors_pin(struct ViewFloors *view, uint32_t gid);

/* A stamp was read. A coming puts the unit in view, and its floors become the stamp's
 * revisions: a record older than them is the past, one equal or newer is taken. A going takes
 * it out of view, and its floors become the stamp's revisions themselves: only a newer record
 * belongs to a later stay. Floors never go back. Returns the unit, or NULL when a stamp says
 * nothing (it is of a pinned unit, or no unit can be made). */
const struct ViewUnit *view_floors_stamp(struct ViewFloors *view, const struct ViewStamp *stamp);

/* Whether a notice of the unit dated `tick` is of a stay that is over: dated before its last
 * coming, or, while it is out of view, not after its last going. What is due at a tick comes
 * before a going of that tick, so a notice dated at the going is of the stay that ended. */
int view_floors_past_notice(const struct ViewFloors *view, uint32_t gid, uint32_t tick);

#endif
