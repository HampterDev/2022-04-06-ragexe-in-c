#include "world/drawn.h"

/* A 256th in the share of one the point keeps beyond its 256ths. */
#define PART 65536

void drawn_place(const struct MoveRecord *record, uint32_t tick, struct DrawnPlace *place)
{
    struct MoveCell cells[MOVE_TRACK_STEPS + 1];
    int count = move_record_cells(record, cells), i;

    place->moving = 0;
    place->x = cells[0].x * DRAWN_CELL;
    place->y = cells[0].y * DRAWN_CELL;
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
        place->x = cells[i - 1].x * DRAWN_CELL + (cells[i].x - cells[i - 1].x) * DRAWN_CELL * gone / span;
        place->y = cells[i - 1].y * DRAWN_CELL + (cells[i].y - cells[i - 1].y) * DRAWN_CELL * gone / span;
        place->moving = 1;
        return;
    }
    place->x = cells[i < count ? i : count - 1].x * DRAWN_CELL;
    place->y = cells[i < count ? i : count - 1].y * DRAWN_CELL;
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

static uint32_t magnitude(int32_t v)
{
    return v < 0 ? (uint32_t)-v : (uint32_t)v;
}

/* Whether (x, y) is at most `cells` cells long, and its length in 256ths, rounded down, when it
   is. Each axis is bounded first, so the squares stay in 32 bits. */
static int within(int32_t x, int32_t y, int cells, uint32_t *length)
{
    uint32_t ax = magnitude(x), ay = magnitude(y), most = (uint32_t)cells * DRAWN_CELL;

    if (ax > most || ay > most)
        return 0;
    *length = isqrt(ax * ax + ay * ay);
    return *length <= most;
}

/* The point is on the place, and nothing of a slide is left. */
static void set_on(struct Drawn *drawn, const struct DrawnPlace *place)
{
    drawn->x = place->x;
    drawn->y = place->y;
    drawn->part_x = drawn->part_y = 0;
    drawn->off_x = drawn->off_y = 0;
    drawn->join_ms = drawn->gone_ms = 0;
    drawn->sliding = 0;
}

/* The point moves `steps` 256ths of the way `d` on one axis of a way `length` long: whole 256ths,
   and the share of one beyond them kept. `steps` is below `length`, at most 1449 256ths, so the
   products stay in 32 bits. */
static void move_axis(int32_t *at, int32_t *part, int32_t d, uint32_t steps, uint32_t length)
{
    int32_t whole = d * (int32_t)steps / (int32_t)length, rest = d * (int32_t)steps % (int32_t)length;

    *at += whole;
    *part += rest * PART / (int32_t)length;
    if (*part >= PART) {
        *part -= PART;
        *at += 1;
    } else if (*part <= -PART) {
        *part += PART;
        *at -= 1;
    }
}

/* A walked frame: the point steps toward the place by the reach `real_ms` adds, at 13/10 of the
   walk speed. The reach is counted in 256ths times ten times the walk speed, 13 * 256 of them a
   millisecond; what is not a whole 256th waits for the next frame. A point on its place keeps up
   to two 256ths of reach waiting, so that it takes the place's next 256th with it. */
static void walk_frame(struct Drawn *drawn, const struct DrawnPlace *place, uint32_t real_ms)
{
    uint32_t per = drawn->walk_ms * DRAWN_RATE_DEN, have, steps, length;
    int32_t dx = place->x - drawn->x, dy = place->y - drawn->y;

    if (!within(dx, dy, DRAWN_WALK_CELLS, &length)) {
        set_on(drawn, place);
        drawn->reach = 0;
        return;
    }
    /* at most 60,000 ms at 3,328 a millisecond, and two 256ths waiting: below 2^28 */
    have = drawn->reach + real_ms * DRAWN_CELL * DRAWN_RATE_NUM;
    steps = have / per;
    if (steps >= length) {
        have -= length * per;
        set_on(drawn, place);
        drawn->reach = have < 2 * per ? have : 2 * per - 1;
        return;
    }
    drawn->reach = have - steps * per;
    move_axis(&drawn->x, &drawn->part_x, dx, steps, length);
    move_axis(&drawn->y, &drawn->part_y, dy, steps, length);
}

/* The offset of a slide `gone_ms` into it, toward zero. */
static void slide_offset(const struct Drawn *drawn, int32_t *x, int32_t *y)
{
    int32_t left, whole;

    if (!drawn->join_ms || drawn->gone_ms >= drawn->join_ms) {
        *x = *y = 0;
        return;
    }
    left = (int32_t)(drawn->join_ms - drawn->gone_ms);
    whole = (int32_t)drawn->join_ms;
    *x = drawn->off_x * left / whole;
    *y = drawn->off_y * left / whole;
}

void drawn_take(struct Drawn *drawn, const struct MoveRecord *record, uint32_t tick, uint32_t walk_ms)
{
    struct DrawnPlace place;
    int cause = record->cause & ~MOVE_RECORD_OLD;
    int32_t x, y;
    uint32_t length;

    drawn->record = *record;
    /* a walk's place moves at the walk's own speed, which the point keeps up with */
    if (record->steps && record->speed_ms && cause == MOVE_CAUSE_WALKED)
        walk_ms = record->speed_ms;
    drawn->walk_ms = !walk_ms ? 150u : walk_ms < DRAWN_WALK_MS_MAX ? walk_ms : DRAWN_WALK_MS_MAX;
    drawn_place(record, tick, &place);
    if (!drawn->has || cause == MOVE_CAUSE_PLACED) {
        drawn->has = 1;
        set_on(drawn, &place);
        drawn->reach = 0;
        drawn->on_place = 1;
        return;
    }
    if (cause != MOVE_CAUSE_PUSHED) {
        /* walked: the point steps on from where it is; a slide under way ends there */
        drawn->sliding = 0;
        drawn->join_ms = drawn->gone_ms = 0;
        return;
    }
    x = drawn->x - place.x;
    y = drawn->y - place.y;
    /* the slide's time is the record's; a push with steps or too long is taken at once, and one
       without a time is over at its first frame */
    if ((!x && !y) || record->steps || !within(x, y, DRAWN_SLIDE_CELLS, &length)) {
        set_on(drawn, &place);
        return;
    }
    drawn->off_x = x;
    drawn->off_y = y;
    drawn->part_x = drawn->part_y = 0;
    drawn->join_ms = record->land_ms > 0xFFFFu ? 0xFFFFu : record->land_ms;
    drawn->gone_ms = 0;
    drawn->sliding = 1;
}

int drawn_frame(struct Drawn *drawn, uint32_t tick, uint32_t real, struct DrawnFrame *frame)
{
    struct DrawnPlace place;
    uint32_t real_ms;
    int32_t was_x = drawn->x, was_y = drawn->y;

    if (!drawn->has) {
        frame->place.x = frame->place.y = frame->place.moving = 0;
        frame->x = frame->y = frame->step_x = frame->step_y = 0;
        frame->motion = DRAWN_RESTS;
        frame->on_place = 1;
        return 0;
    }
    real_ms = drawn->clocked ? real - drawn->real : 0;
    if (real_ms > DRAWN_JOIN_MS_MAX)
        real_ms = DRAWN_JOIN_MS_MAX;
    drawn->real = real;
    drawn->clocked = 1;
    drawn_place(&drawn->record, tick, &place);
    if (drawn->sliding) {
        int32_t ox, oy;

        drawn->gone_ms = drawn->join_ms - drawn->gone_ms > real_ms ? drawn->gone_ms + real_ms : drawn->join_ms;
        slide_offset(drawn, &ox, &oy);
        if (!ox && !oy) {
            set_on(drawn, &place);
        } else {
            drawn->x = place.x + ox;
            drawn->y = place.y + oy;
        }
    } else {
        walk_frame(drawn, &place, real_ms);
    }
    drawn->on_place = drawn->x == place.x && drawn->y == place.y;
    if (drawn->on_place)
        drawn->part_x = drawn->part_y = 0;
    drawn->step_x = drawn->x - was_x;
    drawn->step_y = drawn->y - was_y;
    frame->place = place;
    frame->x = drawn->x;
    frame->y = drawn->y;
    frame->step_x = drawn->step_x;
    frame->step_y = drawn->step_y;
    frame->on_place = drawn->on_place;
    drawn->motion = (unsigned char)(drawn->sliding ? DRAWN_SLIDES
                                    : (!drawn->on_place || place.moving) ? DRAWN_WALKS : DRAWN_RESTS);
    frame->motion = drawn->motion;
    return 1;
}

const struct MoveRecord *drawn_record(const struct Drawn *drawn)
{
    return drawn->has ? &drawn->record : 0;
}

int drawn_on_place(const struct Drawn *drawn)
{
    return !drawn->has || drawn->on_place;
}

int drawn_motion(const struct Drawn *drawn)
{
    return drawn->motion;
}

void drawn_point(const struct Drawn *drawn, int32_t *x, int32_t *y)
{
    *x = drawn->has ? drawn->x : 0;
    *y = drawn->has ? drawn->y : 0;
}
