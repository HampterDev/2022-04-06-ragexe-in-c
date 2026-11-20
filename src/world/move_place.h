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

#endif
