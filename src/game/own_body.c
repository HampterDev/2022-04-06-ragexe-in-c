#include "game/own_body.h"
#include <string.h>
#include "actor/actor_states.h"
#include "actor/types.h"
#include "game/cells.h"
#include "session/session_state.h"
#include "world/actor_lookup.h"
#include "game/gamemode.h"
#include "game/net_clock.h"
#include "world/move_track.h"
#include "world/object_lookup.h"
#include "world/pathfinder_state.h"
#include "world/predicted.h"
#include "game/ground_item_state.h"
#include "skills/attack_range.h"

/* Static: the core's arena inside it needs its 8 bytes of alignment, which a 32-bit build
   gives no local. */
static struct Predicted g_own;
/* The map the prediction was begun on. */
static const struct C3dAttr *g_own_map;
/* The move of the own body the drawn character was given last (while g_drawn_known). */
static struct BodyMove g_drawn;
static int g_drawn_known;
/* The revision of the last record filed for the character: they are made one after the other. */
static uint16_t g_drawn_rev;
/* Whom the newest request on a body is for (an attack, a skill): the world is kept told of that
   body from when the request leaves until the server has answered it, beside the ones the own
   body names. 0: nobody. */
static uint32_t g_ordered;

/* The map as the body core reads it. A prediction that was begun on another map is begun
   again: its requests and the server's last word were of that one. Returns 0 without a map. */
static int own_ground(struct BodyGround *ground)
{
    int width, height;

    if (!path_finder_body_ground(&g_pathFinder, ground, &width, &height))
        return 0;
    if (g_own_map != g_pathFinder.attr || g_own.width != width || g_own.height != height) {
        predicted_init(&g_own, width, height);
        g_own_map = g_pathFinder.attr;
        g_drawn_known = 0;
        g_ordered = 0;
    }
    return 1;
}

/* The drawn character follows the own body: when the body's move is another than the one the
   character was given, it is filed in the character's move track, which is played on the own
   clock as any actor's is on the render clock (actor/actor_state.c). A body that only took
   the steps of its walk says nothing new. A move that was made before the one drawn is the
   past put right (the server's word was not what had been predicted): however the body came
   onto that cell then, the character walks there now. Nothing else takes the character off
   what its record says (plans/drawn-place.md). */
static void own_draw(struct GameMode *self)
{
    struct Player *player = self->world ? self->world->player : NULL;
    struct BodyMove move;
    struct ViewUnit *floors;
    struct MoveRecord record;
    int news;

    if (!player || !predicted_move(&g_own, &move))
        return;
    news = g_drawn_known ? predicted_walk_news(&g_drawn, &move) : PREDICTED_WALK_NEW;
    if (news == PREDICTED_WALK_ON)
        return;
    g_drawn = move;
    g_drawn_known = 1;
    memset(&record, 0, sizeof record);
    record.at_tick = (uint32_t)move.at_tick;
    record.x = move.x;
    record.y = move.y;
    record.land_ms = move.land_ms;
    record.speed_ms = move.speed_ms;
    record.dir = move.dir;
    record.cause = news == PREDICTED_WALK_BACK ? MOVE_CAUSE_WALKED : move.cause;
    record.steps = move.steps > MOVE_TRACK_STEPS ? MOVE_TRACK_STEPS : move.steps;
    memcpy(record.path, move.path, record.steps);
    record.rev = ++g_drawn_rev;
    floors = view_floors_unit(&self->view_floors, (uint32_t)g_session1.ownAid);
    if (floors)
        move_track_file(&player->move_track, &floors->floors, &record);
}

/* A record of the wire as the core's. */
static void record_move(const struct MoveRecord *record, struct BodyMove *move)
{
    memset(move, 0, sizeof *move);
    move->at_tick = predicted_wide(&g_own, record->at_tick);
    move->x = (int16_t)record->x;
    move->y = (int16_t)record->y;
    move->land_ms = (uint16_t)(record->land_ms > 0xFFFFu ? 0xFFFFu : record->land_ms);
    move->speed_ms = (uint16_t)(record->speed_ms > 0xFFFFu ? 0xFFFFu : record->speed_ms);
    move->dir = record->dir;
    move->steps = record->steps > BODY_PATH_STEPS ? BODY_PATH_STEPS : record->steps;
    memcpy(move->path, record->path, move->steps);
}

/* Where the server has another actor, as the client knows it, as a record for the world. The
   newest record of its walk that came and waits for the render clock says it as the server
   did. One that is played is the actor's place (plans/drawn-place.md). An actor without a
   record yet is on the cell it is drawn on. */
static void other_move(struct GameMode *self, struct GameActor *actor, struct BodyMove *move)
{
    int x, y;

    if (actor->move_track.count) {
        record_move(&actor->move_track.held[actor->move_track.count - 1], move);
        return;
    }
    if (drawn_record(&actor->place)) {
        record_move(drawn_record(&actor->place), move);
        return;
    }
    game_mode_world_to_cell(self, actor->pos.x, actor->pos.z, &x, &y);
    memset(move, 0, sizeof *move);
    move->at_tick = predicted_now(&g_own);
    move->x = (int16_t)x;
    move->y = (int16_t)y;
    move->speed_ms = (uint16_t)(actor->walk_speed < 0 ? 0 : actor->walk_speed > 0xFFFF ? 0xFFFF : actor->walk_speed);
}

/* The world is told where the body `id` is, while its unit is in view and has an actor. The
   server's body core names a body by the id the client knows its unit by (a player's account
   id, a monster's own), so the actor of that id is the body's. Its going is told with its tick
   when its stamp is read (game_mode_own_body_gone): an actor that is still shown after that,
   dying, is no body. */
static void own_other(struct GameMode *self, uint32_t id)
{
    struct GameActor *actor;
    struct BodyMove move;

    if (!id || id == (uint32_t)g_session1.ownAid || !self->world || !view_floors_in_view(&self->view_floors, id))
        return;
    actor = world_get_game_actor_by_aid(self->world, id);
    if (!actor)
        return;
    other_move(self, actor, &move);
    predicted_other(&g_own, id, &move);
}

/* The bodies the own body depends on are kept as the client sees them: whom it attacks, swings
   at or has an intent about, and whom the newest request on a body that left is for. */
static void own_others(struct GameMode *self)
{
    struct BodyAttack attack;
    struct BodyIntentState intent;
    uint32_t i;
    int waits = 0;

    if (predicted_attack(&g_own, &attack)) {
        if (attack.has_target)
            own_other(self, attack.target_id);
        if (attack.swing_open && !(attack.has_target && attack.swing_target_id == attack.target_id))
            own_other(self, attack.swing_target_id);
    } else {
        memset(&attack, 0, sizeof attack);
    }
    if (!predicted_intent(&g_own, &intent))
        memset(&intent, 0, sizeof intent);
    if (intent.has && intent.target_id && !(attack.has_target && intent.target_id == attack.target_id))
        own_other(self, intent.target_id);
    if (!g_ordered)
        return;
    for (i = 0; i < g_own.pending_count; i++)
        if ((g_own.pending[i].kind == BODY_REQUEST_ATTACK || g_own.pending[i].kind == BODY_REQUEST_SKILL)
            && g_own.pending[i].target_id == g_ordered)
            waits = 1;
    /* the request is answered: the body names its target itself, or has let it go */
    if (!waits) {
        g_ordered = 0;
        return;
    }
    if (!(attack.has_target && attack.target_id == g_ordered) && !(intent.has && intent.target_id == g_ordered))
        own_other(self, g_ordered);
}

int game_mode_own_body_swing(unsigned *target, DWORD *at_tick, DWORD *lands, unsigned *motion_ms)
{
    struct PredictedSwing swing;

    if (!predicted_swing(&g_own, &swing))
        return 0;
    *target = swing.target_id;
    *at_tick = (DWORD)swing.tick;
    *lands = (DWORD)swing.lands;
    *motion_ms = swing.motion_ms;
    return 1;
}

/* A word that answered a request with its arrival says how long the request's way up took: a
   sample for the own clock's lead, as a timed exchange is (plans/netcode.md). */
static void own_way_up(struct GameMode *self)
{
    if (!g_own.way_new)
        return;
    g_own.way_new = 0;
    way_up_sample(&self->way_up, g_own.way_left, g_own.way_arrival);
}

void game_mode_own_body_heard(unsigned id, DWORD tick)
{
    predicted_heard(&g_own, (uint32_t)id, predicted_wide(&g_own, tick));
}

unsigned game_mode_own_body_unexplained(void)
{
    return g_own.unexplained;
}

void game_mode_zc_own_body(struct GameMode *self, const unsigned char *pkt, size_t size)
{
    struct PredictedBody body;
    struct BodyGround ground;

    if (pkt && predicted_read(pkt, size, &body) && own_ground(&ground)) {
        predicted_confirm(&g_own, &ground, &body);
        own_way_up(self);
        game_mode_own_body_frame(self);
    }
}

void game_mode_own_body_placed(struct GameMode *self, unsigned placement)
{
    struct BodyGround ground;

    if (own_ground(&ground) && predicted_placed(&g_own, &ground, (uint32_t)placement) != PREDICTED_REFUSED) {
        own_way_up(self);
        game_mode_own_body_frame(self);
    }
}

/* The request joins the ones that wait for the server's word, at the tick its stamp named for
   the server to apply it at, the own clock of its frame, and the world takes it at once: what
   the player asks for is in the world, and drawn, in the frame it is asked. Without a stamp it
   has no number the server could confirm it by, and before the server's first word there is no
   world to put it in. */
static void own_request(struct GameMode *self, struct PredictedRequest *request, unsigned number)
{
    if (!number || !g_own.live)
        return;
    request->number = (uint16_t)number;
    request->apply_tick = predicted_wide(&g_own, self->stamp_tick);
    request->left = platform_ticks_ms();
    if (predicted_request(&g_own, request))
        game_mode_own_body_frame(self);
}

void game_mode_own_body_walk(struct GameMode *self, unsigned number, int x, int y)
{
    struct PredictedRequest request = {0};
    const struct Player *player = self->world ? self->world->player : NULL;

    /* The server refuses the walk of a character that sits, is dead or in a state between
       before its core sees the request, and the client knows those states: the walk is
       predicted for one that stands, walks, swings or flinches. A hit's delay on the walk is
       the core's own rule, and the flinch is only its animation (plans/drawn-place.md). What
       else holds a character (a status) is the server's word. */
    if (!player || (player->state != ACTOR_STATE_STAND && player->state != ACTOR_STATE_WALKING
                    && player->state != ACTOR_STATE_ATTACK && player->state != ACTOR_STATE_HURT))
        return;
    request.kind = BODY_REQUEST_WALK;
    request.x = (int16_t)x;
    request.y = (int16_t)y;
    own_request(self, &request, number);
}

/* The distance an item is picked up from: the server's (pc_takeitem's check_distance). */
#define PICKUP_DISTANCE 2

/* A cell of the map as the world has it, from where something is drawn. */
static void drawn_cell(struct GameMode *self, const struct GameActor *actor, int16_t *x, int16_t *y)
{
    int cx, cy;

    game_mode_world_to_cell(self, actor->pos.x, actor->pos.z, &cx, &cy);
    *x = (int16_t)cx;
    *y = (int16_t)cy;
}

/* What an intent is about, as the server takes it (eos-combat's intentPlace): a unit by its id,
   the player's own too for a skill; a skill unit (which is no body), a ground skill's cell and an
   item by their cells. A skill's range is its attack range at its level as the skill list says
   it (skill_attack_range). An item the client does not see is none the server has: that request
   is a cancel there, and here. */
void game_mode_own_body_intent(struct GameMode *self, unsigned number, int kind, int mode, unsigned target, int x, int y,
                               int skill_id, int skill_lv)
{
    struct PredictedRequest request = {0};
    struct GameActor *unit;
    struct Item *item;
    int range;

    switch (kind) {
    case INTENT_ATTACK:
        request.kind = BODY_REQUEST_ATTACK;
        request.target_id = target;
        request.mode = (uint8_t)mode;
        break;
    case INTENT_SKILL_ACTOR:
    case INTENT_SKILL_GROUND:
        range = skill_attack_range(skill_id, skill_lv);
        request.range = (uint8_t)(range < 0 ? 0 : range > (int)BODY_MAX_RANGE ? (int)BODY_MAX_RANGE : range);
        unit = kind == INTENT_SKILL_ACTOR && self->world ? (struct GameActor *)world_get_skill_by_aid(self->world, (int)target) : NULL;
        if (kind == INTENT_SKILL_GROUND) {
            request.kind = BODY_REQUEST_SKILL_CELL;
            request.x = (int16_t)x;
            request.y = (int16_t)y;
        } else if (unit) {
            request.kind = BODY_REQUEST_SKILL_CELL;
            drawn_cell(self, unit, &request.x, &request.y);
        } else {
            request.kind = BODY_REQUEST_SKILL;
            request.target_id = target;
        }
        break;
    case INTENT_PICKUP:
        item = self->world ? world_get_item_by_aid(self->world, (int)target) : NULL;
        request.kind = item ? BODY_REQUEST_PICKUP : BODY_REQUEST_CANCEL;
        request.range = PICKUP_DISTANCE;
        if (item)
            drawn_cell(self, (const struct GameActor *)item, &request.x, &request.y);
        break;
    case INTENT_RELEASE:
        request.kind = BODY_REQUEST_RELEASE;
        break;
    default:
        return;
    }
    /* the world takes a request on a body only for a body it has */
    if ((request.kind == BODY_REQUEST_ATTACK || request.kind == BODY_REQUEST_SKILL) && g_own.live && number) {
        g_ordered = target;
        own_other(self, target);
    }
    own_request(self, &request, number);
}

void game_mode_own_body_gone(unsigned id, DWORD tick)
{
    if (id != (unsigned)g_session1.ownAid)
        predicted_gone(&g_own, id, predicted_wide(&g_own, tick));
}

void game_mode_own_body_frame(struct GameMode *self)
{
    struct BodyGround ground;

    if (!g_own.live || !game_mode_knows_clock(self) || !own_ground(&ground))
        return;
    own_others(self);
    if (!predicted_run(&g_own, &ground, predicted_wide(&g_own, game_mode_own_time(self))))
        return;
    own_draw(self);
}

int game_mode_own_body_walk_waits(void)
{
    uint32_t i;

    for (i = 0; i < g_own.pending_count; i++) {
        if (g_own.pending[i].kind == BODY_REQUEST_WALK)
            return 1;
    }
    return 0;
}

int game_mode_own_body(DWORD *tick, unsigned char *state)
{
    if (!predicted_state(&g_own, state))
        return 0;
    *tick = (DWORD)predicted_now(&g_own);
    return 1;
}
