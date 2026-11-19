#ifndef WORLD_MOVE_TRACK_H
#define WORLD_MOVE_TRACK_H

#include <stddef.h>
#include <stdint.h>
#include "world/view_floors.h"

/* Where an actor is, as the server sends it in ZC_TRACK_MOVE (plans/core-loop.md): a record
 * that holds from a server tick on and is complete without what the client knew before. In
 * freestanding C: no allocation, no globals, integer math only. Ticks are 32 bits and may wrap.
 *
 * The packet, little-endian and packed, 25 bytes and the steps:
 *
 *   off size field
 *     0    2 id        0x0F20
 *     2    2 length    of the packet
 *     4    4 gid
 *     8    4 at_tick   the record holds from this tick on
 *    12    2 x         the cell the actor is on at at_tick
 *    14    2 y
 *    16    2 land_ms   the first step lands this long after at_tick; 0 without steps
 *    18    2 speed_ms  a straight step's time
 *    20    1 dir       the way it faces while it stands
 *    21    1 cause     MOVE_CAUSE_*; with MOVE_RECORD_OLD the record was made before it was
 *                      sent, for a viewer who came to see the actor later
 *    22    1 steps     0 to MOVE_TRACK_STEPS
 *    23    2 rev       the record's revision: the server counts it up for each record it makes
 *                      of the actor's move track, and of two records the one with the later
 *                      revision is the newer (compared by signed difference, as ticks are)
 *    25    n path      three bits a step, lowest bits first, the server's direction codes
 *                      (body/grid_path.h); (3 * steps + 7) / 8 bytes
 *
 * The first step lands at at_tick + land_ms, whatever its own time is: longer, the actor stands
 * on its cell first (a hit delayed its walk); shorter, the step was under way at at_tick. Every
 * later step takes its own time (grid_path_step_ms). */

#define MOVE_TRACK_STEPS 32
/* Records an actor can have waiting for their tick. */
#define MOVE_TRACK_HELD 4

enum { MOVE_CAUSE_WALKED = 0, MOVE_CAUSE_PUSHED = 1, MOVE_CAUSE_PLACED = 2 };
/* The top bit of the cause byte; the cause is the other seven. */
#define MOVE_RECORD_OLD 0x80

struct MoveRecord {
    uint32_t at_tick;
    int x, y;
    unsigned land_ms, speed_ms;
    uint16_t rev;                         /* its revision; the records a client makes count their own */
    unsigned char dir, cause, steps;
    unsigned char old;                    /* sent later than it was made */
    unsigned char path[MOVE_TRACK_STEPS]; /* one direction a step */
};

/* A cell of a record's walk and the tick the actor is on it. */
struct MoveCell {
    int x, y, dir;
    uint32_t tick;
};

/* An actor's move track: the records that wait for their tick. What was played of it is the
 * floor of its unit (world/view_floors.h), which outlives the actor. All zero holds nothing. */
struct MoveTrack {
    struct MoveRecord held[MOVE_TRACK_HELD]; /* by at_tick, earliest first, and so by revision */
    int count;
};

/* Reads a ZC_TRACK_MOVE of `size` bytes. Returns 0 and writes nothing when the packet is
 * shorter than its fields, says another length than it has, has more than MOVE_TRACK_STEPS
 * steps or steps that do not fit it, or an unknown cause. */
int move_record_read(const unsigned char *packet, size_t size, uint32_t *gid, struct MoveRecord *record);

/* The record's cells in walking order, steps + 1 of them: out[0] is the record's own cell at
 * the tick its first step starts (at_tick + land_ms minus that step's time, which is before
 * at_tick for a step under way), out[i] the cell step i ends on and when. Without steps it is
 * the one cell at at_tick. `out` holds MOVE_TRACK_STEPS + 1 cells. Returns the count. */
int move_record_cells(const struct MoveRecord *record, struct MoveCell *out);

/* What a record without steps asks of the actor it is for. The stop comes with the tick it
 * happened at and not with a place to be set to, so an actor that can walk there does. */
enum {
    MOVE_STAND_LEAVE = 0,  /* it stands on the record's cell already: nothing */
    MOVE_STAND_FINISH = 1, /* it walks and is on the record's cell: its walk ends on that cell's middle */
    MOVE_STAND_WALK = 2,   /* it is on another cell: it walks to the record's */
    MOVE_STAND_SET = 3,    /* it is set onto the record's cell */
    MOVE_STAND_HALT = 4    /* it walks, is on the record's cell and is held: its walk ends where it is drawn */
};

struct MoveStandAsk {
    int drawn_x, drawn_y; /* the cell the actor is drawn on */
    int walking;          /* it is walking */
    int held;             /* it is not to be moved now: a hit's motion holds it, or it slides */
    int jump_cells;       /* more cells than these are not walked off */
};

/* An actor that was pushed or placed is set. One that walked is on the record's cell at its
 * tick; one that still walks on that cell walks on to its middle, and stops where it is when
 * it is held, since moving it there would be a jump. One that is drawn on another cell (it
 * runs behind its walk, or the walk it was drawn on was another) walks there from wherever it
 * is, unless that is further than `jump_cells`. */
int move_record_stand(const struct MoveRecord *record, const struct MoveStandAsk *ask);

/* Files a record that arrived, in whatever order and however often records arrive
 * (plans/delivery.md, part 1). It is dropped when it is not newer than the move floor of
 * `floors` (it is the past), when a record of its revision is held (it came twice), or when a newer held
 * record has the same or an earlier tick (that one was made knowing it). Otherwise every older
 * held record with the same or a later tick goes, as the server made this one knowing them.
 * Filed in any order, the same records leave the same track; in the order they were made it is
 * the rule of plans/core-loop.md. When MOVE_TRACK_HELD earlier ones wait, the earliest goes: a
 * later record is complete without it. Returns 1 when the record was filed. */
int move_track_file(struct MoveTrack *track, const struct RecordFloors *floors, const struct MoveRecord *record);

/* Every record that waits for a tick after `now` leaves the track: what was filed here ahead of
 * the server (the player's own walk, planned) and is not to come. */
void move_track_drop_after(struct MoveTrack *track, uint32_t now);

/* Takes the record that holds at `now`: the latest one whose tick has come. It and every
 * earlier one leave the track, and it is the move floor of `floors` unless that is newer: a
 * record not newer than it is the past from then on. Returns 0 when none is due. */
int move_track_take(struct MoveTrack *track, struct RecordFloors *floors, uint32_t now, struct MoveRecord *record);

/* Whether a record that waits, with a tick not after `until`, will move an actor that is drawn
 * standing on the cell (x, y): a walk, or a record without steps that asks anything of such an
 * actor (move_record_stand). The record without steps that only says the actor stands where it
 * is drawn, as the server sends one when an actor stops to swing, will not. */
int move_track_will_move(const struct MoveTrack *track, int x, int y, uint32_t until);

#endif
