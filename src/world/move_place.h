#ifndef WORLD_MOVE_PLACE_H
#define WORLD_MOVE_PLACE_H

#include <stdint.h>
#include "world/move_track.h"

/* The place a move record gives at a tick (plans/drawn-place.md, part 1), in 256ths of a cell,
 * a cell's middle at a whole multiple of MOVE_PLACE_CELL:
 *   - a record without steps: the middle of its cell;
 *   - a record with steps: before its first step's time the middle of the cell it starts on;
 *     during a step on the line between the middles of the step's two cells, by the share of
 *     the step's time that has passed; after its last step the middle of its last cell.
 * The steps' times are the record's own (move_record_cells), which are the body core's.
 * Freestanding and integer: the same record and tick give the same place, whatever was asked
 * before. Ticks are the server's, 32 bits, which wrap. */

#define MOVE_PLACE_CELL 256

struct MovePlace {
    int32_t x, y;     /* 256ths of a cell */
    int32_t moving;   /* a step is under way at the tick */
};

void move_place(const struct MoveRecord *record, uint32_t tick, struct MovePlace *place);

/* The drawn point follows the place (plans/drawn-place.md, part 2): one step a frame toward
 * (to_x, to_y), of at most `real_ms`, the real time since the last frame, at 13/10 of
 * `ms_per_cell`; `ms_per_cell` 0 takes it at once, and so is a gap of more than
 * MOVE_FOLLOW_JUMP_CELLS. (x, y) and the place are in 256ths of a cell. Returns whether the
 * point moved. */
#define MOVE_FOLLOW_RATE_NUM 13
#define MOVE_FOLLOW_RATE_DEN 10
#define MOVE_FOLLOW_JUMP_CELLS 4

int move_follow(int32_t *x, int32_t *y, int32_t to_x, int32_t to_y, uint32_t real_ms, uint32_t ms_per_cell);

#endif
