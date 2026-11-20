#include "world/drawn.h"
#include "body/body.h"
#include "body/grid_path.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* plans/drawn-place.md, checks 1, 2 and 5: the place is a function of the record and the tick,
   and the drawn point is the place and an offset that shrinks by the real time, whatever the
   frames are. */

static uint32_t g_seed = 0x9e3779b9u;

static uint32_t next(void)
{
    g_seed = g_seed * 1664525u + 1013904223u;
    return g_seed >> 8;
}

static int32_t abs32(int32_t v)
{
    return v < 0 ? -v : v;
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
    record->land_ms = steps ? land_ms : 0;
    record->speed_ms = speed_ms;
    record->steps = (unsigned char)steps;
    for (i = 0; i < steps; i++)
        record->path[i] = (unsigned char)(next() % 8);
    record->dir = steps ? record->path[0] : 0;
}

/* A record without steps on (x, y) from `at`, by `cause`. */
static void stand(struct MoveRecord *record, uint32_t at, int x, int y, int cause)
{
    memset(record, 0, sizeof *record);
    record->at_tick = at;
    record->x = x;
    record->y = y;
    record->speed_ms = 150;
    record->cause = (unsigned char)cause;
}

/* ---- the place ---- */

static void a_record_without_steps_is_its_cells_middle(void)
{
    struct MoveRecord record;
    struct DrawnPlace place;

    walk(&record, 1000, 0, 0, 150);
    drawn_place(&record, 0, &place);
    assert(place.x == 100 * DRAWN_CELL && place.y == 100 * DRAWN_CELL && !place.moving);
    drawn_place(&record, 999999, &place);
    assert(place.x == 100 * DRAWN_CELL && place.y == 100 * DRAWN_CELL && !place.moving);
}

/* East, east, north-east at 150 ms a cell, the first landing 100 ms after the record: before the
   first step its start cell, half way through a step half way between, on a step's time its
   cell's middle, after the last step the last cell. */
static void a_walk_is_on_its_lines_by_its_steps_times(void)
{
    struct MoveRecord record;
    struct DrawnPlace place;

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
    drawn_place(&record, 950, &place);
    assert(place.x == 10 * 256 && place.y == 20 * 256 && !place.moving);
    drawn_place(&record, 1025, &place);
    assert(place.x == 10 * 256 + 128 && place.y == 20 * 256 && place.moving);
    drawn_place(&record, 1100, &place);
    assert(place.x == 11 * 256 && place.y == 20 * 256);
    drawn_place(&record, 1175, &place);
    assert(place.x == 11 * 256 + 128 && place.y == 20 * 256);
    drawn_place(&record, 1250, &place);
    assert(place.x == 12 * 256 && place.y == 20 * 256);
    drawn_place(&record, 1300, &place);
    assert(place.x > 12 * 256 && place.x < 13 * 256 && place.x == place.y - 8 * 256 && place.moving);
    drawn_place(&record, 5000, &place);
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
        struct DrawnPlace place;
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
            drawn_place(&record, t, &place);
            if (at.x != lastx || at.y != lasty) {
                assert(place.x == at.x * 256 && place.y == at.y * 256);
                lastx = at.x;
                lasty = at.y;
            }
            assert(place.x - at.x * 256 <= 256 && at.x * 256 - place.x <= 256);
            assert(place.y - at.y * 256 <= 256 && at.y * 256 - place.y <= 256);
            if (at.steps == 0)
                assert(place.x == at.x * 256 && place.y == at.y * 256 && !place.moving);
        }
    }
}

/* The same record and tick give the same place in any order of asking, and between two ticks
   the place moves no further than the walk's speed allows. */
static void any_order_and_no_faster_than_the_walk(void)
{
    int n;

    for (n = 0; n < 200; n++) {
        struct MoveRecord record;
        struct DrawnPlace a, b, c;
        uint32_t base = next(), t, u;
        unsigned speed = 80 + next() % 400;

        walk(&record, base, 1 + (int)(next() % 20), 1 + next() % speed, speed);
        for (t = base - 100; t < base + 20 * speed * 2; t += 1 + next() % 37) {
            drawn_place(&record, t, &a);
            u = base + next() % (20 * speed * 2);
            drawn_place(&record, u, &b);
            drawn_place(&record, t, &c);
            assert(a.x == c.x && a.y == c.y && a.moving == c.moving);
            drawn_place(&record, t + 1, &b);
            assert(abs32(b.x - a.x) <= (int32_t)(256 / speed + 2) && abs32(b.y - a.y) <= (int32_t)(256 / speed + 2));
        }
    }
}

/* ---- frames ---- */

/* Frame times: `pattern` 1 to 50 is that many ms each frame; 0 is a mix of 1 to 50. */
static uint32_t frame_ms(int pattern)
{
    return pattern ? (uint32_t)pattern : 1u + next() % 50u;
}

/* The time a walked gap of `length` 256ths takes at `speed` ms a cell, rounded up. */
static uint32_t walk_off_ms(uint32_t length, uint32_t speed)
{
    return (length * speed * DRAWN_RATE_DEN + 256u * DRAWN_RATE_NUM - 1u) / (256u * DRAWN_RATE_NUM);
}

/* A point that starts on a walking place stays on it at every frame, for walk speeds 100 to
   2000 ms a cell and frame times 1 to 50 ms and mixed. */
static void a_point_on_a_walking_place_stays_on_it(void)
{
    uint32_t speed;
    int pattern;

    for (speed = 100; speed <= 2000; speed += 50) {
        for (pattern = 0; pattern <= 50; pattern += (pattern < 4 ? 1 : 7)) {
            struct Drawn drawn;
            struct DrawnFrame frame;
            struct MoveRecord record;
            uint32_t tick = 0xfffff000u, real = 77, end;

            memset(&drawn, 0, sizeof drawn);
            walk(&record, tick, 12, speed, speed);
            /* the actor's own speed is another: a walk is joined at the walk's */
            drawn_take(&drawn, &record, tick, speed * 4);
            end = tick + 16 * speed * 2;
            while ((int32_t)(tick - end) < 0) {
                drawn_frame(&drawn, tick, real, &frame);
                assert(frame.on_place && frame.x == frame.place.x && frame.y == frame.place.y);
                assert(frame.motion != DRAWN_SLIDES);
                {
                    uint32_t ms = frame_ms(pattern);

                    tick += ms;
                    real += ms;
                }
            }
        }
    }
}

/* From a point on one cell, a record by `cause` on a cell (dx, dy) off, a push with the slide time
   `slide_ms`: the frames until the point is on the place, the real time that took, each walked
   frame's step no longer than 13/10 of the speed allows and a 256th per axis. */
static uint32_t walk_a_gap_off(uint32_t speed, int dx, int dy, int pattern, int cause, uint32_t slide_ms)
{
    struct Drawn drawn;
    struct DrawnFrame frame;
    struct MoveRecord record;
    uint32_t tick = 0x7ffffff0u, real = 0xfffffff0u, gone = 0, ms;
    int frames = 0;

    memset(&drawn, 0, sizeof drawn);
    stand(&record, tick, 50, 50, MOVE_CAUSE_PLACED);
    drawn_take(&drawn, &record, tick, speed);
    drawn_frame(&drawn, tick, real, &frame);
    stand(&record, tick, 50 + dx, 50 + dy, cause);
    if (cause == MOVE_CAUSE_PUSHED)
        record.land_ms = slide_ms;
    drawn_take(&drawn, &record, tick, speed);
    for (;;) {
        int32_t was_x = drawn.x, was_y = drawn.y;

        ms = frame_ms(pattern);
        tick += ms;
        real += ms;
        gone += ms;
        drawn_frame(&drawn, tick, real, &frame);
        /* a gap of more than four cells is taken at once, so only a shorter one is bounded */
        if (cause == MOVE_CAUSE_WALKED && dx * dx + dy * dy <= 16) {
            int32_t most = (int32_t)(ms * 256u * DRAWN_RATE_NUM / (speed * DRAWN_RATE_DEN)) + 1;

            assert(abs32(frame.x - was_x) <= most && abs32(frame.y - was_y) <= most);
        }
        if (frame.on_place) {
            assert(frame.x == (50 + dx) * 256 && frame.y == (50 + dy) * 256);
            return gone;
        }
        assert(frame.motion == (cause == MOVE_CAUSE_PUSHED ? DRAWN_SLIDES : DRAWN_WALKS));
        assert(++frames < 1000000);
    }
}

/* A gap is walked off in the same real time, within one frame, whatever the frame time: for
   walk speeds 100 to 2000 and frame times 1 to 50 ms and mixed, against the time its length takes
   at 13/10 of the speed. */
static void a_gap_is_walked_off_in_the_same_real_time(void)
{
    static const int gaps[][2] = { { 1, 0 }, { 0, -3 }, { 2, 2 }, { -3, 1 }, { 4, 0 } };
    uint32_t speed;
    int g, pattern;

    for (speed = 100; speed <= 2000; speed += 100) {
        for (g = 0; g < (int)(sizeof gaps / sizeof gaps[0]); g++) {
            double length = 256.0 * sqrt((double)(gaps[g][0] * gaps[g][0] + gaps[g][1] * gaps[g][1]));
            uint32_t takes = walk_off_ms((uint32_t)length, speed);

            for (pattern = 0; pattern <= 50; pattern += (pattern < 4 ? 1 : 3)) {
                uint32_t gone = walk_a_gap_off(speed, gaps[g][0], gaps[g][1], pattern, MOVE_CAUSE_WALKED, 0);

                /* on the place no later than a frame after the time, and no sooner than the last
                   256th of the way before it */
                assert(gone <= takes + 50);
                assert(gone + 50 + takes / (uint32_t)length + 1 >= takes);
            }
        }
    }
}

/* A walked gap of more than four cells is taken at once, in the first frame. */
static void a_walked_gap_of_six_cells_is_at_once(void)
{
    int pattern;

    for (pattern = 0; pattern <= 50; pattern += 5) {
        uint32_t gone = walk_a_gap_off(150, 6, 0, pattern, MOVE_CAUSE_WALKED, 0);

        assert(gone == frame_ms(pattern) || pattern == 0);
    }
    assert(walk_a_gap_off(150, 4, 4, 16, MOVE_CAUSE_WALKED, 0) == 16);
    assert(walk_a_gap_off(150, 4, 0, 16, MOVE_CAUSE_WALKED, 0) > 16);
}

/* A push of 6 and of 18 cells slides over the time its record carries, and not in one frame,
   whatever its length: a knockback's 200 ms, a dash's 15 ms a cell, Asura Strike's 55; one
   without a time, or of more than 32 cells, is at once. */
static void a_push_slides_over_its_time(void)
{
    int pattern;

    for (pattern = 0; pattern <= 50; pattern += 5) {
        uint32_t a = walk_a_gap_off(150, 6, 0, pattern, MOVE_CAUSE_PUSHED, 200);
        uint32_t b = walk_a_gap_off(150, -18, 0, pattern, MOVE_CAUSE_PUSHED, 200);
        uint32_t c = walk_a_gap_off(150, 0, 18, pattern, MOVE_CAUSE_PUSHED, 18 * 15);
        uint32_t d = walk_a_gap_off(150, 12, 12, pattern, MOVE_CAUSE_PUSHED, 12 * 55);

        assert(a + 50 >= 200 && a <= 200 + 50);
        assert(b + 50 >= 200 && b <= 200 + 50);
        assert(c + 50 >= 18 * 15 && c <= 18 * 15 + 50);
        assert(d + 50 >= 12 * 55 && d <= 12 * 55 + 50);
    }
    assert(walk_a_gap_off(150, 6, 0, 16, MOVE_CAUSE_PUSHED, 200) > 16);
    assert(walk_a_gap_off(150, 6, 0, 16, MOVE_CAUSE_PUSHED, 0) == 16);
    assert(walk_a_gap_off(150, 33, 0, 16, MOVE_CAUSE_PUSHED, 200) == 16);
    assert(walk_a_gap_off(150, 32, 0, 16, MOVE_CAUSE_PUSHED, 200) > 16);
}

/* A placing, and an actor's first record, are taken at once. */
static void a_placing_and_a_first_record_are_at_once(void)
{
    struct Drawn drawn;
    struct DrawnFrame frame;
    struct MoveRecord record;

    memset(&drawn, 0, sizeof drawn);
    assert(!drawn_frame(&drawn, 5, 5, &frame) && frame.on_place && drawn_on_place(&drawn) && !drawn_record(&drawn));
    stand(&record, 100, 10, 10, MOVE_CAUSE_WALKED);
    drawn_take(&drawn, &record, 100, 150);
    assert(drawn_frame(&drawn, 100, 1000, &frame) && frame.on_place && frame.x == 10 * 256);
    assert(drawn_record(&drawn)->x == 10);
    stand(&record, 120, 13, 9, MOVE_CAUSE_PLACED);
    drawn_take(&drawn, &record, 120, 150);
    drawn_frame(&drawn, 121, 1001, &frame);
    assert(frame.on_place && frame.x == 13 * 256 && frame.y == 9 * 256 && frame.motion == DRAWN_RESTS);
    /* the old mark of a record sent late does not change its cause */
    stand(&record, 130, 14, 9, MOVE_CAUSE_PLACED | MOVE_RECORD_OLD);
    drawn_take(&drawn, &record, 130, 150);
    drawn_frame(&drawn, 131, 1002, &frame);
    assert(frame.on_place && frame.x == 14 * 256);
}

/* A clock that jumps moves the place further than the frame's real time lets it walk: the point
   does not jump with it, and is on the place again in the time the jump takes to walk off. */
static void a_clock_jump_is_walked_off(void)
{
    struct Drawn drawn;
    struct DrawnFrame frame;
    struct MoveRecord record;
    uint32_t tick = 1000, real = 50, gone = 0;
    int32_t was_x;

    memset(&drawn, 0, sizeof drawn);
    memset(&record, 0, sizeof record);
    record.at_tick = tick;
    record.x = 20;
    record.y = 20;
    record.land_ms = 150;
    record.speed_ms = 150;
    record.steps = 10;
    memset(record.path, 6, 10); /* east */
    drawn_take(&drawn, &record, tick, 150);
    drawn_frame(&drawn, tick, real, &frame);
    tick += 16;
    real += 16;
    drawn_frame(&drawn, tick, real, &frame);
    assert(frame.on_place);
    /* the clock jumps 300 ms in a frame of 16 */
    was_x = frame.x;
    tick += 316;
    real += 16;
    drawn_frame(&drawn, tick, real, &frame);
    assert(!frame.on_place && frame.x - was_x <= 16 * 256 * 13 / (150 * 10) + 2 && frame.x > was_x);
    while (!frame.on_place) {
        was_x = frame.x;
        tick += 16;
        real += 16;
        gone += 16;
        drawn_frame(&drawn, tick, real, &frame);
        /* never faster than 13/10 of the walk speed, and a 256th for the place's rounding and one
           for the offset's */
        assert(frame.x - was_x <= 16 * 256 * 13 / (150 * 10) + 2);
        assert(gone < 3000);
    }
    /* two cells behind a place that walks on: the 3/10 of the speed the place's own walk leaves
       of 13/10 takes 1000 ms */
    assert(gone >= 992 && gone <= 1016);
}

/* Records filed through a move track in any order and more than once: the point ends on the
   place of the newest. */
static void records_in_any_order_end_on_the_newest(void)
{
    int n;

    for (n = 0; n < 2000; n++) {
        struct MoveRecord made[8], last;
        struct MoveTrack track;
        struct RecordFloors floors;
        struct Drawn drawn;
        struct DrawnFrame frame;
        struct DrawnPlace place;
        uint32_t tick = 0x7fffff00u + next() % 1000, real = next(), base = tick;
        int count = 2 + (int)(next() % 7), order[24], filings, i, newest = 0;

        memset(&track, 0, sizeof track);
        memset(&floors, 0, sizeof floors);
        memset(&drawn, 0, sizeof drawn);
        for (i = 0; i < count; i++) {
            if (next() % 3)
                walk(&made[i], base + (uint32_t)i * 120, (int)(next() % 5), 1 + next() % 150, 150);
            else
                stand(&made[i], base + (uint32_t)i * 120, 98 + (int)(next() % 5), 98 + (int)(next() % 5),
                      (int)(next() % 3));
            made[i].rev = (uint16_t)(0xfffe + i);
        }
        filings = count + (int)(next() % 12);
        for (i = 0; i < filings; i++)
            order[i] = i < count ? i : (int)(next() % (uint32_t)count);
        for (i = filings - 1; i > 0; i--) {
            int j = (int)(next() % (uint32_t)(i + 1)), t = order[i];

            order[i] = order[j];
            order[j] = t;
        }
        /* filed at random frames, while the frames go on */
        for (i = 0; i < filings || tick - base < (uint32_t)count * 120u + 3000u; ) {
            uint32_t ms = 1 + next() % 50;

            if (i < filings && next() % 3 == 0) {
                move_track_file(&track, &floors, &made[order[i]]);
                if (order[i] > newest)
                    newest = order[i];
                i++;
            }
            if (move_track_take(&track, &floors, tick, &last))
                drawn_take(&drawn, &last, tick, 150);
            drawn_frame(&drawn, tick, real, &frame);
            tick += ms;
            real += ms;
        }
        /* the newest record that came is the place, and the point is on it */
        assert(drawn_record(&drawn) != NULL && drawn_record(&drawn)->rev == made[newest].rev);
        drawn_place(&made[newest], tick, &place);
        drawn_frame(&drawn, tick, real, &frame);
        assert(frame.on_place && frame.x == place.x && frame.y == place.y);
    }
}

/* ---- against a reference ---- */

/* The rule written out in doubles, slowly: the place by walking the record's steps one by one;
   a slide's offset by its share of the slide's time; a walked point by a step toward the place
   of each frame, 13/10 of the walk speed for the frame's time, or onto it. */
struct Reference {
    struct MoveRecord record;
    double x, y, off_x, off_y, join, gone, speed;
    double carried; /* how far the piece's point was from this one at the take */
    int has, slide;
};

static void reference_place(const struct MoveRecord *record, uint32_t tick, double *x, double *y)
{
    static const int dx[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
    static const int dy[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    double cx = record->x, cy = record->y;
    int32_t lands = (int32_t)record->land_ms, starts, at = (int32_t)(tick - record->at_tick);
    int i;

    *x = cx * 256.0;
    *y = cy * 256.0;
    if (!record->steps)
        return;
    starts = lands - (int32_t)grid_path_step_ms((int)record->speed_ms, record->path[0] & 1);
    for (i = 0; i < record->steps; i++) {
        double nx = cx + dx[record->path[i]], ny = cy + dy[record->path[i]];

        if (at <= starts)
            break;
        if (at < lands) {
            double share = (double)(at - starts) / (double)(lands - starts);

            *x = 256.0 * (cx + (nx - cx) * share);
            *y = 256.0 * (cy + (ny - cy) * share);
            return;
        }
        cx = nx;
        cy = ny;
        *x = cx * 256.0;
        *y = cy * 256.0;
        if (i + 1 < record->steps) {
            starts = lands;
            lands += (int32_t)grid_path_step_ms((int)record->speed_ms, record->path[i + 1] & 1);
        }
    }
}

static void reference_take(struct Reference *r, const struct MoveRecord *record, uint32_t tick, uint32_t speed,
                           int piece_slides)
{
    double px, py, length, bound;

    r->record = *record;
    reference_place(record, tick, &px, &py);
    r->speed = record->steps && record->cause == MOVE_CAUSE_WALKED ? record->speed_ms : speed;
    if (!r->has || record->cause == MOVE_CAUSE_PLACED) {
        r->has = 1;
        r->slide = 0;
        r->x = px;
        r->y = py;
        return;
    }
    if (record->cause != MOVE_CAUSE_PUSHED) {
        r->slide = 0;
        return;
    }
    r->off_x = r->x - px;
    r->off_y = r->y - py;
    r->gone = 0;
    /* the length in whole 256ths, as the piece measures it */
    length = floor(sqrt(r->off_x * r->off_x + r->off_y * r->off_y));
    bound = 256.0 * DRAWN_SLIDE_CELLS;
    /* within the two points' difference of the bound the piece's own measure decides */
    if (fabs(length - bound) <= r->carried + 2.0)
        length = piece_slides ? bound : bound + 1;
    if ((r->off_x == 0 && r->off_y == 0) || length > bound || record->land_ms == 0) {
        r->slide = 0;
        r->x = px;
        r->y = py;
        return;
    }
    r->slide = 1;
    r->join = record->land_ms;
}

/* A frame `ms` after the last; `piece_on`: the piece is on its place, which decides a gap within
   the two points' rounding of four cells. */
static void reference_frame(struct Reference *r, uint32_t tick, uint32_t ms, int piece_on)
{
    double px, py;

    reference_place(&r->record, tick, &px, &py);
    if (r->slide) {
        double share;

        r->gone += ms;
        share = r->join > 0 && r->gone < r->join ? (r->join - r->gone) / r->join : 0;
        r->x = px + r->off_x * share;
        r->y = py + r->off_y * share;
        if (share == 0)
            r->slide = 0;
    } else {
        double dx = px - r->x, dy = py - r->y, length = sqrt(dx * dx + dy * dy);
        double reach = ms * 13.0 / 10.0 * 256.0 / r->speed, bound = 256.0 * DRAWN_WALK_CELLS;
        int far = fabs(length - bound) <= 4.0 ? piece_on : length > bound;

        if (far || reach >= length) {
            r->x = px;
            r->y = py;
        } else {
            r->x += dx * reach / length;
            r->y += dy * reach / length;
        }
    }
}

/* Random records (walks, stops, pushes with a slide's time or none, and placings), taken at
   random frames of 1 to 50 ms, on a clock that goes with the real time: the point is the
   reference's within the rounding, every frame. */
static void the_point_is_the_references(void)
{
    int n;

    for (n = 0; n < 3000; n++) {
        struct Drawn drawn;
        struct DrawnFrame frame;
        struct Reference ref;
        struct MoveRecord record;
        uint32_t speed = 100 + next() % 1901, tick = next(), real = next();
        int frames;

        memset(&drawn, 0, sizeof drawn);
        memset(&ref, 0, sizeof ref);
        for (frames = 0; frames < 400; frames++) {
            uint32_t ms = 1 + next() % 50;

            if (frames == 0 || next() % 25 == 0) {
                int kind = (int)(next() % 4);

                if (kind == 0) {
                    walk(&record, tick - next() % 200, 1 + (int)(next() % 8), 1 + next() % speed, speed);
                    record.x = 95 + (int)(next() % 10);
                    record.y = 95 + (int)(next() % 10);
                } else {
                    stand(&record, tick - next() % 200, 90 + (int)(next() % 20), 90 + (int)(next() % 20), kind - 1);
                }
                /* a push carries its slide's time, or none */
                if (record.cause == MOVE_CAUSE_PUSHED)
                    record.land_ms = next() % 5 ? 50 + next() % 950 : 0;
                drawn_take(&drawn, &record, tick, speed);
                /* the two points may differ by their roundings so far: that much goes into the
                   offset, and shrinks with it */
                ref.carried = fabs(drawn.x - ref.x) > fabs(drawn.y - ref.y) ? fabs(drawn.x - ref.x) : fabs(drawn.y - ref.y);
                reference_take(&ref, &record, tick, speed, drawn.sliding);
            }
            tick += ms;
            real += ms;
            drawn_frame(&drawn, tick, real, &frame);
            reference_frame(&ref, tick, frames == 0 ? 0 : ms, frame.on_place);
            /* a 256th for each of the place's roundings and what the two points differed by at the
               take; a slide's way in the time its two joins' times differ by from measuring a
               length in whole 256ths; a walked point's two 256ths of reach a point on its place
               keeps waiting, and the rounding of its steps' directions, which a long chase of a
               walking place carries along: at most eight 256ths, a 32nd of a cell */
            {
                double off = sqrt(ref.off_x * ref.off_x + ref.off_y * ref.off_y);
                double slack = ref.slide ? 3.0 + ref.carried + (ref.join > 0 ? off * (fabs((double)drawn.join_ms - ref.join) + 1.0) / ref.join : 0)
                                         : 8.0;

                assert(fabs(frame.x - ref.x) <= slack && fabs(frame.y - ref.y) <= slack);
            }
        }
    }
}

int main(void)
{
    a_record_without_steps_is_its_cells_middle();
    a_walk_is_on_its_lines_by_its_steps_times();
    the_core_enters_each_cell_when_the_place_is_on_its_middle();
    any_order_and_no_faster_than_the_walk();
    a_point_on_a_walking_place_stays_on_it();
    a_gap_is_walked_off_in_the_same_real_time();
    a_walked_gap_of_six_cells_is_at_once();
    a_push_slides_over_its_time();
    a_placing_and_a_first_record_are_at_once();
    a_clock_jump_is_walked_off();
    records_in_any_order_end_on_the_newest();
    the_point_is_the_references();
    puts("drawn: ok");
    return 0;
}
