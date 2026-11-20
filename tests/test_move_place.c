#include "world/move_place.h"
#include "body/body.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* plans/drawn-place.md, check 1: the place is a function of the record and the tick. */

static uint32_t g_seed = 0x9e3779b9u;

static uint32_t next(void)
{
    g_seed = g_seed * 1664525u + 1013904223u;
    return g_seed >> 8;
}

/* A walk of `steps` random directions from (100, 100), whose first step lands `land_ms` after
   `at`, at `speed_ms` a cell. */
static void walk(struct MoveRecord *record, uint32_t at, int steps, unsigned land_ms, unsigned speed_ms)
{
    int i;

    memset(record, 0, sizeof *record);
    record->at_tick = at;
    record->x = 100;
    record->y = 100;
    record->land_ms = land_ms;
    record->speed_ms = speed_ms;
    record->steps = (unsigned char)steps;
    for (i = 0; i < steps; i++)
        record->path[i] = (unsigned char)(next() % 8);
    record->dir = steps ? record->path[0] : 0;
}

static void a_record_without_steps_is_its_cells_middle(void)
{
    struct MoveRecord record;
    struct MovePlace place;

    walk(&record, 1000, 0, 0, 150);
    move_place(&record, 0, &place);
    assert(place.x == 100 * MOVE_PLACE_CELL && place.y == 100 * MOVE_PLACE_CELL && !place.moving);
    move_place(&record, 999999, &place);
    assert(place.x == 100 * MOVE_PLACE_CELL && place.y == 100 * MOVE_PLACE_CELL && !place.moving);
}

/* East, east, north-east at 150 ms a cell, the first landing 100 ms after the record: before the
   first step its start cell, half way through a step half way between, on a step's time its
   cell's middle, after the last step the last cell. */
static void a_walk_is_on_its_lines_by_its_steps_times(void)
{
    struct MoveRecord record;
    struct MovePlace place;

    memset(&record, 0, sizeof record);
    record.at_tick = 1000;
    record.x = 10;
    record.y = 20;
    record.land_ms = 100;
    record.speed_ms = 150;
    record.steps = 3;
    record.path[0] = 6; /* east */
    record.path[1] = 6;
    record.path[2] = 7; /* north-east */
    /* the first step started 50 ms before the record: it is under way at its tick */
    move_place(&record, 950, &place);
    assert(place.x == 10 * 256 && place.y == 20 * 256 && !place.moving);
    move_place(&record, 1025, &place);
    assert(place.x == 10 * 256 + 128 && place.y == 20 * 256 && place.moving);
    move_place(&record, 1100, &place);
    assert(place.x == 11 * 256 && place.y == 20 * 256);
    move_place(&record, 1175, &place);
    assert(place.x == 11 * 256 + 128 && place.y == 20 * 256);
    move_place(&record, 1250, &place);
    assert(place.x == 12 * 256 && place.y == 20 * 256);
    move_place(&record, 1300, &place);
    assert(place.x > 12 * 256 && place.x < 13 * 256 && place.x == place.y - 8 * 256 && place.moving);
    move_place(&record, 5000, &place);
    assert(place.x == 13 * 256 && place.y == 21 * 256 && !place.moving);
}

/* For random walks, against the body core playing the same record (body_move_at): at every
   millisecond the place lies on the line between the cell the core has the body on and the next
   cell of the walk, and on the tick the core enters a cell the place is on that cell's middle. */
static void the_core_enters_each_cell_when_the_place_is_on_its_middle(void)
{
    int n;

    for (n = 0; n < 400; n++) {
        struct MoveRecord record;
        struct BodyMove core, at;
        struct MovePlace place;
        uint32_t base = 0x7fffff00u + next() % 512, t;
        int steps = 1 + (int)(next() % 12), i, lastx = 100, lasty = 100;
        unsigned speed = 80 + next() % 400;

        walk(&record, base, steps, 1 + next() % speed, speed);
        memset(&core, 0, sizeof core);
        core.at_tick = base;
        core.x = (int16_t)record.x;
        core.y = (int16_t)record.y;
        core.land_ms = (uint16_t)record.land_ms;
        core.speed_ms = (uint16_t)record.speed_ms;
        core.dir = record.dir;
        core.steps = record.steps;
        for (i = 0; i < steps; i++)
            core.path[i] = record.path[i];
        for (t = base; t < base + (uint32_t)(steps + 2) * speed * 2; t++) {
            body_move_at(&core, t, &at);
            move_place(&record, t, &place);
            if (at.x != lastx || at.y != lasty) {
                assert(place.x == at.x * 256 && place.y == at.y * 256);
                lastx = at.x;
                lasty = at.y;
            }
            /* within one cell of the core's cell, on the side of its next one */
            assert(place.x - at.x * 256 <= 256 && at.x * 256 - place.x <= 256);
            assert(place.y - at.y * 256 <= 256 && at.y * 256 - place.y <= 256);
            if (at.steps == 0)
                assert(place.x == at.x * 256 && place.y == at.y * 256 && !place.moving);
        }
    }
}

/* The same record and tick give the same place in any order of asking, and between two ticks
   the place moves no further than the walk's speed allows: a cell on an axis in a straight step's
   time. */
static void any_order_and_no_faster_than_the_walk(void)
{
    int n;

    for (n = 0; n < 200; n++) {
        struct MoveRecord record;
        struct MovePlace a, b, c;
        uint32_t base = next(), t, u;
        unsigned speed = 80 + next() % 400;

        walk(&record, base, 1 + (int)(next() % 20), 1 + next() % speed, speed);
        for (t = base - 100; t < base + 20 * speed * 2; t += 1 + next() % 37) {
            move_place(&record, t, &a);
            u = base + next() % (20 * speed * 2);
            move_place(&record, u, &b);
            move_place(&record, t, &c);
            assert(a.x == c.x && a.y == c.y && a.moving == c.moving);
            move_place(&record, t + 1, &b);
            assert(b.x - a.x <= (int32_t)(256 / speed + 2) && a.x - b.x <= (int32_t)(256 / speed + 2));
            assert(b.y - a.y <= (int32_t)(256 / speed + 2) && a.y - b.y <= (int32_t)(256 / speed + 2));
        }
    }
}

static int32_t abs32(int32_t v)
{
    return v < 0 ? -v : v;
}

/* The longest a frame's step may be: `real_ms` at 13/10 of `ms_per_cell`, in 256ths, and a 256th
   for the rounding. */
static int32_t bound(uint32_t real_ms, uint32_t ms_per_cell)
{
    return (int32_t)(real_ms * 256u * 13u / (ms_per_cell * 10u)) + 1;
}

/* plans/drawn-place.md, check 2: from any drawn point and any sequence of records, no frame's step
   is longer than the bound unless the gap is over four cells, which is taken at once; and once
   there is no news, the drawn point is on the place by the time the gap at 13/10 of the speed
   takes, and a frame more. Records come at random frames: walks and stops, each a newer place
   than the last, as a track hands them over in any order of their filing. */
static void the_drawn_point_arrives(void)
{
    int n;

    for (n = 0; n < 3000; n++) {
        struct MoveRecord record;
        struct MovePlace place;
        uint32_t speed = 80 + next() % 420, tick = 0x7fff0000u + next() % 1000, quiet_from = 0;
        int32_t x = (int32_t)(next() % (16 * 256)) + 92 * 256, y = (int32_t)(next() % (16 * 256)) + 92 * 256;
        int frame, news = (int)(next() % 6), arrived = 0;

        walk(&record, tick, (int)(next() % 6), 1 + next() % speed, speed);
        for (frame = 0; frame < 4000 && !arrived; frame++) {
            uint32_t real = 1 + next() % 60;
            int32_t was_x = x, was_y = y, gap_x, gap_y;

            tick += real;
            if (news > 0 && next() % 40 == 0) {
                news--;
                walk(&record, tick - next() % 300, (int)(next() % 6), 1 + next() % speed, speed);
                record.x = 95 + (int)(next() % 10);
                record.y = 95 + (int)(next() % 10);
                quiet_from = 0;
            }
            move_place(&record, tick, &place);
            gap_x = place.x - x;
            gap_y = place.y - y;
            move_follow(&x, &y, place.x, place.y, real, speed);
            /* more than four cells, straight-line: taken at once */
            if ((double)gap_x * gap_x + (double)gap_y * gap_y > 4.0 * 256 * 4.0 * 256)
                assert(x == place.x && y == place.y);
            else
                assert(abs32(x - was_x) <= bound(real, speed) && abs32(y - was_y) <= bound(real, speed));
            /* no news and the walk over: the place rests */
            if (news == 0 && !place.moving) {
                if (!quiet_from) {
                    int32_t gap = abs32(gap_x) > abs32(gap_y) ? abs32(gap_x) : abs32(gap_y);

                    /* the time the gap takes, by its longer axis at worst 1.42 times that */
                    quiet_from = tick + (uint32_t)gap * speed * 10u * 142u / (256u * 13u * 100u) + 61u;
                }
                if (x == place.x && y == place.y)
                    arrived = 1;
                else
                    assert((int32_t)(tick - quiet_from) < 0);
            } else {
                quiet_from = 0;
            }
        }
        assert(arrived);
    }
}

/* The cases the playground found before the place: a monster whose walk a hit and its swing cut
   between two cells, left 0.47 cells off the cell of the stop that came then; the own character
   0.35 cells off its cell on both axes after its swing cut its walk. Whatever the actor does, the
   drawn point follows the stop's place and is on its cell's middle in the time the gap takes. */
static void a_walk_cut_between_two_cells_ends_on_the_stops_middle(void)
{
    static const int32_t starts[2][2] = { { 151 * 256, 187 * 256 + 120 }, { 156 * 256 + 90, 177 * 256 + 90 } };
    static const int cells[2][2] = { { 151, 187 }, { 156, 177 } };
    int n;

    for (n = 0; n < 2; n++) {
        struct MoveRecord stop;
        struct MovePlace place;
        int32_t x = starts[n][0], y = starts[n][1];
        uint32_t t;

        memset(&stop, 0, sizeof stop);
        stop.at_tick = 5000;
        stop.x = cells[n][0];
        stop.y = cells[n][1];
        for (t = 5000; t < 5000 + 120; t += 16) {
            move_place(&stop, t, &place);
            move_follow(&x, &y, place.x, place.y, 16, 150);
        }
        assert(x == stop.x * 256 && y == stop.y * 256);
    }
}

/* A placing is taken at once, and a point on its place stays put. */
static void a_placing_is_taken_at_once(void)
{
    int32_t x = 10 * 256, y = 10 * 256;

    assert(move_follow(&x, &y, 12 * 256 + 3, 9 * 256, 1, 0) && x == 12 * 256 + 3 && y == 9 * 256);
    assert(!move_follow(&x, &y, x, y, 50, 150));
    /* a frame too short to move a 256th leaves it */
    x = 10 * 256;
    assert(!move_follow(&x, &y, 11 * 256, 9 * 256, 0, 150) && x == 10 * 256);
}

int main(void)
{
    a_record_without_steps_is_its_cells_middle();
    a_walk_is_on_its_lines_by_its_steps_times();
    the_core_enters_each_cell_when_the_place_is_on_its_middle();
    any_order_and_no_faster_than_the_walk();
    the_drawn_point_arrives();
    a_placing_is_taken_at_once();
    a_walk_cut_between_two_cells_ends_on_the_stops_middle();
    puts("move_place: ok");
    return 0;
}
