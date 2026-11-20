#include "world/move_place.h"

void move_place(const struct MoveRecord *record, uint32_t tick, struct MovePlace *place)
{
    struct MoveCell cells[MOVE_TRACK_STEPS + 1];
    int count = move_record_cells(record, cells), i;

    place->moving = 0;
    place->x = cells[0].x * MOVE_PLACE_CELL;
    place->y = cells[0].y * MOVE_PLACE_CELL;
    if (count < 2 || (int32_t)(tick - cells[0].tick) <= 0)
        return;
    for (i = 1; i < count; i++) {
        int32_t span = (int32_t)(cells[i].tick - cells[i - 1].tick);
        int32_t gone = (int32_t)(tick - cells[i - 1].tick);

        if ((int32_t)(tick - cells[i].tick) >= 0)
            continue;
        /* the step to cell i is under way: a share of the line between the two middles */
        if (span <= 0)
            break;
        place->x = cells[i - 1].x * MOVE_PLACE_CELL + (cells[i].x - cells[i - 1].x) * MOVE_PLACE_CELL * gone / span;
        place->y = cells[i - 1].y * MOVE_PLACE_CELL + (cells[i].y - cells[i - 1].y) * MOVE_PLACE_CELL * gone / span;
        place->moving = 1;
        return;
    }
    place->x = cells[i < count ? i : count - 1].x * MOVE_PLACE_CELL;
    place->y = cells[i < count ? i : count - 1].y * MOVE_PLACE_CELL;
}
