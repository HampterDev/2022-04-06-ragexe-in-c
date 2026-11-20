#ifndef WORLD_DRAWN_H
#define WORLD_DRAWN_H

#include <stdint.h>
#include "view/move_place.h"

/* Where an actor is drawn (plans/drawn-place.md). Two things, both functions of what is handed
 * in and nothing else:
 *
 *   the place   where the actor's move record has it at a tick (move_place);
 *   the point   where it is drawn, which joins the place when a record puts the place
 *               elsewhere, by the real time that passes.
 *
 * How the point joins the place is decided from the record's cause and how far it is:
 *
 *   walked      a walked record (a walk, a stop, a correction): each frame the point steps
 *               toward the place of the frame, at most 13/10 of the actor's walk speed for the
 *               real time since the last frame. What a frame's reach leaves of a 256th, and
 *               the share of a 256th a step moves the point by, are carried to the next frame,
 *               so the same real time reaches as far however it is cut into frames, and a point
 *               on a walking place stays on it. A gap of more than DRAWN_WALK_CELLS is taken at
 *               once. A record with steps is joined at its own speed. A clock that jumps moves
 *               the place, and the point walks after it like after any other gap;
 *   slid        a pushed record (a knockback, a slide, a dash): the point is the place and the
 *               offset it had at the push, which shrinks to nothing over the slide's time, which
 *               the record carries (move_track.h); one without a time, or longer than
 *               DRAWN_SLIDE_CELLS, is taken at once;
 *   at once     a placed record, and an actor's first record.
 *
 * Places and points are in 256ths of a cell, a cell's middle at a whole multiple of MOVE_PLACE_CELL.
 * Freestanding integer C: no clock, no float, no library, no allocation, no 64-bit product, and
 * it compiles without /std:c11. Ticks are the server's, 32 bits, which wrap; the caller's real
 * time is milliseconds that only go forward and may wrap. */

/* The point walks at most DRAWN_RATE_NUM / DRAWN_RATE_DEN of the walk speed. */
#define DRAWN_RATE_NUM 13
#define DRAWN_RATE_DEN 10
/* A walked gap longer than this many cells is taken at once. */
#define DRAWN_WALK_CELLS 4
/* A pushed offset longer than this many cells is taken at once. The bound keeps every product
 * in 32 bits: an offset of 32 cells is 8192 256ths, the sum of its squares at most 2^27, and an
 * offset times a slide's time, at most 65535 ms, below 2^31. */
#define DRAWN_SLIDE_CELLS 32
/* The most real time one frame counts, and the slowest walk speed. */
#define DRAWN_JOIN_MS_MAX 60000u
#define DRAWN_WALK_MS_MAX 10000u

/* What the drawn point does in a frame. */
enum { DRAWN_RESTS = 0, DRAWN_WALKS = 1, DRAWN_SLIDES = 2 };

/* One actor's drawn place. All zero is an actor without a record yet; the caller keeps it and
 * changes it only through the functions below. */
struct Drawn {
    struct MoveRecord record; /* the record the place comes from */
    uint32_t real;            /* the caller's real time at the last frame */
    int32_t x, y;             /* the drawn point */
    int32_t part_x, part_y;   /* and the share of a 256th beyond it, in 65536ths */
    uint32_t reach;           /* reach not taken yet: 256ths times ten times the walk speed */
    int32_t off_x, off_y;     /* a slide: the offset when it began, the point less the place */
    uint32_t join_ms;         /* a slide: its whole time */
    uint32_t gone_ms;         /* the real time of the slide gone, at most join_ms */
    uint32_t walk_ms;         /* ms a cell the actor walks */
    int32_t step_x, step_y;   /* how far the point moved in the last frame */
    unsigned char has;        /* a record was taken */
    unsigned char sliding;    /* the point slides */
    unsigned char clocked;    /* `real` holds a frame's real time */
    unsigned char on_place;   /* the point was on the place at the last frame */
    unsigned char motion;     /* DRAWN_* of the last frame */
};

/* `record` becomes the actor's place at `tick`. `walk_ms` is its walk speed, ms a straight
 * cell (0: 150). */
void drawn_take(struct Drawn *drawn, const struct MoveRecord *record, uint32_t tick, uint32_t walk_ms);

/* What a frame shows. */
struct DrawnFrame {
    struct MovePlace place; /* the place at the frame's tick */
    int32_t x, y;            /* the drawn point */
    int32_t step_x, step_y;  /* how far it moved since the last frame: its heading */
    int motion;              /* DRAWN_RESTS, DRAWN_WALKS or DRAWN_SLIDES */
    int on_place;            /* the point is on the place */
};

/* A frame at the actor's `tick` and the caller's real time `real` in ms: the real time since
 * the last frame passes, none at the actor's first frame. Without a record yet it writes zeros
 * and returns 0; otherwise it returns 1. */
int drawn_frame(struct Drawn *drawn, uint32_t tick, uint32_t real, struct DrawnFrame *frame);

/* The record the place comes from, or NULL before the first. */
const struct MoveRecord *drawn_record(const struct Drawn *drawn);

/* Whether the point was on its place at the last frame; 1 before the first record. */
int drawn_on_place(const struct Drawn *drawn);

/* What the point did at the last frame: DRAWN_RESTS before the first. */
int drawn_motion(const struct Drawn *drawn);

/* The drawn point at the last frame, in 256ths: 0, 0 before the first record. */
void drawn_point(const struct Drawn *drawn, int32_t *x, int32_t *y);

#endif
