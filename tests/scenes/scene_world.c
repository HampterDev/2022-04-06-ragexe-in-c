/* The fake map server's units (tests/scenes/README.md, "Staging actors"): where the actors a scene
   stages are and what they do, sent to the client as the project's server sends it
   (plans/core-loop.md, the server's docs/tracks.md):

   - the server's clock is the harness's virtual clock (scene_clock.c), so the ticks of every
     record are the times scene.ticks() gives; a timed exchange (CZ_REQUEST_TIME2) is answered at
     once, with ZC_NOTIFY_TIME and ZC_TIME_SYNC, both stamped with that clock;
   - every map the player is placed on is told when it has loaded (CZ_NOTIFY_ACTORINIT): the
     placement's number with an ordinary map's hold-back, 0 (ZC_HOLD_BACK), and the player's own
     body as the actor core has it (ZC_OWN_BODY);
   - every unit's coming and going is stamped (ZC_VIEW_STAMP) with the tick and its revisions;
   - a walk is the actor core's (actor/actor.h), run here as the server's spatial runs it: the
     server's path and steps at the unit's speed, sent to the client as a move record
     (ZC_TRACK_MOVE), or for the player's own character as its body (ZC_OWN_BODY);
   - a basic attack is a swing record (ZC_TRACK_ACT) and its hit a notice dated when it lands
     (ZC_NOTICE_HIT), as the server sends a hit that was decided when its swing began; a cast is
     a cast record.

   Records carry a revision per unit and track, counted from 1 as the server counts them, for as
   long as the run lasts. The map's cells are the client's own copy of the map (the server reads
   the same .gat). */
#include "scene.h"
#include "actor/actor.h"
#include "platform/clock.h"
#include "world/pathfinder_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    ZC_NOTIFY_TIME = 0x007f,
    ZC_TIME_SYNC = 0x0f12,
    ZC_HOLD_BACK = 0x0f14,
    ZC_TRACK_MOVE = 0x0f20,
    ZC_TRACK_ACT = 0x0f21,
    ZC_NOTICE_HIT = 0x0f23,
    ZC_OWN_BODY = 0x0f25,
    ZC_VIEW_STAMP = 0x0f26
};
enum { TRACK_MOVE, TRACK_ACT, TRACK_VITALS };
enum { VIEW_COMES = 0, VIEW_GOES = 1 };
enum { ACT_SWING = 1, ACT_CAST = 2 };

/* The units a run can name, the bodies a map can hold at once and the events between two runs
   of the world: a load scene's crowd of two hundred, and the monsters that keep coming. */
#define UNITS 4096
#define SLOTS 1024
#define EVENTS 8192
/* The longest walk a player may ask for (rAthena's max_walk_path), which the server's core and the
   own body's word carry. */
#define REQUEST_STEPS 17

struct Unit {
    uint32_t aid;
    uint16_t rev[3];      /* the last revision of each track */
    int slot;             /* its body's slot on the map, or -1 */
    unsigned char player; /* a player's death is no going: she lies where she fell */
};

static struct {
    struct Unit units[UNITS];
    int count;
    void *arena;
    struct ActorWorld *world; /* the map the player is on, once it has loaded */
    const void *map;         /* the client's map the world was made for (g_pathFinder.attr) */
    int width, height;
    unsigned char used[SLOTS];
    uint32_t own;            /* the player's account id */
    int own_x, own_y, own_dir, own_speed; /* where the next map loaded puts the player */
    uint32_t placement;      /* the player's placements on a map so far */
    uint32_t outside;        /* inputs to the own body the client does not make, since its placement */
} W;

static void put16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

static void put32(unsigned char *p, uint32_t v)
{
    put16(p, v & 0xffff);
    put16(p + 2, v >> 16);
}

static void put64(unsigned char *p, uint64_t v)
{
    put32(p, (uint32_t)v);
    put32(p + 4, (uint32_t)(v >> 32));
}

static int deliver_packet(const unsigned char *packet, int size)
{
    if (scene_net_deliver(SCENE_MAP_PORT, packet, size))
        return 1;
    fprintf(stderr, "scene: no map connection to deliver to\n");
    return 0;
}

static struct Unit *unit_of(uint32_t aid)
{
    int i;

    for (i = 0; i < W.count; i++)
        if (W.units[i].aid == aid)
            return &W.units[i];
    if (W.count == UNITS)
        return NULL;
    memset(&W.units[W.count], 0, sizeof W.units[0]);
    W.units[W.count].aid = aid;
    W.units[W.count].slot = -1;
    return &W.units[W.count++];
}

static uint16_t next_rev(struct Unit *unit, int track)
{
    unit->rev[track] = (uint16_t)(unit->rev[track] + 1);
    return unit->rev[track];
}

/* The cells of the world's map, while the client is on it. */
static int ground(struct ActorGround *out)
{
    int width, height;

    return W.world && g_pathFinder.attr == W.map && path_finder_actor_ground(&g_pathFinder, out, &width, &height)
        && width == W.width && height == W.height;
}

/* ZC_TRACK_MOVE: the unit's body's move, as the server's renderer sends a walk's record. */
static int send_move(struct Unit *unit)
{
    unsigned char packet[25 + (3 * ACTOR_PATH_STEPS + 7) / 8];
    struct ActorMove move;
    int size, i;

    if (actor_move(W.world, (uint32_t)unit->slot, &move) != ACTOR_OK)
        return 0;
    size = 25 + (3 * move.steps + 7) / 8;
    memset(packet, 0, sizeof packet);
    put16(packet, ZC_TRACK_MOVE);
    put16(packet + 2, (unsigned)size);
    put32(packet + 4, unit->aid);
    put32(packet + 8, (uint32_t)move.at_tick);
    put16(packet + 12, (unsigned)move.x);
    put16(packet + 14, (unsigned)move.y);
    put16(packet + 16, move.land_ms);
    put16(packet + 18, move.speed_ms);
    packet[20] = move.dir;
    packet[21] = move.cause;
    packet[22] = move.steps;
    put16(packet + 23, next_rev(unit, TRACK_MOVE));
    for (i = 0; i < move.steps; i++) {
        unsigned bit = 3u * (unsigned)i, dir = move.path[i] & 7u;

        packet[25 + bit / 8] |= (unsigned char)(dir << bit % 8);
        if (bit % 8 > 5)
            packet[25 + bit / 8 + 1] |= (unsigned char)(dir >> (8 - bit % 8));
    }
    return deliver_packet(packet, size);
}

/* ZC_OWN_BODY: the player's body as the core has it now. */
static int send_own_body(const struct Unit *own)
{
    unsigned char packet[36 + ACTOR_STATE_SIZE];
    struct ActorIntentState intent;

    if (actor_state(W.world, (uint32_t)own->slot, packet + 36) != ACTOR_OK
        || actor_intent(W.world, (uint32_t)own->slot, &intent) != ACTOR_OK)
        return 0;
    put16(packet, ZC_OWN_BODY);
    put16(packet + 2, sizeof packet);
    put32(packet + 4, ACTOR_RULES_VERSION);
    put64(packet + 8, actor_now(W.world));
    put16(packet + 16, intent.request);
    put16(packet + 18, REQUEST_STEPS);
    put32(packet + 20, 0);
    put32(packet + 24, W.placement);
    put32(packet + 28, 0);
    put32(packet + 32, W.outside);
    return deliver_packet(packet, sizeof packet);
}

/* ZC_VIEW_STAMP, directly before the unit's entry or vanish. */
static int send_stamp(const struct Unit *unit, int kind)
{
    unsigned char packet[17];

    put16(packet, ZC_VIEW_STAMP);
    put32(packet + 2, unit->aid);
    put32(packet + 6, platform_ticks_ms());
    packet[10] = (unsigned char)kind;
    put16(packet + 11, unit->rev[TRACK_MOVE]);
    put16(packet + 13, unit->rev[TRACK_ACT]);
    put16(packet + 15, unit->rev[TRACK_VITALS]);
    return deliver_packet(packet, sizeof packet);
}

/* What the world says by itself: a body whose move is otherwise than its record said is told
   again. Nothing that walks only by the steps of its record says anything. */
static void drain(void)
{
    struct ActorEvent event;
    struct Unit *unit;

    while (actor_take_event(W.world, &event))
        if (event.kind == ACTOR_EVENT_MOVE && event.id != W.own && (unit = unit_of(event.id)) != NULL
            && unit->slot == event.slot)
            send_move(unit);
}

/* Runs the world up to `now`, so that an input of that tick is taken. */
static int run_to(uint64_t now)
{
    struct ActorGround g;
    uint64_t due;
    int result;

    if (!ground(&g))
        return 0;
    while (actor_next_due(W.world, &due) && due <= now) {
        result = actor_run(W.world, &g, due);
        drain();
        if (result != ACTOR_OK && result != ACTOR_EVENTS_FULL) {
            fprintf(stderr, "scene: the units' world stopped at tick %llu (%d)\n", (unsigned long long)due, result);
            return 0;
        }
    }
    return 1;
}

static int input(struct ActorInput *in)
{
    struct ActorGround g;
    int result;

    in->tick = platform_ticks_ms();
    if (!run_to(in->tick) || !ground(&g))
        return ACTOR_BAD_WORLD;
    result = actor_input(W.world, &g, in);
    drain();
    return result;
}

static int world_add_unit(struct Unit *unit, int x, int y, int dir, int speed)
{
    struct ActorInput in;
    int slot;

    if (!W.world)
        return 0;
    for (slot = 0; slot < SLOTS && W.used[slot]; slot++)
        ;
    if (slot == SLOTS) {
        fprintf(stderr, "scene: more than %d units on the map\n", SLOTS);
        return 0;
    }
    memset(&in, 0, sizeof in);
    in.kind = ACTOR_INPUT_ADD;
    in.id = unit->aid;
    in.slot = (uint16_t)slot;
    in.x = (int16_t)x;
    in.y = (int16_t)y;
    in.dir = (uint8_t)(dir & 7);
    in.value = (uint16_t)(speed < 1 ? 150 : speed > (int)ACTOR_MAX_SPEED_MS ? (int)ACTOR_MAX_SPEED_MS : speed);
    if (input(&in) != ACTOR_OK) {
        fprintf(stderr, "scene: the unit %u has no body on %d,%d\n", unit->aid, x, y);
        return 0;
    }
    W.used[slot] = 1;
    unit->slot = slot;
    return 1;
}

static void world_remove_unit(struct Unit *unit)
{
    struct ActorInput in;

    if (unit->slot < 0)
        return;
    memset(&in, 0, sizeof in);
    in.kind = ACTOR_INPUT_REMOVE;
    in.id = unit->aid;
    in.slot = (uint16_t)unit->slot;
    if (input(&in) == ACTOR_OK)
        W.used[unit->slot] = 0;
    unit->slot = -1;
}

void scene_world_place(uint32_t aid, int x, int y, int dir, int speed)
{
    /* the player leaves the map she is on, and its units with it */
    W.world = NULL;
    W.own = aid;
    W.own_x = x;
    W.own_y = y;
    W.own_dir = dir;
    if (speed > 0)
        W.own_speed = speed;
}

int scene_world_loaded(void)
{
    struct ActorGround g;
    struct Unit *own;
    unsigned char hold[8];
    size_t size = actor_world_size(SLOTS, EVENTS);
    int width, height, i;

    if (!W.arena)
        W.arena = malloc(size);
    if (!W.arena || !path_finder_actor_ground(&g_pathFinder, &g, &width, &height))
        return 0;
    W.world = actor_world_init(W.arena, size, SLOTS, EVENTS, width, height, platform_ticks_ms());
    if (!W.world)
        return 0;
    W.map = g_pathFinder.attr;
    W.width = width;
    W.height = height;
    actor_set_request_steps(W.world, REQUEST_STEPS);
    memset(W.used, 0, sizeof W.used);
    for (i = 0; i < W.count; i++)
        W.units[i].slot = -1;
    W.placement++;
    W.outside = 0;
    put16(hold, ZC_HOLD_BACK);
    put16(hold + 2, 0);
    put32(hold + 4, W.placement);
    own = unit_of(W.own);
    return deliver_packet(hold, sizeof hold) && own && world_add_unit(own, W.own_x, W.own_y, W.own_dir, W.own_speed)
        && send_own_body(own);
}

int scene_world_time(uint32_t client_tick)
{
    unsigned char tick[6], sync[14];
    uint32_t now = platform_ticks_ms();

    put16(tick, ZC_NOTIFY_TIME);
    put32(tick + 2, now);
    put16(sync, ZC_TIME_SYNC);
    put32(sync + 2, client_tick);
    put32(sync + 6, now);
    put32(sync + 10, now);
    return deliver_packet(tick, sizeof tick) && deliver_packet(sync, sizeof sync);
}

void scene_world_frame(void)
{
    run_to(platform_ticks_ms());
}

int scene_world_spawn(uint32_t aid, int player, int x, int y, int dir, int speed)
{
    struct Unit *unit = unit_of(aid);

    if (!unit)
        return 0;
    unit->player = (unsigned char)(player != 0);
    world_remove_unit(unit);
    world_add_unit(unit, x, y, dir, speed);
    return send_stamp(unit, VIEW_COMES);
}

int scene_world_vanish(uint32_t aid, int how)
{
    struct Unit *unit = unit_of(aid);

    /* a player who died lies where she fell, a body that can be raised: no going */
    if (!unit || (unit->player && how == 1))
        return 1;
    world_remove_unit(unit);
    return send_stamp(unit, VIEW_GOES);
}

int scene_world_walk(uint32_t aid, int x, int y)
{
    struct Unit *unit = unit_of(aid);
    struct ActorInput in;
    int result;

    if (!unit || unit->slot < 0)
        return 0;
    memset(&in, 0, sizeof in);
    in.kind = ACTOR_INPUT_WALK;
    in.id = aid;
    in.slot = (uint16_t)unit->slot;
    in.x = (int16_t)x;
    in.y = (int16_t)y;
    result = input(&in);
    if (result != ACTOR_OK) {
        if (getenv("RAGEXE_SCENE_TRACE"))
            fprintf(stderr, "scene: no walk of %u to %d,%d (%d)\n", aid, x, y, result);
        return 0;
    }
    /* another unit's record went with the move the walk made (drain): at once from a cell, at
       the next cell from the middle of a step */
    if (aid != W.own)
        return 1;
    W.outside++;
    return send_own_body(unit);
}

int scene_world_attack(const struct SceneAttack *attack)
{
    struct Unit *unit = unit_of(attack->source);
    unsigned char act[37], hit[32];
    uint32_t now = platform_ticks_ms(), lands = now + attack->motion_ms;

    if (!unit)
        return 0;
    memset(act, 0, sizeof act);
    put16(act, ZC_TRACK_ACT);
    put32(act + 2, attack->source);
    put32(act + 6, now);
    act[10] = ACT_SWING;
    put32(act + 11, lands);
    put32(act + 15, lands);
    put32(act + 19, attack->target);
    put16(act + 31, attack->motion_ms);
    put16(act + 35, next_rev(unit, TRACK_ACT));
    memset(hit, 0, sizeof hit);
    put16(hit, ZC_NOTICE_HIT);
    put32(hit + 2, attack->source);
    put32(hit + 6, attack->target);
    put32(hit + 10, lands);
    put32(hit + 14, (uint32_t)attack->damage);
    put32(hit + 18, (uint32_t)attack->left_damage);
    put16(hit + 22, (unsigned)attack->count);
    hit[24] = (unsigned char)attack->kind;
    hit[25] = (unsigned char)(attack->sp_damage != 0);
    put16(hit + 26, attack->flinch_ms);
    return deliver_packet(act, sizeof act) && deliver_packet(hit, sizeof hit);
}

int scene_world_cast(const struct SceneCast *cast)
{
    struct Unit *unit = unit_of(cast->source);
    unsigned char act[37];
    uint32_t now = platform_ticks_ms();

    if (!unit)
        return 0;
    memset(act, 0, sizeof act);
    put16(act, ZC_TRACK_ACT);
    put32(act + 2, cast->source);
    put32(act + 6, now);
    act[10] = ACT_CAST;
    put32(act + 11, now + cast->cast_ms);
    put32(act + 19, cast->target);
    put16(act + 23, (unsigned)cast->x);
    put16(act + 25, (unsigned)cast->y);
    put16(act + 27, cast->skill_id);
    put16(act + 29, cast->skill_lv);
    put16(act + 31, cast->motion_ms);
    act[33] = (unsigned char)(signed char)cast->element;
    act[34] = (unsigned char)(cast->disposable != 0);
    put16(act + 35, next_rev(unit, TRACK_ACT));
    return deliver_packet(act, sizeof act);
}
