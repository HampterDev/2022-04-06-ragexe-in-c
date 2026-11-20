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

/* The integer square root, rounded down. */
static uint32_t isqrt(uint32_t n)
{
    uint32_t root = 0, bit = 1u << 30;

    while (bit > n)
        bit >>= 2;
    while (bit) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

int move_follow(int32_t *x, int32_t *y, int32_t to_x, int32_t to_y, uint32_t real_ms, uint32_t ms_per_cell)
{
    int32_t dx = to_x - *x, dy = to_y - *y;
    uint32_t ax = (uint32_t)(dx < 0 ? -dx : dx), ay = (uint32_t)(dy < 0 ? -dy : dy), gap, reach;
    const uint32_t jump = MOVE_FOLLOW_JUMP_CELLS * MOVE_PLACE_CELL;

    if (!dx && !dy)
        return 0;
    /* a gap beyond the jump is taken at once, before its square can overflow */
    if (ms_per_cell == 0 || ax > jump || ay > jump) {
        *x = to_x;
        *y = to_y;
        return 1;
    }
    gap = isqrt(ax * ax + ay * ay);
    if (ax * ax + ay * ay > jump * jump) {
        *x = to_x;
        *y = to_y;
        return 1;
    }
    if (real_ms > 60000u)
        real_ms = 60000u;
    reach = real_ms * MOVE_PLACE_CELL * MOVE_FOLLOW_RATE_NUM / (ms_per_cell * MOVE_FOLLOW_RATE_DEN);
    if (reach >= gap) {
        *x = to_x;
        *y = to_y;
        return 1;
    }
    if (reach == 0)
        return 0;
    /* both within four cells: the products fit 32 bits */
    *x += dx * (int32_t)reach / (int32_t)gap;
    *y += dy * (int32_t)reach / (int32_t)gap;
    return 1;
}
