#include "body/body.h"

#include "body/grid_path.h"

#define BODY_MAGIC 0x59444F42u /* "BODY" */

/* The arena: this header, `bodies` slots, then `events` ring entries. */
struct BodyWorld {
    uint32_t magic, version;
    uint64_t now;
    uint32_t size; /* of the whole arena */
    uint32_t bodies, events;
    uint32_t event_first, event_count; /* the waiting events, oldest first */
    int32_t width, height;
    uint32_t asked;             /* bodies that wait for an answer */
    uint32_t cancel_penalty_ms; /* after a swing cancelled in its windup the next waits this long */
    uint32_t request_steps;     /* the longest way a request's walk takes, 1 to BODY_PATH_STEPS */
};

/* What a walk needs beyond its record; zero while the body stands. */
/* What a walk is towards. */
enum { GOAL_CELL = 0, GOAL_NEAR = 1, GOAL_BODY = 2 };

struct BodyWalk {
    int16_t to_x, to_y;  /* where the walk goes: the cell, or the cell the body it is after was on
                            when the way was found */
    uint8_t change;      /* 1: this is a new goal, taken when the step under way lands */
    uint8_t goal;        /* GOAL_CELL: onto (to_x, to_y). GOAL_NEAR: within `range` of it.
                            GOAL_BODY: within `range` of the body `target_id` in slot `target` */
    uint16_t range;
    uint32_t target_id;
    uint16_t target;
    uint8_t attack;      /* 1: the attack began this walk after its target (unit_walktobl's flag 2): when
                            it ends within its range the attack looks at that body again */
    uint8_t reserved;    /* zero */
};

/* What a body is asked about. */
enum { ASK_NONE = 0, ASK_SWING = 1, ASK_HIT_NOW = 2, ASK_IMPACT = 3, ASK_CAST = 4, ASK_BEGIN = 5 };

/* A body's attack (unit.c's unit_data, as far as a basic attack goes). */
struct BodyAct {
    uint64_t can_act;    /* the act clock (canact_tick): no skill is used before this; it holds no swing */
    uint64_t attackable; /* nor before this: the attack delay (attackabletime) */
    uint64_t attack_due; /* when the attack looks at its target again, while `armed` (attacktimer) */
    uint64_t impact_due; /* when the swing lands, while `swing_open` */
    uint32_t target_id;  /* whom it attacks, while `has_target`; with the slot `target` */
    uint32_t swing_target_id;
    uint16_t target, swing_target;
    uint16_t swing_reach; /* how far the swing's target may be when it lands (check_distance) */
    uint16_t chase_range;
    uint16_t adelay, amotion; /* attack delay and attack motion, ms */
    uint8_t range;            /* attack range, cells */
    uint8_t flags;            /* BODY_FIGHTS_*: a swing holds the body's walk for its attack motion
                                 (attack_walk_delay); it may swing while it casts (Free Cast) */
    uint16_t windup;          /* ms from a swing's start to its impact; 0: it lands as it starts */
    int16_t aim_x, aim_y;     /* where its target was when it last asked */
    uint8_t has_target, attack_continue, armed, swing_open;
    uint8_t asking;    /* ASK_*: the question that is open */
    uint8_t requested; /* the look that asked came from a request, not from the timer */
    uint8_t kind;      /* BODY_FIGHTER_* */
    uint8_t casting;   /* a cast is under way, until cast_due */
    uint32_t held_ms;    /* for how long after its start the swing in its windup found the body held
                            from walking anyway, while `swing_open` */
    uint64_t cast_due;   /* when the cast is over, while `casting` */
};

/* What an intent asks the game. */
enum { WANT_NONE = 0, WANT_VIEW = 1, WANT_STAND = 2, WANT_ACT = 3, WANT_WALK = 4 };

/* A body's intent: what its owner wants, and how far that is. */
struct BodyIntent {
    uint64_t due;        /* when its step next runs by itself, while `has` */
    uint64_t walk_at;    /* when it last asked for a walk, while `walked` */
    uint32_t target_id;  /* whom it is about, with the slot `target`: an attack, a skill on a body */
    uint16_t target;
    uint16_t seq;
    int16_t x, y;        /* the cell it is about: a skill on a cell, a pickup */
    uint8_t has;
    uint8_t kind;        /* BODY_WANTS_* */
    uint8_t mode;        /* an attack's BODY_ATTACK_*; 0 for the rest */
    uint8_t walked;
    uint8_t walk_tries;  /* walks asked for since it last acted */
    uint8_t swung;       /* an attack: a swing began since it was set */
    uint8_t asking;      /* WANT_*: the question that is open */
    uint8_t acting;      /* an attack: it was begun and its first look is not answered yet: whether the
                            intent acts waits for the answer */
    uint8_t range;       /* a skill's range or a pickup's distance, in cells; 0 for an attack, whose
                            reach is its attack's */
    uint8_t reserved[11]; /* zero */
};

struct Body {
    uint32_t id;
    uint16_t used;    /* 0 or 1; a free slot is all zero */
    uint16_t request; /* the number of the last request it took; 0: none yet */
    struct BodyMove move;
    struct BodyWalk walk;
    uint64_t cell_tick; /* when the body came onto its cell */
    uint64_t can_move;  /* no step lands before this tick */
    struct BodyAct act;
    struct BodyIntent intent;
};

/* The layout is part of the rules: a snapshot made by one build is read by another. A
   size that is not what it has to be is an array of -1 elements, which no compiler takes;
   _Static_assert would need every build of this file to be in C11 mode. */
#define LAYOUT(name, holds) typedef char layout_##name[(holds) ? 1 : -1]
LAYOUT(move_size, sizeof(struct BodyMove) == 56);
LAYOUT(input_size, sizeof(struct BodyInput) == 32);
LAYOUT(event_size, sizeof(struct BodyEvent) == 32);
LAYOUT(world_size, sizeof(struct BodyWorld) == 56);
LAYOUT(walk_size, sizeof(struct BodyWalk) == 16);
LAYOUT(body_size, sizeof(struct Body) == 224);
LAYOUT(state_size, sizeof(struct Body) == BODY_STATE_SIZE);
LAYOUT(intent_size, sizeof(struct BodyIntent) == 48);
LAYOUT(body_intent, offsetof(struct Body, intent) == 176);
LAYOUT(intent_target, offsetof(struct BodyIntent, target_id) == 16);
LAYOUT(intent_state_size, sizeof(struct BodyIntentState) == 16);
LAYOUT(act_size, sizeof(struct BodyAct) == 80);
LAYOUT(attack_size, sizeof(struct BodyAttack) == 40);
LAYOUT(body_act, offsetof(struct Body, act) == 96);
LAYOUT(move_path, offsetof(struct BodyMove, path) == 24);
LAYOUT(body_move, offsetof(struct Body, move) == 8);
LAYOUT(body_walk, offsetof(struct Body, walk) == 64);
LAYOUT(path_steps, BODY_PATH_STEPS == GRID_PATH_MAX_STEPS);
LAYOUT(map_side, BODY_MAX_SIDE == GRID_PATH_MAX_SIDE);
LAYOUT(scratch_size, BODY_SCRATCH_SIZE == GRID_PATH_SCRATCH_SIZE);
/* a held first step lands at most this long after its record: within land_ms */
LAYOUT(landing_fits, BODY_MAX_DELAY_MS + BODY_MAX_SPEED_MS * 14u / 10u <= 0xFFFFu);

/* The server's direction codes: 0 north (y + 1), then counter-clockwise. */
static const int8_t DIR_X[8] = {0, -1, -1, -1, 0, 1, 1, 1};
static const int8_t DIR_Y[8] = {1, 1, 0, -1, -1, -1, 0, 1};

static struct Body *slots(struct BodyWorld *world)
{
    return (struct Body *)(world + 1);
}

static const struct Body *slots_of(const struct BodyWorld *world)
{
    return (const struct Body *)(world + 1);
}

static struct BodyEvent *ring(struct BodyWorld *world)
{
    return (struct BodyEvent *)(slots(world) + world->bodies);
}

static const struct BodyEvent *ring_of(const struct BodyWorld *world)
{
    return (const struct BodyEvent *)(slots_of(world) + world->bodies);
}

static void zero(void *memory, size_t size)
{
    unsigned char *bytes = memory;
    size_t i;

    for (i = 0; i < size; i++)
        bytes[i] = 0;
}

static int is_zero(const void *memory, size_t size)
{
    const unsigned char *bytes = memory;
    size_t i;

    for (i = 0; i < size; i++)
        if (bytes[i] != 0)
            return 0;
    return 1;
}

static int on_map(int32_t width, int32_t height, int x, int y)
{
    return x >= 0 && y >= 0 && x < width && y < height;
}

static int walkable(const struct BodyWorld *world, const struct BodyGround *ground, int x, int y)
{
    return on_map(world->width, world->height, x, y)
           && ground->walkable[(size_t)y * (size_t)world->width + (size_t)x] != 0;
}

static uint16_t step_ms(uint8_t dir, uint16_t speed)
{
    return (uint16_t)grid_path_step_ms(speed, dir & 1);
}

static uint64_t landing(const struct BodyMove *move)
{
    return move->at_tick + move->land_ms;
}

/* Whether a walking body has left its cell: the step that lands next started before `tick`.
 * One that lands its step now and starts the next, or whose first step is still held back,
 * is on its cell. */
static int step_under_way(const struct BodyMove *move, uint64_t tick)
{
    return move->steps > 0 && tick + step_ms(move->path[0], move->speed_ms) > landing(move);
}

/* The walk ends with the step under way: its record, from `tick` on, is that one step. Returns
 * whether that changed anything. */
static int end_walk_with_step(struct Body *body, uint64_t tick)
{
    struct BodyMove *move = &body->move;
    uint64_t lands = landing(move);
    int i, changed = move->steps > 1 || body->walk.goal != 0 || body->walk.change != 0;

    zero(&body->walk, sizeof body->walk);
    if (!changed)
        return 0;
    move->at_tick = tick;
    move->land_ms = (uint16_t)(lands - tick);
    move->steps = 1;
    for (i = 1; i < BODY_PATH_STEPS; i++)
        move->path[i] = 0;
    return 1;
}

uint32_t body_rules_version(void)
{
    return BODY_RULES_VERSION;
}

size_t body_world_size(uint32_t bodies, uint32_t events)
{
    if (bodies == 0 || bodies > BODY_MAX_BODIES || events < BODY_MIN_EVENTS || events > BODY_MAX_EVENTS)
        return 0;
    return sizeof(struct BodyWorld) + (size_t)bodies * sizeof(struct Body) + (size_t)events * sizeof(struct BodyEvent);
}

struct BodyWorld *body_world_init(void *arena, size_t size, uint32_t bodies, uint32_t events,
                                  int32_t width, int32_t height, uint64_t now)
{
    size_t need = body_world_size(bodies, events);
    struct BodyWorld *world = arena;

    if (arena == NULL || ((uintptr_t)arena & 7u) != 0 || need == 0 || size < need)
        return NULL;
    if (width < 1 || height < 1 || width > BODY_MAX_SIDE || height > BODY_MAX_SIDE)
        return NULL;
    zero(arena, need);
    world->magic = BODY_MAGIC;
    world->version = BODY_RULES_VERSION;
    world->now = now;
    world->size = (uint32_t)need;
    world->bodies = bodies;
    world->events = events;
    world->width = width;
    world->height = height;
    world->request_steps = BODY_PATH_STEPS;
    return world;
}

/* An attack the world's own rules could have left: flags that are flags, a target and a swing
 * only while there is one, nothing due behind the clock. */
static int act_valid(const struct BodyAct *act, const struct BodyWorld *world)
{
    if (act->has_target > 1 || act->attack_continue > 1 || act->armed > 1 || act->swing_open > 1
        || act->asking > ASK_BEGIN || act->requested > 1 || act->kind > BODY_FIGHTER_MONSTER
        || act->flags > (BODY_FIGHTS_HOLDS | BODY_FIGHTS_FREE_CAST)
        || act->casting > 1)
        return 0;
    /* only a monster is asked before its swing */
    if (act->asking == ASK_BEGIN && act->kind != BODY_FIGHTER_MONSTER)
        return 0;
    if (act->casting ? act->cast_due < world->now : act->cast_due != 0)
        return 0;
    if (act->has_target ? act->target >= world->bodies : act->target != 0 || act->target_id != 0)
        return 0;
    if (act->swing_open ? act->swing_target >= world->bodies || act->impact_due < world->now
                        : act->swing_target != 0 || act->swing_target_id != 0 || act->impact_due != 0
                              || act->swing_reach != 0)
        return 0;
    if (act->swing_open ? act->held_ms > BODY_MAX_DELAY_MS : act->held_ms != 0)
        return 0;
    if (act->armed ? act->attack_due < world->now : act->attack_due != 0)
        return 0;
    return act->range <= BODY_MAX_RANGE && act->chase_range <= BODY_MAX_RANGE && act->amotion <= BODY_MAX_DELAY_MS
           && (act->asking || !act->requested);
}

/* Whether an intent is about a body (an attack, a skill on a body); the rest are about a cell. */
static int on_body(const struct BodyIntent *intent)
{
    return intent->kind == BODY_WANTS_ATTACK || intent->kind == BODY_WANTS_SKILL;
}

/* An intent the world's own rules could have left: nothing without one, a known kind with what
 * that kind has, a step that is not overdue, and no more walks than it may ask for. */
static int intent_valid(const struct BodyIntent *intent, const struct BodyWorld *world)
{
    if (!intent->has)
        return is_zero(intent, sizeof *intent);
    if (intent->has > 1 || intent->kind < BODY_WANTS_ATTACK || intent->kind > BODY_WANTS_PICKUP || intent->walked > 1
        || intent->walk_tries > BODY_INTENT_WALKS || intent->asking > WANT_WALK || intent->range > BODY_MAX_RANGE
        || !is_zero(intent->reserved, sizeof intent->reserved))
        return 0;
    /* an attack has a mode, may have swung and waits for its first look; the rest have a range */
    if (intent->kind == BODY_WANTS_ATTACK ? intent->mode > BODY_ATTACK_ONCE || intent->swung > 1 || intent->acting > 1
                                                || intent->range != 0
                                          : intent->mode != 0 || intent->swung != 0 || intent->acting != 0)
        return 0;
    /* one about a body names it; one about a cell has it on the map */
    if (on_body(intent) ? intent->target >= world->bodies || intent->x != 0 || intent->y != 0
                        : intent->target != 0 || intent->target_id != 0
                              || !on_map(world->width, world->height, intent->x, intent->y))
        return 0;
    if (intent->walked ? intent->walk_at > world->now : intent->walk_at != 0)
        return 0;
    return intent->due >= world->now;
}

/* A body the world's own rules could have left: on the map, known direction, cause and speed,
 * steps within the path and nothing behind them, a record made no later than `now` and a step
 * that is not overdue, and a walk's own state only while it walks. */
static int body_valid(const struct Body *body, const struct BodyWorld *world)
{
    const struct BodyMove *move = &body->move;
    int i;

    if (body->used != 1 || move->at_tick > world->now || body->cell_tick > move->at_tick)
        return 0;
    if (!act_valid(&body->act, world) || !intent_valid(&body->intent, world))
        return 0;
    if (!on_map(world->width, world->height, move->x, move->y))
        return 0;
    if (move->dir > 7 || move->cause > BODY_CAUSE_PLACED || move->steps > BODY_PATH_STEPS
        || move->speed_ms > BODY_MAX_SPEED_MS)
        return 0;
    if (!is_zero(move->reserved, sizeof move->reserved) || body->walk.reserved != 0)
        return 0;
    for (i = 0; i < BODY_PATH_STEPS; i++)
        if (i < move->steps ? move->path[i] > 7 : move->path[i] != 0)
            return 0;
    if (body->can_move > world->now && body->can_move - world->now > BODY_MAX_DELAY_MS)
        return 0;
    if (move->steps == 0)
        return move->land_ms == 0 && is_zero(&body->walk, sizeof body->walk);
    if (body->walk.goal > GOAL_BODY || body->walk.range > BODY_MAX_RANGE)
        return 0;
    /* a goal has only what its kind has: no range for a cell, no body but for a chase */
    if (body->walk.goal == GOAL_CELL ? body->walk.range != 0 : 0)
        return 0;
    if (body->walk.goal == GOAL_BODY ? body->walk.target >= world->bodies || body->walk.range == 0 || body->walk.attack > 1
                                     : body->walk.target != 0 || body->walk.target_id != 0 || body->walk.attack != 0)
        return 0;
    return landing(move) >= world->now && body->walk.change <= 1
           && on_map(world->width, world->height, body->walk.to_x, body->walk.to_y);
}

int body_world_check(const void *arena, size_t size)
{
    const struct BodyWorld *world = arena;
    const struct Body *body;
    const struct BodyEvent *events;
    uint64_t last = 0;
    size_t need;
    uint32_t i, place, asked;

    if (arena == NULL || ((uintptr_t)arena & 7u) != 0 || size < sizeof *world)
        return BODY_BAD_WORLD;
    if (world->magic != BODY_MAGIC || world->version != BODY_RULES_VERSION || world->request_steps < 1
        || world->request_steps > BODY_PATH_STEPS)
        return BODY_BAD_WORLD;
    asked = world->asked;
    need = body_world_size(world->bodies, world->events);
    if (need == 0 || need > size || world->size != need)
        return BODY_BAD_WORLD;
    if (world->width < 1 || world->height < 1 || world->width > BODY_MAX_SIDE || world->height > BODY_MAX_SIDE)
        return BODY_BAD_WORLD;
    if (world->event_first >= world->events || world->event_count > world->events)
        return BODY_BAD_WORLD;
    body = slots_of(world);
    for (i = 0; i < world->bodies; i++) {
        if (body[i].used == 0 ? !is_zero(&body[i], sizeof body[i]) : !body_valid(&body[i], world))
            return BODY_BAD_WORLD;
        if (body[i].used && body[i].act.asking)
            asked--;
        if (body[i].used && body[i].intent.asking)
            asked--;
    }
    /* as many questions as bodies that ask */
    if (asked != 0)
        return BODY_BAD_WORLD;
    events = ring_of(world);
    for (i = 0; i < world->events; i++) {
        const struct BodyEvent *event = &events[i];

        /* how far behind the oldest waiting event this entry is */
        place = (i + world->events - world->event_first) % world->events;
        if (place >= world->event_count) {
            if (!is_zero(event, sizeof *event))
                return BODY_BAD_WORLD;
        } else if (event->kind < BODY_EVENT_MOVE || event->kind > BODY_EVENT_UNATTACKABLE || event->slot >= world->bodies
                   || event->tick > world->now || !on_map(world->width, world->height, event->x, event->y)
                   || event->dir > 7 || event->reserved != 0
                   /* an intent's events carry its number where the others name a slot */
                   || ((event->kind < BODY_EVENT_ASK_INTENT || event->kind > BODY_EVENT_INTENT_END)
                       && event->other >= world->bodies)
                   || !is_zero(event->reserved2, sizeof event->reserved2)) {
            return BODY_BAD_WORLD;
        }
    }
    for (i = 0; i < world->event_count; i++) {
        const struct BodyEvent *event = &events[(world->event_first + i) % world->events];

        if (i > 0 && event->tick < last)
            return BODY_BAD_WORLD;
        last = event->tick;
    }
    return BODY_OK;
}

uint64_t body_now(const struct BodyWorld *world)
{
    return world->now;
}

void body_set_cancel_penalty(struct BodyWorld *world, uint32_t ms)
{
    world->cancel_penalty_ms = ms;
}

void body_set_request_steps(struct BodyWorld *world, uint32_t steps)
{
    world->request_steps = steps < 1 ? 1 : steps > BODY_PATH_STEPS ? BODY_PATH_STEPS : steps;
}

uint32_t body_asked(const struct BodyWorld *world)
{
    return world->asked;
}

/* The body an input names by slot and id, or NULL. */
static struct Body *named(struct BodyWorld *world, const struct BodyInput *input)
{
    struct Body *body;

    if (input->slot >= world->bodies)
        return NULL;
    body = &slots(world)[input->slot];
    return body->used && body->id == input->id ? body : NULL;
}

static uint32_t event_room(const struct BodyWorld *world)
{
    return world->events - world->event_count;
}

static struct BodyEvent *push_event(struct BodyWorld *world, uint64_t tick, uint16_t kind, const struct Body *body)
{
    struct BodyEvent *event = &ring(world)[(world->event_first + world->event_count) % world->events];

    event->tick = tick;
    event->id = body->id;
    event->kind = kind;
    event->slot = (uint16_t)(body - slots(world));
    event->x = body->move.x;
    event->y = body->move.y;
    event->dir = body->move.dir;
    world->event_count++;
    return event;
}

/* An event about a body and another, or with a value. */
static void push_about(struct BodyWorld *world, uint64_t tick, uint16_t kind, const struct Body *body,
                       const struct Body *other, uint16_t value)
{
    struct BodyEvent *event = push_event(world, tick, kind, body);

    event->value = value;
    if (other != NULL) {
        event->other = (uint16_t)(other - slots(world));
        event->other_id = other->id;
    }
}

/* The body stands on (x, y) from `tick` on: no steps, no walk. */
static void stand(struct Body *body, uint64_t tick, int x, int y, uint8_t dir, uint8_t cause)
{
    uint16_t speed = body->move.speed_ms;

    zero(&body->move, sizeof body->move);
    zero(&body->walk, sizeof body->walk);
    body->move.at_tick = tick;
    body->move.x = (int16_t)x;
    body->move.y = (int16_t)y;
    body->move.speed_ms = speed;
    body->move.dir = dir;
    body->move.cause = cause;
}

/* The server's path between two cells: its directions in `dirs` and their count, 0 when the
 * cells are the same, -1 without a path of at most max_steps. */
static int find_path(const struct BodyWorld *world, const struct BodyGround *ground, int x0, int y0, int x1, int y1,
                     int max_steps, uint8_t *dirs)
{
    struct GridPathMap map;
    struct GridPathStep steps[BODY_PATH_STEPS];
    int count, i;

    map.width = world->width;
    map.height = world->height;
    map.walkable = ground->walkable;
    count = grid_path_search(&map, x0, y0, x1, y1, steps, max_steps, ground->scratch, BODY_SCRATCH_SIZE);
    for (i = 0; i < count; i++)
        dirs[i] = (uint8_t)steps[i].dir;
    return count;
}

static int within(int ax, int ay, int bx, int by, int range)
{
    int dx = ax > bx ? ax - bx : bx - ax, dy = ay > by ? ay - by : by - ay;

    return (dx > dy ? dx : dy) <= range;
}

/* The cell a goal is on now: its own, or the one the body it is after is on. 0 when that body
 * is gone. */
static int goal_cell(const struct BodyWorld *world, const struct BodyWalk *goal, int *x, int *y)
{
    const struct Body *target;

    if (goal->goal != GOAL_BODY) {
        *x = goal->to_x;
        *y = goal->to_y;
        return 1;
    }
    if (goal->target >= world->bodies)
        return 0;
    target = &slots_of(world)[goal->target];
    if (!target->used || target->id != goal->target_id)
        return 0;
    *x = target->move.x;
    *y = target->move.y;
    return 1;
}

/* The cell a walk to a body on (x, y), which no walk ends on (an Ice Wall's), heads for from
 * (cx, cy): the one beside it towards the walker, or else the first beside it that a walk ends
 * on, row by row from (x - 1, y - 1) (unit_can_reach_bl). 0 when there is none. */
static int beside_cell(const struct BodyWorld *world, const struct BodyGround *ground, int cx, int cy, int x, int y,
                       int *bx, int *by)
{
    int i;

    *bx = x - (x > cx) + (x < cx);
    *by = y - (y > cy) + (y < cy);
    if (walkable(world, ground, *bx, *by))
        return 1;
    for (i = 0; i < 9; i++) {
        *bx = x - 1 + i % 3;
        *by = y - 1 + i / 3;
        if (walkable(world, ground, *bx, *by))
            return 1;
    }
    return 0;
}

/* The way of a body from the cell it is on to a goal, at most max_steps long: the count of its
 * steps in `dirs`; 0 when it is there already (on the cell, or within the range); -1 without a
 * way, or when the body it is after is gone. A way to within a range ends on the first of its
 * cells that is within it. The goal's cell is brought up to where its body is now; a body on a
 * cell no walk ends on is walked to by the cell beside it. */
static int find_way(const struct BodyWorld *world, const struct BodyGround *ground, const struct Body *body,
                    struct BodyWalk *goal, int max_steps, uint8_t *dirs)
{
    int x, y, cx = body->move.x, cy = body->move.y, count, i, px, py;

    if (!goal_cell(world, goal, &x, &y))
        return -1;
    goal->to_x = (int16_t)x;
    goal->to_y = (int16_t)y;
    if (goal->goal != GOAL_CELL && within(cx, cy, x, y, goal->range))
        return 0;
    px = x;
    py = y;
    if (goal->goal == GOAL_BODY && !walkable(world, ground, x, y) && !beside_cell(world, ground, cx, cy, x, y, &px, &py))
        return -1;
    count = find_path(world, ground, cx, cy, px, py, max_steps, dirs);
    if (count <= 0 || goal->goal == GOAL_CELL)
        return count;
    for (i = 0; i < count; i++) {
        cx += DIR_X[dirs[i]];
        cy += DIR_Y[dirs[i]];
        if (within(cx, cy, x, y, goal->range))
            return i + 1;
    }
    return count;
}

/* The body walks `count` steps from the cell it is on, towards `goal`. Its first step starts
 * at `tick`, or when its move clock lets it. Without steps it stands. */
static void take_path(struct Body *body, uint64_t tick, const uint8_t *dirs, int count, const struct BodyWalk *goal)
{
    uint64_t start = body->can_move > tick ? body->can_move : tick;
    struct BodyWalk walk = *goal;
    int i;

    body->move.at_tick = tick;
    body->move.cause = BODY_CAUSE_WALKED;
    body->move.steps = (uint8_t)count;
    for (i = 0; i < BODY_PATH_STEPS; i++)
        body->move.path[i] = i < count ? dirs[i] : 0;
    body->move.land_ms = count > 0 ? (uint16_t)(start - tick + step_ms(dirs[0], body->move.speed_ms)) : 0;
    zero(&body->walk, sizeof body->walk);
    if (count > 0) {
        walk.change = 0;
        body->walk = walk;
    }
}

/* A new walk from the cell the body is on towards its walk's goal, begun at `tick`. Returns
 * the count of its steps; 0 when the body is there and stands; -1, with the body as it was,
 * when there is no way. */
static int walk_again(const struct BodyWorld *world, const struct BodyGround *ground, struct Body *body, uint64_t tick)
{
    uint8_t dirs[BODY_PATH_STEPS];
    struct BodyWalk goal = body->walk;
    int count = find_way(world, ground, body, &goal, BODY_PATH_STEPS, dirs);

    if (count >= 0)
        take_path(body, tick, dirs, count, &goal);
    return count;
}

/* The record's first step lands: the record of the cell it leads to, from the landing on. */
static void land_step(struct BodyMove *move)
{
    uint8_t dir = (uint8_t)(move->path[0] & 7u);
    int i;

    move->at_tick = landing(move);
    move->x = (int16_t)(move->x + DIR_X[dir]);
    move->y = (int16_t)(move->y + DIR_Y[dir]);
    move->dir = dir;
    for (i = 1; i < BODY_PATH_STEPS; i++)
        move->path[i - 1] = move->path[i];
    move->path[BODY_PATH_STEPS - 1] = 0;
    move->steps--;
    move->land_ms = move->steps > 0 ? step_ms(move->path[0], move->speed_ms) : 0;
}

void body_move_at(const struct BodyMove *record, uint64_t tick, struct BodyMove *at)
{
    *at = *record;
    if (at->steps > BODY_PATH_STEPS)
        at->steps = BODY_PATH_STEPS;
    while (at->steps > 0 && landing(at) <= tick)
        land_step(at);
}

/* The intent and the attack, further down: what a walk does to an intent, and to the attack
 * that began it. */
static void intent_look(struct BodyWorld *world, struct Body *body, uint64_t tick);
static void chase_ended(struct BodyWorld *world, struct Body *body, const struct BodyWalk *was, uint64_t tick);
static void intent_swung(struct BodyWorld *world, struct Body *body, uint64_t tick);
static void intent_walk_lost(struct BodyWorld *world, struct Body *body, uint64_t tick);
static int intent_is_after(const struct BodyWorld *world, const struct Body *body, const struct Body *target);

/* The step a body has under way lands now (unit_walk_toxy_timer). A cell that closed since
 * the path was found is not entered: the body looks for another way from where it is, and
 * stands when there is none. On the new cell a goal that waits is taken, and a body that is
 * after another looks where that one is now: within the range it stands, and if the other
 * has moved it finds its way anew. Without a way, or with the other gone, it stands there and
 * the caller hears of it. A walk the attack began after its target that ends within its range
 * ends in the attack's look. An intent looks from every cell the body comes onto. The caller has
 * room for five events. */
static void land(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body)
{
    struct BodyMove *move = &body->move;
    uint64_t tick = landing(move);
    uint8_t dir = move->path[0];
    int x = move->x + DIR_X[dir], y = move->y + DIR_Y[dir];
    int changed = body->walk.change, more = move->steps > 1, goal_x, goal_y, way;
    struct BodyWalk was;

    if (!walkable(world, ground, x, y)) {
        if (walk_again(world, ground, body, tick) <= 0)
            stand(body, tick, move->x, move->y, move->dir, BODY_CAUSE_WALKED);
        push_event(world, tick, BODY_EVENT_MOVE, body);
        return;
    }
    body->cell_tick = tick;
    was = body->walk;
    /* nothing waits and the goal is where it was: the walk goes on as its record says */
    if (!changed && (body->walk.goal != GOAL_BODY
                     || (goal_cell(world, &body->walk, &goal_x, &goal_y) && goal_x == body->walk.to_x
                         && goal_y == body->walk.to_y))) {
        land_step(move);
        if (move->steps == 0)
            zero(&body->walk, sizeof body->walk);
        push_event(world, tick, BODY_EVENT_CELL, body);
        if (move->steps == 0)
            chase_ended(world, body, &was, tick);
        intent_look(world, body, tick);
        return;
    }
    move->x = (int16_t)x;
    move->y = (int16_t)y;
    move->dir = dir;
    push_event(world, tick, BODY_EVENT_CELL, body);
    way = walk_again(world, ground, body, tick);
    if (way > 0) {
        push_event(world, tick, BODY_EVENT_MOVE, body);
        intent_look(world, body, tick);
        return;
    }
    stand(body, tick, x, y, dir, BODY_CAUSE_WALKED);
    /* a body that stops where its record ended anyway has said nothing new */
    if (way < 0 || changed || more)
        push_event(world, tick, BODY_EVENT_MOVE, body);
    if (way < 0) {
        push_event(world, tick, BODY_EVENT_NO_PATH, body);
        intent_walk_lost(world, body, tick);
    } else {
        chase_ended(world, body, &was, tick);
    }
    intent_look(world, body, tick);
}

/* ---------------------------------------------------------------------------------------
 * The attack: unit.c's unit_attack, unit_attack_timer_sub and unit_stop_attack, with the
 * project's windup (a swing begins, and lands `windup` ms later if its target is still in
 * reach). What the fork reads from the game there, the core asks: see BODY_EVENT_ASK_*.
 * ------------------------------------------------------------------------------------- */

/* check_distance: the rounded distance is within the range. round(sqrt(n)) <= r is
 * 4n < (2r + 1)^2. */
static int within_circle(int dx, int dy, int range)
{
    int64_t n = (int64_t)dx * dx + (int64_t)dy * dy, side = 2 * (int64_t)range + 1;

    return 4 * n < side * side;
}

/* check_distance_client: the truncated distance, as the client measures an attack it asks
 * for. trunc(sqrt(n)) <= r is n < (r + 1)^2. */
static int within_client(int dx, int dy, int range)
{
    int64_t n = (int64_t)dx * dx + (int64_t)dy * dy, side = (int64_t)range + 1;

    return n < side * side;
}

/* distance: 123/128 of the longer side and 51/128 of the shorter. */
static int apart(int dx, int dy)
{
    int longer, shorter;

    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    if (dx == 0)
        return dy;
    if (dy == 0)
        return dx;
    longer = dx > dy ? dx : dy;
    shorter = dx > dy ? dy : dx;
    return ((longer << 8) + (longer << 3) - (longer << 4) - (longer << 1) + (shorter << 7) - (shorter << 5)
            + (shorter << 3) - (shorter << 1)) >> 8;
}

/* map_calc_dir: the direction of a cell as seen from another; its own cell is east. */
static uint8_t dir_towards(int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return 6;
    if (dx >= 0 && dy >= 0)
        return (uint8_t)(dx * 2 < dy || dx == 0 ? 0 : dx > dy * 2 + 1 || dy == 0 ? 6 : 7);
    if (dx >= 0 && dy <= 0)
        return (uint8_t)(dx * 2 < -dy || dx == 0 ? 4 : dx > -dy * 2 + 1 || dy == 0 ? 6 : 5);
    if (dx <= 0 && dy <= 0)
        return (uint8_t)(dx * 2 > dy || dx == 0 ? 4 : dx < dy * 2 + 1 || dy == 0 ? 2 : 3);
    return (uint8_t)(-dx * 2 < dy || dx == 0 ? 0 : -dx > dy * 2 + 1 || dy == 0 ? 2 : 1);
}

static struct Body *body_named(struct BodyWorld *world, uint32_t slot, uint32_t id)
{
    struct Body *body;

    if (slot >= world->bodies)
        return NULL;
    body = &slots(world)[slot];
    return body->used && body->id == id ? body : NULL;
}

static void arm(struct Body *body, uint64_t tick)
{
    body->act.armed = 1;
    body->act.attack_due = tick;
}

static void disarm(struct Body *body)
{
    body->act.armed = 0;
    body->act.attack_due = 0;
}

/* unit_unattackable. */
static void forget_target(struct Body *body)
{
    body->act.attack_continue = 0;
    body->act.has_target = 0;
    body->act.target = 0;
    body->act.target_id = 0;
}

/* A look of the attack's timer found no target it may fight: unit_unattackable, which the game
 * is told (a monster gives up the one it was after). */
static void unattackable(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    forget_target(body);
    push_event(world, tick, BODY_EVENT_UNATTACKABLE, body);
}

/* unit_cancel_combo's tail: the next swing may begin at `tick`, and an attack that waits
 * looks then. */
static void rearm(struct Body *body, uint64_t tick)
{
    body->act.attackable = tick;
    if (body->act.armed)
        body->act.attack_due = tick;
}

static void close_swing(struct Body *body)
{
    body->act.swing_open = 0;
    body->act.impact_due = 0;
    body->act.swing_target = 0;
    body->act.swing_target_id = 0;
    body->act.swing_reach = 0;
    body->act.held_ms = 0;
}

/* Ends the swing in its windup without a hit. The rate clock goes back to now and the
 * penalty. A swing that a command ends was not made, and holds the body from walking no longer
 * (not the fork's rule, which has no windup to end a swing in): the move clock, which the swing
 * put at the end of its attack motion, goes back to where the swing found it, unless something
 * else has held the body since. A walk that was asked for in the windup and waits for that
 * clock starts now, or when what held the body before lets it, which is a new record. The
 * caller has room for two events. Returns 0 when no swing is open. */
static int cancel_swing(struct BodyWorld *world, struct Body *body, uint16_t reason, uint64_t tick)
{
    struct BodyAct *act = &body->act;
    struct BodyMove *move = &body->move;
    uint64_t began, held_to, start;
    int waits;

    if (!act->swing_open)
        return 0;
    began = act->impact_due - act->windup;
    held_to = began + act->amotion;
    if (reason == BODY_CANCEL_COMMAND && (act->flags & BODY_FIGHTS_HOLDS) && body->can_move == held_to && held_to > tick) {
        body->can_move = began + act->held_ms;
        waits = move->steps > 0 && landing(move) == held_to + step_ms(move->path[0], move->speed_ms);
        close_swing(body);
        rearm(body, tick + world->cancel_penalty_ms);
        push_about(world, tick, BODY_EVENT_SWING_CANCEL, body, NULL, reason);
        if (waits) {
            start = body->can_move > tick ? body->can_move : tick;
            move->at_tick = tick;
            move->land_ms = (uint16_t)(start - tick + step_ms(move->path[0], move->speed_ms));
            push_event(world, tick, BODY_EVENT_MOVE, body);
        }
        return 1;
    }
    close_swing(body);
    rearm(body, tick + world->cancel_penalty_ms);
    push_about(world, tick, BODY_EVENT_SWING_CANCEL, body, NULL, reason);
    return 1;
}

static void ask(struct BodyWorld *world, struct Body *body, uint8_t question, int requested, const struct Body *target,
                uint64_t tick)
{
    body->act.asking = question;
    body->act.requested = (uint8_t)(requested != 0);
    world->asked++;
    if (question == ASK_CAST) {
        push_event(world, tick, BODY_EVENT_ASK_CAST, body);
        return;
    }
    body->act.aim_x = target->move.x;
    body->act.aim_y = target->move.y;
    /* a hit that is asked for at the end of a windup says so */
    push_about(world, tick,
               question == ASK_SWING   ? BODY_EVENT_ASK_SWING
               : question == ASK_BEGIN ? BODY_EVENT_ASK_BEGIN
                                       : BODY_EVENT_ASK_HIT,
               body, target, question == ASK_IMPACT);
}

/* The body sets out on a way that was found for `goal` (find_way), of `count` steps: at once
 * from the cell it stands on or came onto at this very tick, which is a new record; with a step
 * under way, when that step lands. One that stands within the range it was to come within is
 * left as it is. */
static void set_out(struct BodyWorld *world, struct Body *body, struct BodyWalk *goal, const uint8_t *dirs, int count,
                    uint64_t tick)
{
    if (body->move.steps > 0 && body->cell_tick != tick) {
        goal->change = 1;
        body->walk = *goal;
        return;
    }
    if (count == 0 && goal->goal != GOAL_CELL && body->move.steps == 0)
        return;
    take_path(body, tick, dirs, count, goal);
    push_event(world, tick, BODY_EVENT_MOVE, body);
}

/* unit_walktobl with the attack going on: the body walks after `target` until it is within
 * `range` of it, as by BODY_INPUT_CHASE, and the attack looks again when it gets there. A body
 * that stands within that range already (out of the line of fire) takes one step on the way to
 * the cell beside its target, as unit_walktoxy_sub moves at least one cell when asked to, and
 * the attack looks when it lands. Returns 0, and does nothing, when there is no way. */
static int walk_after(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body,
                      const struct Body *target, uint16_t range, uint64_t tick)
{
    uint8_t dirs[BODY_PATH_STEPS];
    struct BodyWalk goal, beside;
    int count;

    zero(&goal, sizeof goal);
    goal.goal = GOAL_BODY;
    goal.attack = 1;
    goal.range = range > 0 ? range : 1;
    goal.target = (uint16_t)(target - slots(world));
    goal.target_id = target->id;
    count = find_way(world, ground, body, &goal, BODY_PATH_STEPS, dirs);
    if (count < 0)
        return 0;
    if (count == 0 && body->move.steps == 0) {
        beside = goal;
        beside.range = 1;
        count = find_way(world, ground, body, &beside, BODY_PATH_STEPS, dirs);
        if (count < 0)
            return 0;
        if (count > 1)
            count = 1;
    }
    set_out(world, body, &goal, dirs, count, tick);
    return 1;
}

/* Whether a swing of `body` reaches `target` now (unit_attack_timer_sub's checks, which the
 * intent of an attack makes too). `walks`: the target walks by a walk that is not this
 * world's; one that walks is reached a cell further. `line`: no wall is in the line of fire.
 * Out of reach a player is told so (FAR), a monster beyond its range goes after its target
 * (CHASE), and the rest is out of the line of fire or, for another unit, of its range too
 * (BLOCKED). */
enum { REACH_OK = 0, REACH_FAR = 1, REACH_BLOCKED = 2, REACH_CHASE = 3 };

static int reach(const struct Body *body, const struct Body *target, int walks, int line)
{
    int dx = body->move.x - target->move.x, dy = body->move.y - target->move.y;
    int range = body->act.range + (target->move.steps > 0 || walks), far = apart(dx, dy);

    /* a player is told when its target is beyond the distance its client measures */
    if (body->act.kind == BODY_FIGHTER_PLAYER && !within_client(dx, dy, range))
        return REACH_FAR;
    /* battle_check_range: within the range (check_distance), and beyond two cells a line of fire
       no longer than the view */
    if (!within_circle(dx, dy, range))
        return body->act.kind == BODY_FIGHTER_MONSTER ? REACH_CHASE : REACH_BLOCKED;
    if (far >= 2 && (far > BODY_SIGHT || !line))
        return REACH_BLOCKED;
    return REACH_OK;
}

/* unit_attack_timer_sub, up to what the game has to say: a swing still in its windup lands
 * before the next is looked at; an attack without its target ends. `requested`: the look is
 * unit_attack's own, and not the timer's. */
static void attack_look(struct BodyWorld *world, struct Body *body, int requested, uint64_t tick)
{
    struct BodyAct *act = &body->act;
    struct Body *target;

    disarm(body);
    if (act->swing_open) {
        if (act->impact_due > act->attackable)
            act->attackable = act->impact_due;
        arm(body, act->attackable);
        return;
    }
    target = act->has_target ? body_named(world, act->target, act->target_id) : NULL;
    if (target == NULL) {
        if (!requested)
            unattackable(world, body, tick);
        return;
    }
    ask(world, body, ASK_SWING, requested, target, tick);
}

/* A swing began or was made: the attack delay runs, and a body whose swing holds its walk does
 * not walk during its attack motion (attack_walk_delay). */
static void swing_begun(struct Body *body, uint64_t tick)
{
    if (tick + body->act.adelay > body->act.attackable)
        body->act.attackable = tick + body->act.adelay;
    if ((body->act.flags & BODY_FIGHTS_HOLDS) && tick + body->act.amotion > body->can_move)
        body->can_move = tick + body->act.amotion;
}

/* The end of a look that swung or had nothing to wait for: an attack that goes on turns to
 * its target and looks again when the attack delay is over. */
static void attack_goes_on(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    struct BodyAct *act = &body->act;
    const struct Body *target = act->has_target ? body_named(world, act->target, act->target_id) : NULL;
    int x = target != NULL ? target->move.x : act->aim_x, y = target != NULL ? target->move.y : act->aim_y;
    uint8_t dir;

    if (act->attack_continue) {
        dir = dir_towards(x - body->move.x, y - body->move.y);
        if (dir != body->move.dir) {
            body->move.dir = dir;
            push_event(world, tick, BODY_EVENT_FACE, body);
        }
        arm(body, act->attackable);
    }
    push_about(world, tick, BODY_EVENT_ATTACKING, body, target, act->attack_continue);
}

/* The swing at `target` begins (battle_weapon_attack): without a windup the hit is made now,
 * which the game is asked for, and only one that was made counts; with one the swing opens and
 * lands that much later. Returns 1 when the swing opened, 0 when a question waits. */
static int swing(struct BodyWorld *world, struct Body *body, const struct Body *target, int requested, uint64_t tick)
{
    struct BodyAct *act = &body->act;
    int far;

    if (act->windup == 0) {
        ask(world, body, ASK_HIT_NOW, requested, target, tick);
        return 0;
    }
    /* battle_delay_damage's distance, where the target was and ten cells; a monster's swing
       reaches its range and a cell more (the project's monster windup) */
    far = act->kind == BODY_FIGHTER_MONSTER ? act->range + 1
                                             : apart(body->move.x - target->move.x, body->move.y - target->move.y) + 10;
    act->swing_open = 1;
    act->swing_target = act->target;
    act->swing_target_id = act->target_id;
    act->impact_due = tick + act->windup;
    act->swing_reach = (uint16_t)(far > 0xFFFF ? 0xFFFF : far);
    act->held_ms = body->can_move > tick ? (uint32_t)(body->can_move - tick) : 0;
    push_about(world, tick, BODY_EVENT_SWING, body, target, act->windup);
    swing_begun(body, tick);
    intent_swung(world, body, tick);
    return 1;
}

/* The rest of unit_attack_timer_sub, with the game's answer. The act clock holds no swing (the
 * fork's skill_delay_attack_enable); a cast does, the world's own or one the game says of
 * (CASTING), unless the body may swing while it casts (Free Cast). */
static void swing_answered(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body, int requested,
                           unsigned answer, uint64_t tick)
{
    struct BodyAct *act = &body->act;
    struct Body *target = act->has_target ? body_named(world, act->target, act->target_id) : NULL;
    int casting = (answer & BODY_ANSWER_CASTING) || (act->casting && !(act->flags & BODY_FIGHTS_FREE_CAST));
    int reached, under_way;

    if (target == NULL || !(answer & BODY_ANSWER_CAN_FIGHT) || casting) {
        if (!requested)
            unattackable(world, body, tick);
        return;
    }
    reached = reach(body, target, (answer & BODY_ANSWER_WALKS) != 0, (answer & BODY_ANSWER_LINE) != 0);
    if (reached == REACH_FAR) {
        /* an attack intent takes it over: it looks, and walks the body after its target */
        if (intent_is_after(world, body, target))
            intent_look(world, body, tick);
        else
            push_about(world, tick, BODY_EVENT_FAR, body, target, 0);
        return;
    }
    /* a monster goes after a target beyond its range, whether the attack goes on or not; the game
       finds its way (unit_walktobl with walk_easy | 2) */
    if (reached == REACH_CHASE) {
        push_about(world, tick, BODY_EVENT_CHASE, body, target, act->chase_range);
        return;
    }
    if (reached == REACH_BLOCKED) {
        if (act->attack_continue) {
            if (act->chase_range > 2)
                act->chase_range -= 2;
            if (act->kind == BODY_FIGHTER_MONSTER)
                push_about(world, tick, BODY_EVENT_CHASE, body, target, act->chase_range);
            else
                walk_after(world, ground, body, target, act->chase_range, tick);
        }
        return;
    }
    under_way = step_under_way(&body->move, tick);
    if (requested && act->kind == BODY_FIGHTER_PLAYER && !under_way)
        push_event(world, tick, BODY_EVENT_FIX, body);
    if (act->attackable <= tick) {
        if (under_way) {
            /* The step lands first (not the fork's rule, which stops the body on the cell it
               is leaving): the walk ends with it, and the attack looks again on that cell. */
            if (end_walk_with_step(body, tick))
                push_event(world, tick, BODY_EVENT_MOVE, body);
            arm(body, landing(&body->move));
            return;
        }
        if (body->move.steps > 0) {
            stand(body, tick, body->move.x, body->move.y, body->move.dir, BODY_CAUSE_WALKED);
            push_event(world, tick, BODY_EVENT_MOVE, body);
        }
        if (act->windup != 0 && !(answer & BODY_ANSWER_READY)) {
            push_about(world, tick, BODY_EVENT_UNREADY, body, target, 0);
            return;
        }
        /* a monster's swing is the game's to begin or not (a skill in its place) */
        if (act->kind == BODY_FIGHTER_MONSTER) {
            ask(world, body, ASK_BEGIN, requested, target, tick);
            return;
        }
        if (!swing(world, body, target, requested, tick))
            return;
    }
    attack_goes_on(world, body, tick);
}

/* The rest of a monster's swing, with the game's answer: no swing when the game did something
 * in its place, and the attack looks no more (mob_use_skill's `return 1`). */
static void begin_answered(struct BodyWorld *world, struct Body *body, int requested, unsigned answer, uint64_t tick)
{
    struct BodyAct *act = &body->act;
    const struct Body *target = act->has_target ? body_named(world, act->target, act->target_id) : NULL;

    if (!(answer & BODY_ANSWER_BEGIN) || target == NULL)
        return;
    if (swing(world, body, target, requested, tick))
        attack_goes_on(world, body, tick);
}

static void begin_attack(struct BodyWorld *world, struct Body *body, uint16_t slot, uint32_t id, int goes_on,
                         uint64_t tick);

/* unit_walk_toxy_timer's end of a walk after a unit to attack it: within its range the place is
 * told again and the attack looks at the one it was after, as unit_attack does. Not when the
 * attack has another target or none by then, nor when an attack intent is after that body: the
 * intent attacks itself. The caller has room for two events. */
static void chase_ended(struct BodyWorld *world, struct Body *body, const struct BodyWalk *was, uint64_t tick)
{
    struct BodyAct *act = &body->act;
    const struct Body *target;

    if (was->goal != GOAL_BODY || !was->attack)
        return;
    target = body_named(world, was->target, was->target_id);
    if (target == NULL || !act->has_target || act->target != was->target || act->target_id != was->target_id
        || act->asking || intent_is_after(world, body, target))
        return;
    push_event(world, tick, BODY_EVENT_FIX, body);
    begin_attack(world, body, was->target, was->target_id, 1, tick);
}

/* The swing lands (battle_delay_damage's checks): on a target that is still there and within
 * reach the hit is made, which the game does and answers; otherwise the swing is lost. */
static void swing_lands(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    struct BodyAct *act = &body->act;
    struct Body *target = body_named(world, act->swing_target, act->swing_target_id);

    if (target == NULL
        || !within_circle(body->move.x - target->move.x, body->move.y - target->move.y, act->swing_reach)) {
        cancel_swing(world, body, BODY_CANCEL_LOST, tick);
        return;
    }
    /* closed before the hit: what the hit sets off (a death, a stop) finds no swing to cancel */
    close_swing(body);
    ask(world, body, ASK_IMPACT, 0, target, tick);
}

static void hit_answered(struct BodyWorld *world, struct Body *body, uint8_t question, unsigned answer, uint64_t tick)
{
    if (question == ASK_HIT_NOW) {
        if (!(answer & BODY_ANSWER_HIT))
            return;
        swing_begun(body, tick);
        intent_swung(world, body, tick);
        attack_goes_on(world, body, tick);
    } else if (!(answer & BODY_ANSWER_HIT)) {
        /* nothing was swung after all: the ammunition went, or the target died, in the windup */
        rearm(body, tick + world->cancel_penalty_ms);
        push_about(world, tick, BODY_EVENT_SWING_CANCEL, body, NULL, BODY_CANCEL_LOST);
    }
}

static int input_fighter(struct Body *body, const struct BodyInput *input)
{
    /* an attack delay of 0 would swing without end at one tick */
    if (input->value == 0 || input->x < 0 || input->y < 0 || input->y > (int)BODY_MAX_RANGE
        || input->dir > BODY_FIGHTER_MONSTER || input->cause > (BODY_FIGHTS_HOLDS | BODY_FIGHTS_FREE_CAST))
        return BODY_BAD_VALUE;
    body->act.adelay = input->value;
    body->act.amotion = (uint16_t)input->x;
    body->act.range = (uint8_t)input->y;
    body->act.windup = input->target;
    body->act.kind = input->dir;
    body->act.flags = input->cause;
    return BODY_OK;
}

/* unit_attack, after its refusals: the body attacks the one in `slot`, again and again with
 * `goes_on`. A request for the target of the swing in its windup neither ends nor begins it
 * again: only the attack's kind follows. The caller has room for one event, and no question of
 * the attack is open. */
static void begin_attack(struct BodyWorld *world, struct Body *body, uint16_t slot, uint32_t id, int goes_on,
                         uint64_t tick)
{
    struct BodyAct *act = &body->act;

    act->has_target = 1;
    act->target = slot;
    act->target_id = id;
    act->attack_continue = (uint8_t)(goes_on != 0);
    if (goes_on)
        act->chase_range = act->range;
    if (act->swing_open && act->swing_target == slot && act->swing_target_id == id) {
        /* the attack delay may be over while the windup is not: then the attack looks now,
           and waits for the swing */
        if (act->attack_continue && !act->armed)
            arm(body, act->attackable > tick ? act->attackable : tick);
        return;
    }
    if (act->armed)
        return; /* the attack that waits takes the new target */
    if (act->attackable > tick)
        arm(body, act->attackable);
    else
        attack_look(world, body, 1, tick);
}

static int input_attack(struct BodyWorld *world, struct Body *body, const struct BodyInput *input)
{
    const struct Body *target = body_named(world, input->target, input->target_id);

    if (target == NULL || target == body)
        return BODY_BAD_SLOT;
    /* a body that was not told how it fights does not */
    if (input->value > 1 || body->act.adelay == 0)
        return BODY_BAD_VALUE;
    if (body->act.asking)
        return BODY_ANSWER_FIRST;
    if (event_room(world) < 6)
        return BODY_EVENTS_FULL;
    begin_attack(world, body, input->target, input->target_id, input->value, input->tick);
    return BODY_OK;
}

/* unit_stop_attack: the target is forgotten and the attack looks no more; a swing in its windup
 * ends too, unless it is kept. The caller has room for two events when it is not. */
static void stop_attack(struct BodyWorld *world, struct Body *body, int keep_swing, uint64_t tick)
{
    body->act.has_target = 0;
    body->act.target = 0;
    body->act.target_id = 0;
    disarm(body);
    if (!keep_swing)
        cancel_swing(world, body, BODY_CANCEL_COMMAND, tick);
}

static int input_attack_stop(struct BodyWorld *world, struct Body *body, const struct BodyInput *input)
{
    if (input->value > 2)
        return BODY_BAD_VALUE;
    if (input->value == 2) {
        forget_target(body);
        return BODY_OK;
    }
    if (!input->value && body->act.swing_open && event_room(world) < 2)
        return BODY_EVENTS_FULL;
    stop_attack(world, body, input->value, input->tick);
    return BODY_OK;
}

static void intent_acted(struct BodyWorld *world, struct Body *body, uint64_t tick);

static int input_answer(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body,
                        const struct BodyInput *input)
{
    uint8_t question = body->act.asking;
    int requested = body->act.requested;

    if (question == ASK_NONE)
        return BODY_BAD_VALUE;
    /* what the answer makes, with what an intent does once its attack's first look is answered:
       it ends when the attack lost its target */
    if (event_room(world) < 6)
        return BODY_EVENTS_FULL;
    body->act.asking = ASK_NONE;
    body->act.requested = 0;
    world->asked--;
    if (question == ASK_SWING) {
        swing_answered(world, ground, body, requested, input->value, input->tick);
    } else if (question == ASK_BEGIN) {
        begin_answered(world, body, requested, input->value, input->tick);
    } else if (question == ASK_CAST) {
        /* skill_castend_id: a skill that was carried out puts its delay under the clock, which
           the cast's start may already have put later; one that was not gives the clock back */
        if (!(input->value & BODY_ANSWER_ACTED) || input->tick + input->target_id > body->act.can_act)
            body->act.can_act = input->tick + input->target_id;
    } else {
        hit_answered(world, body, question, input->value, input->tick);
    }
    if (!body->act.asking && body->intent.acting)
        intent_acted(world, body, input->tick);
    return BODY_OK;
}

/* unit_skilluse_id2 and unit_skilluse_pos2, as far as time goes: the cast, and the act clock
 * at its end or at the floor the host gives, whichever is later. */
static int input_cast(struct Body *body, const struct BodyInput *input)
{
    struct BodyAct *act = &body->act;
    uint64_t hold = input->target_id > input->value ? input->target_id : input->value;

    if (input->target_id > 0 && act->casting)
        return BODY_BAD_VALUE;
    act->can_act = input->tick + hold;
    if (input->target_id > 0) {
        act->casting = 1;
        act->cast_due = input->tick + input->target_id;
    }
    return BODY_OK;
}

/* unit_skillcastcancel. */
static int input_cast_cancel(struct Body *body, const struct BodyInput *input)
{
    if (!body->act.casting)
        return BODY_BAD_VALUE;
    body->act.casting = 0;
    body->act.cast_due = 0;
    body->act.can_act = input->tick;
    return BODY_OK;
}

/* --- the intent ---------------------------------------------------------------------------- */

static void intent_event(struct BodyWorld *world, uint64_t tick, uint16_t kind, const struct Body *body, uint16_t value,
                         uint64_t ms)
{
    struct BodyEvent *event = push_event(world, tick, kind, body);

    event->value = value;
    event->other = body->intent.seq;
    event->other_id = ms > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)ms;
}

static void intent_ask(struct BodyWorld *world, struct Body *body, uint8_t question, uint64_t tick)
{
    static const uint16_t KIND[] = {0, BODY_EVENT_ASK_INTENT, BODY_EVENT_ASK_STAND, BODY_EVENT_ASK_ACT, BODY_EVENT_ASK_WALK};

    body->intent.asking = question;
    world->asked++;
    intent_event(world, tick, KIND[question], body, question == WANT_WALK ? body->intent.walk_tries : 0, 0);
}

/* The intent is gone, with what it asked. */
static void intent_drop(struct BodyWorld *world, struct Body *body)
{
    if (body->intent.asking)
        world->asked--;
    zero(&body->intent, sizeof body->intent);
}

/* The intent ends with a word. An attack that was given up, or whose button came up after its
 * swing, goes on no more; a swing in its windup still lands. */
static void intent_end(struct BodyWorld *world, struct Body *body, uint16_t reason, uint64_t ms, uint64_t tick)
{
    int stops = body->intent.kind == BODY_WANTS_ATTACK && (reason != BODY_INTENT_DONE || body->intent.mode == BODY_ATTACK_HOLD);

    intent_event(world, tick, BODY_EVENT_INTENT_END, body, reason, ms);
    intent_drop(world, body);
    if (stops)
        stop_attack(world, body, 1, tick);
}

/* Whether the body has an attack intent that is about `target`. */
static int intent_is_after(const struct BodyWorld *world, const struct Body *body, const struct Body *target)
{
    return body->intent.has && body->intent.kind == BODY_WANTS_ATTACK && target->id == body->intent.target_id
           && target - slots_of(world) == body->intent.target;
}

/* The intent's step runs now, unless it waits for an answer: the body came onto a cell, or its
 * attack is out of reach. The caller has room for an event. */
static void intent_look(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    if (body->intent.has && !body->intent.asking)
        intent_ask(world, body, WANT_VIEW, tick);
}

/* A swing of the body began: an attack for one swing is done with it. */
static void intent_swung(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    if (!body->intent.has || body->intent.kind != BODY_WANTS_ATTACK)
        return;
    body->intent.swung = 1;
    if (body->intent.mode == BODY_ATTACK_ONCE)
        intent_end(world, body, BODY_INTENT_DONE, 0, tick);
}

/* The walk the body was on ended without a way: one its intent had asked for ends the intent. */
static void intent_walk_lost(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    if (body->intent.has && body->intent.walked)
        intent_end(world, body, BODY_INTENT_NO_PATH, 0, tick);
}

/* Where what the intent is about is: its target's cell, with the target, or its own cell. 0 when
 * its target is gone. */
static int intent_place(struct BodyWorld *world, const struct BodyIntent *intent, const struct Body **target, int *x, int *y)
{
    *target = NULL;
    if (!on_body(intent)) {
        *x = intent->x;
        *y = intent->y;
        return 1;
    }
    *target = body_named(world, intent->target, intent->target_id);
    if (*target == NULL)
        return 0;
    *x = (*target)->move.x;
    *y = (*target)->move.y;
    return 1;
}

/* Whether the body reaches what its intent is about from its cell: an attack by the attack's
 * reach; a skill within its range (check_distance) and, beyond two cells, by a line of fire no
 * longer than the view (battle_check_range); a pickup within its distance (check_distance). */
static int intent_reaches(const struct Body *body, const struct Body *target, int x, int y, unsigned sees)
{
    const struct BodyIntent *intent = &body->intent;
    int dx = body->move.x - x, dy = body->move.y - y, far = apart(dx, dy);

    if (intent->kind == BODY_WANTS_ATTACK)
        return reach(body, target, (sees & BODY_SEES_TARGET_WALKS) != 0, (sees & BODY_SEES_LINE) != 0) == REACH_OK;
    if (!within_circle(dx, dy, intent->range))
        return 0;
    return intent->kind == BODY_WANTS_PICKUP || far < 2 || (far <= BODY_SIGHT && (sees & BODY_SEES_LINE) != 0);
}

/* The attack of an attack intent has had its first look: the intent ends, refused, if the attack
 * lost its target by then. */
static void intent_acted(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    const struct BodyIntent *intent = &body->intent;
    const struct BodyAct *act = &body->act;

    body->intent.acting = 0;
    if (!(act->has_target && act->target == intent->target && act->target_id == intent->target_id))
        intent_end(world, body, BODY_INTENT_REFUSED, 0, tick);
}

/* The game lets the body of an attack intent attack: the attack begins, for one swing or to go
 * on, as the intent's mode says. A body that was not told how it fights does not, nor one whose
 * attack still waits for an answer. */
static void intent_attack(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    struct BodyIntent *intent = &body->intent;

    if (body->act.adelay == 0 || body->act.asking || body_named(world, intent->target, intent->target_id) == NULL) {
        intent_end(world, body, BODY_INTENT_REFUSED, 0, tick);
        return;
    }
    begin_attack(world, body, intent->target, intent->target_id, intent->mode != BODY_ATTACK_ONCE, tick);
    if (body->act.asking)
        intent->acting = 1;
    else
        intent_acted(world, body, tick);
}

/* In reach and ready: the act is asked for. A skill or a pickup is carried out from a cell the body
 * stands on: its walk ends there, and with a step under way it first comes onto that step's cell,
 * where it looks again. The caller has room for two events. */
static void intent_act(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    struct BodyMove *move = &body->move;

    if (body->intent.kind != BODY_WANTS_ATTACK && move->steps > 0) {
        if (step_under_way(move, tick)) {
            if (end_walk_with_step(body, tick))
                push_event(world, tick, BODY_EVENT_MOVE, body);
            return;
        }
        stand(body, tick, move->x, move->y, move->dir, BODY_CAUSE_WALKED);
        push_event(world, tick, BODY_EVENT_MOVE, body);
    }
    body->intent.walk_tries = 0;
    intent_ask(world, body, WANT_ACT, tick);
}

/* The game lets the body walk into reach: after its target, or to its
 * cell, to within the attack's range or the skill's less a cell for every walk asked for before,
 * one at the least; a pickup to within a cell of the item, and then onto it. Without a way the
 * intent ends. */
static void intent_walk(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body, uint64_t tick)
{
    struct BodyIntent *intent = &body->intent;
    const struct Body *target;
    uint8_t dirs[BODY_PATH_STEPS];
    struct BodyWalk goal;
    int count = -1, x, y, range, least = intent->kind == BODY_WANTS_PICKUP ? 0 : 1;

    zero(&goal, sizeof goal);
    if (intent_place(world, intent, &target, &x, &y)) {
        range = intent->kind == BODY_WANTS_ATTACK ? body->act.range : intent->kind == BODY_WANTS_PICKUP ? 1 : intent->range;
        range = range - intent->walk_tries > least ? range - intent->walk_tries : least;
        if (target != NULL) {
            goal.goal = GOAL_BODY;
            goal.target = intent->target;
            goal.target_id = intent->target_id;
        } else {
            goal.goal = range > 0 ? GOAL_NEAR : GOAL_CELL;
            goal.to_x = (int16_t)x;
            goal.to_y = (int16_t)y;
        }
        goal.range = (uint16_t)range;
        count = find_way(world, ground, body, &goal, BODY_PATH_STEPS, dirs);
    }
    if (count < 0) {
        intent_end(world, body, BODY_INTENT_NO_PATH, 0, tick);
        return;
    }
    intent->walk_at = tick;
    intent->walked = 1;
    intent->walk_tries++;
    set_out(world, body, &goal, dirs, count, tick);
}

/* The intent's step (the server's intentStep), with what the game sees: what to do about the
 * intent now. Reach, the attack and the walk the core reads from its own bodies. */
static void intent_seen(struct BodyWorld *world, struct Body *body, unsigned sees, uint32_t ready_ms, uint64_t tick)
{
    struct BodyIntent *intent = &body->intent;
    const struct BodyAct *act = &body->act;
    const struct Body *target;
    int x, y;

    if (!intent_place(world, intent, &target, &x, &y))
        sees &= ~BODY_SEES_TARGET;
    if (!(sees & BODY_SEES_TARGET)) {
        intent_end(world, body, BODY_INTENT_TARGET_GONE, 0, tick);
    } else if (sees & BODY_SEES_CANNOT_ACT) {
        intent_end(world, body, BODY_INTENT_CANNOT_ACT, 0, tick);
    } else if (sees & BODY_SEES_SITTING) {
        intent_ask(world, body, WANT_STAND, tick);
    } else if (sees & BODY_SEES_CASTING) {
        /* it waits for the cast */
    } else if (intent_reaches(body, target, x, y, sees)) {
        if (intent->kind == BODY_WANTS_ATTACK && act->has_target && act->target == intent->target
            && act->target_id == intent->target_id && (act->armed || act->swing_open)) {
            /* the attack runs */
        } else if (ready_ms == 0) {
            intent_act(world, body, tick);
        } else if (ready_ms > BODY_INTENT_WAIT_MS) {
            intent_end(world, body, BODY_INTENT_TOO_EARLY, ready_ms, tick);
        }
    } else if (intent->walked && body->move.steps > 0) {
        /* the walk it asked for is under way */
    } else if (act->swing_open) {
        /* the swing in its windup lands before any walk */
    } else if (intent->walked && tick - intent->walk_at < BODY_INTENT_REWALK_MS) {
        /* it asked for a walk a moment ago: not again yet */
    } else if (intent->walk_tries >= BODY_INTENT_WALKS) {
        intent_end(world, body, BODY_INTENT_NO_PATH, 0, tick);
    } else {
        intent_ask(world, body, WANT_WALK, tick);
    }
}

static int intent_answered(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body,
                           const struct BodyInput *input)
{
    struct BodyIntent *intent = &body->intent;
    uint8_t question = intent->asking;
    int ok = (input->value & BODY_INTENT_OK) != 0;

    if (question == WANT_NONE || input->target != intent->seq)
        return BODY_BAD_VALUE;
    if (event_room(world) < 2)
        return BODY_EVENTS_FULL;
    intent->asking = WANT_NONE;
    world->asked--;
    switch (question) {
    case WANT_VIEW:
        intent_seen(world, body, input->value, input->target_id, input->tick);
        break;
    case WANT_STAND:
        /* standing, the step runs again at once; a body that stays seated waits for its step */
        if (ok)
            intent_ask(world, body, WANT_VIEW, input->tick);
        break;
    case WANT_ACT:
        if (!ok)
            intent_end(world, body, BODY_INTENT_REFUSED, 0, input->tick);
        else if (intent->kind == BODY_WANTS_ATTACK)
            intent_attack(world, body, input->tick);
        else
            intent_end(world, body, BODY_INTENT_DONE, 0, input->tick);
        break;
    default:
        if (!ok)
            intent_end(world, body, (input->value & BODY_INTENT_STUCK) ? BODY_INTENT_CANNOT_ACT : BODY_INTENT_NO_PATH, 0,
                       input->tick);
        else
            intent_walk(world, ground, body, input->tick);
        break;
    }
    return BODY_OK;
}

/* The button of a Hold attack came up: done if it swung since it was set, Once otherwise. The
 * caller has room for an event. */
static void intent_release(struct BodyWorld *world, struct Body *body, uint64_t tick)
{
    struct BodyIntent *intent = &body->intent;

    if (!intent->has || intent->kind != BODY_WANTS_ATTACK || intent->mode != BODY_ATTACK_HOLD)
        return;
    if (intent->swung)
        intent_end(world, body, BODY_INTENT_DONE, 0, tick);
    else
        intent->mode = BODY_ATTACK_ONCE;
}

static int input_intent(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body,
                        const struct BodyInput *input)
{
    if (input->dir == BODY_INTENT_ANSWER)
        return intent_answered(world, ground, body, input);
    if (input->dir != BODY_INTENT_CLEAR)
        return BODY_BAD_VALUE;
    intent_drop(world, body);
    return BODY_OK;
}

/* An attack, a skill or a pickup: its intent takes the place of the one the body had, and the
 * attack ends, a swing in its windup with it, unless the request is an attack on the body
 * attacked already. `wants` is the intent's kind. */
static int request_intent(struct BodyWorld *world, struct Body *body, const struct BodyInput *input, uint8_t wants)
{
    struct BodyIntent *intent = &body->intent;
    struct BodyAct *act = &body->act;
    const struct Body *target = NULL;

    if (wants == BODY_WANTS_ATTACK || wants == BODY_WANTS_SKILL) {
        target = body_named(world, input->target, input->target_id);
        /* a skill may be on the body itself, an attack not */
        if (target == NULL || (target == body && wants == BODY_WANTS_ATTACK))
            return BODY_BAD_SLOT;
    } else if (!on_map(world->width, world->height, input->x, input->y)) {
        return BODY_BAD_CELL;
    }
    if (input->cause > (wants == BODY_WANTS_ATTACK ? BODY_ATTACK_ONCE : BODY_MAX_RANGE))
        return BODY_BAD_VALUE;
    /* a swing that ends and the walk it held back, and the step's question */
    if (event_room(world) < 3)
        return BODY_EVENTS_FULL;
    if (!(wants == BODY_WANTS_ATTACK && act->has_target && act->target == input->target && act->target_id == input->target_id))
        stop_attack(world, body, 0, input->tick);
    intent_drop(world, body);
    intent->has = 1;
    intent->kind = wants;
    intent->seq = input->value;
    if (wants == BODY_WANTS_ATTACK)
        intent->mode = input->cause;
    else
        intent->range = input->cause;
    if (target != NULL) {
        intent->target = input->target;
        intent->target_id = input->target_id;
    } else {
        intent->x = input->x;
        intent->y = input->y;
    }
    intent->due = input->tick + BODY_INTENT_STEP_MS;
    intent_ask(world, body, WANT_VIEW, input->tick);
    return BODY_OK;
}

/* A request of the body's owner (BODY_REQUEST_*): what the server's game code did around one,
 * as one rule. Whatever it asks for, the body keeps its number. */
static int input_request(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body,
                         const struct BodyInput *input)
{
    /* the intent each request gives the body; 0 for none */
    static const uint8_t WANTS[] = {0, 0, BODY_WANTS_ATTACK, 0, 0, BODY_WANTS_SKILL, BODY_WANTS_SKILL_CELL, BODY_WANTS_PICKUP};
    struct BodyIntent *intent = &body->intent;
    struct BodyAct *act = &body->act;
    uint8_t dirs[BODY_PATH_STEPS];
    struct BodyWalk goal;
    int count, result;

    switch (input->dir) {
    case BODY_REQUEST_NONE:
        break;
    case BODY_REQUEST_WALK:
        if (!on_map(world->width, world->height, input->x, input->y))
            return BODY_BAD_CELL;
        /* a swing that ends and the walk it held back, then the walk or the word that there is none */
        if (event_room(world) < 3)
            return BODY_EVENTS_FULL;
        /* a walk ends the intent and the attack (unit_walk_toxy: unit_stop_attack) */
        intent_drop(world, body);
        stop_attack(world, body, 0, input->tick);
        zero(&goal, sizeof goal);
        goal.to_x = input->x;
        goal.to_y = input->y;
        count = find_way(world, ground, body, &goal, (int)world->request_steps, dirs);
        if (count < 0)
            push_about(world, input->tick, BODY_EVENT_REFUSED, body, NULL, input->value);
        else
            set_out(world, body, &goal, dirs, count, input->tick);
        break;
    case BODY_REQUEST_RELEASE:
        if (event_room(world) == 0)
            return BODY_EVENTS_FULL;
        intent_release(world, body, input->tick);
        if (intent->has && intent->kind == BODY_WANTS_ATTACK && intent->mode == BODY_ATTACK_ONCE)
            act->attack_continue = 0;
        break;
    case BODY_REQUEST_CANCEL:
        if (event_room(world) < 2)
            return BODY_EVENTS_FULL;
        intent_drop(world, body);
        stop_attack(world, body, 0, input->tick);
        break;
    default:
        if (input->dir >= sizeof WANTS || WANTS[input->dir] == 0)
            return BODY_BAD_VALUE;
        result = request_intent(world, body, input, WANTS[input->dir]);
        if (result != BODY_OK)
            return result;
        break;
    }
    if (input->value != 0)
        body->request = input->value;
    return BODY_OK;
}

/* The tick the next thing is due at. `free_only`: what a question holds back does not count. */
static int next_due(const struct BodyWorld *world, uint64_t *tick, int free_only)
{
    const struct Body *body = slots_of(world);
    int found = 0;
    uint32_t i;

    for (i = 0; i < world->bodies; i++) {
        uint64_t due[5];
        int count = 0, k;

        if (!body[i].used)
            continue;
        if (body[i].move.steps > 0)
            due[count++] = landing(&body[i].move);
        if (!(free_only && body[i].act.asking)) {
            if (body[i].act.casting)
                due[count++] = body[i].act.cast_due;
            if (body[i].act.swing_open)
                due[count++] = body[i].act.impact_due;
            if (body[i].act.armed)
                due[count++] = body[i].act.attack_due;
        }
        if (body[i].intent.has && !(free_only && body[i].intent.asking))
            due[count++] = body[i].intent.due;
        for (k = 0; k < count; k++) {
            if (!found || due[k] < *tick) {
                *tick = due[k];
                found = 1;
            }
        }
    }
    return found;
}

int body_next_due(const struct BodyWorld *world, uint64_t *tick)
{
    return next_due(world, tick, 0);
}

int body_run(struct BodyWorld *world, const struct BodyGround *ground, uint64_t tick)
{
    struct Body *body = slots(world);
    uint64_t due;
    uint32_t i;

    if (tick < world->now)
        return BODY_BAD_TICK;
    if (world->asked > 0 && tick > world->now)
        return BODY_ANSWER_FIRST;
    if (body_next_due(world, &due) && due < tick)
        return BODY_DUE_FIRST;
    world->now = tick;
    /* one pass: nothing that runs makes anything due at its own tick. A step takes at least
       1 ms; a look of the attack that is put off is put off behind this tick, or asks. A body
       that asks waits with the rest of what it has due. */
    for (i = 0; i < world->bodies; i++) {
        if (!body[i].used)
            continue;
        if (body[i].move.steps > 0 && landing(&body[i].move) == tick) {
            if (event_room(world) < 5)
                return BODY_EVENTS_FULL;
            land(world, ground, &body[i]);
        }
        if (!body[i].act.asking && body[i].act.casting && body[i].act.cast_due == tick) {
            if (event_room(world) == 0)
                return BODY_EVENTS_FULL;
            /* the cast is over before its skill is carried out: what that sets off finds none */
            body[i].act.casting = 0;
            body[i].act.cast_due = 0;
            ask(world, &body[i], ASK_CAST, 0, NULL, tick);
        }
        if (!body[i].act.asking && body[i].act.swing_open && body[i].act.impact_due == tick) {
            if (event_room(world) < 6)
                return BODY_EVENTS_FULL;
            swing_lands(world, &body[i], tick);
        }
        if (!body[i].act.asking && body[i].act.armed && body[i].act.attack_due == tick) {
            if (event_room(world) < 6)
                return BODY_EVENTS_FULL;
            attack_look(world, &body[i], 0, tick);
        }
        /* the intent's own step, every BODY_INTENT_STEP_MS from when it was set */
        if (body[i].intent.has && !body[i].intent.asking && body[i].intent.due == tick) {
            if (event_room(world) == 0)
                return BODY_EVENTS_FULL;
            body[i].intent.due = tick + BODY_INTENT_STEP_MS;
            intent_ask(world, &body[i], WANT_VIEW, tick);
        }
    }
    return world->asked > 0 ? BODY_ANSWER_FIRST : BODY_OK;
}

static int input_add(struct BodyWorld *world, const struct BodyInput *input)
{
    struct Body *body;

    if (input->slot >= world->bodies || slots(world)[input->slot].used)
        return BODY_BAD_SLOT;
    if (!on_map(world->width, world->height, input->x, input->y))
        return BODY_BAD_CELL;
    if (input->dir > 7 || input->value > BODY_MAX_SPEED_MS)
        return BODY_BAD_VALUE;
    if (event_room(world) == 0)
        return BODY_EVENTS_FULL;
    body = &slots(world)[input->slot];
    body->id = input->id;
    body->used = 1;
    body->move.speed_ms = input->value;
    stand(body, input->tick, input->x, input->y, input->dir, BODY_CAUSE_PLACED);
    body->cell_tick = input->tick;
    push_event(world, input->tick, BODY_EVENT_MOVE, body);
    return BODY_OK;
}

static int input_place(struct BodyWorld *world, struct Body *body, const struct BodyInput *input)
{
    if (!on_map(world->width, world->height, input->x, input->y))
        return BODY_BAD_CELL;
    if (input->dir > 7 || input->cause > BODY_CAUSE_PLACED)
        return BODY_BAD_VALUE;
    if (event_room(world) == 0)
        return BODY_EVENTS_FULL;
    stand(body, input->tick, input->x, input->y, input->dir, input->cause);
    body->cell_tick = input->tick;
    push_event(world, input->tick, BODY_EVENT_MOVE, body);
    return BODY_OK;
}

/* unit_walk_toxy, and the walks to within a range of a cell and after another body
 * (unit_walk_tobl). The walk is refused unless there is a way from the cell the body is on. A
 * body that came onto its cell at this very tick, or stands, starts from it at once. One with
 * a step under way keeps the target for when that step lands (state.change_walk_target), so a
 * request turns the walk at the first cell reached after it was applied. */
static int input_walk(struct BodyWorld *world, const struct BodyGround *ground, struct Body *body,
                      const struct BodyInput *input)
{
    uint8_t dirs[BODY_PATH_STEPS];
    struct BodyWalk goal;
    int max_steps = BODY_PATH_STEPS, count;

    zero(&goal, sizeof goal);
    if (input->kind == BODY_INPUT_CHASE) {
        if (input->target >= world->bodies || input->target == input->slot || !slots(world)[input->target].used
            || slots(world)[input->target].id != input->target_id)
            return BODY_BAD_SLOT;
        if (input->value == 0 || input->value > BODY_MAX_RANGE)
            return BODY_BAD_VALUE;
        goal.goal = GOAL_BODY;
        goal.range = input->value;
        goal.target = input->target;
        goal.target_id = input->target_id;
    } else {
        if (!on_map(world->width, world->height, input->x, input->y))
            return BODY_BAD_CELL;
        goal.to_x = input->x;
        goal.to_y = input->y;
        if (input->kind == BODY_INPUT_NEAR) {
            if (input->value > BODY_MAX_RANGE)
                return BODY_BAD_VALUE;
            if (input->value > 0) {
                goal.goal = GOAL_NEAR;
                goal.range = input->value;
            }
        } else if (input->value != 0 && input->value < BODY_PATH_STEPS) {
            max_steps = input->value;
        }
    }
    count = find_way(world, ground, body, &goal, max_steps, dirs);
    if (count < 0)
        return BODY_NO_PATH;
    if (body->move.steps > 0 && body->cell_tick != input->tick) {
        goal.change = 1;
        body->walk = goal;
        return BODY_OK;
    }
    /* within the range already: a body that stands is left as it is */
    if (count == 0 && goal.goal != GOAL_CELL && body->move.steps == 0)
        return BODY_OK;
    if (event_room(world) == 0)
        return BODY_EVENTS_FULL;
    take_path(body, input->tick, dirs, count, &goal);
    push_event(world, input->tick, BODY_EVENT_MOVE, body);
    return BODY_OK;
}

static int input_stop(struct BodyWorld *world, struct Body *body, const struct BodyInput *input)
{
    if (body->move.steps == 0)
        return BODY_OK;
    if (event_room(world) == 0)
        return BODY_EVENTS_FULL;
    stand(body, input->tick, body->move.x, body->move.y, body->move.dir, BODY_CAUSE_WALKED);
    push_event(world, input->tick, BODY_EVENT_MOVE, body);
    return BODY_OK;
}

/* The move clock goes to the end of the hold when that is later. A step under way lands no
 * earlier, which is a new record: on its cell from now, landing when the hold ends. */
static int input_delay(struct BodyWorld *world, struct Body *body, const struct BodyInput *input)
{
    uint64_t until = input->tick + input->value;
    int held;

    if (input->value > BODY_MAX_DELAY_MS)
        return BODY_BAD_VALUE;
    if (until <= body->can_move)
        return BODY_OK;
    held = body->move.steps > 0 && landing(&body->move) < until;
    if (held && event_room(world) == 0)
        return BODY_EVENTS_FULL;
    body->can_move = until;
    if (held) {
        body->move.at_tick = input->tick;
        body->move.land_ms = input->value;
        push_event(world, input->tick, BODY_EVENT_MOVE, body);
    }
    return BODY_OK;
}

/* The step under way lands when it was going to; the steps after it take the new speed's time. */
static int input_speed(struct BodyWorld *world, struct Body *body, const struct BodyInput *input)
{
    uint64_t lands = landing(&body->move);

    if (input->value > BODY_MAX_SPEED_MS)
        return BODY_BAD_VALUE;
    if (input->value == body->move.speed_ms)
        return BODY_OK;
    if (event_room(world) == 0)
        return BODY_EVENTS_FULL;
    body->move.at_tick = input->tick;
    body->move.land_ms = body->move.steps > 0 ? (uint16_t)(lands - input->tick) : 0;
    body->move.speed_ms = input->value;
    push_event(world, input->tick, BODY_EVENT_MOVE, body);
    return BODY_OK;
}

int body_input(struct BodyWorld *world, const struct BodyGround *ground, const struct BodyInput *input)
{
    struct Body *body;
    uint64_t due;
    int result;

    if (input->tick < world->now)
        return BODY_BAD_TICK;
    if (input->kind < BODY_INPUT_ADD || input->kind > BODY_INPUT_REQUEST)
        return BODY_BAD_KIND;
    if (world->asked > 0 && input->tick > world->now)
        return BODY_ANSWER_FIRST;
    /* an answer is to a question of this tick, which holds back what its body has due */
    if (input->kind != BODY_INPUT_ANSWER && !(input->kind == BODY_INPUT_INTENT && input->dir == BODY_INTENT_ANSWER)
        && next_due(world, &due, 1) && due <= input->tick)
        return BODY_DUE_FIRST;
    if (input->kind == BODY_INPUT_ADD) {
        result = input_add(world, input);
    } else {
        body = named(world, input);
        if (body == NULL)
            return BODY_BAD_SLOT;
        switch (input->kind) {
        case BODY_INPUT_REMOVE:
            if (body->act.asking)
                world->asked--;
            if (body->intent.asking)
                world->asked--;
            zero(body, sizeof *body);
            result = BODY_OK;
            break;
        case BODY_INPUT_FIGHTER:
            result = input_fighter(body, input);
            break;
        case BODY_INPUT_ATTACK:
            result = input_attack(world, body, input);
            break;
        case BODY_INPUT_ATTACK_STOP:
            result = input_attack_stop(world, body, input);
            break;
        case BODY_INPUT_SWING_CANCEL:
            if (body->act.asking)
                result = BODY_ANSWER_FIRST;
            else if (body->act.swing_open && event_room(world) < 2)
                result = BODY_EVENTS_FULL;
            else {
                cancel_swing(world, body, BODY_CANCEL_COMMAND, input->tick);
                result = BODY_OK;
            }
            break;
        case BODY_INPUT_ACT_CLOCK:
            if (input->value > 1) {
                result = BODY_BAD_VALUE;
            } else {
                if (!input->value || input->tick + input->target_id > body->act.can_act)
                    body->act.can_act = input->tick + input->target_id;
                result = BODY_OK;
            }
            break;
        case BODY_INPUT_CAST:
            result = input_cast(body, input);
            break;
        case BODY_INPUT_CAST_CANCEL:
            result = input_cast_cancel(body, input);
            break;
        case BODY_INPUT_INTENT:
            result = input_intent(world, ground, body, input);
            break;
        case BODY_INPUT_REQUEST:
            result = input_request(world, ground, body, input);
            break;
        case BODY_INPUT_ANSWER:
            result = input_answer(world, ground, body, input);
            break;
        case BODY_INPUT_ATTACK_CLOCK:
            if (input->value > 1) {
                result = BODY_BAD_VALUE;
            } else {
                if (input->value)
                    rearm(body, input->tick + input->target_id);
                else
                    body->act.attackable = input->tick + input->target_id;
                result = BODY_OK;
            }
            break;
        case BODY_INPUT_ATTACK_ONCE:
            body->act.attack_continue = 0;
            result = BODY_OK;
            break;
        case BODY_INPUT_PLACE:
            result = input_place(world, body, input);
            break;
        case BODY_INPUT_WALK:
        case BODY_INPUT_NEAR:
        case BODY_INPUT_CHASE:
            result = input_walk(world, ground, body, input);
            break;
        case BODY_INPUT_STOP:
            result = input_stop(world, body, input);
            break;
        case BODY_INPUT_DELAY:
            result = input_delay(world, body, input);
            break;
        default:
            result = input_speed(world, body, input);
            break;
        }
    }
    if (result == BODY_OK)
        world->now = input->tick;
    return result;
}

int body_put_move(struct BodyWorld *world, uint32_t slot, uint32_t id, const struct BodyMove *record)
{
    struct Body *body, put;
    uint64_t began;
    int x = record->x, y = record->y, i;

    if (slot >= world->bodies || !slots(world)[slot].used || slots(world)[slot].id != id)
        return BODY_BAD_SLOT;
    body = &slots(world)[slot];
    if (record->at_tick > world->now || (record->steps > 0 && landing(record) < world->now))
        return BODY_BAD_TICK;
    if (record->steps > BODY_PATH_STEPS || record->dir > 7 || record->cause > BODY_CAUSE_PLACED
        || record->speed_ms > BODY_MAX_SPEED_MS || (record->steps == 0 && record->land_ms != 0)
        || !is_zero(record->reserved, sizeof record->reserved))
        return BODY_BAD_VALUE;
    for (i = 0; i < BODY_PATH_STEPS; i++) {
        if (i < record->steps ? record->path[i] > 7 : record->path[i] != 0)
            return BODY_BAD_VALUE;
        if (i < record->steps) {
            x += DIR_X[record->path[i]];
            y += DIR_Y[record->path[i]];
        }
    }
    if (!on_map(world->width, world->height, record->x, record->y) || !on_map(world->width, world->height, x, y))
        return BODY_BAD_CELL;
    if (event_room(world) == 0)
        return BODY_EVENTS_FULL;
    put = *body;
    put.move = *record;
    zero(&put.walk, sizeof put.walk);
    put.cell_tick = record->at_tick;
    if (record->steps > 0) {
        put.walk.to_x = (int16_t)x;
        put.walk.to_y = (int16_t)y;
        /* a step that lands sooner than its own time was under way: the body came onto its
         * cell when that step began */
        began = landing(record) - step_ms(record->path[0], record->speed_ms);
        if (began < record->at_tick && landing(record) >= step_ms(record->path[0], record->speed_ms))
            put.cell_tick = began;
    }
    *body = put;
    push_event(world, world->now, BODY_EVENT_MOVE, body);
    return BODY_OK;
}

int body_take_event(struct BodyWorld *world, struct BodyEvent *event)
{
    struct BodyEvent *oldest;

    if (world->event_count == 0)
        return 0;
    oldest = &ring(world)[world->event_first];
    *event = *oldest;
    zero(oldest, sizeof *oldest);
    world->event_first = (world->event_first + 1) % world->events;
    world->event_count--;
    return 1;
}

int body_move(const struct BodyWorld *world, uint32_t slot, struct BodyMove *move)
{
    if (slot >= world->bodies || !slots_of(world)[slot].used)
        return BODY_BAD_SLOT;
    *move = slots_of(world)[slot].move;
    return BODY_OK;
}

int body_attack(const struct BodyWorld *world, uint32_t slot, struct BodyAttack *attack)
{
    const struct BodyAct *act;

    if (slot >= world->bodies || !slots_of(world)[slot].used)
        return BODY_BAD_SLOT;
    act = &slots_of(world)[slot].act;
    zero(attack, sizeof *attack);
    attack->attackable = act->attackable;
    attack->can_act = act->can_act;
    attack->casting = act->casting;
    attack->target_id = act->target_id;
    attack->swing_target_id = act->swing_target_id;
    attack->target = act->target;
    attack->swing_target = act->swing_target;
    attack->has_target = act->has_target;
    attack->attack_continue = act->attack_continue;
    attack->armed = act->armed;
    attack->swing_open = act->swing_open;
    attack->amotion = act->amotion;
    attack->windup = act->windup;
    attack->looking = act->asking == ASK_SWING || act->asking == ASK_BEGIN || act->asking == ASK_HIT_NOW;
    if (act->swing_open && act->impact_due > world->now)
        attack->swing_ms = (uint16_t)(act->impact_due - world->now > 0xFFFFu ? 0xFFFFu : act->impact_due - world->now);
    return BODY_OK;
}

int body_intent(const struct BodyWorld *world, uint32_t slot, struct BodyIntentState *intent)
{
    const struct BodyIntent *own;

    if (slot >= world->bodies || !slots_of(world)[slot].used)
        return BODY_BAD_SLOT;
    own = &slots_of(world)[slot].intent;
    zero(intent, sizeof *intent);
    intent->has = own->has;
    intent->seq = own->seq;
    intent->kind = own->kind;
    intent->mode = own->mode;
    intent->walked = own->walked;
    intent->target = own->target;
    intent->target_id = own->target_id;
    intent->request = slots_of(world)[slot].request;
    return BODY_OK;
}

int body_state(const struct BodyWorld *world, uint32_t slot, void *state)
{
    struct Body body;
    const unsigned char *from = (const unsigned char *)&body;
    unsigned char *to = state;
    size_t i;

    if (slot >= world->bodies || !slots_of(world)[slot].used)
        return BODY_BAD_SLOT;
    body = slots_of(world)[slot];
    /* a question it waits on is this world's host's to answer, and not part of what it is */
    body.act.asking = ASK_NONE;
    body.act.requested = 0;
    body.intent.asking = WANT_NONE;
    /* whom it names, it names by id: a slot is this world's business */
    body.walk.target = 0;
    body.act.target = 0;
    body.act.swing_target = 0;
    body.intent.target = 0;
    for (i = 0; i < sizeof body; i++)
        to[i] = from[i];
    return BODY_OK;
}

/* The slot of the body `id` as `slot` is to name it: where it is in this world, or `slot`
 * itself, which is never that body, when it is not here. */
static uint16_t slot_named(const struct BodyWorld *world, uint32_t slot, uint32_t id)
{
    const struct Body *body = slots_of(world);
    uint32_t i;

    for (i = 0; i < world->bodies; i++)
        if (i != slot && body[i].used && body[i].id == id)
            return (uint16_t)i;
    return (uint16_t)slot;
}

int body_put_state(struct BodyWorld *world, uint32_t slot, const void *state)
{
    struct Body put, *body;
    unsigned char *to = (unsigned char *)&put;
    const unsigned char *from = state;
    size_t i;

    if (slot >= world->bodies)
        return BODY_BAD_SLOT;
    for (i = 0; i < sizeof put; i++)
        to[i] = from[i];
    /* a question's event went to the host that made the state: nobody here would answer it */
    if (put.used != 1 || put.act.asking || put.intent.asking)
        return BODY_BAD_VALUE;
    /* the fields it names bodies by are in use only where the rest says so, and name no slot */
    if (put.walk.target != 0 || put.act.target != 0 || put.act.swing_target != 0 || put.intent.target != 0)
        return BODY_BAD_VALUE;
    if (put.move.steps > 0 && put.walk.goal == GOAL_BODY) {
        if (put.walk.target_id == put.id)
            return BODY_BAD_VALUE;
        put.walk.target = slot_named(world, slot, put.walk.target_id);
    }
    if (put.act.has_target == 1) {
        if (put.act.target_id == put.id)
            return BODY_BAD_VALUE;
        put.act.target = slot_named(world, slot, put.act.target_id);
    }
    if (put.act.swing_open == 1) {
        if (put.act.swing_target_id == put.id)
            return BODY_BAD_VALUE;
        put.act.swing_target = slot_named(world, slot, put.act.swing_target_id);
    }
    if (put.intent.has == 1 && on_body(&put.intent)) {
        /* an attack is on another body; a skill may be on the body itself */
        if (put.intent.kind == BODY_WANTS_ATTACK && put.intent.target_id == put.id)
            return BODY_BAD_VALUE;
        put.intent.target = slot_named(world, slot, put.intent.target_id);
    }
    if (!body_valid(&put, world))
        return BODY_BAD_VALUE;
    if (event_room(world) == 0)
        return BODY_EVENTS_FULL;
    body = &slots(world)[slot];
    if (body->used && body->act.asking)
        world->asked--;
    if (body->used && body->intent.asking)
        world->asked--;
    *body = put;
    push_event(world, world->now, BODY_EVENT_MOVE, body);
    return BODY_OK;
}
