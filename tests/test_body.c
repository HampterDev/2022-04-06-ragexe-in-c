/* The native tests of the body core (docs/body-core.md). `body_tests` runs them and exits 0
 * when all pass; `body_tests --digests` prints the rows of sim/body/body_vectors.h as the core
 * gives them now. */
#include "body/body.h"
#include "body_scenario.h"
#include "body_vectors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            failures++;                                                     \
        }                                                                   \
    } while (0)

/* The arena's layout, which sim/body/body.c holds itself to: the header, the slots, the ring. */
enum {
    HEADER_BYTES = 56,
    BODY_BYTES = 224,
    EVENT_BYTES = 32,
    AT_MAGIC = 0,
    AT_VERSION = 4,
    AT_NOW = 8,
    AT_SIZE = 16,
    AT_BODIES = 20,
    AT_EVENTS = 24,
    AT_EVENT_FIRST = 28,
    AT_EVENT_COUNT = 32,
    AT_WIDTH = 36,
    AT_HEIGHT = 40,
    AT_ASKED = 44,
    AT_REQUEST_STEPS = 52,
    BODY_AT_ID = 0,
    BODY_AT_USED = 4,
    BODY_AT_REQUEST = 6,
    BODY_AT_TICK = 8,
    BODY_AT_X = 16,
    BODY_AT_Y = 18,
    BODY_AT_LAND = 20,
    BODY_AT_SPEED = 22,
    BODY_AT_DIR = 24,
    BODY_AT_CAUSE = 25,
    BODY_AT_STEPS = 26,
    BODY_AT_MOVE_RESERVED = 27,
    BODY_AT_PATH = 32,
    BODY_AT_TO_X = 64,
    BODY_AT_CHANGE = 68,
    BODY_AT_GOAL = 69,
    BODY_AT_RANGE = 70,
    BODY_AT_TARGET_ID = 72,
    BODY_AT_TARGET = 76,
    BODY_AT_WALK_ATTACK = 78,
    BODY_AT_WALK_RESERVED = 79,
    BODY_AT_CELL_TICK = 80,
    BODY_AT_CAN_MOVE = 88,
    BODY_AT_ATTACK_DUE = 112,
    BODY_AT_IMPACT_DUE = 120,
    BODY_AT_ATTACK_TARGET_ID = 128,
    BODY_AT_ATTACK_TARGET = 136,
    BODY_AT_SWING_REACH = 140,
    BODY_AT_ATTACK_RANGE = 148,
    BODY_AT_FLAGS = 149,
    BODY_AT_HAS_TARGET = 156,
    BODY_AT_ARMED = 158,
    BODY_AT_SWING_OPEN = 159,
    BODY_AT_ASKING = 160,
    BODY_AT_REQUESTED = 161,
    BODY_AT_KIND = 162,
    BODY_AT_CASTING = 163,
    BODY_AT_HELD_MS = 164,
    BODY_AT_CAST_DUE = 168,
    BODY_AT_INTENT_DUE = 176,
    BODY_AT_INTENT_WALK_AT = 184,
    BODY_AT_INTENT_TARGET_ID = 192,
    BODY_AT_INTENT_TARGET = 196,
    BODY_AT_INTENT_SEQ = 198,
    BODY_AT_INTENT_X = 200,
    BODY_AT_INTENT_Y = 202,
    BODY_AT_INTENT_HAS = 204,
    BODY_AT_INTENT_KIND = 205,
    BODY_AT_INTENT_MODE = 206,
    BODY_AT_INTENT_WALKED = 207,
    BODY_AT_INTENT_TRIES = 208,
    BODY_AT_INTENT_SWUNG = 209,
    BODY_AT_INTENT_ASKING = 210,
    BODY_AT_INTENT_ACTING = 211,
    BODY_AT_INTENT_RANGE = 212,
    BODY_AT_INTENT_RESERVED = 213,
    EVENT_AT_TICK = 0,
    EVENT_AT_KIND = 12,
    EVENT_AT_SLOT = 14,
    EVENT_AT_X = 16,
    EVENT_AT_DIR = 20,
    EVENT_AT_RESERVED = 21,
    EVENT_AT_OTHER = 28,
    EVENT_AT_RESERVED2 = 30
};

/* The directions the tests walk in. */
enum { NORTH = 0, WEST = 2, SOUTH = 4, EAST = 6, NORTH_EAST = 7 };

/* A map of 10 by 8 cells, all open until a test closes one, and a search's working memory. */
enum { MAP_WIDTH = 10, MAP_HEIGHT = 8 };
static uint8_t cells[MAP_WIDTH * MAP_HEIGHT];
static uint32_t scratch[BODY_SCRATCH_SIZE / 4];
static const struct BodyGround ground = {cells, scratch};

static void open_map(void)
{
    memset(cells, 1, sizeof cells);
}

static void close_cell(int x, int y)
{
    cells[y * MAP_WIDTH + x] = 0;
}

static void *memory(size_t size)
{
    void *block = calloc(1, size);

    if (block == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(2);
    }
    return block;
}

static struct BodyInput input(uint64_t tick, int kind, int slot, uint32_t id, int x, int y, int value)
{
    struct BodyInput in;

    in.tick = tick;
    in.id = id;
    in.kind = (uint16_t)kind;
    in.slot = (uint16_t)slot;
    in.x = (int16_t)x;
    in.y = (int16_t)y;
    in.value = (uint16_t)value;
    in.dir = 0;
    in.cause = 0;
    in.target_id = 0;
    in.target = 0;
    in.reserved[0] = in.reserved[1] = 0;
    return in;
}

/* The body `id` in `slot` is to walk after the body `target_id` in slot `target` until it is
 * within `range` cells of it. */
static struct BodyInput chase(uint64_t tick, int slot, uint32_t id, int target, uint32_t target_id, int range)
{
    struct BodyInput in = input(tick, BODY_INPUT_CHASE, slot, id, 0, 0, range);

    in.target = (uint16_t)target;
    in.target_id = target_id;
    return in;
}

static int apply(struct BodyWorld *world, struct BodyInput in)
{
    return body_input(world, &ground, &in);
}

/* The input is refused with `code` and the world's bytes are as they were. */
static int refused(struct BodyWorld *world, size_t size, struct BodyInput in, int code)
{
    void *before = memory(size);
    int result, unchanged;

    memcpy(before, world, size);
    result = body_input(world, &ground, &in);
    unchanged = memcmp(before, world, size) == 0;
    free(before);
    return result == code && unchanged;
}

/* The next waiting event is of `kind`, for the body in `slot`, at `tick`, on (x, y). */
static int event_is(struct BodyWorld *world, int kind, int slot, uint64_t tick, int x, int y)
{
    struct BodyEvent event;

    return body_take_event(world, &event) && event.kind == kind && event.slot == slot && event.tick == tick
           && event.x == x && event.y == y;
}

static int no_event(struct BodyWorld *world)
{
    struct BodyEvent event;

    return !body_take_event(world, &event);
}

/* A world of four slots and a ring of `events` on the test map at tick 1000, with body 7 in
 * slot 1 on (2, 2) at 100 ms a cell, its first event taken. */
static struct BodyWorld *walker_world(uint32_t events, size_t *size)
{
    struct BodyWorld *world;
    struct BodyEvent event;

    open_map();
    *size = body_world_size(4, events);
    world = body_world_init(memory(*size), *size, 4, events, MAP_WIDTH, MAP_HEIGHT, 1000);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 1, 7, 2, 2, 100)) == BODY_OK);
    CHECK(body_take_event(world, &event) == 1);
    return world;
}

static void test_sizes(void)
{
    CHECK(body_rules_version() == BODY_RULES_VERSION);
    CHECK(body_world_size(8, 16) == HEADER_BYTES + 8 * BODY_BYTES + 16 * EVENT_BYTES);
    CHECK(body_world_size(0, 16) == 0);
    CHECK(body_world_size(8, BODY_MIN_EVENTS - 1) == 0);
    CHECK(body_world_size(8, BODY_MIN_EVENTS) != 0);
    CHECK(body_world_size(BODY_MAX_BODIES, BODY_MAX_EVENTS) != 0);
    CHECK(body_world_size(BODY_MAX_BODIES + 1, 16) == 0);
    CHECK(body_world_size(8, BODY_MAX_EVENTS + 1) == 0);
}

static void test_init(void)
{
    size_t size = body_world_size(4, 8);
    unsigned char *arena = memory(size + 8);
    struct BodyWorld *world;
    struct BodyMove move;
    uint64_t due;

    CHECK(body_world_init(NULL, size, 4, 8, 10, 8, 100) == NULL);
    CHECK(body_world_init(arena + 4, size, 4, 8, 10, 8, 100) == NULL);
    CHECK(body_world_init(arena, size - 1, 4, 8, 10, 8, 100) == NULL);
    CHECK(body_world_init(arena, size, 0, 4, 10, 8, 100) == NULL);
    CHECK(body_world_init(arena, size, 4, 0, 10, 8, 100) == NULL);
    CHECK(body_world_init(arena, size, 4, 8, 0, 8, 100) == NULL);
    CHECK(body_world_init(arena, size, 4, 8, 10, 0, 100) == NULL);
    CHECK(body_world_init(arena, size, 4, 8, BODY_MAX_SIDE + 1, 8, 100) == NULL);
    CHECK(body_world_init(arena, size, 4, 8, 10, BODY_MAX_SIDE + 1, 100) == NULL);

    /* what was in the memory before does not matter */
    memset(arena, 0xA5, size);
    world = body_world_init(arena, size, 4, 8, 10, 8, 100);
    CHECK(world == (struct BodyWorld *)arena);
    if (world != NULL) {
        CHECK(body_world_check(arena, size) == BODY_OK);
        CHECK(body_now(world) == 100);
        CHECK(body_move(world, 0, &move) == BODY_BAD_SLOT);
        CHECK(body_move(world, 4, &move) == BODY_BAD_SLOT);
        CHECK(body_next_due(world, &due) == 0);
        CHECK(body_run(world, &ground, 99) == BODY_BAD_TICK);
        CHECK(body_run(world, &ground, 150) == BODY_OK && body_now(world) == 150);
    }
    world = body_world_init(arena, size, 4, 8, BODY_MAX_SIDE, BODY_MAX_SIDE, 0);
    CHECK(world != NULL && body_world_check(arena, size) == BODY_OK);
    free(arena);
}

/* Bodies are added, placed and removed; what does not fit is refused. */
static void test_inputs(void)
{
    size_t size = body_world_size(4, 8);
    void *arena = memory(size);
    struct BodyWorld *world = body_world_init(arena, size, 4, 8, MAP_WIDTH, MAP_HEIGHT, 100);
    int i;
    struct BodyInput in;
    struct BodyEvent event;
    struct BodyMove move;

    open_map();
    in = input(100, BODY_INPUT_ADD, 1, 7, 3, 4, 150);
    in.dir = 2;
    CHECK(body_input(world, &ground, &in) == BODY_OK);
    CHECK(body_now(world) == 100);
    CHECK(body_move(world, 1, &move) == BODY_OK);
    CHECK(move.at_tick == 100 && move.x == 3 && move.y == 4 && move.dir == 2 && move.speed_ms == 150);
    CHECK(move.cause == BODY_CAUSE_PLACED && move.steps == 0 && move.land_ms == 0);

    CHECK(refused(world, size, input(99, BODY_INPUT_PLACE, 1, 7, 1, 1, 0), BODY_BAD_TICK));
    CHECK(refused(world, size, input(100, 0, 1, 7, 1, 1, 0), BODY_BAD_KIND));
    CHECK(refused(world, size, input(100, BODY_INPUT_REQUEST + 1, 1, 7, 1, 1, 0), BODY_BAD_KIND));
    CHECK(refused(world, size, input(100, BODY_INPUT_ADD, 4, 9, 1, 1, 150), BODY_BAD_SLOT));
    CHECK(refused(world, size, input(100, BODY_INPUT_ADD, 1, 9, 1, 1, 150), BODY_BAD_SLOT));
    CHECK(refused(world, size, input(100, BODY_INPUT_ADD, 2, 9, MAP_WIDTH, 1, 150), BODY_BAD_CELL));
    CHECK(refused(world, size, input(100, BODY_INPUT_ADD, 2, 9, 1, MAP_HEIGHT, 150), BODY_BAD_CELL));
    CHECK(refused(world, size, input(100, BODY_INPUT_ADD, 2, 9, -1, 1, 150), BODY_BAD_CELL));
    CHECK(refused(world, size, input(100, BODY_INPUT_ADD, 2, 9, 1, -1, 150), BODY_BAD_CELL));
    CHECK(refused(world, size, input(100, BODY_INPUT_ADD, 2, 9, 1, 1, BODY_MAX_SPEED_MS + 1), BODY_BAD_VALUE));
    in = input(100, BODY_INPUT_ADD, 2, 9, 1, 1, 150);
    in.dir = 8;
    CHECK(refused(world, size, in, BODY_BAD_VALUE));
    for (in.kind = BODY_INPUT_REMOVE; in.kind <= BODY_INPUT_ANSWER; in.kind++) {
        /* a free slot, a slot that does not exist, another body's id */
        CHECK(refused(world, size, input(100, in.kind, 2, 7, 1, 1, 0), BODY_BAD_SLOT));
        CHECK(refused(world, size, input(100, in.kind, 4, 7, 1, 1, 0), BODY_BAD_SLOT));
        CHECK(refused(world, size, input(100, in.kind, 1, 8, 1, 1, 0), BODY_BAD_SLOT));
    }
    CHECK(refused(world, size, input(100, BODY_INPUT_PLACE, 1, 7, MAP_WIDTH, 1, 0), BODY_BAD_CELL));
    in = input(100, BODY_INPUT_PLACE, 1, 7, 1, 1, 0);
    in.dir = 8;
    CHECK(refused(world, size, in, BODY_BAD_VALUE));
    in.dir = 0;
    in.cause = 3;
    CHECK(refused(world, size, in, BODY_BAD_VALUE));

    /* it is pushed to the map's far corner: a new state from that tick on, at its old speed */
    in = input(250, BODY_INPUT_PLACE, 1, 7, 9, 7, 999);
    in.dir = 5;
    in.cause = BODY_CAUSE_PUSHED;
    CHECK(body_input(world, &ground, &in) == BODY_OK);
    CHECK(body_now(world) == 250);
    CHECK(body_move(world, 1, &move) == BODY_OK);
    CHECK(move.at_tick == 250 && move.x == 9 && move.y == 7 && move.dir == 5 && move.speed_ms == 150);
    CHECK(move.cause == BODY_CAUSE_PUSHED);

    /* the ring holds eight events; the ninth input that makes one is refused until one is taken */
    CHECK(apply(world, input(250, BODY_INPUT_ADD, 0, 5, 0, 0, 150)) == BODY_OK);
    CHECK(apply(world, input(260, BODY_INPUT_ADD, 2, 6, 1, 1, 150)) == BODY_OK);
    for (i = 0; i < 4; i++)
        CHECK(apply(world, input(260, BODY_INPUT_PLACE, 0, 5, 0, 0, 0)) == BODY_OK);
    CHECK(refused(world, size, input(270, BODY_INPUT_ADD, 3, 8, 2, 2, 150), BODY_EVENTS_FULL));
    CHECK(refused(world, size, input(270, BODY_INPUT_PLACE, 1, 7, 2, 2, 0), BODY_EVENTS_FULL));
    CHECK(refused(world, size, input(270, BODY_INPUT_WALK, 1, 7, 8, 7, 0), BODY_EVENTS_FULL));
    CHECK(refused(world, size, input(270, BODY_INPUT_SPEED, 1, 7, 0, 0, 300), BODY_EVENTS_FULL));
    CHECK(body_take_event(world, &event) == 1);
    CHECK(event.tick == 100 && event.id == 7 && event.kind == BODY_EVENT_MOVE && event.slot == 1);
    CHECK(event.x == 3 && event.y == 4 && event.dir == 2);
    CHECK(apply(world, input(270, BODY_INPUT_ADD, 3, 8, 2, 2, 150)) == BODY_OK);
    CHECK(body_world_check(arena, size) == BODY_OK);

    /* a body leaves without an event, also while the ring is full; its slot can be used again */
    CHECK(apply(world, input(300, BODY_INPUT_REMOVE, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(body_now(world) == 300);
    CHECK(body_move(world, 1, &move) == BODY_BAD_SLOT);
    CHECK(body_world_check(arena, size) == BODY_OK);

    /* the events come out oldest first, and then there are none */
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 250, 9, 7));
    CHECK(event_is(world, BODY_EVENT_MOVE, 0, 250, 0, 0));
    CHECK(event_is(world, BODY_EVENT_MOVE, 2, 260, 1, 1));
    for (i = 0; i < 4; i++)
        CHECK(event_is(world, BODY_EVENT_MOVE, 0, 260, 0, 0));
    CHECK(event_is(world, BODY_EVENT_MOVE, 3, 270, 2, 2));
    CHECK(no_event(world));
    CHECK(body_world_check(arena, size) == BODY_OK);

    CHECK(apply(world, input(300, BODY_INPUT_ADD, 1, 70, 5, 5, 150)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 300 && move.x == 5);
    CHECK(body_world_check(arena, size) == BODY_OK);
    free(arena);
}

/* A walk: one record at its start, a cell event for every step, each step on its own tick. */
static void test_walk(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove move;
    uint64_t due;

    CHECK(body_next_due(world, &due) == 0);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 5, 2, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK);
    CHECK(move.at_tick == 1000 && move.x == 2 && move.y == 2 && move.cause == BODY_CAUSE_WALKED);
    CHECK(move.steps == 3 && move.path[0] == EAST && move.path[1] == EAST && move.path[2] == EAST && move.path[3] == 0);
    CHECK(move.land_ms == 100);
    CHECK(body_next_due(world, &due) == 1 && due == 1100);

    /* nothing passes the step that is due */
    CHECK(refused(world, size, input(1100, BODY_INPUT_STOP, 1, 7, 0, 0, 0), BODY_DUE_FIRST));
    CHECK(refused(world, size, input(1500, BODY_INPUT_ADD, 0, 5, 0, 0, 150), BODY_DUE_FIRST));
    CHECK(body_run(world, &ground, 1101) == BODY_DUE_FIRST && body_now(world) == 1000);
    /* a tick before it only moves the clock */
    CHECK(body_run(world, &ground, 1050) == BODY_OK && body_now(world) == 1050 && no_event(world));

    CHECK(body_run(world, &ground, 1100) == BODY_OK && body_now(world) == 1100);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK);
    CHECK(move.at_tick == 1100 && move.x == 3 && move.dir == EAST && move.steps == 2 && move.land_ms == 100);
    CHECK(body_run(world, &ground, 1200) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1200, 4, 2));
    CHECK(body_next_due(world, &due) == 1 && due == 1300);
    CHECK(body_run(world, &ground, 1300) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1300, 5, 2));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.x == 5 && move.steps == 0 && move.land_ms == 0);
    CHECK(body_next_due(world, &due) == 0 && no_event(world));
    CHECK(body_world_check(world, size) == BODY_OK);

    /* a diagonal step takes 14/10 of the time */
    CHECK(apply(world, input(1300, BODY_INPUT_WALK, 1, 7, 7, 4, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 2 && move.path[0] == NORTH_EAST && move.land_ms == 140);
    CHECK(body_run(world, &ground, 1440) == BODY_OK && body_run(world, &ground, 1580) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.x == 7 && move.y == 4 && move.steps == 0);

    /* a walk to the cell it stands on is a record without steps */
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1300, 5, 2) && event_is(world, BODY_EVENT_CELL, 1, 1440, 6, 3));
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1580, 7, 4) && no_event(world));
    CHECK(apply(world, input(1600, BODY_INPUT_WALK, 1, 7, 7, 4, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1600, 7, 4));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1600 && move.steps == 0);

    /* no way, or none short enough */
    close_cell(9, 7);
    CHECK(refused(world, size, input(1600, BODY_INPUT_WALK, 1, 7, 9, 7, 0), BODY_NO_PATH));
    CHECK(refused(world, size, input(1600, BODY_INPUT_WALK, 1, 7, MAP_WIDTH, 4, 0), BODY_BAD_CELL));
    CHECK(refused(world, size, input(1600, BODY_INPUT_WALK, 1, 7, 2, 4, 4), BODY_NO_PATH));
    CHECK(apply(world, input(1600, BODY_INPUT_WALK, 1, 7, 2, 4, 5)) == BODY_OK);
    free(world);
}

/* A new target while a step is under way is taken when that step lands. */
static void test_turn(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove move, before;

    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 5, 2, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2));
    CHECK(body_move(world, 1, &before) == BODY_OK);
    CHECK(apply(world, input(1050, BODY_INPUT_WALK, 1, 7, 2, 5, 0)) == BODY_OK);
    CHECK(body_now(world) == 1050 && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && memcmp(&move, &before, sizeof move) == 0);

    /* it enters the cell it was going to, and its record from there is the way to the new target */
    CHECK(body_run(world, &ground, 1100) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2) && event_is(world, BODY_EVENT_MOVE, 1, 1100, 3, 2));
    CHECK(no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1100 && move.x == 3 && move.y == 2 && move.steps == 3);
    body_move_at(&move, 5000, &move);
    CHECK(move.x == 2 && move.y == 5 && move.steps == 0);

    /* a request at the very tick a step landed starts from that cell at once */
    CHECK(body_move(world, 1, &move) == BODY_OK);
    CHECK(body_run(world, &ground, move.at_tick + move.land_ms) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1 && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 2);
    CHECK(apply(world, input(body_now(world), BODY_INPUT_WALK, 1, 7, 8, 2, 0)) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(body_move(world, 1, &before) == BODY_OK && before.at_tick == body_now(world) && before.x == move.x);
    body_move_at(&before, 9000, &move);
    CHECK(move.x == 8 && move.y == 2);

    /* the target that waits has no way from the cell the step lands on: the body stands there */
    free(world);
    world = walker_world(8, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 5, 2, 0)) == BODY_OK);
    CHECK(apply(world, input(1050, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    close_cell(2, 6);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(body_run(world, &ground, 1100) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2) && event_is(world, BODY_EVENT_MOVE, 1, 1100, 3, 2));
    CHECK(event_is(world, BODY_EVENT_NO_PATH, 1, 1100, 3, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.x == 3 && move.steps == 0 && move.at_tick == 1100);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);
}

/* A cell that closed since the path was found is not entered. */
static void test_closed_cell(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove move;
    int x;

    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 5, 2, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2));
    close_cell(3, 2);
    CHECK(body_run(world, &ground, 1100) == BODY_OK);
    /* no cell event: it is still on (2, 2), with a way round from this tick */
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1100, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1100 && move.x == 2 && move.steps > 3);
    body_move_at(&move, 9000, &move);
    CHECK(move.x == 5 && move.y == 2);

    /* walled in, it stands */
    for (x = 0; x < MAP_WIDTH; x++) {
        close_cell(x, 1);
        close_cell(x, 3);
    }
    close_cell(1, 2);
    CHECK(body_move(world, 1, &move) == BODY_OK);
    CHECK(body_run(world, &ground, move.at_tick + move.land_ms) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, move.at_tick + move.land_ms, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.x == 2 && move.y == 2 && move.steps == 0);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);
}

/* A walk after a body that stands on a cell no walk ends on (an Ice Wall's): its way heads for
 * the cell beside it towards the walker, or else the first open one around it, row by row from
 * the corner (unit_can_reach_bl); walled in all round, there is no way. */
static void test_chase_closed_cell(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove move;

    /* body 7 on (2, 2), body 9 in slot 3 on (6, 2), whose cell is closed */
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 3, 9, 6, 2, 100)) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    close_cell(6, 2);
    CHECK(apply(world, chase(1000, 1, 7, 3, 9, 1)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 3 && move.path[0] == EAST && move.path[2] == EAST);
    body_move_at(&move, 9000, &move);
    CHECK(move.x == 5 && move.y == 2);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);

    /* the cell towards the walker closed too: the first open one around, (5, 1) */
    world = walker_world(8, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 3, 9, 6, 2, 100)) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    close_cell(6, 2);
    close_cell(5, 2);
    CHECK(apply(world, chase(1000, 1, 7, 3, 9, 1)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 3);
    body_move_at(&move, 9000, &move);
    CHECK(move.x == 5 && move.y == 1);
    free(world);

    /* walled in all round: no way, and the body stands */
    world = walker_world(8, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 3, 9, 6, 2, 100)) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    close_cell(5, 1); close_cell(6, 1); close_cell(7, 1);
    close_cell(5, 2); close_cell(6, 2); close_cell(7, 2);
    close_cell(5, 3); close_cell(6, 3); close_cell(7, 3);
    CHECK(refused(world, size, chase(1000, 1, 7, 3, 9, 1), BODY_NO_PATH));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.x == 2 && move.y == 2 && move.steps == 0);
    free(world);
}

/* A walk to within a range of a cell, and a walk after another body. */
static void test_near_and_chase(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove move;
    struct BodyInput in;
    uint64_t due;

    /* to within two cells of (8, 2): the way ends on the first cell that is */
    CHECK(apply(world, input(1000, BODY_INPUT_NEAR, 1, 7, 8, 2, 2)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 4 && move.path[3] == EAST && move.path[4] == 0);
    CHECK(body_run(world, &ground, 1100) == BODY_OK && body_run(world, &ground, 1200) == BODY_OK);
    CHECK(body_run(world, &ground, 1300) == BODY_OK && body_run(world, &ground, 1400) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.x == 6 && move.y == 2 && move.steps == 0);
    CHECK(body_next_due(world, &due) == 0 && body_world_check(world, size) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    /* there already: nothing happens */
    CHECK(apply(world, input(1400, BODY_INPUT_NEAR, 1, 7, 8, 2, 2)) == BODY_OK && no_event(world));
    CHECK(apply(world, input(1400, BODY_INPUT_NEAR, 1, 7, 6, 4, 5)) == BODY_OK && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1400 && move.steps == 0);
    /* a range of 0 is the cell itself */
    CHECK(apply(world, input(1400, BODY_INPUT_NEAR, 1, 7, 7, 2, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1400, 6, 2));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 1);
    CHECK(refused(world, size, input(1400, BODY_INPUT_NEAR, 1, 7, 7, 2, BODY_MAX_RANGE + 1), BODY_BAD_VALUE));
    CHECK(refused(world, size, input(1400, BODY_INPUT_NEAR, 1, 7, MAP_WIDTH, 2, 1), BODY_BAD_CELL));
    free(world);

    /* a chase: body 7 on (2, 2), body 9 in slot 3 on (8, 2) */
    world = walker_world(8, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 3, 9, 8, 2, 100)) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(refused(world, size, chase(1000, 1, 7, 2, 9, 1), BODY_BAD_SLOT));  /* nobody in slot 2 */
    CHECK(refused(world, size, chase(1000, 1, 7, 3, 8, 1), BODY_BAD_SLOT));  /* slot 3 is another's */
    CHECK(refused(world, size, chase(1000, 1, 7, 4, 9, 1), BODY_BAD_SLOT));  /* no such slot */
    CHECK(refused(world, size, chase(1000, 1, 7, 1, 7, 1), BODY_BAD_SLOT));  /* itself */
    CHECK(refused(world, size, chase(1000, 1, 7, 3, 9, 0), BODY_BAD_VALUE)); /* onto its cell */
    CHECK(refused(world, size, chase(1000, 1, 7, 3, 9, BODY_MAX_RANGE + 1), BODY_BAD_VALUE));
    CHECK(apply(world, chase(1000, 1, 7, 3, 9, 1)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 5);
    /* the other stays: the walk goes on as its record says */
    CHECK(body_run(world, &ground, 1100) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2) && no_event(world));
    /* the other moves away: on its next cell the body finds its way anew, to beside the new place */
    in = input(1150, BODY_INPUT_PLACE, 3, 9, 4, 6, 0);
    CHECK(body_input(world, &ground, &in) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 3, 1150, 4, 6));
    CHECK(body_run(world, &ground, 1200) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1200, 4, 2) && event_is(world, BODY_EVENT_MOVE, 1, 1200, 4, 2));
    CHECK(no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1200 && move.steps == 3 && move.path[0] == NORTH);
    body_move_at(&move, 9000, &move);
    CHECK(move.x == 4 && move.y == 5);
    /* the other comes to meet it: within the range, it stands where it is, short of its record */
    in = input(1250, BODY_INPUT_PLACE, 3, 9, 5, 4, 0);
    CHECK(body_input(world, &ground, &in) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(body_run(world, &ground, 1300) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1300, 4, 3) && event_is(world, BODY_EVENT_MOVE, 1, 1300, 4, 3));
    CHECK(no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && move.x == 4 && move.y == 3);
    CHECK(body_next_due(world, &due) == 0 && body_world_check(world, size) == BODY_OK);
    /* within the range already: nothing happens */
    CHECK(apply(world, chase(1300, 1, 7, 3, 9, 1)) == BODY_OK && no_event(world));
    CHECK(apply(world, chase(1300, 1, 7, 3, 9, 3)) == BODY_OK && no_event(world));

    /* the other goes away and is gone: the body stands on the next cell it comes onto, and
       the caller hears of it */
    in = input(1300, BODY_INPUT_PLACE, 3, 9, 9, 7, 0);
    CHECK(body_input(world, &ground, &in) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, chase(1300, 1, 7, 3, 9, 1)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1300, 4, 3));
    CHECK(apply(world, input(1350, BODY_INPUT_REMOVE, 3, 9, 0, 0, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps > 1);
    due = move.at_tick + move.land_ms;
    CHECK(body_run(world, &ground, due) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1); /* the cell it came onto */
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && move.at_tick == due);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, due, move.x, move.y) && event_is(world, BODY_EVENT_NO_PATH, 1, due, move.x, move.y));
    CHECK(no_event(world) && body_world_check(world, size) == BODY_OK);

    /* another body in that slot is not the one it was after */
    CHECK(apply(world, input(due, BODY_INPUT_ADD, 3, 9, 9, 7, 100)) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, chase(due, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, input(due, BODY_INPUT_REMOVE, 3, 9, 0, 0, 0)) == BODY_OK);
    CHECK(apply(world, input(due, BODY_INPUT_ADD, 3, 10, 9, 7, 100)) == BODY_OK);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(body_move(world, 1, &move) == BODY_OK);
    CHECK(body_run(world, &ground, move.at_tick + move.land_ms) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0);

    /* asked for while a step is under way, a chase waits for that step like any walk */
    free(world);
    world = walker_world(8, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 3, 9, 2, 6, 100)) == BODY_OK);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 6, 2, 0)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, chase(1050, 1, 7, 3, 9, 2)) == BODY_OK && no_event(world));
    CHECK(body_world_check(world, size) == BODY_OK);
    CHECK(body_run(world, &ground, 1100) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2) && event_is(world, BODY_EVENT_MOVE, 1, 1100, 3, 2));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 2);
    body_move_at(&move, 9000, &move);
    CHECK(move.y == 4);
    /* no way to the other: refused */
    close_cell(2, 5);
    close_cell(1, 5);
    close_cell(3, 5);
    close_cell(1, 6);
    close_cell(3, 6);
    close_cell(1, 7);
    close_cell(2, 7);
    close_cell(3, 7);
    CHECK(refused(world, size, chase(1100, 1, 7, 3, 9, 1), BODY_NO_PATH));
    free(world);
}

/* How body `id` in `slot` fights: a fighter of `kind` (BODY_FIGHTER_*), whose swing holds its
 * walk or not. */
static struct BodyInput fighter_of(uint64_t tick, int slot, uint32_t id, int adelay, int amotion, int range, int windup,
                                   int kind, int holds)
{
    struct BodyInput in = input(tick, BODY_INPUT_FIGHTER, slot, id, amotion, range, adelay);

    in.target = (uint16_t)windup;
    in.dir = (uint8_t)kind;
    in.cause = (uint8_t)holds;
    return in;
}

/* And one whose swing holds its walk, as a player's does. */
static struct BodyInput fighter(uint64_t tick, int slot, uint32_t id, int adelay, int amotion, int range, int windup,
                                int kind)
{
    return fighter_of(tick, slot, id, adelay, amotion, range, windup, kind, 1);
}

static struct BodyInput attack(uint64_t tick, int slot, uint32_t id, int target, uint32_t target_id, int again)
{
    struct BodyInput in = input(tick, BODY_INPUT_ATTACK, slot, id, 0, 0, again);

    in.target = (uint16_t)target;
    in.target_id = target_id;
    return in;
}

static struct BodyInput answer(uint64_t tick, int slot, uint32_t id, unsigned bits)
{
    return input(tick, BODY_INPUT_ANSWER, slot, id, 0, 0, (int)bits);
}

static struct BodyInput act_clock(uint64_t tick, int slot, uint32_t id, uint32_t ms)
{
    struct BodyInput in = input(tick, BODY_INPUT_ACT_CLOCK, slot, id, 0, 0, 0);

    in.target_id = ms;
    return in;
}

static struct BodyInput attack_clock(uint64_t tick, int slot, uint32_t id, uint32_t ms, int look)
{
    struct BodyInput in = input(tick, BODY_INPUT_ATTACK_CLOCK, slot, id, 0, 0, look);

    in.target_id = ms;
    return in;
}

static struct BodyInput cast(uint64_t tick, int slot, uint32_t id, uint32_t ms, int floor_ms)
{
    struct BodyInput in = input(tick, BODY_INPUT_CAST, slot, id, 0, 0, floor_ms);

    in.target_id = ms;
    return in;
}

/* An answer with ms: to a cast's end what goes under the act clock, to a look the game's own clock. */
static struct BodyInput answer_ms(uint64_t tick, int slot, uint32_t id, unsigned bits, uint32_t ms)
{
    struct BodyInput in = answer(tick, slot, id, bits);

    in.target_id = ms;
    return in;
}

static struct BodyInput act_clock_later(uint64_t tick, int slot, uint32_t id, uint32_t ms)
{
    struct BodyInput in = act_clock(tick, slot, id, ms);

    in.value = 1;
    return in;
}

/* The next waiting event is of `kind`, of the body in `slot`, at `tick`, about the body in
 * `other` (-1: about none), with `value`. */
static int event_about(struct BodyWorld *world, int kind, int slot, uint64_t tick, int other, int value)
{
    struct BodyEvent event;

    return body_take_event(world, &event) && event.kind == kind && event.slot == slot && event.tick == tick
           && event.value == value && (other < 0 ? event.other == 0 && event.other_id == 0 : event.other == other);
}

enum { YES = BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_LINE | BODY_ANSWER_READY };

/* A world with the player 7 in slot 1 on (2, 2), who swings every 1000 ms with an attack
 * motion of 500 and a range of 1, and the body 9 in slot 3 on (x, y). */
static struct BodyWorld *fight_world(int windup, int x, int y, size_t *size)
{
    struct BodyWorld *world;
    struct BodyEvent event;

    open_map();
    *size = body_world_size(4, 16);
    world = body_world_init(memory(*size), *size, 4, 16, MAP_WIDTH, MAP_HEIGHT, 1000);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 1, 7, 2, 2, 100)) == BODY_OK);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 3, 9, x, y, 100)) == BODY_OK);
    CHECK(apply(world, fighter(1000, 1, 7, 1000, 500, 1, windup, 1)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    return world;
}

/* Takes the world's events up to the first of `kind` at `tick`; 0 when none comes. */
static int taken_until(struct BodyWorld *world, int kind, uint64_t tick)
{
    struct BodyEvent event;

    while (body_take_event(world, &event))
        if (event.kind == kind && event.tick == tick)
            return 1;
    return 0;
}

/* The fork's swing: it lands as it starts. */
static void test_attack(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 3, 2, &size);
    struct BodyMove move;
    uint64_t due;

    /* asked once: the core asks the game, and nothing later runs before the answer */
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ASK_SWING, 1, 1000, 3, 0) && no_event(world));
    CHECK(body_asked(world) == 1 && body_world_check(world, size) == BODY_OK);
    CHECK(body_run(world, &ground, 1001) == BODY_ANSWER_FIRST && body_now(world) == 1000);
    CHECK(refused(world, size, input(1001, BODY_INPUT_PLACE, 3, 9, 3, 3, 0), BODY_ANSWER_FIRST));
    CHECK(refused(world, size, attack(1000, 1, 7, 3, 9, 0), BODY_ANSWER_FIRST));
    CHECK(body_run(world, &ground, 1000) == BODY_ANSWER_FIRST);
    /* the answer: the place is told again, and the hit is made now */
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1000, -1, 0) && event_about(world, BODY_EVENT_ASK_HIT, 1, 1000, 3, 0));
    CHECK(no_event(world) && body_asked(world) == 1);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ATTACKING, 1, 1000, 3, 0) && no_event(world));
    CHECK(body_asked(world) == 0 && body_next_due(world, &due) == 0);
    CHECK(refused(world, size, answer(1000, 1, 7, YES), BODY_BAD_VALUE)); /* nothing was asked */
    /* it does not walk during its attack motion */
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 2, 4, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.land_ms == 500 + 100);
    CHECK(apply(world, input(1000, BODY_INPUT_STOP, 1, 7, 0, 0, 0)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }

    /* asked again inside the attack delay, it waits for the delay, and is not told its place */
    CHECK(apply(world, attack(1100, 1, 7, 3, 9, 0)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 2000);
    CHECK(body_run(world, &ground, 2000) == BODY_ANSWER_FIRST);
    CHECK(event_about(world, BODY_EVENT_ASK_SWING, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_HIT, 1, 2000, 3, 0));
    /* no hit was made (the ammunition is gone): nothing was swung, and the attack delay does not run */
    CHECK(apply(world, answer(2000, 1, 7, 0)) == BODY_OK && no_event(world));
    CHECK(apply(world, attack(2000, 1, 7, 3, 9, 0)) == BODY_OK && event_about(world, BODY_EVENT_ASK_SWING, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, 0)) == BODY_OK && no_event(world)); /* may not fight: nothing */

    /* again and again: it turns to its target and looks again each attack delay */
    CHECK(apply(world, attack(2000, 1, 7, 3, 9, 1)) == BODY_OK && event_about(world, BODY_EVENT_ASK_SWING, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 2000, -1, 0) && event_about(world, BODY_EVENT_ASK_HIT, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_FACE, 1, 2000, 2, 2) && event_about(world, BODY_EVENT_ATTACKING, 1, 2000, 3, 1));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.dir == EAST);
    CHECK(body_next_due(world, &due) == 1 && due == 3000);
    CHECK(body_run(world, &ground, 3000) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 3000, 3, 0));
    /* the timer's look does not tell the place again, nor turn a body that faces its target */
    CHECK(apply(world, answer(3000, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_HIT, 1, 3000, 3, 0));
    CHECK(apply(world, answer(3000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ATTACKING, 1, 3000, 3, 1) && no_event(world));

    /* the act clock holds no swing (the fork's skill_delay_attack_enable): with the clock at 4500,
       the look at 4000 swings */
    CHECK(apply(world, act_clock(3000, 1, 7, 1500)) == BODY_OK);
    CHECK(body_run(world, &ground, 4000) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 4000, 3, 0));
    CHECK(apply(world, answer(4000, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_HIT, 1, 4000, 3, 0));
    CHECK(apply(world, answer(4000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ATTACKING, 1, 4000, 3, 1) && no_event(world));
    CHECK(apply(world, input(4000, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 0)) == BODY_OK && body_next_due(world, &due) == 0);

    /* a target that may not be fought, or a body that casts: the timer's attack ends */
    CHECK(apply(world, attack(5000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(5000, 1, 7, YES)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(5000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_run(world, &ground, 6000) == BODY_ANSWER_FIRST && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(6000, 1, 7, YES | BODY_ANSWER_CASTING)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_UNATTACKABLE, 1, 6000, -1, 0) && no_event(world));
    CHECK(body_next_due(world, &due) == 0 && body_world_check(world, size) == BODY_OK);

    /* whom it may attack: a body, and not itself */
    CHECK(refused(world, size, attack(6000, 1, 7, 2, 9, 0), BODY_BAD_SLOT));
    CHECK(refused(world, size, attack(6000, 1, 7, 3, 8, 0), BODY_BAD_SLOT));
    CHECK(refused(world, size, attack(6000, 1, 7, 1, 7, 0), BODY_BAD_SLOT));
    CHECK(refused(world, size, attack(6000, 1, 7, 3, 9, 2), BODY_BAD_VALUE));
    CHECK(refused(world, size, fighter(6000, 1, 7, 1000, 500, BODY_MAX_RANGE + 1, 0, 1), BODY_BAD_VALUE));
    CHECK(refused(world, size, fighter(6000, 1, 7, 1000, -1, 1, 0, 1), BODY_BAD_VALUE));
    CHECK(refused(world, size, fighter(6000, 1, 7, 1000, 500, 1, 0, BODY_FIGHTER_MONSTER + 1), BODY_BAD_VALUE));
    CHECK(refused(world, size, fighter_of(6000, 1, 7, 1000, 500, 1, 0, BODY_FIGHTER_PLAYER, 4), BODY_BAD_VALUE));
    CHECK(refused(world, size, fighter(6000, 1, 7, 0, 500, 1, 0, 1), BODY_BAD_VALUE));
    /* a body that was not told how it fights does not */
    CHECK(refused(world, size, attack(6000, 3, 9, 1, 7, 0), BODY_BAD_VALUE));
    free(world);
}

/* Out of reach, and without a line of fire. */
static void test_attack_reach(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 5, 2, &size);
    struct BodyMove move;
    struct BodyInput in;

    /* a player whose target is beyond its reach is told so */
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FAR, 1, 1000, 3, 0) && no_event(world));
    /* a target that walks is reached a cell further */
    in = input(1000, BODY_INPUT_PLACE, 3, 9, 4, 2, 0);
    CHECK(body_input(world, &ground, &in) == BODY_OK);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 3, 9, 8, 2, 0)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1000, -1, 0) && event_about(world, BODY_EVENT_ASK_HIT, 1, 1000, 3, 0));
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    free(world);

    /* within the range but no line of fire: an attack that goes on walks after its target */
    world = fight_world(0, 5, 2, &size);
    CHECK(apply(world, fighter(1000, 1, 7, 1000, 500, 4, 0, 1)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_READY)) == BODY_OK && no_event(world));
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_READY)) == BODY_OK);
    /* its chase range of 4 is cut to 2: it walks to within two cells */
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 1 && move.path[0] == EAST);
    /* beside its target no line is asked for */
    in = input(1000, BODY_INPUT_PLACE, 3, 9, 3, 3, 0);
    CHECK(body_input(world, &ground, &in) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_READY)) == BODY_OK);
    /* it stops walking to swing */
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1000, -1, 0) && event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2));
    CHECK(event_about(world, BODY_EVENT_ASK_HIT, 1, 1000, 3, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);

    /* within the cut range already, and no line of fire: it takes one step on the way to the cell
       beside its target (unit_walktoxy_sub moves at least one cell), and looks when it lands */
    world = fight_world(0, 4, 2, &size);
    CHECK(apply(world, fighter(1000, 1, 7, 1000, 500, 4, 0, 1)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_READY)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 1 && move.path[0] == EAST);
    CHECK(body_run(world, &ground, 1100) == BODY_ANSWER_FIRST && taken_until(world, BODY_EVENT_ASK_SWING, 1100));
    /* beside it now, the look swings */
    CHECK(apply(world, answer(1100, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_READY)) == BODY_OK);
    CHECK(taken_until(world, BODY_EVENT_ASK_HIT, 1100));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.x == 3 && move.steps == 0);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);
}

/* A body that is to swing in the middle of a step: the step lands first, the walk ends on that
 * cell, and the swing is looked at again there. */
static void test_attack_lands_step(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 3, 2, &size);
    struct BodyMove move;
    struct BodyAttack state;
    uint64_t due;

    /* the player walks north past its target, a step every 100 ms, and attacks 60 ms into the first */
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2));
    CHECK(body_run(world, &ground, 1060) == BODY_OK && no_event(world));
    CHECK(apply(world, attack(1060, 1, 7, 3, 9, 0)) == BODY_OK && event_about(world, BODY_EVENT_ASK_SWING, 1, 1060, 3, 0));
    CHECK(apply(world, answer(1060, 1, 7, YES)) == BODY_OK);
    /* no place told again, no hit yet: its record is the one step, from now on */
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1060, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 1 && move.path[0] == NORTH && move.path[1] == 0);
    CHECK(move.at_tick == 1060 && move.land_ms == 40 && move.x == 2 && move.y == 2);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.has_target && state.armed && !state.swing_open);
    CHECK(body_next_due(world, &due) == 1 && due == 1100 && body_world_check(world, size) == BODY_OK);
    /* the step lands, and the swing is looked at on that cell */
    CHECK(body_run(world, &ground, 1100) == BODY_ANSWER_FIRST && event_is(world, BODY_EVENT_CELL, 1, 1100, 2, 3));
    CHECK(event_about(world, BODY_EVENT_ASK_SWING, 1, 1100, 3, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && move.x == 2 && move.y == 3);
    CHECK(apply(world, answer(1100, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_HIT, 1, 1100, 3, 0));
    CHECK(apply(world, answer(1100, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ATTACKING, 1, 1100, 3, 0) && no_event(world));
    free(world);

    /* asked again in the same step, nothing more is told; a turn that waited is off */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, input(1030, BODY_INPUT_WALK, 1, 7, 0, 3, 0)) == BODY_OK && no_event(world));
    CHECK(apply(world, attack(1050, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1050, 1, 7, YES)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1050, 2, 2) && no_event(world));
    CHECK(apply(world, attack(1070, 1, 7, 3, 9, 1)) == BODY_OK && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 1 && move.at_tick == 1050 && move.land_ms == 50);
    CHECK(body_run(world, &ground, 1100) == BODY_ANSWER_FIRST && event_is(world, BODY_EVENT_CELL, 1, 1100, 2, 3));
    CHECK(event_about(world, BODY_EVENT_ASK_SWING, 1, 1100, 3, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && move.x == 2 && move.y == 3);
    CHECK(apply(world, answer(1100, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_HIT, 1, 1100, 3, 0));
    CHECK(apply(world, answer(1100, 1, 7, BODY_ANSWER_HIT)) == BODY_OK && body_world_check(world, size) == BODY_OK);
    free(world);

    /* on the tick a step lands the body is on its cell: it stands at once, as before */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(body_run(world, &ground, 1100) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1100, 2, 3) && no_event(world));
    CHECK(apply(world, attack(1100, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1100, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1100, -1, 0) && event_is(world, BODY_EVENT_MOVE, 1, 1100, 2, 3));
    CHECK(event_about(world, BODY_EVENT_ASK_HIT, 1, 1100, 3, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0);
    CHECK(apply(world, answer(1100, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    free(world);

    /* a first step that a hit holds back has not been taken: the body stands at once */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_DELAY, 1, 7, 0, 0, 300)) == BODY_OK);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_run(world, &ground, 1250) == BODY_OK && no_event(world));
    CHECK(apply(world, attack(1250, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1250, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1250, -1, 0) && event_is(world, BODY_EVENT_MOVE, 1, 1250, 2, 2));
    CHECK(event_about(world, BODY_EVENT_ASK_HIT, 1, 1250, 3, 0) && no_event(world));
    CHECK(apply(world, answer(1250, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    free(world);

    /* an attack that waits for its delay stops no walk */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_run(world, &ground, 1500) == BODY_OK);
    CHECK(apply(world, input(1500, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(body_run(world, &ground, 1560) == BODY_OK);
    CHECK(apply(world, attack(1560, 1, 7, 3, 9, 1)) == BODY_OK && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 4);
    free(world);
}

/* The project's windup: a swing begins, and lands later if its target is still in reach. */
static void test_swing(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(300, 3, 2, &size);
    uint64_t due;

    body_set_cancel_penalty(world, 200);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && event_about(world, BODY_EVENT_ASK_SWING, 1, 1000, 3, 0));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1000, -1, 0) && event_about(world, BODY_EVENT_SWING, 1, 1000, 3, 300));
    CHECK(event_is(world, BODY_EVENT_FACE, 1, 1000, 2, 2) && event_about(world, BODY_EVENT_ATTACKING, 1, 1000, 3, 1));
    CHECK(no_event(world) && body_asked(world) == 0 && body_world_check(world, size) == BODY_OK);
    CHECK(body_next_due(world, &due) == 1 && due == 1300);
    /* asked again for the target of the swing under way: it is neither ended nor begun again */
    CHECK(apply(world, attack(1100, 1, 7, 3, 9, 1)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 1300);
    /* it lands: the hit is made then, and the game says whether there was one */
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST);
    CHECK(event_about(world, BODY_EVENT_ASK_HIT, 1, 1300, 3, 1) && no_event(world));
    CHECK(apply(world, answer(1300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 2000);

    /* the next swing; no hit is made at its end (the target died in the windup): it is lost,
       and the next may begin after the penalty */
    CHECK(body_run(world, &ground, 2000) == BODY_ANSWER_FIRST && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(2000, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_SWING, 1, 2000, 3, 300));
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_run(world, &ground, 2300) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_HIT, 1, 2300, 3, 1));
    CHECK(apply(world, answer(2300, 1, 7, 0)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 2300, -1, BODY_CANCEL_LOST) && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 2500);

    /* a swing whose target is gone when it lands is lost without a question */
    CHECK(body_run(world, &ground, 2500) == BODY_ANSWER_FIRST && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(2500, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_SWING, 1, 2500, 3, 300));
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, input(2600, BODY_INPUT_REMOVE, 3, 9, 0, 0, 0)) == BODY_OK);
    CHECK(body_run(world, &ground, 2800) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 2800, -1, BODY_CANCEL_LOST) && no_event(world));
    /* and the attack that went on finds no target and ends: it has none, and does not go on */
    CHECK(body_next_due(world, &due) == 1 && due == 3000);
    CHECK(((const unsigned char *)world)[HEADER_BYTES + BODY_BYTES + BODY_AT_HAS_TARGET] == 1);
    CHECK(body_run(world, &ground, 3000) == BODY_OK && event_about(world, BODY_EVENT_UNATTACKABLE, 1, 3000, -1, 0));
    CHECK(no_event(world) && body_next_due(world, &due) == 0);
    CHECK(((const unsigned char *)world)[HEADER_BYTES + BODY_BYTES + BODY_AT_HAS_TARGET] == 0);
    CHECK(((const unsigned char *)world)[HEADER_BYTES + BODY_BYTES + BODY_AT_HAS_TARGET + 1] == 0);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);

    /* a windup longer than the attack delay: asked again for the swing's target when the
       delay is over, the attack looks at once and waits for the swing to land */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, fighter(1000, 1, 7, 100, 50, 1, 300, 1)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_run(world, &ground, 1200) == BODY_OK);
    CHECK(apply(world, attack(1200, 1, 7, 3, 9, 1)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 1200 && body_world_check(world, size) == BODY_OK);
    CHECK(body_run(world, &ground, 1200) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 1300);
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_HIT, 1, 1300, 3, 1));
    CHECK(apply(world, answer(1300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    /* the attack that waited for it looks at the same tick, and swings again */
    CHECK(body_next_due(world, &due) == 1 && due == 1300);
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 1300, 3, 0));
    CHECK(apply(world, answer(1300, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_SWING, 1, 1300, 3, 300));
    free(world);

    /* a swing ended by command; a stop that keeps the swing lets it land */
    world = fight_world(300, 3, 2, &size);
    body_set_cancel_penalty(world, 200);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, input(1100, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 1100, -1, BODY_CANCEL_COMMAND) && no_event(world));
    CHECK(apply(world, input(1100, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK && no_event(world));
    CHECK(apply(world, attack(1200, 1, 7, 3, 9, 0)) == BODY_OK && no_event(world)); /* waits for the penalty */
    CHECK(body_next_due(world, &due) == 1 && due == 1300);
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1300, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_SWING, 1, 1300, 3, 300));
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, input(1400, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 1)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 1600);
    CHECK(body_run(world, &ground, 1600) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_HIT, 1, 1600, 3, 1));
    CHECK(apply(world, answer(1600, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    /* a stop that does not keep it ends it */
    CHECK(apply(world, attack(2400, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(2400, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, input(2500, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 2500, -1, BODY_CANCEL_COMMAND) && no_event(world));
    CHECK(body_next_due(world, &due) == 0);
    /* without its ammunition no swing begins */
    CHECK(apply(world, attack(3000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(3000, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_LINE)) == BODY_OK);
    CHECK(body_world_check(world, size) == BODY_OK); /* with the event that says so waiting */
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 3000, -1, 0) && event_about(world, BODY_EVENT_UNREADY, 1, 3000, 3, 0));
    CHECK(no_event(world) && body_next_due(world, &due) == 0);
    /* a body that asks and leaves takes its question with it */
    CHECK(apply(world, attack(3000, 1, 7, 3, 9, 0)) == BODY_OK && body_asked(world) == 1);
    CHECK(apply(world, input(3000, BODY_INPUT_REMOVE, 1, 7, 0, 0, 0)) == BODY_OK && body_asked(world) == 0);
    CHECK(body_run(world, &ground, 3100) == BODY_OK && body_world_check(world, size) == BODY_OK);
    free(world);
}

/* What a host with clocks and targets of its own puts in, and reads. */
/* A swing holds the body from walking for its attack motion. One that a command ends in its
   windup was not made: the body walks at once, or when what held it before lets it. */
static void test_swing_cancel_frees_walk(void)
{
    size_t size;
    struct BodyWorld *world;
    struct BodyMove move;
    struct BodyEvent event;

    /* the swing lands: a walk asked for in its attack motion (500 ms) starts when that is over */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST && apply(world, answer(1300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, input(1350, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 4 && move.at_tick == 1350 && move.land_ms == 150 + 100);
    free(world);

    /* ended by a command 100 ms into its windup: the walk starts then */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(body_world_check(world, size) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 4 && move.at_tick == 1100 && move.land_ms == 100);
    free(world);
    /* and by the attack's stop, which ends the swing with it */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1100 && move.land_ms == 100);
    free(world);

    /* A walk asked for in the windup waits for the end of the attack motion (1500). When the
       swing is then ended by a command, the walk starts at once: a new record */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1100, 2, 2));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1100 && move.land_ms == 400 + 100);
    CHECK(apply(world, input(1100, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 1100, -1, BODY_CANCEL_COMMAND));
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1100, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 4 && move.at_tick == 1100 && move.land_ms == 100);
    CHECK(body_world_check(world, size) == BODY_OK);
    CHECK(body_run(world, &ground, 1200) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1200, 2, 3));
    free(world);
    /* a later cancel, 40 ms into that wait */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    CHECK(apply(world, input(1140, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1140 && move.land_ms == 100);
    free(world);
    /* with the ring too full for both events the cancel is refused and nothing changes */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    {
        int filled = 1;

        /* the walk's own record is one event; the other body's speed changes fill the rest */
        while (filled < 16 && apply(world, input(1100, BODY_INPUT_SPEED, 3, 9, 0, 0, 100 + filled)) == BODY_OK)
            filled++;
        CHECK(filled == 16 && apply(world, input(1100, BODY_INPUT_SPEED, 3, 9, 0, 0, 90)) == BODY_EVENTS_FULL);
        CHECK(body_take_event(world, &event) == 1);
        CHECK(apply(world, input(1100, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_EVENTS_FULL);
        CHECK(body_move(world, 1, &move) == BODY_OK && move.land_ms == 400 + 100);
        CHECK(body_take_event(world, &event) == 1);
        CHECK(apply(world, input(1100, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK);
        CHECK(body_move(world, 1, &move) == BODY_OK && move.land_ms == 100);
    }
    free(world);

    /* a hold the swing found (a hit, until 1200) holds on: the walk starts when that is over */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_DELAY, 1, 7, 0, 0, 200)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(body_world_check(world, size) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1100 && move.land_ms == 100 + 100);
    free(world);
    /* so does one that came in the windup and lasts longer than the attack motion (until 1700) */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(apply(world, input(1050, BODY_INPUT_DELAY, 1, 7, 0, 0, 650)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1100 && move.land_ms == 600 + 100);
    free(world);

    /* a swing that is lost at its end (its target is gone) was begun all the same: it holds */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_REMOVE, 3, 9, 0, 0, 0)) == BODY_OK);
    CHECK(body_run(world, &ground, 1300) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, input(1300, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1300 && move.land_ms == 200 + 100);
    free(world);
}

static void test_attack_steered(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 3, 2, &size);
    struct BodyAttack state;
    uint64_t due;

    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.has_target && !state.armed && !state.swing_open);
    CHECK(state.attackable == 0 && !state.attack_continue);
    CHECK(body_attack(world, 0, &state) == BODY_BAD_SLOT && body_attack(world, 4, &state) == BODY_BAD_SLOT);
    /* a swing, with more to come */
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.has_target && state.target == 3 && state.target_id == 9);
    CHECK(state.attack_continue && state.armed && !state.swing_open && state.attackable == 2000);
    /* the next swing put later: the attack that waits looks when it was to, and waits on */
    CHECK(apply(world, attack_clock(1100, 1, 7, 1500, 0)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 2000);
    CHECK(body_run(world, &ground, 2000) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ATTACKING, 1, 2000, 3, 1) && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 2600);
    /* put sooner, and the look with it (a combo that was cancelled) */
    CHECK(apply(world, attack_clock(2100, 1, 7, 200, 1)) == BODY_OK && body_next_due(world, &due) == 1 && due == 2300);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.attackable == 2300);
    CHECK(refused(world, size, attack_clock(2100, 1, 7, 200, 2), BODY_BAD_VALUE));
    /* once: the swing at 2300 is the last, and its target stays */
    CHECK(apply(world, input(2200, BODY_INPUT_ATTACK_ONCE, 1, 7, 0, 0, 0)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.has_target && !state.attack_continue && state.armed);
    CHECK(body_run(world, &ground, 2300) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 2300, 3, 0));
    CHECK(apply(world, answer(2300, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_HIT, 1, 2300, 3, 0));
    CHECK(apply(world, answer(2300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ATTACKING, 1, 2300, 3, 0) && no_event(world) && body_next_due(world, &due) == 0);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.has_target && !state.armed && state.attackable == 3300);
    /* a clock put without a look moves no look that does not wait */
    CHECK(apply(world, attack_clock(2400, 1, 7, 100, 1)) == BODY_OK && body_next_due(world, &due) == 0);
    CHECK(apply(world, attack_clock(2400, 1, 7, 900, 0)) == BODY_OK && body_next_due(world, &due) == 0);
    /* nor is the clock only ever put later */
    CHECK(apply(world, attack_clock(2400, 1, 7, 500, 0)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.attackable == 2900);

    /* a target that may not be attacked is forgotten; the look that waits still comes, and finds none */
    CHECK(apply(world, attack(2400, 1, 7, 3, 9, 1)) == BODY_OK && no_event(world));
    CHECK(apply(world, input(2500, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 2)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.has_target && !state.attack_continue && state.armed);
    CHECK(state.target == 0 && state.target_id == 0);
    CHECK(body_next_due(world, &due) == 1 && due == 2900);
    CHECK(refused(world, size, input(2500, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 3), BODY_BAD_VALUE));
    CHECK(body_run(world, &ground, 2900) == BODY_OK && event_about(world, BODY_EVENT_UNATTACKABLE, 1, 2900, -1, 0));
    CHECK(no_event(world) && body_next_due(world, &due) == 0 && body_world_check(world, size) == BODY_OK);
    free(world);

    /* a swing in its windup is told with whom it is at */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.swing_open && state.swing_target == 3 && state.swing_target_id == 9);
    /* and with how it fights and how long until the swing lands */
    CHECK(state.amotion == 500 && state.windup == 300 && state.swing_ms == 300 && !state.looking);
    /* the unattackable target's swing still lands */
    CHECK(apply(world, input(1100, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 2)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.has_target && state.target_id == 0);
    CHECK(state.swing_open && state.swing_target == 3 && state.swing_target_id == 9 && state.swing_ms == 200);
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_HIT, 1, 1300, 3, 1));
    CHECK(apply(world, answer(1300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.swing_open && state.swing_target == 0);
    CHECK(state.swing_ms == 0 && state.amotion == 500 && state.windup == 300);
    free(world);

    /* a target two cells away is out of a reach of one; one that walks by a walk the world
       does not know of is reached, as one whose walk it knows */
    world = fight_world(0, 4, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FAR, 1, 1000, 3, 0) && no_event(world));
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES | BODY_ANSWER_WALKS)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1000, -1, 0) && event_about(world, BODY_EVENT_ASK_HIT, 1, 1000, 3, 0));
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    free(world);
}

static int caught(const unsigned char *sample, size_t size, size_t offset, int bytes, uint32_t value);

/* A world as fight_world's with a windup of 300, the body 7 in slot 1 a monster whose swing holds
 * its walk, or does not (a boss). */
static struct BodyWorld *monster_world(int holds, int x, int y, size_t *size)
{
    struct BodyWorld *world = fight_world(300, x, y, size);
    struct BodyEvent event;

    CHECK(apply(world, fighter_of(1000, 1, 7, 1000, 500, 1, 300, BODY_FIGHTER_MONSTER, holds)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    return world;
}

/* A monster's attack: the game is asked once more right before each swing, a target beyond its
 * range is gone after by a way the game finds, and the swing lands within its range and a cell
 * more. */
static void test_monster_attack(void)
{
    size_t size;
    struct BodyWorld *world = monster_world(1, 3, 2, &size);
    struct BodyAttack state;
    struct BodyMove move;
    struct BodyEvent event;
    unsigned char *arena = (unsigned char *)world;
    size_t body = HEADER_BYTES + BODY_BYTES;
    uint64_t due;
    int holds;

    /* asked for: a look, and with the game's leave the question before the swing; no place is
       told again, as for no body but a player's */
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && event_about(world, BODY_EVENT_ASK_SWING, 1, 1000, 3, 0));
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.looking && !state.armed);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ASK_BEGIN, 1, 1000, 3, 0) && no_event(world));
    CHECK(body_asked(world) == 1 && body_world_check(world, size) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.looking && !state.swing_open);
    CHECK(body_run(world, &ground, 1001) == BODY_ANSWER_FIRST && body_now(world) == 1000);
    CHECK(refused(world, size, attack(1000, 1, 7, 3, 9, 1), BODY_ANSWER_FIRST));
    /* only a monster is asked before its swing */
    CHECK(caught(arena, size, body + BODY_AT_KIND, 1, BODY_FIGHTER_PLAYER));
    /* the swing begins: it turns to its target and looks again when the attack delay is over */
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_BEGIN)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING, 1, 1000, 3, 300));
    CHECK(event_is(world, BODY_EVENT_FACE, 1, 1000, 2, 2) && event_about(world, BODY_EVENT_ATTACKING, 1, 1000, 3, 1));
    CHECK(no_event(world) && body_asked(world) == 0);
    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.looking && state.armed && state.swing_open);
    CHECK(refused(world, size, answer(1000, 1, 7, BODY_ANSWER_BEGIN), BODY_BAD_VALUE)); /* nothing was asked */
    /* a target that stepped a cell further off in the windup is within its range and a cell more */
    CHECK(apply(world, input(1100, BODY_INPUT_PLACE, 3, 9, 4, 2, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_HIT, 1, 1300, 3, 1));
    CHECK(apply(world, answer(1300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK && no_event(world));
    /* at its next look the target is beyond its range: it is to go after it to within its chase
       range, by a way the game finds, and the attack looks no more */
    CHECK(body_run(world, &ground, 2000) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_CHASE, 1, 2000, 3, 1) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && body_next_due(world, &due) == 0);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.has_target && state.attack_continue && !state.armed);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);

    /* a swing holds a monster's walk for its attack motion, and not a boss's (attack_walk_delay) */
    for (holds = 0; holds < 2; holds++) {
        world = monster_world(holds, 3, 2, &size);
        CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
        CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_BEGIN)) == BODY_OK);
        while (body_take_event(world, &event)) {
        }
        CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
        CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1100 && move.land_ms == (holds ? 400 + 100 : 100));
        free(world);
    }
    /* a boss's swing ended by a command gives back no hold it did not put: a hit's until 1500,
       the end of its attack motion, holds on */
    world = monster_world(0, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_BEGIN)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_DELAY, 1, 7, 0, 0, 400)) == BODY_OK);
    CHECK(apply(world, input(1200, BODY_INPUT_SWING_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(apply(world, input(1200, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1200 && move.land_ms == 300 + 100);
    free(world);

    /* the game did something in the swing's place (a skill): no swing, and no look waits */
    world = monster_world(1, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(body_take_event(world, &event) && body_take_event(world, &event) && event.kind == BODY_EVENT_ASK_BEGIN);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 0 && body_attack(world, 1, &state) == BODY_OK && state.has_target && !state.swing_open);
    /* nor when the game stopped the attack while it was asked, even with leave */
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(body_take_event(world, &event) && body_take_event(world, &event) && event.kind == BODY_EVENT_ASK_BEGIN);
    CHECK(apply(world, input(1000, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 1)) == BODY_OK);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_BEGIN)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 0 && body_attack(world, 1, &state) == BODY_OK && !state.has_target && !state.swing_open);
    free(world);

    /* within its range but without a line of fire: an attack that goes on is to go after its
       target, its chase range cut by two; one for one swing does nothing more */
    world = monster_world(1, 5, 2, &size);
    CHECK(apply(world, fighter_of(1000, 1, 7, 1000, 500, 4, 300, BODY_FIGHTER_MONSTER, 1)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_READY)) == BODY_OK && no_event(world));
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_READY)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_CHASE, 1, 1000, 3, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0);
    free(world);
    /* beyond its range it is to go after its target to within the whole chase range */
    world = monster_world(1, 8, 2, &size);
    CHECK(apply(world, fighter_of(1000, 1, 7, 1000, 500, 4, 300, BODY_FIGHTER_MONSTER, 1)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_CHASE, 1, 1000, 3, 4) && no_event(world));
    free(world);
    /* a player is no monster: a target beyond its range but within what its client measures (two
       cells off on both sides, at a range of two) is taken as out of the line of fire, and the
       walk after it, which finds the body within two cells already, takes one step towards it */
    world = fight_world(0, 4, 4, &size);
    CHECK(apply(world, fighter(1000, 1, 7, 1000, 500, 2, 0, BODY_FIGHTER_PLAYER)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2));
    CHECK(no_event(world) && body_move(world, 1, &move) == BODY_OK && move.steps == 1);
    free(world);

    /* the timer's look at a target it may not fight forgets it, and says so */
    world = monster_world(1, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_BEGIN)) == BODY_OK);
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST && apply(world, answer(1300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(body_run(world, &ground, 2000) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, 0)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_UNATTACKABLE, 1, 2000, -1, 0) && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.has_target && !state.attack_continue && body_next_due(world, &due) == 0);
    free(world);

    /* a target three cells off when the swing lands is beyond its range and a cell more: the
       swing is lost (a player's would reach ten cells more than the target was off) */
    world = monster_world(1, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_BEGIN)) == BODY_OK);
    CHECK(apply(world, input(1100, BODY_INPUT_PLACE, 3, 9, 5, 2, 0)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(body_run(world, &ground, 1300) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 1300, -1, BODY_CANCEL_LOST) && no_event(world));
    free(world);

    /* a monster in the middle of a step lands it first, as every body does, and is asked on that cell */
    world = monster_world(1, 3, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 2, 6, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(body_run(world, &ground, 1060) == BODY_OK);
    CHECK(apply(world, attack(1060, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1060, 1, 7, YES)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1060, 2, 2) && no_event(world));
    CHECK(body_run(world, &ground, 1100) == BODY_ANSWER_FIRST && event_is(world, BODY_EVENT_CELL, 1, 1100, 2, 3));
    CHECK(event_about(world, BODY_EVENT_ASK_SWING, 1, 1100, 3, 0) && no_event(world));
    CHECK(apply(world, answer(1100, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_BEGIN, 1, 1100, 3, 0));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && move.x == 2 && move.y == 3);
    free(world);
}

/* Another unit (a homunculus, a mercenary): nothing a player's client is told, a target beyond
 * its range or its line of fire gone after by the core's own walk, which ends in a look. */
static void test_other_attack(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 6, 2, &size);
    struct BodyMove move;
    struct BodyEvent event;

    CHECK(apply(world, fighter_of(1000, 1, 7, 1000, 500, 2, 0, BODY_FIGHTER_OTHER, 0)) == BODY_OK);
    /* a target four cells off, for one swing: no word that it is out of reach, nothing more */
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK && no_event(world));
    /* going on: it walks after its target to within its chase range */
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 2 && body_world_check(world, size) == BODY_OK);
    /* that walk ends within the range: its place is told again, and the attack looks */
    CHECK(body_run(world, &ground, 1100) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2) && no_event(world));
    CHECK(body_run(world, &ground, 1200) == BODY_ANSWER_FIRST && event_is(world, BODY_EVENT_CELL, 1, 1200, 4, 2));
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1200, -1, 0) && event_about(world, BODY_EVENT_ASK_SWING, 1, 1200, 3, 0));
    CHECK(no_event(world));
    /* in reach: the hit is made at once, and the swing holds no walk (a mercenary's) */
    CHECK(apply(world, answer(1200, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_HIT, 1, 1200, 3, 0));
    CHECK(apply(world, answer(1200, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ATTACKING, 1, 1200, 3, 1) && no_event(world));
    CHECK(apply(world, input(1200, BODY_INPUT_WALK, 1, 7, 4, 6, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1200 && move.land_ms == 100);
    free(world);

    /* a walk after the target whose attack was stopped on the way ends in no look */
    world = fight_world(0, 6, 2, &size);
    CHECK(apply(world, fighter_of(1000, 1, 7, 1000, 500, 2, 0, BODY_FIGHTER_OTHER, 1)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, input(1050, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(body_run(world, &ground, 1100) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2));
    CHECK(body_run(world, &ground, 1200) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1200, 4, 2));
    CHECK(no_event(world) && body_asked(world) == 0);
    free(world);

    /* a player's walk after the target that ends looks the same way */
    world = fight_world(0, 5, 2, &size);
    CHECK(apply(world, fighter(1000, 1, 7, 1000, 500, 4, 0, BODY_FIGHTER_PLAYER)) == BODY_OK);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_READY)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_run(world, &ground, 1100) == BODY_ANSWER_FIRST && event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2));
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1100, -1, 0) && event_about(world, BODY_EVENT_ASK_SWING, 1, 1100, 3, 0));
    CHECK(no_event(world) && body_world_check(world, size) == BODY_OK);
    free(world);
}

/* A cast: the act clock at its end or at the floor, the question when it is over, its cancel;
 * and the act clock as a host with skills steers it. */
static void test_cast(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 3, 2, &size);
    struct BodyAttack state;
    uint64_t due;

    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.casting && state.can_act == 0);
    /* a cast of 800 ms with a floor of 500: the clock is at its end */
    CHECK(apply(world, cast(1000, 1, 7, 800, 500)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.casting && state.can_act == 1800);
    CHECK(body_next_due(world, &due) == 1 && due == 1800 && body_world_check(world, size) == BODY_OK);
    /* no second cast while one is under way; a skill without a cast time only sets the clock */
    CHECK(refused(world, size, cast(1100, 1, 7, 300, 0), BODY_BAD_VALUE));
    CHECK(apply(world, cast(1100, 1, 7, 0, 250)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.casting && state.can_act == 1350);
    CHECK(body_next_due(world, &due) == 1 && due == 1800);
    /* an attack asked for during the cast: the game says that it casts, and nothing begins */
    CHECK(apply(world, attack(1200, 1, 7, 3, 9, 1)) == BODY_OK && event_about(world, BODY_EVENT_ASK_SWING, 1, 1200, 3, 0));
    CHECK(apply(world, answer(1200, 1, 7, YES | BODY_ANSWER_CASTING)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.has_target && !state.armed && state.attackable == 0);
    /* the world's own cast holds it back by itself, without the game's word */
    CHECK(apply(world, attack(1200, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1200, 1, 7, YES)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.has_target && !state.armed && state.attackable == 0);
    /* a body that may swing while it casts (Free Cast) swings, inside the act clock (1350) too */
    CHECK(apply(world, fighter_of(1200, 1, 7, 1000, 500, 1, 0, BODY_FIGHTER_PLAYER, BODY_FIGHTS_HOLDS | BODY_FIGHTS_FREE_CAST)) == BODY_OK);
    CHECK(apply(world, attack(1200, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1200, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1200, -1, 0) && event_about(world, BODY_EVENT_ASK_HIT, 1, 1200, 3, 0));
    CHECK(apply(world, answer(1200, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.casting && state.attackable == 2200);
    /* the cast is over: it is none any more, the core asks, and nothing later runs before the answer */
    CHECK(body_run(world, &ground, 1800) == BODY_ANSWER_FIRST);
    CHECK(event_about(world, BODY_EVENT_ASK_CAST, 1, 1800, -1, 0) && no_event(world) && body_asked(world) == 1);
    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.casting && body_world_check(world, size) == BODY_OK);
    CHECK(body_run(world, &ground, 1801) == BODY_ANSWER_FIRST && body_now(world) == 1800);
    /* its skill was carried out and has a delay of 700 */
    CHECK(apply(world, answer_ms(1800, 1, 7, BODY_ANSWER_ACTED, 700)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.can_act == 2500 && body_asked(world) == 0);
    CHECK(body_next_due(world, &due) == 0);

    /* a floor longer than the cast; a skill carried out with a shorter delay leaves the clock */
    CHECK(apply(world, cast(2500, 1, 7, 600, 2000)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.can_act == 4500);
    CHECK(body_run(world, &ground, 3100) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_CAST, 1, 3100, -1, 0));
    CHECK(apply(world, answer_ms(3100, 1, 7, BODY_ANSWER_ACTED, 300)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.can_act == 4500);
    /* a skill that was not carried out gives the clock back */
    CHECK(apply(world, cast(4500, 1, 7, 400, 1000)) == BODY_OK);
    CHECK(body_run(world, &ground, 4900) == BODY_ANSWER_FIRST && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer_ms(4900, 1, 7, 0, 0)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.can_act == 4900);
    /* an answer to a cast's end is no answer to another question, and the other way round */
    CHECK(refused(world, size, answer_ms(4900, 1, 7, BODY_ANSWER_ACTED, 100), BODY_BAD_VALUE));

    /* cancelled: no question, the clock is now */
    CHECK(apply(world, cast(5000, 1, 7, 400, 1000)) == BODY_OK);
    CHECK(apply(world, input(5100, BODY_INPUT_CAST_CANCEL, 1, 7, 0, 0, 0)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.casting && state.can_act == 5100);
    CHECK(body_next_due(world, &due) == 0);
    CHECK(refused(world, size, input(5100, BODY_INPUT_CAST_CANCEL, 1, 7, 0, 0, 0), BODY_BAD_VALUE));
    CHECK(body_run(world, &ground, 6000) == BODY_OK && no_event(world));

    /* the act clock put there, and put later only */
    CHECK(apply(world, act_clock_later(6000, 1, 7, 500)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.can_act == 6500);
    CHECK(apply(world, act_clock_later(6000, 1, 7, 200)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.can_act == 6500);
    CHECK(apply(world, act_clock(6000, 1, 7, 200)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.can_act == 6200);
    CHECK(refused(world, size, input(6000, BODY_INPUT_ACT_CLOCK, 1, 7, 0, 0, 2), BODY_BAD_VALUE));
    CHECK(body_world_check(world, size) == BODY_OK);

    /* the act clock, at 6200, holds no swing asked for at 6100 */
    CHECK(apply(world, input(6100, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(apply(world, attack(6100, 1, 7, 3, 9, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(6100, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 6100, -1, 0) && event_about(world, BODY_EVENT_ASK_HIT, 1, 6100, 3, 0));
    CHECK(apply(world, answer(6100, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(body_attack(world, 1, &state) == BODY_OK && state.can_act == 6200 && state.attackable == 7100);
    free(world);

    /* an attack that goes on, its look and a cast's end on one tick: the cast's question comes
       first, and the look sees what its answer put under the clock */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}) == 1);
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, cast(1400, 1, 7, 600, 0)) == BODY_OK);
    CHECK(body_next_due(world, &due) == 1 && due == 2000);
    CHECK(body_run(world, &ground, 2000) == BODY_ANSWER_FIRST);
    CHECK(event_about(world, BODY_EVENT_ASK_CAST, 1, 2000, -1, 0) && no_event(world));
    CHECK(body_run(world, &ground, 2000) == BODY_ANSWER_FIRST && no_event(world));
    CHECK(apply(world, answer_ms(2000, 1, 7, BODY_ANSWER_ACTED, 300)) == BODY_OK && no_event(world));
    /* the act clock, at 2300 now, holds no swing: the look swings */
    CHECK(body_run(world, &ground, 2000) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, YES)) == BODY_OK && event_about(world, BODY_EVENT_ASK_HIT, 1, 2000, 3, 0));
    CHECK(apply(world, answer(2000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_next_due(world, &due) == 1 && due == 3000);
    /* the look of an attack that goes on during the world's own cast: the cast holds the swing,
       and the attack ends, as when the game says that the body casts */
    CHECK(apply(world, cast(2100, 1, 7, 1500, 0)) == BODY_OK);
    CHECK(body_run(world, &ground, 3000) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 3000, 3, 0));
    CHECK(apply(world, answer(3000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_UNATTACKABLE, 1, 3000, -1, 0) && no_event(world));
    CHECK(body_attack(world, 1, &state) == BODY_OK && !state.has_target && state.casting);
    /* a body that casts and leaves takes its cast with it */
    CHECK(apply(world, input(3000, BODY_INPUT_REMOVE, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(body_next_due(world, &due) == 0 && body_run(world, &ground, 5000) == BODY_OK && no_event(world));
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);
}

/* About the intent of body 7 in slot 1: `what` is BODY_INTENT_*. */
static struct BodyInput intent_input(uint64_t tick, int what)
{
    struct BodyInput in = input(tick, BODY_INPUT_INTENT, 1, 7, 0, 0, 0);

    in.dir = (uint8_t)what;
    return in;
}

/* The answer to the open question of the intent `seq` of the body `id` in `slot`. */
static struct BodyInput sees_of(uint64_t tick, int slot, uint32_t id, int seq, unsigned bits, uint32_t ms)
{
    struct BodyInput in = input(tick, BODY_INPUT_INTENT, slot, id, 0, 0, (int)bits);

    in.dir = BODY_INTENT_ANSWER;
    in.target = (uint16_t)seq;
    in.target_id = ms;
    return in;
}

/* The answer to the open question of the intent `seq` of body 7 in slot 1. */
static struct BodyInput sees(uint64_t tick, int seq, unsigned bits, uint32_t ms)
{
    return sees_of(tick, 1, 7, seq, bits, ms);
}

/* The next waiting event is of `kind`, about the intent `seq` of the body in slot 1, at `tick`,
 * with `value` and `ms`. */
static int intent_says(struct BodyWorld *world, int kind, uint64_t tick, int seq, int value, uint32_t ms)
{
    struct BodyEvent event;

    return body_take_event(world, &event) && event.kind == kind && event.slot == 1 && event.tick == tick
           && event.other == seq && event.value == value && event.other_id == ms;
}

/* What a game says it sees of an intent: the target is there, and no wall is in the way. */
enum { SEEN = BODY_SEES_TARGET | BODY_SEES_LINE };

/* The step of a live intent that is due at `tick`: it asks, and is answered. */
static void step(struct BodyWorld *world, uint64_t tick, int seq, unsigned bits, uint32_t ms)
{
    CHECK(body_run(world, &ground, tick) == BODY_ANSWER_FIRST);
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, tick, seq, 0, 0));
    CHECK(apply(world, sees(tick, seq, bits, ms)) == BODY_OK);
}

/* Body 7 comes onto (x, y) at `tick`, and its intent `seq` looks from there. */
static void lands_and_looks(struct BodyWorld *world, uint64_t tick, int seq, int x, int y)
{
    CHECK(body_run(world, &ground, tick) == BODY_ANSWER_FIRST);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, tick, x, y) && intent_says(world, BODY_EVENT_ASK_INTENT, tick, seq, 0, 0));
    CHECK(no_event(world));
}

/* A request of body 7 in slot 1, with its number. */
static struct BodyInput request(uint64_t tick, int what, int number)
{
    struct BodyInput in = input(tick, BODY_INPUT_REQUEST, 1, 7, 0, 0, number);

    in.dir = (uint8_t)what;
    return in;
}

static struct BodyInput request_walk(uint64_t tick, int x, int y, int number)
{
    struct BodyInput in = request(tick, BODY_REQUEST_WALK, number);

    in.x = (int16_t)x;
    in.y = (int16_t)y;
    return in;
}

static struct BodyInput request_attack(uint64_t tick, int target, uint32_t target_id, int mode, int number)
{
    struct BodyInput in = request(tick, BODY_REQUEST_ATTACK, number);

    in.target = (uint16_t)target;
    in.target_id = target_id;
    in.cause = (uint8_t)mode;
    return in;
}

/* A skill on the body `target_id` in slot `target`, within `range` cells. */
static struct BodyInput request_skill(uint64_t tick, int target, uint32_t target_id, int range, int number)
{
    struct BodyInput in = request(tick, BODY_REQUEST_SKILL, number);

    in.target = (uint16_t)target;
    in.target_id = target_id;
    in.cause = (uint8_t)range;
    return in;
}

/* A skill on (x, y) or a pickup there (`what`), within `range` cells. */
static struct BodyInput request_cell(uint64_t tick, int what, int x, int y, int range, int number)
{
    struct BodyInput in = request(tick, what, number);

    in.x = (int16_t)x;
    in.y = (int16_t)y;
    in.cause = (uint8_t)range;
    return in;
}

/* An intent's step, case by case. A skill on a cell shows what every kind shares. */
static void test_intent(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 3, 2, &size);
    struct BodyIntentState state;
    struct BodyMove move;
    uint64_t due;
    int i;

    CHECK(body_intent(world, 1, &state) == BODY_OK && !state.has && state.seq == 0);
    CHECK(body_intent(world, 0, &state) == BODY_BAD_SLOT && body_intent(world, 4, &state) == BODY_BAD_SLOT);
    /* of an intent the game says only that it goes, and answers; without one, the one is nothing
       and the other is to no question */
    CHECK(apply(world, intent_input(1000, BODY_INTENT_CLEAR)) == BODY_OK && no_event(world));
    CHECK(refused(world, size, sees(1000, 21, SEEN, 0), BODY_BAD_VALUE));
    CHECK(refused(world, size, intent_input(1000, BODY_INTENT_ANSWER + 1), BODY_BAD_VALUE));

    /* a skill on (8, 2), within 3 cells: its step runs now, which asks what the game sees, and
       nothing later runs before the answer */
    CHECK(apply(world, request_cell(1000, BODY_REQUEST_SKILL_CELL, 8, 2, 3, 21)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 1000, 21, 0, 0) && no_event(world));
    CHECK(body_asked(world) == 1 && body_world_check(world, size) == BODY_OK);
    CHECK(body_run(world, &ground, 1001) == BODY_ANSWER_FIRST && body_now(world) == 1000);
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.has && state.seq == 21 && state.request == 21);
    CHECK(state.kind == BODY_WANTS_SKILL_CELL && state.mode == 0 && !state.walked && state.target_id == 0);
    /* an answer names its intent */
    CHECK(refused(world, size, sees(1000, 22, SEEN, 0), BODY_BAD_VALUE));
    /* six cells off: out of reach, nothing under way, so a walk is asked for; with leave, to
       within three cells of (8, 2) */
    CHECK(apply(world, sees(1000, 21, SEEN, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_WALK, 1000, 21, 0, 0) && no_event(world));
    CHECK(apply(world, sees(1000, 21, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 3 && move.path[0] == EAST);
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.walked && body_asked(world) == 0);
    CHECK(body_next_due(world, &due) == 1 && due == 1100 && body_world_check(world, size) == BODY_OK);
    /* it looks from every cell, beside its own step every 100 ms; walking, it waits */
    lands_and_looks(world, 1100, 21, 3, 2);
    CHECK(apply(world, sees(1100, 21, SEEN, 0)) == BODY_OK && no_event(world));
    step(world, 1100, 21, SEEN, 0);
    CHECK(no_event(world) && body_next_due(world, &due) == 1 && due == 1200);
    lands_and_looks(world, 1200, 21, 4, 2);
    CHECK(apply(world, sees(1200, 21, SEEN, 0)) == BODY_OK && no_event(world));
    step(world, 1200, 21, SEEN, 0);
    /* three cells off, with a line of fire: in reach. Its act 200 ms off: it waits, and nothing is
       told */
    lands_and_looks(world, 1300, 21, 5, 2);
    CHECK(apply(world, sees(1300, 21, SEEN, 200)) == BODY_OK && no_event(world));
    step(world, 1300, 21, SEEN, 200);
    step(world, 1400, 21, SEEN, 100);
    CHECK(no_event(world));
    /* the clock lets it: the act, which ends a skill's intent when it was carried out */
    step(world, 1500, 21, SEEN, 0);
    CHECK(intent_says(world, BODY_EVENT_ASK_ACT, 1500, 21, 0, 0) && no_event(world));
    CHECK(apply(world, sees(1500, 21, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1500, 21, BODY_INTENT_DONE, 0) && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && !state.has && state.seq == 0 && state.request == 21);
    CHECK(body_next_due(world, &due) == 0 && body_world_check(world, size) == BODY_OK);
    /* beyond a cell without a line of fire it is out of reach; within a cell it needs none */
    CHECK(apply(world, request_cell(1500, BODY_REQUEST_SKILL_CELL, 8, 2, 3, 22)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1500, 22, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 1500, 22, 0, 0));
    CHECK(apply(world, request_cell(1500, BODY_REQUEST_SKILL_CELL, 7, 2, 3, 22)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1500, 22, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 1500, 22, 0, 0));
    CHECK(apply(world, request_cell(1500, BODY_REQUEST_SKILL_CELL, 6, 2, 3, 23)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1500, 23, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1500, 23, 0, 0));
    CHECK(apply(world, intent_input(1500, BODY_INTENT_CLEAR)) == BODY_OK && body_asked(world) == 0);

    /* what the step says, case by case: a body that cannot act; an act the clock lets run only
       later than 300 ms, which ends the intent with how long; one within 300 is waited for; the
       target gone, by the game's word; an act that was refused */
    CHECK(apply(world, request_cell(2000, BODY_REQUEST_SKILL_CELL, 6, 2, 3, 24)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(2000, 24, SEEN | BODY_SEES_CANNOT_ACT, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 2000, 24, BODY_INTENT_CANNOT_ACT, 0) && no_event(world));
    CHECK(apply(world, request_cell(2000, BODY_REQUEST_SKILL_CELL, 6, 2, 3, 25)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(2000, 25, SEEN, 301)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 2000, 25, BODY_INTENT_TOO_EARLY, 301) && no_event(world));
    CHECK(apply(world, request_cell(2000, BODY_REQUEST_SKILL_CELL, 6, 2, 3, 26)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(2000, 26, SEEN, 300)) == BODY_OK && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.has && state.seq == 26);
    step(world, 2100, 26, SEEN | BODY_SEES_CANNOT_ACT | BODY_SEES_SITTING, 0); /* before sitting */
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 2100, 26, BODY_INTENT_CANNOT_ACT, 0) && no_event(world));
    CHECK(apply(world, request_cell(2100, BODY_REQUEST_SKILL_CELL, 6, 2, 3, 27)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(2100, 27, BODY_SEES_CANNOT_ACT | BODY_SEES_LINE, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 2100, 27, BODY_INTENT_TARGET_GONE, 0) && no_event(world));
    CHECK(apply(world, request_cell(2100, BODY_REQUEST_SKILL_CELL, 6, 2, 3, 28)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(2100, 28, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 2100, 28, 0, 0));
    CHECK(apply(world, sees(2100, 28, 0, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 2100, 28, BODY_INTENT_REFUSED, 0) && no_event(world));
    /* casting: it waits. Sitting: the body is to stand up; seated still, it waits for its step;
       standing, the step runs again at once */
    CHECK(apply(world, request_cell(2200, BODY_REQUEST_SKILL_CELL, 6, 2, 3, 29)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(2200, 29, SEEN | BODY_SEES_CASTING, 0)) == BODY_OK && no_event(world));
    step(world, 2300, 29, SEEN | BODY_SEES_SITTING | BODY_SEES_CASTING, 0);
    CHECK(intent_says(world, BODY_EVENT_ASK_STAND, 2300, 29, 0, 0) && no_event(world));
    CHECK(apply(world, sees(2300, 29, 0, 0)) == BODY_OK && no_event(world) && body_asked(world) == 0);
    step(world, 2400, 29, SEEN | BODY_SEES_SITTING, 0);
    CHECK(intent_says(world, BODY_EVENT_ASK_STAND, 2400, 29, 0, 0));
    CHECK(apply(world, sees(2400, 29, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 2400, 29, 0, 0) && no_event(world));
    CHECK(apply(world, sees(2400, 29, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 2400, 29, 0, 0));
    /* a new intent takes the place of the one that asks without a word, and of its question; one
       the game clears goes the same way; a body that leaves takes its intent's question along */
    CHECK(apply(world, request_cell(2400, BODY_REQUEST_PICKUP, 3, 2, 2, 30)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 2400, 30, 0, 0) && no_event(world));
    CHECK(body_asked(world) == 1 && refused(world, size, sees(2400, 29, BODY_INTENT_OK, 0), BODY_BAD_VALUE));
    CHECK(apply(world, intent_input(2400, BODY_INTENT_CLEAR)) == BODY_OK && no_event(world) && body_asked(world) == 0);
    CHECK(body_intent(world, 1, &state) == BODY_OK && !state.has && body_next_due(world, &due) == 0);
    CHECK(apply(world, request_cell(2500, BODY_REQUEST_PICKUP, 3, 2, 2, 31)) == BODY_OK && body_asked(world) == 1);
    CHECK(apply(world, input(2500, BODY_INPUT_REMOVE, 1, 7, 0, 0, 0)) == BODY_OK && body_asked(world) == 0);
    CHECK(body_run(world, &ground, 3000) == BODY_OK && body_world_check(world, size) == BODY_OK);
    free(world);

    /* the walk: without the game's leave the intent ends for want of a way, or because the body
       may not walk at all; with leave and no way, the same */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, request_cell(1000, BODY_REQUEST_SKILL_CELL, 8, 2, 3, 41)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1000, 41, SEEN, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1000, 41, 0, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1000, 41, BODY_INTENT_NO_PATH, 0) && no_event(world));
    CHECK(apply(world, request_cell(1000, BODY_REQUEST_SKILL_CELL, 8, 2, 3, 42)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1000, 42, SEEN, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1000, 42, BODY_INTENT_STUCK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1000, 42, BODY_INTENT_CANNOT_ACT, 0) && no_event(world));
    for (i = 0; i < MAP_HEIGHT; i++)
        close_cell(5, i);
    CHECK(apply(world, request_cell(1000, BODY_REQUEST_SKILL_CELL, 8, 2, 3, 43)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1000, 43, SEEN, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1000, 43, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1000, 43, BODY_INTENT_NO_PATH, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && body_world_check(world, size) == BODY_OK);
    open_map();
    /* a skill of range 0 on the next cell: each walk ends within a cell of it, where it is still
       out of reach. Not asked for again within 300 ms of the last; after eight, the ninth is not
       asked for */
    CHECK(apply(world, request_cell(2000, BODY_REQUEST_SKILL_CELL, 3, 2, 0, 44)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(2000, 44, SEEN, 0)) == BODY_OK);
    for (i = 0; i < 8; i++) {
        CHECK(intent_says(world, BODY_EVENT_ASK_WALK, 2000 + (uint64_t)i * 300, 44, i, 0));
        CHECK(apply(world, sees(2000 + (uint64_t)i * 300, 44, BODY_INTENT_OK, 0)) == BODY_OK && no_event(world));
        step(world, 2100 + (uint64_t)i * 300, 44, SEEN, 0);
        CHECK(no_event(world));
        step(world, 2200 + (uint64_t)i * 300, 44, SEEN, 0);
        CHECK(no_event(world));
        CHECK(body_run(world, &ground, 2300 + (uint64_t)i * 300) == BODY_ANSWER_FIRST && body_take_event(world, &(struct BodyEvent){0}));
        CHECK(apply(world, sees(2300 + (uint64_t)i * 300, 44, SEEN, 0)) == BODY_OK);
    }
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 4400, 44, BODY_INTENT_NO_PATH, 0) && no_event(world));
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);

    /* a step that is due while the intent asks waits for the answer, and comes after it */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, request_cell(1000, BODY_REQUEST_SKILL_CELL, 3, 2, 0, 51)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1000, 51, SEEN | BODY_SEES_CASTING, 0)) == BODY_OK && no_event(world));
    CHECK(body_run(world, &ground, 1100) == BODY_ANSWER_FIRST && intent_says(world, BODY_EVENT_ASK_INTENT, 1100, 51, 0, 0));
    CHECK(body_run(world, &ground, 1100) == BODY_ANSWER_FIRST && no_event(world));
    CHECK(apply(world, request(1100, BODY_REQUEST_NONE, 52)) == BODY_OK); /* an input of the question's tick */
    CHECK(refused(world, size, request(1101, BODY_REQUEST_NONE, 53), BODY_ANSWER_FIRST));
    CHECK(apply(world, sees(1100, 51, SEEN | BODY_SEES_CASTING, 0)) == BODY_OK && no_event(world));
    CHECK(body_run(world, &ground, 1100) == BODY_OK && body_next_due(world, &due) == 1 && due == 1200);
    free(world);

    /* an act starts the count of walks again: an attack walks after its target, acts, and the walk
       it asks for after the target moved off is its first again */
    world = fight_world(0, 4, 2, &size);
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 61)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    CHECK(apply(world, sees(1000, 61, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 1000, 61, 0, 0));
    CHECK(apply(world, sees(1000, 61, BODY_INTENT_OK, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    lands_and_looks(world, 1100, 61, 3, 2);
    CHECK(apply(world, sees(1100, 61, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1100, 61, 0, 0));
    CHECK(apply(world, sees(1100, 61, BODY_INTENT_OK, 0)) == BODY_OK && event_about(world, BODY_EVENT_ASK_SWING, 1, 1100, 3, 0));
    CHECK(apply(world, answer(1100, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(apply(world, answer(1100, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    step(world, 1100, 61, SEEN, 0); /* its own step: the attack runs */
    CHECK(no_event(world));
    CHECK(apply(world, input(1150, BODY_INPUT_PLACE, 3, 9, 8, 2, 0)) == BODY_OK && body_take_event(world, &(struct BodyEvent){0}));
    step(world, 1200, 61, SEEN, 0);
    CHECK(no_event(world)); /* 300 ms since the last walk are not over */
    step(world, 1300, 61, SEEN, 0);
    CHECK(intent_says(world, BODY_EVENT_ASK_WALK, 1300, 61, 0, 0) && no_event(world));
    free(world);
}

/* A skill on a body, a skill on a cell and a pickup: requests whose intent the core sees for, as
 * an attack's. */
static void test_skill_request(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(300, 6, 2, &size);
    struct BodyIntentState state;
    struct BodyAttack fight;
    struct BodyMove move;
    struct BodyEvent event;

    /* what is none: a skill on a body that is not there, a cell off the map, a range beyond the
       widest; a skill may be on the body itself, an attack not */
    CHECK(refused(world, size, request_skill(1000, 2, 9, 3, 1), BODY_BAD_SLOT));
    CHECK(refused(world, size, request_skill(1000, 3, 8, 3, 1), BODY_BAD_SLOT));
    CHECK(refused(world, size, request_skill(1000, 3, 9, BODY_MAX_RANGE + 1, 1), BODY_BAD_VALUE));
    CHECK(refused(world, size, request_cell(1000, BODY_REQUEST_SKILL_CELL, MAP_WIDTH, 2, 3, 1), BODY_BAD_CELL));
    CHECK(refused(world, size, request_cell(1000, BODY_REQUEST_PICKUP, 2, -1, 2, 1), BODY_BAD_CELL));
    CHECK(refused(world, size, request_cell(1000, BODY_REQUEST_PICKUP, 2, 3, BODY_MAX_RANGE + 1, 1), BODY_BAD_VALUE));
    CHECK(refused(world, size, request(1000, BODY_REQUEST_PICKUP + 1, 1), BODY_BAD_VALUE));
    CHECK(refused(world, size, request_attack(1000, 1, 7, 0, 1), BODY_BAD_SLOT));
    CHECK(apply(world, request_skill(1000, 1, 7, 0, 1)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 1000, 1, 0, 0) && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.kind == BODY_WANTS_SKILL && state.target == 1 && state.target_id == 7);
    /* on itself it is in reach at once, whatever its range */
    CHECK(apply(world, sees(1000, 1, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1000, 1, 0, 0));
    /* its bytes name it by its own id, and another world finds it there as itself */
    {
        size_t size2 = body_world_size(3, 16);
        struct BodyWorld *twin = body_world_init(memory(size2), size2, 3, 16, MAP_WIDTH, MAP_HEIGHT, 1000);
        unsigned char state_bytes[BODY_STATE_SIZE], again[BODY_STATE_SIZE];

        CHECK(body_state(world, 1, state_bytes) == BODY_OK && body_put_state(twin, 0, state_bytes) == BODY_OK);
        CHECK(body_intent(twin, 0, &state) == BODY_OK && state.has && state.target == 0 && state.target_id == 7);
        CHECK(body_state(twin, 0, again) == BODY_OK && memcmp(state_bytes, again, sizeof again) == 0);
        CHECK(body_world_check(twin, size2) == BODY_OK);
        free(twin);
    }
    CHECK(apply(world, sees(1000, 1, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1000, 1, BODY_INTENT_DONE, 0) && no_event(world));

    /* a skill on a body four cells off, within 2: the body walks after it to within two cells,
       and is carried out there */
    CHECK(apply(world, request_skill(1000, 3, 9, 2, 2)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 2, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 1000, 2, 0, 0));
    CHECK(apply(world, sees(1000, 2, BODY_INTENT_OK, 0)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2));
    CHECK(no_event(world) && body_move(world, 1, &move) == BODY_OK && move.steps == 2 && body_world_check(world, size) == BODY_OK);
    lands_and_looks(world, 1100, 2, 3, 2);
    CHECK(apply(world, sees(1100, 2, SEEN, 0)) == BODY_OK && no_event(world));
    step(world, 1100, 2, SEEN, 0);
    lands_and_looks(world, 1200, 2, 4, 2);
    CHECK(apply(world, sees(1200, 2, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1200, 2, 0, 0) && no_event(world));
    CHECK(apply(world, sees(1200, 2, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1200, 2, BODY_INTENT_DONE, 0) && no_event(world));
    /* a skill ends the attack and the swing in its windup, as a request of another target does;
       and the target that left ends its intent, whatever the game says */
    CHECK(apply(world, input(1300, BODY_INPUT_PLACE, 3, 9, 5, 2, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, attack(1300, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1300, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(body_attack(world, 1, &fight) == BODY_OK && fight.swing_open);
    CHECK(apply(world, request_skill(1350, 3, 9, 2, 3)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 1350, -1, BODY_CANCEL_COMMAND));
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 1350, 3, 0, 0) && no_event(world));
    CHECK(body_attack(world, 1, &fight) == BODY_OK && !fight.has_target && !fight.swing_open);
    CHECK(apply(world, input(1350, BODY_INPUT_REMOVE, 3, 9, 0, 0, 0)) == BODY_OK);
    CHECK(apply(world, sees(1350, 3, SEEN, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1350, 3, BODY_INTENT_TARGET_GONE, 0) && no_event(world));
    free(world);

    /* a pickup two cells off is picked up from where the body stands; three cells off, the body
       walks to within a cell of the item and picks it up as it comes within two */
    world = fight_world(0, 6, 6, &size);
    CHECK(apply(world, request_cell(1000, BODY_REQUEST_PICKUP, 4, 2, 2, 4)) == BODY_OK && body_take_event(world, &event));
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.kind == BODY_WANTS_PICKUP && state.target_id == 0);
    CHECK(apply(world, sees(1000, 4, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1000, 4, 0, 0));
    CHECK(apply(world, sees(1000, 4, BODY_INTENT_OK, 0)) == BODY_OK && body_take_event(world, &event) && no_event(world));
    CHECK(apply(world, request_cell(1000, BODY_REQUEST_PICKUP, 5, 2, 2, 5)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 5, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 1000, 5, 0, 0));
    CHECK(apply(world, sees(1000, 5, BODY_INTENT_OK, 0)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 2);
    /* in reach on (3, 2), where its next step begins: it stands there, and the act is asked */
    lands_and_looks(world, 1100, 5, 3, 2);
    CHECK(apply(world, sees(1100, 5, BODY_SEES_TARGET, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1100, 3, 2) && intent_says(world, BODY_EVENT_ASK_ACT, 1100, 5, 0, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && move.at_tick == 1100);
    CHECK(apply(world, sees(1100, 5, BODY_INTENT_OK, 0)) == BODY_OK && body_take_event(world, &event) && no_event(world));
    /* a pickup of distance 0: within a cell of the item first, and then onto it */
    CHECK(apply(world, request_cell(1200, BODY_REQUEST_PICKUP, 5, 2, 0, 6)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1200, 6, BODY_SEES_TARGET, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1200, 6, BODY_INTENT_OK, 0)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1200, 3, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 1);
    lands_and_looks(world, 1300, 6, 4, 2);
    CHECK(apply(world, sees(1300, 6, BODY_SEES_TARGET, 0)) == BODY_OK && no_event(world));
    step(world, 1300, 6, BODY_SEES_TARGET, 0);
    step(world, 1400, 6, BODY_SEES_TARGET, 0);
    CHECK(no_event(world));
    step(world, 1500, 6, BODY_SEES_TARGET, 0);
    CHECK(intent_says(world, BODY_EVENT_ASK_WALK, 1500, 6, 1, 0));
    CHECK(apply(world, sees(1500, 6, BODY_INTENT_OK, 0)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1500, 4, 2) && no_event(world));
    lands_and_looks(world, 1600, 6, 5, 2);
    CHECK(apply(world, sees(1600, 6, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1600, 6, 0, 0));
    /* the way to an item on a cell that cannot be walked is none */
    close_cell(7, 2);
    CHECK(apply(world, request_cell(1600, BODY_REQUEST_PICKUP, 7, 2, 0, 7)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1600, 7, BODY_SEES_TARGET, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1600, 7, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1600, 7, BODY_INTENT_NO_PATH, 0) && no_event(world));
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);

    /* in reach with a step under way: the walk ends with that step, and the act is asked from the
       cell it lands on. The target came nearer while the body was between two cells */
    world = fight_world(0, 8, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_SPEED, 1, 7, 0, 0, 150)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, request_skill(1000, 3, 9, 2, 7)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 7, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 7, BODY_INTENT_OK, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 4);
    step(world, 1100, 7, SEEN, 0);
    CHECK(no_event(world));
    lands_and_looks(world, 1150, 7, 3, 2);
    CHECK(apply(world, sees(1150, 7, SEEN, 0)) == BODY_OK && no_event(world));
    CHECK(apply(world, input(1170, BODY_INPUT_PLACE, 3, 9, 4, 3, 0)) == BODY_OK && body_take_event(world, &event));
    step(world, 1200, 7, SEEN, 0); /* on (3, 2), its step to (4, 2) under way */
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1200, 3, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 1 && move.at_tick + move.land_ms == 1300);
    lands_and_looks(world, 1300, 7, 4, 2);
    CHECK(apply(world, sees(1300, 7, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1300, 7, 0, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && body_world_check(world, size) == BODY_OK);
    free(world);
}

/* A world's bytes with an intent in them: what body_world_check holds an intent to. */
static int caught(const unsigned char *sample, size_t size, size_t offset, int bytes, uint32_t value);
static void put16(unsigned char *at, unsigned value);
static void put32(unsigned char *at, uint32_t value);

static void test_intent_check(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 6, 2, &size);
    unsigned char *arena = (unsigned char *)world;
    size_t body = HEADER_BYTES + BODY_BYTES;

    /* a skill on a cell, a walk asked for at 1000, its step due at 1100 */
    CHECK(apply(world, request_cell(1000, BODY_REQUEST_SKILL_CELL, 8, 2, 3, 21)) == BODY_OK);
    CHECK(apply(world, sees(1000, 21, SEEN, 0)) == BODY_OK && apply(world, sees(1000, 21, BODY_INTENT_OK, 0)) == BODY_OK);
    while (body_take_event(world, &(struct BodyEvent){0})) {
    }
    CHECK(body_world_check(arena, size) == BODY_OK);
    CHECK(arena[body + BODY_AT_INTENT_HAS] == 1 && arena[body + BODY_AT_INTENT_KIND] == BODY_WANTS_SKILL_CELL);
    CHECK(arena[body + BODY_AT_INTENT_RANGE] == 3 && arena[body + BODY_AT_INTENT_X] == 8 && arena[body + BODY_AT_INTENT_Y] == 2);
    CHECK(arena[body + BODY_AT_INTENT_TRIES] == 1 && arena[body + BODY_AT_INTENT_WALKED] == 1 && arena[body + BODY_AT_INTENT_SEQ] == 21);
    CHECK(caught(arena, size, body + BODY_AT_INTENT_KIND, 1, 0));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_KIND, 1, BODY_WANTS_PICKUP + 1));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_KIND, 1, BODY_WANTS_PICKUP) == 0);
    CHECK(caught(arena, size, body + BODY_AT_INTENT_KIND, 1, BODY_WANTS_SKILL)); /* a skill on a body names one */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_MODE, 1, 1));    /* only an attack has a mode */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_SWUNG, 1, 1));   /* nor swings */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_ACTING, 1, 1));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_RANGE, 1, BODY_MAX_RANGE + 1));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_X, 2, MAP_WIDTH)); /* a cell off the map */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_TARGET_ID, 4, 9)); /* a body, for a cell */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_TARGET, 2, 3));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_TRIES, 1, BODY_INTENT_WALKS + 1));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_WALKED, 1, 2));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_ASKING, 1, 1)); /* the world counts no question */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_ASKING, 1, 5));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_DUE, 4, 999));     /* a step that is overdue */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_WALK_AT, 4, 1001)); /* a walk asked for after now */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_HAS, 1, 2));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_HAS, 1, 0));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_RESERVED, 1, 1));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_RESERVED + 10, 1, 1));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_SEQ, 2, 22) == 0); /* any number is one */
    free(world);
}

/* A request: what the body's owner asks for, and all that follows from it. */
static void test_request(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(300, 3, 2, &size);
    unsigned char *arena;
    struct BodyIntentState state;
    struct BodyAttack fight;
    struct BodyMove move;
    struct BodyInput in;
    struct BodyEvent event;
    uint64_t due;
    size_t body = HEADER_BYTES + BODY_BYTES;
    int x, y;

    /* its number is kept, whatever it asks for; one without a number leaves the last */
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.request == 0);
    CHECK(apply(world, request(1000, BODY_REQUEST_NONE, 5)) == BODY_OK && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.request == 5 && !state.has);
    CHECK(apply(world, request(1000, BODY_REQUEST_NONE, 0)) == BODY_OK && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.request == 5);
    CHECK(refused(world, size, request(1000, BODY_REQUEST_PICKUP + 1, 6), BODY_BAD_VALUE));
    CHECK(refused(world, size, request_walk(1000, MAP_WIDTH, 2, 6), BODY_BAD_CELL));
    CHECK(refused(world, size, request_attack(1000, 2, 9, 0, 6), BODY_BAD_SLOT));
    CHECK(refused(world, size, request_attack(1000, 3, 8, 0, 6), BODY_BAD_SLOT));
    CHECK(refused(world, size, request_attack(1000, 1, 7, 0, 6), BODY_BAD_SLOT));
    CHECK(refused(world, size, request_attack(1000, 3, 9, BODY_ATTACK_ONCE + 1, 6), BODY_BAD_VALUE));

    /* an attack on a body in reach: the intent is the core's to see for. The game says the
       target is there; that it is in reach the core sees, and asks leave to attack */
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 6)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 1000, 6, 0, 0) && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.has && state.seq == 6 && state.request == 6);
    CHECK(state.kind == BODY_WANTS_ATTACK && state.mode == BODY_ATTACK_REPEAT && state.target == 3 && state.target_id == 9);
    CHECK(body_world_check(world, size) == BODY_OK);
    CHECK(apply(world, sees(1000, 6, SEEN, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_ACT, 1000, 6, 0, 0) && no_event(world));
    /* with leave the core begins the attack itself, whose look asks; the intent says it acts
       when that look is answered, after what the swing says */
    CHECK(apply(world, sees(1000, 6, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ASK_SWING, 1, 1000, 3, 0) && no_event(world) && body_asked(world) == 1);
    arena = (unsigned char *)world;
    CHECK(arena[body + BODY_AT_INTENT_ACTING] == 1 && body_world_check(world, size) == BODY_OK);
    CHECK(caught(arena, size, body + BODY_AT_INTENT_ACTING, 1, 2));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_TARGET, 2, 4)); /* about a slot that is none */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_KIND, 1, BODY_WANTS_PICKUP)); /* a pickup names no body */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_KIND, 1, BODY_WANTS_SKILL)); /* a skill has no mode */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_RANGE, 1, 1)); /* an attack's reach is its attack's */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_MODE, 1, BODY_ATTACK_ONCE + 1));
    CHECK(caught(arena, size, body + BODY_AT_INTENT_X, 2, 1)); /* a cell, for a body */
    CHECK(caught(arena, size, body + BODY_AT_INTENT_TARGET_ID, 4, 8) == 0); /* a body that left */
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1000, -1, 0) && event_about(world, BODY_EVENT_SWING, 1, 1000, 3, 300));
    CHECK(event_is(world, BODY_EVENT_FACE, 1, 1000, 2, 2) && event_about(world, BODY_EVENT_ATTACKING, 1, 1000, 3, 1));
    CHECK(no_event(world) && body_intent(world, 1, &state) == BODY_OK && state.has && state.seq == 6);
    CHECK(body_attack(world, 1, &fight) == BODY_OK && fight.has_target && fight.target == 3 && fight.attack_continue && fight.swing_open);
    /* asked again for the one it attacks: the swing is kept, and the new intent finds the attack running */
    CHECK(apply(world, request_attack(1050, 3, 9, BODY_ATTACK_ONCE, 7)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 1050, 7, 0, 0) && no_event(world));
    CHECK(body_attack(world, 1, &fight) == BODY_OK && fight.has_target && fight.swing_open);
    CHECK(apply(world, sees(1050, 7, SEEN, 0)) == BODY_OK && no_event(world));
    /* a walk: the intent goes without a word, the attack ends, the swing with it, and the walk
       the swing would have held starts at once */
    CHECK(apply(world, request_walk(1100, 2, 5, 8)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 1100, -1, BODY_CANCEL_COMMAND));
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1100, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 3 && move.land_ms == 100);
    CHECK(body_intent(world, 1, &state) == BODY_OK && !state.has && state.request == 8);
    CHECK(body_attack(world, 1, &fight) == BODY_OK && !fight.has_target && !fight.swing_open && !fight.armed);
    CHECK(body_asked(world) == 0 && body_next_due(world, &due) == 1 && due == 1200 && body_world_check(world, size) == BODY_OK);
    /* a walk without a way: said, and the walk under way goes on; the number is kept all the same */
    close_cell(7, 7);
    CHECK(apply(world, request_walk(1150, 7, 7, 9)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_REFUSED, 1, 1150, -1, 9) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 3 && move.at_tick == 1100);
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.request == 9);
    /* nor is a walk taken whose way is longer than the world lets a request take */
    body_set_request_steps(world, 3);
    CHECK(apply(world, request_walk(1150, 8, 6, 12)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_REFUSED, 1, 1150, -1, 12) && no_event(world));
    body_set_request_steps(world, 0); /* one step at the least */
    CHECK(arena[AT_REQUEST_STEPS] == 1);
    body_set_request_steps(world, 99);
    CHECK(arena[AT_REQUEST_STEPS] == BODY_PATH_STEPS && body_world_check(world, size) == BODY_OK);
    CHECK(apply(world, request_walk(1150, 8, 6, 13)) == BODY_OK && no_event(world)); /* it waits for the step */
    /* an attack on another body ends the one that runs */
    free(world);
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 0, 4, 2, 3, 100)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, attack(1000, 1, 7, 3, 9, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, request_attack(1050, 0, 4, BODY_ATTACK_REPEAT, 10)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_SWING_CANCEL, 1, 1050, -1, BODY_CANCEL_COMMAND));
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 1050, 10, 0, 0) && no_event(world));
    CHECK(body_attack(world, 1, &fight) == BODY_OK && !fight.has_target && !fight.swing_open);
    free(world);

    /* out of reach: with leave the core walks the body after its target, to within its attack
       range, and its intent looks from every cell it comes onto, beside its own step */
    world = fight_world(0, 6, 2, &size);
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_ONCE, 11)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 11, SEEN, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_WALK, 1000, 11, 0, 0) && no_event(world));
    CHECK(apply(world, sees(1000, 11, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 3 && move.path[0] == EAST);
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.walked && body_world_check(world, size) == BODY_OK);
    lands_and_looks(world, 1100, 11, 3, 2);
    CHECK(apply(world, sees(1100, 11, SEEN, 0)) == BODY_OK && no_event(world)); /* walking, as told */
    CHECK(body_next_due(world, &due) == 1 && due == 1100);
    step(world, 1100, 11, SEEN, 0); /* its own step was due then too, and comes after */
    CHECK(no_event(world) && body_next_due(world, &due) == 1 && due == 1200);
    lands_and_looks(world, 1200, 11, 4, 2);
    CHECK(apply(world, sees(1200, 11, SEEN, 0)) == BODY_OK && no_event(world));
    step(world, 1200, 11, SEEN, 0);
    /* in reach: the attack, which for one swing ends the intent as the swing is made */
    lands_and_looks(world, 1300, 11, 5, 2);
    CHECK(apply(world, sees(1300, 11, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1300, 11, 0, 0));
    CHECK(apply(world, sees(1300, 11, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_ASK_SWING, 1, 1300, 3, 0) && no_event(world));
    CHECK(apply(world, answer(1300, 1, 7, YES)) == BODY_OK);
    CHECK(event_about(world, BODY_EVENT_FIX, 1, 1300, -1, 0) && event_about(world, BODY_EVENT_ASK_HIT, 1, 1300, 3, 0));
    CHECK(apply(world, answer(1300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1300, 11, BODY_INTENT_DONE, 0));
    CHECK(event_about(world, BODY_EVENT_ATTACKING, 1, 1300, 3, 0) && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && !state.has && state.request == 11 && body_asked(world) == 0);
    CHECK(body_attack(world, 1, &fight) == BODY_OK && fight.has_target && !fight.attack_continue && !fight.armed);
    CHECK(body_next_due(world, &due) == 0 && body_world_check(world, size) == BODY_OK);

    /* the game's leave to walk: without it the intent ends for want of a way, or because the
       body may not walk at all; with it and no way, the same, and nothing is told under way */
    CHECK(apply(world, request_attack(3000, 3, 9, BODY_ATTACK_HOLD, 21)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, input(3000, BODY_INPUT_PLACE, 3, 9, 8, 6, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(3000, 21, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 3000, 21, 0, 0));
    CHECK(apply(world, sees(3000, 21, BODY_INTENT_STUCK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 3000, 21, BODY_INTENT_CANNOT_ACT, 0) && no_event(world));
    CHECK(apply(world, request_attack(3000, 3, 9, BODY_ATTACK_HOLD, 22)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(3000, 22, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(3000, 22, 0, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 3000, 22, BODY_INTENT_NO_PATH, 0) && no_event(world));
    for (x = 4; x <= 6; x++)
        for (y = 1; y <= 3; y++)
            if (x != 5 || y != 2)
                close_cell(x, y);
    CHECK(apply(world, request_attack(3000, 3, 9, BODY_ATTACK_HOLD, 23)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(3000, 23, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(3000, 23, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 3000, 23, BODY_INTENT_NO_PATH, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0 && body_world_check(world, size) == BODY_OK);
    free(world);

    /* an attack that goes on and finds its target out of reach is taken over by the intent,
       which looks at once and says nothing of the distance; a target that is gone ends the
       intent, and the attack with it */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 31)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 31, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 31, BODY_INTENT_OK, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK && body_take_event(world, &event) && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_FACE, 1, 1000, 2, 2) && event_about(world, BODY_EVENT_ATTACKING, 1, 1000, 3, 1));
    CHECK(no_event(world));
    CHECK(apply(world, input(1050, BODY_INPUT_PLACE, 3, 9, 8, 2, 0)) == BODY_OK && body_take_event(world, &event));
    step(world, 1100, 31, SEEN | BODY_SEES_CASTING, 0); /* kept from walking after it meanwhile */
    CHECK(no_event(world));
    CHECK(apply(world, attack_clock(1150, 1, 7, 100, 1)) == BODY_OK);
    step(world, 1200, 31, SEEN | BODY_SEES_CASTING, 0);
    CHECK(body_run(world, &ground, 1250) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_SWING, 1, 1250, 3, 0));
    CHECK(apply(world, answer(1250, 1, 7, YES)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 1250, 31, 0, 0) && no_event(world));
    CHECK(apply(world, sees(1250, 31, BODY_SEES_LINE, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1250, 31, BODY_INTENT_TARGET_GONE, 0) && no_event(world));
    CHECK(body_attack(world, 1, &fight) == BODY_OK && !fight.has_target && !fight.armed && body_next_due(world, &due) == 0);
    /* the game sees the target, and the core does not have it: gone too */
    CHECK(apply(world, request_attack(1300, 3, 9, BODY_ATTACK_REPEAT, 32)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, input(1300, BODY_INPUT_REMOVE, 3, 9, 0, 0, 0)) == BODY_OK);
    CHECK(apply(world, sees(1300, 32, SEEN, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1300, 32, BODY_INTENT_TARGET_GONE, 0) && no_event(world));
    free(world);

    /* hold: the button comes up after a swing, and the attack goes on no more, while the swing
       in its windup still lands; before any swing, the attack becomes one for one swing */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_HOLD, 41)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 41, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 41, BODY_INTENT_OK, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, request(1050, BODY_REQUEST_RELEASE, 42)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1050, 41, BODY_INTENT_DONE, 0) && no_event(world));
    CHECK(body_attack(world, 1, &fight) == BODY_OK && !fight.has_target && !fight.armed && fight.swing_open);
    CHECK(body_intent(world, 1, &state) == BODY_OK && !state.has && state.request == 42);
    CHECK(body_run(world, &ground, 1300) == BODY_ANSWER_FIRST && event_about(world, BODY_EVENT_ASK_HIT, 1, 1300, 3, 1));
    CHECK(apply(world, answer(1300, 1, 7, BODY_ANSWER_HIT)) == BODY_OK && no_event(world));
    /* inside the attack delay the attack waits, and the intent acts without a look to wait for */
    CHECK(apply(world, request_attack(1400, 3, 9, BODY_ATTACK_HOLD, 43)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1400, 43, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1400, 43, BODY_INTENT_OK, 0)) == BODY_OK && no_event(world));
    CHECK(body_attack(world, 1, &fight) == BODY_OK && fight.armed && fight.attack_continue && body_asked(world) == 0);
    CHECK(apply(world, request(1450, BODY_REQUEST_RELEASE, 44)) == BODY_OK && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && state.has && state.mode == BODY_ATTACK_ONCE && state.request == 44);
    CHECK(body_attack(world, 1, &fight) == BODY_OK && fight.armed && !fight.attack_continue);
    /* a cancel: the intent goes without a word and the attack ends */
    CHECK(apply(world, request(1460, BODY_REQUEST_CANCEL, 45)) == BODY_OK && no_event(world));
    CHECK(body_intent(world, 1, &state) == BODY_OK && !state.has && state.request == 45);
    CHECK(body_attack(world, 1, &fight) == BODY_OK && !fight.has_target && !fight.armed && body_next_due(world, &due) == 0);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);

    /* an attack that lost its target before its first look was answered was refused; so is one
       of a body that was not told how it fights */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 51)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 51, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 51, BODY_INTENT_OK, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, input(1000, BODY_INPUT_ATTACK_STOP, 1, 7, 0, 0, 0)) == BODY_OK && no_event(world));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_INTENT_END, 1000, 51, BODY_INTENT_REFUSED, 0) && no_event(world));
    CHECK(apply(world, input(1000, BODY_INPUT_PLACE, 3, 9, 2, 2, 0)) == BODY_OK && body_take_event(world, &event));
    in = input(1000, BODY_INPUT_REQUEST, 3, 9, 0, 0, 61);
    in.dir = BODY_REQUEST_ATTACK;
    in.target = 1;
    in.target_id = 7;
    CHECK(apply(world, in) == BODY_OK && body_take_event(world, &event) && event.kind == BODY_EVENT_ASK_INTENT && event.slot == 3);
    in = sees_of(1000, 3, 9, 61, SEEN, 0);
    CHECK(apply(world, in) == BODY_OK && body_take_event(world, &event) && event.kind == BODY_EVENT_ASK_ACT);
    in.value = BODY_INTENT_OK;
    CHECK(apply(world, in) == BODY_OK);
    CHECK(body_take_event(world, &event) && event.kind == BODY_EVENT_INTENT_END && event.slot == 3 && event.value == BODY_INTENT_REFUSED);
    CHECK(no_event(world) && body_asked(world) == 0 && body_world_check(world, size) == BODY_OK);
    free(world);
    open_map();
}

/* What an intent the core sees for sees: reach by the game's two facts, the attack and the walk
 * by the world's own bodies. */
static void test_request_sees(void)
{
    size_t size;
    struct BodyWorld *world = fight_world(0, 4, 2, &size);
    struct BodyMove move;
    struct BodyEvent event;

    /* a walk to the cell it stands on is a walk of no steps, as the game's own is */
    CHECK(apply(world, request_walk(1000, 2, 2, 1)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    /* two cells off with a range of one: out of reach, unless the target walks, which the game says */
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 81)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 81, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 1000, 81, 0, 0));
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 82)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 82, SEEN | BODY_SEES_TARGET_WALKS, 0)) == BODY_OK);
    CHECK(intent_says(world, BODY_EVENT_ASK_ACT, 1000, 82, 0, 0) && no_event(world));
    /* with a range of three it is in reach, unless a wall is in the line of fire */
    CHECK(apply(world, request(1000, BODY_REQUEST_CANCEL, 0)) == BODY_OK);
    CHECK(apply(world, fighter(1000, 1, 7, 1000, 500, 3, 0, 1)) == BODY_OK);
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 83)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 83, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1000, 83, 0, 0));
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 84)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 84, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 1000, 84, 0, 0));
    /* each walk it asks for comes a cell closer: to within three of a target six cells off,
       then, still without a line of fire, to within two */
    CHECK(apply(world, input(1000, BODY_INPUT_PLACE, 3, 9, 8, 2, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 84, BODY_INTENT_OK, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 3);
    lands_and_looks(world, 1100, 84, 3, 2);
    CHECK(apply(world, sees(1100, 84, BODY_SEES_TARGET, 0)) == BODY_OK && no_event(world));
    step(world, 1100, 84, BODY_SEES_TARGET, 0);
    lands_and_looks(world, 1200, 84, 4, 2);
    CHECK(apply(world, sees(1200, 84, BODY_SEES_TARGET, 0)) == BODY_OK && no_event(world));
    step(world, 1200, 84, BODY_SEES_TARGET, 0);
    lands_and_looks(world, 1300, 84, 5, 2);
    CHECK(apply(world, sees(1300, 84, BODY_SEES_TARGET, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_WALK, 1300, 84, 1, 0));
    CHECK(apply(world, sees(1300, 84, BODY_INTENT_OK, 0)) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1300, 5, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 1 && body_world_check(world, size) == BODY_OK);
    free(world);

    /* an attack on another body is not this intent's act; and a walk it asked for that the game
       stopped is not under way: it asks again, once 300 ms are over */
    world = fight_world(0, 3, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 0, 4, 2, 3, 100)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 85)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, attack(1000, 1, 7, 0, 4, 1)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK && body_take_event(world, &event) && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, BODY_ANSWER_HIT)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, sees(1000, 85, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1000, 85, 0, 0));
    CHECK(apply(world, request(1000, BODY_REQUEST_CANCEL, 0)) == BODY_OK && no_event(world));
    CHECK(apply(world, input(1000, BODY_INPUT_PLACE, 3, 9, 7, 2, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 86)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 86, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 86, BODY_INTENT_OK, 0)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, input(1050, BODY_INPUT_STOP, 1, 7, 0, 0, 0)) == BODY_OK && body_take_event(world, &event));
    step(world, 1100, 86, SEEN, 0);
    CHECK(no_event(world));
    step(world, 1200, 86, SEEN, 0);
    CHECK(no_event(world));
    step(world, 1300, 86, SEEN, 0);
    CHECK(intent_says(world, BODY_EVENT_ASK_WALK, 1300, 86, 1, 0) && no_event(world));
    free(world);

    /* a swing in its windup lands before any walk: out of reach meanwhile, the intent is held */
    world = fight_world(300, 3, 2, &size);
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 87)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 87, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 87, BODY_INTENT_OK, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, answer(1000, 1, 7, YES)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(apply(world, input(1050, BODY_INPUT_PLACE, 3, 9, 8, 2, 0)) == BODY_OK && body_take_event(world, &event));
    step(world, 1100, 87, SEEN, 0);
    CHECK(no_event(world) && body_asked(world) == 0);
    free(world);

    /* a look from a cell is the intent's own, also where its step is not due; so is the one from
       the cell a walk ends on because its target came closer */
    world = fight_world(0, 7, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_SPEED, 1, 7, 0, 0, 150)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 88)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 88, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 88, BODY_INTENT_OK, 0)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    step(world, 1100, 88, SEEN, 0);
    CHECK(apply(world, input(1120, BODY_INPUT_PLACE, 3, 9, 4, 2, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(body_run(world, &ground, 1150) == BODY_ANSWER_FIRST);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1150, 3, 2) && event_is(world, BODY_EVENT_MOVE, 1, 1150, 3, 2));
    CHECK(intent_says(world, BODY_EVENT_ASK_INTENT, 1150, 88, 0, 0) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 0);
    CHECK(apply(world, sees(1150, 88, SEEN, 0)) == BODY_OK && intent_says(world, BODY_EVENT_ASK_ACT, 1150, 88, 0, 0));
    free(world);

    /* the body it walked after is gone: the walk ends without a way, and the intent with it */
    world = fight_world(0, 7, 2, &size);
    CHECK(apply(world, input(1000, BODY_INPUT_SPEED, 1, 7, 0, 0, 150)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 89)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 89, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 89, BODY_INTENT_OK, 0)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    step(world, 1100, 89, SEEN, 0);
    CHECK(no_event(world) && apply(world, input(1120, BODY_INPUT_REMOVE, 3, 9, 0, 0, 0)) == BODY_OK);
    CHECK(body_run(world, &ground, 1150) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1150, 3, 2) && event_is(world, BODY_EVENT_MOVE, 1, 1150, 3, 2));
    CHECK(event_is(world, BODY_EVENT_NO_PATH, 1, 1150, 3, 2) && intent_says(world, BODY_EVENT_INTENT_END, 1150, 89, BODY_INTENT_NO_PATH, 0));
    CHECK(no_event(world) && body_asked(world) == 0 && body_world_check(world, size) == BODY_OK);
    free(world);
}

/* Runs the world to `tick` as a host that says yes to everything the body `id` in `slot` asks. */
static void yes_to(struct BodyWorld *world, int slot, uint32_t id, uint64_t tick)
{
    struct BodyEvent event;
    struct BodyInput in;
    uint64_t due;
    int rounds = 0, result;

    for (;;) {
        while (body_take_event(world, &event)) {
            if (event.slot != slot)
                continue;
            if (event.kind == BODY_EVENT_ASK_SWING) {
                CHECK(apply(world, answer(event.tick, slot, id, YES)) == BODY_OK);
            } else if (event.kind == BODY_EVENT_ASK_HIT) {
                CHECK(apply(world, answer(event.tick, slot, id, BODY_ANSWER_HIT)) == BODY_OK);
            } else if (event.kind >= BODY_EVENT_ASK_INTENT && event.kind <= BODY_EVENT_ASK_WALK) {
                in = sees_of(event.tick, slot, id, event.other, event.kind == BODY_EVENT_ASK_INTENT ? SEEN : BODY_INTENT_OK, 0);
                CHECK(apply(world, in) == BODY_OK);
            }
        }
        if (!body_next_due(world, &due) || due > tick || ++rounds > 4000)
            break;
        result = body_run(world, &ground, due);
        CHECK(result == BODY_OK || result == BODY_ANSWER_FIRST);
    }
    CHECK(rounds <= 4000 && body_asked(world) == 0 && body_run(world, &ground, tick) == BODY_OK);
}

/* A body as bytes: taken from one world and put into another, it is the same body there. */
static void test_state(void)
{
    static const uint64_t ticks[] = {1000, 1100, 1250, 1300, 1450, 1600, 2000, 2300, 2650, 3000};
    size_t size, size2 = body_world_size(3, 16), size3 = body_world_size(3, 8);
    struct BodyWorld *world = fight_world(300, 6, 2, &size), *twin, *small;
    unsigned char state[BODY_STATE_SIZE], again[BODY_STATE_SIZE], bad[BODY_STATE_SIZE], *before;
    struct BodyIntentState intent_state;
    struct BodyAttack fight;
    struct BodyEvent event;
    size_t i;

    /* a body with all of it: an intent on body 9, walking after it. A question it waits on is
       not in its bytes */
    CHECK(apply(world, request_attack(1000, 3, 9, BODY_ATTACK_REPEAT, 71)) == BODY_OK && body_take_event(world, &event));
    CHECK(body_asked(world) == 1 && ((const unsigned char *)world)[HEADER_BYTES + BODY_BYTES + BODY_AT_INTENT_ASKING] != 0);
    CHECK(body_state(world, 1, state) == BODY_OK && state[BODY_AT_INTENT_ASKING] == 0 && state[BODY_AT_INTENT_HAS] == 1);
    CHECK(apply(world, sees(1000, 71, SEEN, 0)) == BODY_OK && body_take_event(world, &event));
    CHECK(apply(world, sees(1000, 71, BODY_INTENT_OK, 0)) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    CHECK(body_state(world, 0, state) == BODY_BAD_SLOT && body_state(world, 4, state) == BODY_BAD_SLOT);
    CHECK(body_state(world, 1, state) == BODY_OK);
    /* whom it names it names by id: no slot of this world is in the bytes */
    CHECK(state[BODY_AT_TARGET] == 0 && state[BODY_AT_INTENT_TARGET] == 0 && state[BODY_AT_INTENT_TARGET_ID] == 9);

    /* another world, of other slots, at the same clock: the target in slot 0, the body into the
       free slot 2. It names its target where that is here, and gives the same bytes */
    twin = body_world_init(memory(size2), size2, 3, 16, MAP_WIDTH, MAP_HEIGHT, 1000);
    body_set_cancel_penalty(twin, 0);
    CHECK(apply(twin, input(1000, BODY_INPUT_ADD, 0, 9, 6, 2, 100)) == BODY_OK && body_take_event(twin, &event));
    CHECK(body_put_state(twin, 2, state) == BODY_OK && event_is(twin, BODY_EVENT_MOVE, 2, 1000, 2, 2) && no_event(twin));
    CHECK(body_state(twin, 2, again) == BODY_OK && memcmp(state, again, sizeof state) == 0);
    CHECK(body_world_check(twin, size2) == BODY_OK);
    CHECK(body_intent(twin, 2, &intent_state) == BODY_OK && intent_state.has && intent_state.seq == 71);
    CHECK(intent_state.target == 0 && intent_state.target_id == 9 && intent_state.request == 71);
    /* and from there on the two do the same at every tick: the walk into reach, the swings */
    for (i = 0; i < sizeof ticks / sizeof ticks[0]; i++) {
        yes_to(world, 1, 7, ticks[i]);
        yes_to(twin, 2, 7, ticks[i]);
        CHECK(body_state(world, 1, state) == BODY_OK && body_state(twin, 2, again) == BODY_OK);
        CHECK(memcmp(state, again, sizeof state) == 0);
    }
    CHECK(body_attack(world, 1, &fight) == BODY_OK && fight.has_target && fight.armed && fight.attackable == 3300);
    /* whom it attacks it finds in a third world too, where that body is in yet another slot */
    small = body_world_init(memory(size3), size3, 3, 8, MAP_WIDTH, MAP_HEIGHT, 3000);
    CHECK(apply(small, input(3000, BODY_INPUT_ADD, 1, 9, 6, 2, 100)) == BODY_OK && body_take_event(small, &event));
    CHECK(body_state(world, 1, state) == BODY_OK && body_put_state(small, 0, state) == BODY_OK && body_take_event(small, &event));
    CHECK(body_attack(small, 0, &fight) == BODY_OK && fight.has_target && fight.target == 1 && fight.target_id == 9);
    CHECK(body_world_check(small, size3) == BODY_OK);
    yes_to(world, 1, 7, 3400);
    yes_to(small, 0, 7, 3400);
    CHECK(body_state(world, 1, state) == BODY_OK && body_state(small, 0, again) == BODY_OK && memcmp(state, again, sizeof state) == 0);
    CHECK(body_attack(small, 0, &fight) == BODY_OK && fight.swing_open && fight.swing_target == 1);
    free(small);
    yes_to(twin, 2, 7, 3400);

    /* bytes from outside: what is no body this world could hold at its clock is refused, and
       nothing is changed */
    before = memory(size2);
    memcpy(before, twin, size2);
    CHECK(body_put_state(twin, 3, state) == BODY_BAD_SLOT);
    memcpy(bad, state, sizeof bad);
    bad[BODY_AT_USED] = 0;
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    bad[BODY_AT_ASKING] = 1; /* a question another host was asked */
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    bad[BODY_AT_INTENT_ASKING] = 1;
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    put32(bad + BODY_AT_TICK, 3401); /* a record from after the clock */
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    put32(bad + BODY_AT_ATTACK_DUE, 3399); /* a look that is overdue */
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    put16(bad + BODY_AT_X, MAP_WIDTH);
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    put32(bad + BODY_AT_ATTACK_TARGET_ID, 7); /* it attacks itself */
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    put32(bad + BODY_AT_INTENT_TARGET_ID, 7);
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    put16(bad + BODY_AT_ATTACK_TARGET, 1); /* a slot, which is no world's to say for another */
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    memcpy(bad, state, sizeof bad);
    put16(bad + BODY_AT_INTENT_TARGET, 1);
    CHECK(body_put_state(twin, 2, bad) == BODY_BAD_VALUE);
    CHECK(memcmp(before, twin, size2) == 0);
    free(before);

    /* a ring without room; a body in the slot that waits for an answer, whose question goes
       with it; and a world its target is not in, where the target is gone for it */
    small = body_world_init(memory(size3), size3, 3, 8, MAP_WIDTH, MAP_HEIGHT, 3400);
    CHECK(apply(small, input(3400, BODY_INPUT_ADD, 2, 7, 1, 1, 100)) == BODY_OK);
    for (i = 0; i < 7; i++)
        CHECK(apply(small, input(3400, BODY_INPUT_PLACE, 2, 7, 1, 1 + (int)(i & 1), 0)) == BODY_OK);
    before = memory(size3);
    memcpy(before, small, size3);
    CHECK(body_put_state(small, 2, state) == BODY_EVENTS_FULL && memcmp(before, small, size3) == 0);
    free(before);
    while (body_take_event(small, &event)) {
    }
    CHECK(apply(small, input(3400, BODY_INPUT_ADD, 0, 4, 1, 2, 100)) == BODY_OK && body_take_event(small, &event));
    {
        struct BodyInput in = input(3400, BODY_INPUT_REQUEST, 2, 7, 0, 0, 80);

        in.dir = BODY_REQUEST_ATTACK;
        in.target = 0;
        in.target_id = 4;
        CHECK(apply(small, in) == BODY_OK && body_take_event(small, &event) && body_asked(small) == 1);
    }
    CHECK(body_put_state(small, 2, state) == BODY_OK && body_asked(small) == 0 && body_world_check(small, size3) == BODY_OK);
    CHECK(body_take_event(small, &event) && event.kind == BODY_EVENT_MOVE && no_event(small));
    CHECK(body_state(small, 2, again) == BODY_OK && memcmp(state, again, sizeof state) == 0);
    yes_to(small, 2, 7, 3700);
    CHECK(body_intent(small, 2, &intent_state) == BODY_OK && !intent_state.has && intent_state.request == 71);
    CHECK(body_attack(small, 2, &fight) == BODY_OK && !fight.has_target && !fight.armed);
    CHECK(body_world_check(small, size3) == BODY_OK);
    free(small);
    free(twin);
    free(world);
}

/* A hold: no step lands before it ends. */
static void test_delay(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove move;
    uint64_t due;

    /* a step under way lands when the hold ends, which is a new record */
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 5, 2, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2));
    CHECK(apply(world, input(1050, BODY_INPUT_DELAY, 1, 7, 0, 0, 300)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1050, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1050 && move.x == 2 && move.land_ms == 300 && move.steps == 3);
    CHECK(body_next_due(world, &due) == 1 && due == 1350);
    /* a hold that ends sooner changes nothing; one that ends before the step lands, neither */
    CHECK(apply(world, input(1060, BODY_INPUT_DELAY, 1, 7, 0, 0, 100)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 1350);
    CHECK(refused(world, size, input(1060, BODY_INPUT_DELAY, 1, 7, 0, 0, BODY_MAX_DELAY_MS + 1), BODY_BAD_VALUE));
    CHECK(body_run(world, &ground, 1350) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1350, 3, 2));
    /* the steps after it take their own time again */
    CHECK(body_next_due(world, &due) == 1 && due == 1450);
    CHECK(apply(world, input(1360, BODY_INPUT_DELAY, 1, 7, 0, 0, 50)) == BODY_OK && no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 1450);

    /* a body that stands is held too: asked to walk, it stands until the hold ends and then
     * takes its first step */
    CHECK(apply(world, input(1360, BODY_INPUT_STOP, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1360, 3, 2));
    CHECK(apply(world, input(1400, BODY_INPUT_DELAY, 1, 7, 0, 0, 400)) == BODY_OK && no_event(world));
    CHECK(apply(world, input(1500, BODY_INPUT_WALK, 1, 7, 3, 4, 0)) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1500 && move.land_ms == 300 + 100 && move.path[0] == NORTH);
    CHECK(body_next_due(world, &due) == 1 && due == 1900);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);
}

static void test_stop_speed_place(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove move;
    struct BodyInput in;
    uint64_t due;

    /* a stop leaves the body on the cell it is on; the step under way is not taken */
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 5, 2, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2));
    CHECK(apply(world, input(1090, BODY_INPUT_STOP, 1, 7, 0, 0, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1090, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1090 && move.x == 2 && move.steps == 0 && move.path[0] == 0);
    CHECK(body_next_due(world, &due) == 0);
    CHECK(apply(world, input(1095, BODY_INPUT_STOP, 1, 7, 0, 0, 0)) == BODY_OK && no_event(world));

    /* a new speed: the step under way lands when it was going to, the next takes the new time */
    CHECK(apply(world, input(1100, BODY_INPUT_WALK, 1, 7, 5, 2, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1100, 2, 2));
    CHECK(apply(world, input(1150, BODY_INPUT_SPEED, 1, 7, 0, 0, 200)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1150, 2, 2) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.at_tick == 1150 && move.land_ms == 50 && move.speed_ms == 200);
    CHECK(apply(world, input(1160, BODY_INPUT_SPEED, 1, 7, 0, 0, 200)) == BODY_OK && no_event(world));
    CHECK(refused(world, size, input(1160, BODY_INPUT_SPEED, 1, 7, 0, 0, BODY_MAX_SPEED_MS + 1), BODY_BAD_VALUE));
    CHECK(body_run(world, &ground, 1200) == BODY_OK && event_is(world, BODY_EVENT_CELL, 1, 1200, 3, 2));
    CHECK(body_next_due(world, &due) == 1 && due == 1400);

    /* a place ends the walk: nothing of the steps is left */
    in = input(1250, BODY_INPUT_PLACE, 1, 7, 8, 6, 0);
    in.cause = BODY_CAUSE_PLACED;
    CHECK(body_input(world, &ground, &in) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1250, 8, 6) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK);
    CHECK(move.at_tick == 1250 && move.x == 8 && move.y == 6 && move.steps == 0 && move.land_ms == 0 && move.path[0] == 0);
    CHECK(move.cause == BODY_CAUSE_PLACED && move.speed_ms == 200);
    CHECK(body_next_due(world, &due) == 0);
    CHECK(body_world_check(world, size) == BODY_OK);
    free(world);
}

/* Two bodies land at the same tick, in slot order, and a ring without room stops the run
 * until events are taken. */
static void test_same_tick(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove move;
    uint64_t due;
    int i;

    CHECK(apply(world, input(1000, BODY_INPUT_ADD, 3, 9, 2, 5, 100)) == BODY_OK);
    for (i = 0; i < 3; i++)
        CHECK(apply(world, input(1000, BODY_INPUT_PLACE, 3, 9, 2, 5, 0)) == BODY_OK);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 3, 9, 4, 5, 0)) == BODY_OK);
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 4, 2, 0)) == BODY_OK);
    /* six events wait in a ring of eight: no room for a landing's three */
    CHECK(body_run(world, &ground, 1100) == BODY_EVENTS_FULL && body_now(world) == 1100);
    CHECK(body_move(world, 1, &move) == BODY_OK && move.x == 2);
    CHECK(body_next_due(world, &due) == 1 && due == 1100);
    CHECK(body_world_check(world, size) == BODY_OK);
    for (i = 0; i < 5; i++)
        CHECK(event_is(world, BODY_EVENT_MOVE, 3, 1000, 2, 5));
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 2, 2) && no_event(world));
    CHECK(body_run(world, &ground, 1100) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1100, 3, 2) && event_is(world, BODY_EVENT_CELL, 3, 1100, 3, 5));
    CHECK(no_event(world));
    CHECK(body_next_due(world, &due) == 1 && due == 1200);
    free(world);
}

/* Playing a record: the body at any tick, from the record alone. */
static void test_move_at(void)
{
    struct BodyMove record, at;

    memset(&record, 0, sizeof record);
    record.at_tick = 1000;
    record.x = 2;
    record.y = 2;
    record.land_ms = 250; /* it stands 150 ms first */
    record.speed_ms = 100;
    record.steps = 3;
    record.path[0] = EAST;
    record.path[1] = NORTH_EAST;
    record.path[2] = NORTH;
    body_move_at(&record, 999, &at);
    CHECK(memcmp(&at, &record, sizeof at) == 0);
    body_move_at(&record, 1249, &at);
    CHECK(memcmp(&at, &record, sizeof at) == 0);
    body_move_at(&record, 1250, &at);
    CHECK(at.at_tick == 1250 && at.x == 3 && at.y == 2 && at.dir == EAST && at.steps == 2 && at.land_ms == 140);
    CHECK(at.path[0] == NORTH_EAST && at.path[1] == NORTH && at.path[2] == 0);
    body_move_at(&record, 1389, &at);
    CHECK(at.at_tick == 1250 && at.x == 3);
    body_move_at(&record, 1390, &at);
    CHECK(at.at_tick == 1390 && at.x == 4 && at.y == 3 && at.steps == 1 && at.land_ms == 100);
    body_move_at(&record, 99999, &at);
    CHECK(at.at_tick == 1490 && at.x == 4 && at.y == 4 && at.dir == NORTH && at.steps == 0 && at.land_ms == 0);

    /* a record from outside: too many steps, directions that do not exist */
    memset(&record, 0xFF, sizeof record);
    record.at_tick = 0;
    body_move_at(&record, 0xFFFFFFFFFFFFull, &at);
    CHECK(at.steps == 0);
}

/* A record put into a body: what a host does that is told records and does not make them. */
static void test_put_move(void)
{
    size_t size;
    struct BodyWorld *world = walker_world(8, &size);
    struct BodyMove record, move, bad;
    void *before = memory(size);
    uint64_t due;

    /* a walk east from (4, 4), told at tick 900: its first step was under way, landing at 1040 */
    memset(&record, 0, sizeof record);
    record.at_tick = 900;
    record.x = 4;
    record.y = 4;
    record.land_ms = 140;
    record.speed_ms = 200;
    record.steps = 2;
    record.path[0] = EAST;
    record.path[1] = NORTH;
    CHECK(body_put_move(world, 1, 7, &record) == BODY_OK);
    CHECK(body_now(world) == 1000);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1000, 4, 4) && no_event(world));
    CHECK(body_move(world, 1, &move) == BODY_OK && memcmp(&move, &record, sizeof move) == 0);
    CHECK(body_world_check(world, size) == BODY_OK);
    CHECK(body_next_due(world, &due) == 1 && due == 1040);
    /* the body walks on from it: a request waits for the step under way */
    CHECK(apply(world, input(1000, BODY_INPUT_WALK, 1, 7, 5, 7, 0)) == BODY_OK && no_event(world));
    CHECK(body_run(world, &ground, 1040) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_CELL, 1, 1040, 5, 4) && event_is(world, BODY_EVENT_MOVE, 1, 1040, 5, 4));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.steps == 3 && move.land_ms == 200);

    /* a record whose step starts at its tick has the body on its cell then: a request at that
     * tick is taken at once */
    record.at_tick = 1040;
    record.x = 5;
    record.land_ms = 200;
    CHECK(body_put_move(world, 1, 7, &record) == BODY_OK && event_is(world, BODY_EVENT_MOVE, 1, 1040, 5, 4));
    CHECK(apply(world, input(1040, BODY_INPUT_WALK, 1, 7, 5, 6, 0)) == BODY_OK);
    CHECK(event_is(world, BODY_EVENT_MOVE, 1, 1040, 5, 4));
    CHECK(body_move(world, 1, &move) == BODY_OK && move.path[0] == NORTH && move.steps == 2);

    /* what is not a state at the world's clock is refused and changes nothing */
    memcpy(before, world, size);
    bad = record;
    CHECK(body_put_move(world, 2, 7, &bad) == BODY_BAD_SLOT && body_put_move(world, 1, 8, &bad) == BODY_BAD_SLOT);
    CHECK(body_put_move(world, 4, 7, &bad) == BODY_BAD_SLOT);
    bad.at_tick = 1041;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_TICK);
    bad = record;
    bad.at_tick = 800; /* its step landed at 1000, before the clock */
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_TICK);
    bad = record;
    bad.steps = BODY_PATH_STEPS + 1;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_VALUE);
    bad = record;
    bad.path[1] = 8;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_VALUE);
    bad = record;
    bad.path[2] = 1; /* behind the steps */
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_VALUE);
    bad = record;
    bad.dir = 8;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_VALUE);
    bad = record;
    bad.cause = 3;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_VALUE);
    bad = record;
    bad.speed_ms = BODY_MAX_SPEED_MS + 1;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_VALUE);
    bad = record;
    bad.reserved[2] = 1;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_VALUE);
    bad = record;
    bad.steps = 0;
    bad.path[0] = bad.path[1] = 0;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_VALUE); /* a landing time without steps */
    bad = record;
    bad.x = MAP_WIDTH;
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_CELL);
    bad = record;
    bad.x = MAP_WIDTH - 1; /* its first step leads off the map */
    CHECK(body_put_move(world, 1, 7, &bad) == BODY_BAD_CELL);
    CHECK(memcmp(before, world, size) == 0);
    CHECK(body_world_check(world, size) == BODY_OK);

    /* standing */
    memset(&record, 0, sizeof record);
    record.at_tick = 1000;
    record.x = 1;
    record.y = 1;
    record.speed_ms = 150;
    CHECK(body_put_move(world, 1, 7, &record) == BODY_OK);
    CHECK(body_move(world, 1, &move) == BODY_OK && memcmp(&move, &record, sizeof move) == 0);
    CHECK(body_next_due(world, &due) == 0 && body_world_check(world, size) == BODY_OK);
    free(before);
    free(world);
}

static void put16(unsigned char *at, unsigned value)
{
    at[0] = (unsigned char)value;
    at[1] = (unsigned char)(value >> 8);
}

static void put32(unsigned char *at, uint32_t value)
{
    put16(at, value & 0xffffu);
    put16(at + 2, value >> 16);
}

/* A world of four slots and four events on the test map at tick 500: body 7 in slot 1, on
 * (3, 4) since tick 200 and walking east to (6, 4) since tick 500; body 8 in slot 2 standing
 * on (5, 6) since tick 300; slots 0 and 3 used and free again. Three events wait, in the
 * ring's fourth to sixth entry: slot 2's of tick 300, the one of tick 500 for the body that
 * was in slot 3, and slot 1's walk. The three entries before them are taken ones. */
static size_t sample_world(unsigned char **arena)
{
    size_t size = body_world_size(4, 8);
    struct BodyWorld *world;
    struct BodyEvent event;

    open_map();
    *arena = memory(size);
    world = body_world_init(*arena, size, 4, 8, MAP_WIDTH, MAP_HEIGHT, 100);
    apply(world, input(100, BODY_INPUT_ADD, 0, 1, 0, 0, 150));
    apply(world, input(100, BODY_INPUT_PLACE, 0, 1, 1, 1, 0));
    body_take_event(world, &event);
    body_take_event(world, &event);
    apply(world, input(150, BODY_INPUT_REMOVE, 0, 1, 0, 0, 0));
    apply(world, input(200, BODY_INPUT_ADD, 1, 7, 3, 4, 150));
    apply(world, input(300, BODY_INPUT_ADD, 2, 8, 5, 6, 150));
    apply(world, input(500, BODY_INPUT_ADD, 3, 9, 0, 0, 150));
    apply(world, input(500, BODY_INPUT_REMOVE, 3, 9, 0, 0, 0));
    body_take_event(world, &event); /* slot 1's of tick 200 */
    apply(world, input(500, BODY_INPUT_WALK, 1, 7, 6, 4, 0));
    return size;
}

/* body_world_check refuses a copy of the sample world with `value` written at `offset`. */
static int caught(const unsigned char *sample, size_t size, size_t offset, int bytes, uint32_t value)
{
    unsigned char *copy = memory(size);
    int result;

    memcpy(copy, sample, size);
    if (bytes == 1)
        copy[offset] = (unsigned char)value;
    else if (bytes == 2)
        put16(copy + offset, value);
    else
        put32(copy + offset, value);
    result = body_world_check(copy, size);
    free(copy);
    return result == BODY_BAD_WORLD;
}

static void test_check(void)
{
    unsigned char *arena, *copy;
    size_t size = sample_world(&arena);
    size_t walker = HEADER_BYTES + BODY_BYTES, stander = HEADER_BYTES + 2 * BODY_BYTES, empty = HEADER_BYTES + 3 * BODY_BYTES;
    size_t ring = HEADER_BYTES + 4 * BODY_BYTES;
    size_t live1 = ring + 3 * EVENT_BYTES, live2 = ring + 4 * EVENT_BYTES, live3 = ring + 5 * EVENT_BYTES;
    size_t dead = ring + 2 * EVENT_BYTES;

    CHECK(body_world_check(arena, size) == BODY_OK);
    CHECK(arena[AT_EVENT_FIRST] == 3 && arena[AT_EVENT_COUNT] == 3);
    CHECK(arena[walker + BODY_AT_STEPS] == 3 && arena[walker + BODY_AT_TO_X] == 6 && arena[stander + BODY_AT_STEPS] == 0);
    CHECK(body_world_check(arena, size + 100) == BODY_OK);

    CHECK(body_world_check(NULL, size) == BODY_BAD_WORLD);
    CHECK(body_world_check(arena, size - 1) == BODY_BAD_WORLD);
    CHECK(body_world_check(arena, HEADER_BYTES - 1) == BODY_BAD_WORLD);
    CHECK(body_world_check(arena, 0) == BODY_BAD_WORLD);
    copy = memory(size + 8);
    memcpy(copy + 4, arena, size);
    CHECK(body_world_check(copy + 4, size) == BODY_BAD_WORLD);
    free(copy);

    /* the header */
    CHECK(caught(arena, size, AT_MAGIC, 1, 0));
    CHECK(caught(arena, size, AT_VERSION, 4, BODY_RULES_VERSION + 1));
    CHECK(caught(arena, size, AT_REQUEST_STEPS, 4, 0));
    CHECK(caught(arena, size, AT_REQUEST_STEPS, 4, BODY_PATH_STEPS + 1));
    CHECK(caught(arena, size, AT_REQUEST_STEPS, 4, 17) == 0);
    CHECK(caught(arena, size, AT_SIZE, 4, (uint32_t)size + 8));
    CHECK(caught(arena, size, AT_BODIES, 4, 0));
    CHECK(caught(arena, size, AT_BODIES, 4, 5));
    CHECK(caught(arena, size, AT_BODIES, 4, 3));
    CHECK(caught(arena, size, AT_BODIES, 4, 0xFFFFFFFFu));
    CHECK(caught(arena, size, AT_EVENTS, 4, 0));
    CHECK(caught(arena, size, AT_EVENTS, 4, 9));
    CHECK(caught(arena, size, AT_ASKED, 4, 1)); /* a question nobody asks */
    CHECK(caught(arena, size, AT_EVENTS, 4, 0xFFFFFFFFu));
    CHECK(caught(arena, size, AT_WIDTH, 4, 0));
    CHECK(caught(arena, size, AT_WIDTH, 4, 0xFFFFFFFFu));
    CHECK(caught(arena, size, AT_HEIGHT, 4, BODY_MAX_SIDE + 1));
    CHECK(caught(arena, size, AT_EVENT_FIRST, 4, 8));
    CHECK(caught(arena, size, AT_EVENT_COUNT, 4, 9));
    /* a clock before what the world holds, and one past a step that is due */
    CHECK(caught(arena, size, AT_NOW, 4, 250));
    CHECK(caught(arena, size, AT_NOW, 4, 650) == 0);
    CHECK(caught(arena, size, AT_NOW, 4, 651));

    /* a body */
    CHECK(caught(arena, size, walker + BODY_AT_USED, 4, 2));
    CHECK(caught(arena, size, walker + BODY_AT_X, 2, MAP_WIDTH));
    CHECK(caught(arena, size, walker + BODY_AT_Y, 2, 0xFFFF));
    CHECK(caught(arena, size, walker + BODY_AT_DIR, 1, 8));
    CHECK(caught(arena, size, walker + BODY_AT_CAUSE, 1, 3));
    CHECK(caught(arena, size, walker + BODY_AT_SPEED, 2, BODY_MAX_SPEED_MS + 1));
    CHECK(caught(arena, size, walker + BODY_AT_MOVE_RESERVED, 1, 1));
    CHECK(caught(arena, size, walker + BODY_AT_MOVE_RESERVED + 4, 1, 1));
    CHECK(caught(arena, size, walker + BODY_AT_TICK, 4, 501));
    CHECK(caught(arena, size, walker + BODY_AT_TICK + 4, 4, 1));
    /* its walk */
    CHECK(caught(arena, size, walker + BODY_AT_STEPS, 1, BODY_PATH_STEPS + 1));
    CHECK(caught(arena, size, walker + BODY_AT_PATH, 1, 8));
    CHECK(caught(arena, size, walker + BODY_AT_PATH + 3, 1, 1)); /* a direction behind the steps */
    CHECK(caught(arena, size, walker + BODY_AT_TO_X, 2, MAP_WIDTH));
    CHECK(caught(arena, size, walker + BODY_AT_CHANGE, 1, 1) == 0); /* a target that waits */
    CHECK(caught(arena, size, walker + BODY_AT_CHANGE, 1, 2));
    CHECK(caught(arena, size, walker + BODY_AT_WALK_RESERVED, 1, 1));
    CHECK(caught(arena, size, walker + BODY_AT_WALK_ATTACK, 1, 1)); /* only a chase is the attack's */
    /* what it walks to: a cell has no range and no body, a chase has both */
    CHECK(caught(arena, size, walker + BODY_AT_GOAL, 1, 1) == 0);
    CHECK(caught(arena, size, walker + BODY_AT_GOAL, 1, 3));
    CHECK(caught(arena, size, walker + BODY_AT_RANGE, 2, 1));
    CHECK(caught(arena, size, walker + BODY_AT_TARGET, 2, 2));
    CHECK(caught(arena, size, walker + BODY_AT_TARGET_ID, 4, 8));
    CHECK(caught(arena, size, walker + BODY_AT_GOAL, 1, 2)); /* a chase without a range */
    CHECK(caught(arena, size, walker + BODY_AT_CELL_TICK, 4, 501)); /* on its cell later than its record */
    CHECK(caught(arena, size, walker + BODY_AT_CAN_MOVE, 4, 500 + BODY_MAX_DELAY_MS) == 0);
    CHECK(caught(arena, size, walker + BODY_AT_CAN_MOVE, 4, 500 + BODY_MAX_DELAY_MS + 1));
    /* its attack: a target and a swing only while there is one, nothing due behind the clock */
    CHECK(caught(arena, size, walker + BODY_AT_HAS_TARGET, 1, 2));
    CHECK(caught(arena, size, walker + BODY_AT_HAS_TARGET, 1, 1) == 0);
    CHECK(caught(arena, size, walker + BODY_AT_ATTACK_TARGET, 2, 2));
    CHECK(caught(arena, size, walker + BODY_AT_ATTACK_TARGET_ID, 4, 8));
    CHECK(caught(arena, size, walker + BODY_AT_ARMED, 1, 1));       /* due at tick 0 */
    CHECK(caught(arena, size, walker + BODY_AT_ATTACK_DUE, 4, 600)); /* a due tick and no timer */
    CHECK(caught(arena, size, walker + BODY_AT_SWING_OPEN, 1, 1));
    CHECK(caught(arena, size, walker + BODY_AT_IMPACT_DUE, 4, 600));
    CHECK(caught(arena, size, walker + BODY_AT_SWING_REACH, 2, 11));
    CHECK(caught(arena, size, walker + BODY_AT_ASKING, 1, 1));      /* the world counts no question */
    CHECK(caught(arena, size, walker + BODY_AT_ASKING, 1, 4));
    CHECK(caught(arena, size, walker + BODY_AT_ASKING, 1, 6)); /* no such question */
    /* nothing of an intent on a body that has none (one that has: test_intent) */
    CHECK(caught(arena, size, walker + BODY_AT_INTENT_SEQ, 2, 9));
    CHECK(caught(arena, size, walker + BODY_AT_INTENT_DUE, 4, 600));
    CHECK(caught(arena, size, walker + BODY_AT_INTENT_HAS, 1, 1));
    CHECK(caught(arena, size, walker + BODY_AT_INTENT_ASKING, 1, 1));
    CHECK(caught(arena, size, walker + BODY_AT_INTENT_RESERVED, 1, 1));
    /* a cast only with its end, and its end not behind the clock */
    CHECK(caught(arena, size, walker + BODY_AT_CASTING, 1, 1));
    CHECK(caught(arena, size, walker + BODY_AT_CASTING, 1, 2));
    CHECK(caught(arena, size, walker + BODY_AT_CAST_DUE, 4, 600));
    CHECK(caught(arena, size, walker + BODY_AT_REQUESTED, 1, 1));
    CHECK(caught(arena, size, walker + BODY_AT_ATTACK_RANGE, 1, BODY_MAX_RANGE + 1));
    CHECK(caught(arena, size, walker + BODY_AT_HELD_MS, 1, 1)); /* held by a swing that is not open */
    /* how it fights: a kind there is, and flags there are */
    CHECK(caught(arena, size, walker + BODY_AT_KIND, 1, BODY_FIGHTER_MONSTER) == 0);
    CHECK(caught(arena, size, walker + BODY_AT_KIND, 1, BODY_FIGHTER_MONSTER + 1));
    CHECK(caught(arena, size, walker + BODY_AT_FLAGS, 1, BODY_FIGHTS_HOLDS | BODY_FIGHTS_FREE_CAST) == 0);
    CHECK(caught(arena, size, walker + BODY_AT_FLAGS, 1, 4));
    /* a body that stands has no walk */
    CHECK(caught(arena, size, stander + BODY_AT_LAND, 2, 5));
    CHECK(caught(arena, size, stander + BODY_AT_PATH, 1, 1));
    CHECK(caught(arena, size, stander + BODY_AT_TO_X, 2, 1));
    CHECK(caught(arena, size, stander + BODY_AT_CHANGE, 1, 1));
    CHECK(caught(arena, size, stander + BODY_AT_STEPS, 1, 1)); /* a step that was due at tick 300 */
    /* a free slot that is not empty */
    CHECK(caught(arena, size, empty + BODY_AT_ID, 4, 9));
    CHECK(caught(arena, size, empty + BODY_BYTES - 1, 1, 1));

    /* the ring */
    CHECK(caught(arena, size, live1 + EVENT_AT_KIND, 2, 0));
    CHECK(caught(arena, size, live1 + EVENT_AT_KIND, 2, BODY_EVENT_CELL) == 0);
    CHECK(caught(arena, size, live1 + EVENT_AT_KIND, 2, BODY_EVENT_UNATTACKABLE) == 0);
    CHECK(caught(arena, size, live1 + EVENT_AT_KIND, 2, BODY_EVENT_UNATTACKABLE + 1));
    CHECK(caught(arena, size, live1 + EVENT_AT_SLOT, 2, 4));
    CHECK(caught(arena, size, live1 + EVENT_AT_X, 2, MAP_WIDTH));
    CHECK(caught(arena, size, live1 + EVENT_AT_DIR, 1, 8));
    CHECK(caught(arena, size, live1 + EVENT_AT_RESERVED, 1, 1));
    CHECK(caught(arena, size, live1 + EVENT_AT_RESERVED2, 1, 1));
    CHECK(caught(arena, size, live1 + EVENT_AT_OTHER, 2, 4)); /* about a slot that is none */
    CHECK(caught(arena, size, live3 + EVENT_AT_TICK, 4, 501));
    CHECK(caught(arena, size, live2 + EVENT_AT_TICK, 4, 299)); /* earlier than the event before it */
    CHECK(caught(arena, size, dead + EVENT_AT_TICK, 1, 1));
    CHECK(caught(arena, size, dead + EVENT_BYTES - 1, 1, 1));
    free(arena);
}

static void test_vectors(void)
{
    size_t i;
    void *arena = memory(body_scenario_arena_size()), *work = memory(body_scenario_work_size());

    for (i = 0; i < sizeof body_vectors / sizeof body_vectors[0]; i++) {
        const struct BodyVector *v = &body_vectors[i];
        uint64_t digest = body_scenario(v->seed, v->start_tick, v->inputs, arena, work);

        if (digest != v->digest) {
            fprintf(stderr, "vector %u: seed 0x%08X gives 0x%016llX, the file says 0x%016llX\n", (unsigned)i,
                    (unsigned)v->seed, (unsigned long long)digest, (unsigned long long)v->digest);
            failures++;
        }
        /* the same again, in memory that held another run */
        CHECK(body_scenario(v->seed, v->start_tick, v->inputs, arena, work) == digest);
    }
    free(arena);
    free(work);
}

/* A scenario stopped after `stop` inputs, its world copied to other memory and run on there,
 * ends as the uninterrupted one. */
static uint64_t through_snapshot(const struct BodyVector *v, uint32_t stop)
{
    size_t size = body_scenario_arena_size(), work_size = body_scenario_work_size();
    void *arena = memory(size), *copy = memory(size), *work = memory(work_size);
    struct BodyScenario scenario;
    uint64_t digest = 0;

    if (body_scenario_begin(&scenario, arena, v->seed, v->start_tick)) {
        body_scenario_run(&scenario, arena, work, stop);
        memcpy(copy, arena, size);
        memset(arena, 0xA5, size);
        memset(work, 0x5A, work_size);
        CHECK(body_world_check(copy, size) == BODY_OK);
        body_scenario_run(&scenario, copy, work, v->inputs - stop);
        digest = body_scenario_end(&scenario, copy);
    }
    free(arena);
    free(copy);
    free(work);
    return digest;
}

static void test_snapshot(void)
{
    const struct BodyVector *v = &body_vectors[0];

    CHECK(through_snapshot(v, 0) == v->digest);
    CHECK(through_snapshot(v, 1) == v->digest);
    CHECK(through_snapshot(v, 777) == v->digest);
    CHECK(through_snapshot(v, v->inputs - 1) == v->digest);
    CHECK(through_snapshot(v, v->inputs) == v->digest);
}

/* The scenarios are worth their digests only if they reach what the core does. */
static void test_scenario_reach(void)
{
    size_t i;
    void *arena = memory(body_scenario_arena_size()), *work = memory(body_scenario_work_size());
    struct BodyScenario scenario;
    int code;

    for (i = 0; i < sizeof body_vectors / sizeof body_vectors[0]; i++) {
        const struct BodyVector *v = &body_vectors[i];

        CHECK(body_scenario_begin(&scenario, arena, v->seed, v->start_tick));
        body_scenario_run(&scenario, arena, work, v->inputs);
        if (scenario.failed)
            fprintf(stderr, "vector %u: a check failed at input %u\n", (unsigned)i, (unsigned)scenario.failed);
        CHECK(scenario.failed == 0);
        CHECK(scenario.inputs == v->inputs);
        CHECK(scenario.results[BODY_OK] > v->inputs / 10);
        CHECK(scenario.results[BODY_BAD_WORLD] == 0);
        for (code = BODY_BAD_KIND; code <= BODY_DUE_FIRST; code++)
            CHECK(scenario.results[code] > 0);
        for (code = BODY_EVENT_MOVE; code <= BODY_EVENT_UNATTACKABLE; code++)
            CHECK(scenario.events[code] > 0);
        CHECK(scenario.events[BODY_EVENT_MOVE] > 1000 && scenario.events[BODY_EVENT_CELL] > 1000);
        CHECK(scenario.swings > 120 && scenario.events[BODY_EVENT_SWING_CANCEL] > 10);
        CHECK(scenario.events[BODY_EVENT_ASK_CAST] > 100);
        CHECK(scenario.events[BODY_EVENT_ASK_ACT] > 30 && scenario.events[BODY_EVENT_ASK_STAND] > 20);
        CHECK(scenario.intents > 200 && scenario.intents_ended > 100);
        for (code = BODY_WANTS_ATTACK; code <= BODY_WANTS_PICKUP; code++)
            CHECK(scenario.wanted[code] > 50 && scenario.acts[code] > 5);
        CHECK(scenario.refused_runs > 100);
        CHECK(body_now(arena) > v->start_tick);
    }
    /* the third run starts below 2^32 and ends above it */
    CHECK(body_vectors[2].start_tick < 0x100000000ull);
    CHECK(body_scenario_begin(&scenario, arena, body_vectors[2].seed, body_vectors[2].start_tick));
    body_scenario_run(&scenario, arena, work, body_vectors[2].inputs);
    CHECK(body_now(arena) > 0x100000000ull);
    free(arena);
    free(work);
}

static void print_digests(void)
{
    size_t i;
    void *arena = memory(body_scenario_arena_size()), *work = memory(body_scenario_work_size());
    struct BodyScenario scenario;

    for (i = 0; i < sizeof body_vectors / sizeof body_vectors[0]; i++) {
        const struct BodyVector *v = &body_vectors[i];

        printf("    {0x%08Xu, 0x%016llXull, %uu, 0x%016llXull},\n", (unsigned)v->seed,
               (unsigned long long)v->start_tick, (unsigned)v->inputs,
               (unsigned long long)body_scenario(v->seed, v->start_tick, v->inputs, arena, work));
    }
    /* what the runs reached, for whoever changes the scenario */
    for (i = 0; i < sizeof body_vectors / sizeof body_vectors[0]; i++) {
        const struct BodyVector *v = &body_vectors[i];
        int code;

        body_scenario_begin(&scenario, arena, v->seed, v->start_tick);
        body_scenario_run(&scenario, arena, work, v->inputs);
        fprintf(stderr, "vector %u: failed at %u; results", (unsigned)i, (unsigned)scenario.failed);
        for (code = 0; code < 12; code++)
            fprintf(stderr, " %u", (unsigned)scenario.results[code]);
        fprintf(stderr, "; events");
        for (code = BODY_EVENT_MOVE; code <= BODY_EVENT_UNATTACKABLE; code++)
            fprintf(stderr, " %u", (unsigned)scenario.events[code]);
        fprintf(stderr, "; intents");
        for (code = BODY_WANTS_ATTACK; code <= BODY_WANTS_PICKUP; code++)
            fprintf(stderr, " %u/%u", (unsigned)scenario.wanted[code], (unsigned)scenario.acts[code]);
        fprintf(stderr, "; swings %u; refused runs %u\n", (unsigned)scenario.swings, (unsigned)scenario.refused_runs);
    }
    free(arena);
    free(work);
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--digests") == 0) {
        print_digests();
        return 0;
    }
    test_sizes();
    test_init();
    test_inputs();
    test_walk();
    test_turn();
    test_closed_cell();
    test_near_and_chase();
    test_chase_closed_cell();
    test_attack();
    test_attack_reach();
    test_attack_lands_step();
    test_swing_cancel_frees_walk();
    test_swing();
    test_attack_steered();
    test_monster_attack();
    test_other_attack();
    test_cast();
    test_intent();
    test_skill_request();
    test_intent_check();
    test_request();
    test_request_sees();
    test_state();
    test_delay();
    test_stop_speed_place();
    test_same_tick();
    test_move_at();
    test_put_move();
    test_check();
    test_vectors();
    test_snapshot();
    test_scenario_reach();
    if (failures) {
        fprintf(stderr, "body_tests: %d failed\n", failures);
        return 1;
    }
    puts("body_tests: ok");
    return 0;
}
