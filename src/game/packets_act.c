#include <stdlib.h>
#include <string.h>
#include "game/packet_handlers.h"
#include "game/packets.h"
#include "game/mode_msg.h"
#include "session/state.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "world/actor_lookup.h"
#include "world/object_lookup.h"
#include "world/act_track.h"
#include "actors/actor_ops.h"
#include "world/presence.h"
#include "game/net_clock.h"
#include "game/own_body.h"
#include "game/cells.h"
#include "actors/actor_states.h"
#include "actors/render_object.h"
#include "actors/cast_stop.h"
#include "game/skill_cast.h"

#define ACTOR_MSG_ATTACK        0xb
#define DAMAGE_NONE             (-30000)
#define DEFAULT_MOTION_TIME     1440
#define PET_EMOTION_CHANCE         25
#define PET_EMOTION_INTERVAL       60000

static struct GameActor *attacker(struct GameMode *self, unsigned aid)
{
    struct GameActor *actor;

    if (aid == (unsigned)g_session1.ownAid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, aid);
    if (!actor)
        actor = (struct GameActor *)world_get_skill_by_aid(self->world, (int)aid);
    return actor;
}

/* Forgets the swing the server announced for `aid`, if one is open, and gives its target. */
static int swing_close(struct GameMode *self, unsigned aid, unsigned *target)
{
    int i, n = self->open_swing_count;

    for (i = 0; i < n; i++) {
        if (self->open_swings[i].aid != aid)
            continue;
        *target = self->open_swings[i].target;
        for (n--; i < n; i++)
            self->open_swings[i] = self->open_swings[i + 1];
        self->open_swing_count = n;
        return 1;
    }
    return 0;
}

/* Remembers the swing the server announced for `aid`; when the list is full the oldest goes. */
static void swing_open(struct GameMode *self, unsigned aid, unsigned target)
{
    int max = (int)(sizeof self->open_swings / sizeof self->open_swings[0]);
    unsigned old;

    swing_close(self, aid, &old);
    if (self->open_swing_count == max)
        swing_close(self, self->open_swings[0].aid, &old);
    self->open_swings[self->open_swing_count].aid = aid;
    self->open_swings[self->open_swing_count].target = target;
    self->open_swing_count++;
}

static void pet_attack_emotion(struct GameMode *self, unsigned aid)
{
    if (aid == (unsigned)g_session1.ownAid) {
        if (world_get_game_actor_by_aid(self->world, (unsigned)g_session1.aid)) {
            int hunger = session_pet_hunger_level(g_session1.petFullness);
            int intimacy = session_pet_intimacy_level(g_session1.petIntimacy);
            int emotion = session_get_pet_emotion(&g_session1, hunger, intimacy, 1);

            if (rand() % 100 < PET_EMOTION_CHANCE && session_pet_emotion_due(&g_session1, PET_EMOTION_INTERVAL) == 1) {
                mode_send_msg(self, MODE_MSG_PET_ACT, emotion, 0, 0, 0);
            }
        }
    }
}

void game_mode_notify_act(struct GameMode *self, unsigned aid, unsigned target, int start_time,
                                       int attack_mt, int attacked_mt, int damage, int count, int action,
                                       int left_damage, int is_sp_damage)
{
    struct ActorAttack attack;
    struct GameActor *actor;
    unsigned swing_target;

    if (damage == DAMAGE_NONE)
        return;
    attack.damage = damage;
    attack.left_damage = left_damage;
    attack.action = (unsigned char)action;
    attack.target_aid = (int)target;
    attack.start_time = start_time;
    attack.count = (short)count;
    attack.cell_x = -1;
    attack.cell_y = -1;
    attack.attack_mt = attack_mt ? attack_mt : DEFAULT_MOTION_TIME;
    attack.attacked_mt = attacked_mt;
    if (attacked_mt == 0 && !session_is_attack(&g_session1, attack.action))
        attack.attacked_mt = DEFAULT_MOTION_TIME;
    attack.is_sp_damage = (unsigned char)is_sp_damage != 0;
    attack.impact = swing_close(self, aid, &swing_target) && swing_target == target;

    actor = attacker(self, aid);
    if (!actor)
        return;
    if (actor->state == 0 && (unsigned)actor->attack_target_aid != target
        && attack.action != 1 && attack.action != 2 && attack.action != 3)
        pet_attack_emotion(self, aid);
    actor_send(actor, ACTOR_MSG_ATTACK, (intptr_t)&attack, 0);
}

static struct GameActor *swing_actor(struct GameMode *self, unsigned aid)
{
    if (aid == (unsigned)g_session1.ownAid)
        return (struct GameActor *)self->world->player;
    return world_get_game_actor_by_aid(self->world, aid);
}

/* The server's action timing (plans/action-timing.md): a swing of `source` at `target` starts,
   with nothing decided about its hit. Then either a hit brings its impact (game_mode_notify_act)
   or the swing is cancelled (swing_cancel). Its hit is to show `lands_ms` from now, and the
   actor paces its attack animation to that (game_actor_swing_begin). */
static void swing_begin(struct GameMode *self, unsigned source, unsigned target, unsigned attack_motion_ms, int lands_ms)
{
    struct GameActor *actor = swing_actor(self, source);

    if (!actor)
        return;
    swing_open(self, source, target);
    render_object_send(actor, NULL, ACTOR_MSG_SWING_BEGIN, (int)target,
                       attack_motion_ms ? attack_motion_ms : DEFAULT_MOTION_TIME, lands_ms, 0);
}

/* The swing of `source` that is open ends without a hit. */
static void swing_cancel(struct GameMode *self, unsigned source)
{
    struct GameActor *actor;
    unsigned target;

    if (!swing_close(self, source, &target))
        return;
    actor = swing_actor(self, source);
    if (actor)
        actor_send(actor, ACTOR_MSG_SWING_CANCEL, 0, 0);
}

/* The server sends what an actor does as a record (ZC_TRACK_ACT, plans/core-loop.md), a hit as
   a notice (ZC_NOTICE_HIT) and a monster's HP as a record (ZC_TRACK_VITALS), each with the
   server tick it belongs to and often ahead of it. They wait in the mode's queue and are played
   when the clock they are played on reaches the tick: the render clock, or the own clock for
   what the player's own character does and for what is aimed at it (world/act_track.h). Played,
   a swing begins (swing_begin) and idle ends it without a hit (swing_cancel), a cast is
   ZC_USESKILL_ACK with the time the cast still has on that clock, so its bar ends at the cast's
   tick however late the record came, idle also ends a cast as ZC_DISPEL does, a hit is the
   impact of ZC_NOTIFY_ACT2, and the HP is ZC_HP_INFO's. */
#define AM_SET_HP_BAR   0x91

static void play_vitals(struct GameMode *self, const struct VitalsRecord *vitals)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, vitals->gid);

    if (actor)
        render_object_send(actor, NULL, AM_SET_HP_BAR, vitals->hp, vitals->max_hp, 0, 0);
}

/* The cast `gid` shows ends without its skill, if it shows one. */
static void cast_cancel(struct GameMode *self, unsigned gid)
{
    struct GameActor *actor = swing_actor(self, gid);

    if (actor && (actor->casting || actor->state == ACTOR_STATE_CASTING))
        game_actor_stop_cast(actor, gid == (unsigned)g_session1.ownAid);
}

static void play_taken(struct GameMode *self, const struct TimedItem *item, DWORD now);

/* How long after its impact a swing's damage number is here, on the clock the character is
   drawn on: the way down and the own clock's lead over the server's. */
static unsigned own_swing_late(struct GameMode *self)
{
    return self->clock_sync.round_trip / 2 + (unsigned)(game_mode_own_time(self) - game_mode_true_time(self));
}

/* `now` is the clock the item is played on. A swing's hit is to show when that clock reaches
   the tick the swing lands at, and the swing begins now, however late on that clock: its
   animation is paced to the time that is left. The player's own swing is paced to when its
   damage number is here, which is later by own_swing_late: nothing waits on that swing, and its
   hit and its number show together. */
static void play_item(struct GameMode *self, const struct TimedItem *item, DWORD now)
{
    struct TimedItem hp;
    int left;

    switch (item->kind) {
    case TIMED_ACT:
        if (item->u.act.kind == ACT_SWING) {
            left = (int)(item->u.act.mark_tick - now);
            if (item->u.act.gid == (uint32_t)g_session1.ownAid)
                left += (int)own_swing_late(self);
            swing_begin(self, item->u.act.gid, item->u.act.target, item->u.act.motion_ms, left);
        } else if (item->u.act.kind == ACT_CAST) {
            left = (int)(item->u.act.end_tick - now);
            game_mode_zc_useskill_ack(self, item->u.act.gid, item->u.act.target, item->u.act.x, item->u.act.y,
                                      (int)item->u.act.skill_id, item->u.act.element, left > 0 ? (unsigned)left : 0,
                                      item->u.act.disposable, (int)item->u.act.motion_ms);
        } else {
            swing_cancel(self, item->u.act.gid);
            cast_cancel(self, item->u.act.gid);
        }
        break;
    case TIMED_HIT:
        /* the HP this hit took moves with its number, also when the hit is the player's own and
           shown ahead of the render clock */
        while (timed_queue_take_vitals(&self->tracks, item->u.hit.target, item->u.hit.at_tick, &hp))
            play_taken(self, &hp, now);
        game_mode_notify_act(self, item->u.hit.source, item->u.hit.target, (int)item->u.hit.at_tick, 0,
                             (int)item->u.hit.flinch_ms, item->u.hit.damage, item->u.hit.count, item->u.hit.kind,
                             item->u.hit.left_damage, item->u.hit.sp_damage);
        break;
    case TIMED_VITALS:
        play_vitals(self, &item->u.vitals);
        break;
    }
}

/* The own clock: the one the player's own character is drawn on once the server's clock is
   known, and the render clock until then. */
static DWORD own_clock(struct GameMode *self)
{
    if (game_mode_knows_clock(self))
        return game_mode_own_time(self);
    return session_get_server_time(&g_session1);
}

/* The clock `item` is played on (timed_queue_on_own_clock). */
static DWORD item_clock(struct GameMode *self, const struct TimedItem *item)
{
    if (timed_queue_on_own_clock(&self->tracks, item, (uint32_t)g_session1.ownAid))
        return own_clock(self);
    return session_get_server_time(&g_session1);
}

/* What another actor aims at the player's own character is played on the own clock, which is
   ahead of the clock the actor's walk is drawn on. So when the server has the actor standing
   and beginning a swing, the screen can still have it on the last of its walk; an attack or a
   cast begun then would stop it short of its cell's middle. Such a swing or cast waits here
   until the actor is drawn where it stands, as the player's own swing waits for the character
   (game_mode_own_swing), and no longer than until the clock the walk is drawn on reaches the
   act's tick: the walk the server ended before the act is over on that clock then, and what
   is left of it on the screen is an actor that runs behind its walk. One waits for an actor at
   most. */

/* When an act that begins at `tick` can begin on `actor` as it is drawn. */
enum {
    AIMED_NOW,   /* it is drawn where it stands for the act */
    AIMED_WAITS, /* it has yet to arrive there */
    AIMED_PAST   /* it is drawn on what it did after the act began */
};

/* An actor has yet to arrive when the walk of its place (plans/drawn-place.md) ends by `tick`,
   so the server had ended it before the act, and the drawn point is not on that walk's end yet;
   or when a move record that waits, of a tick not after it, will move the actor. The record
   that says the actor stands where it is drawn, which the server sends when an actor stops to
   swing, is not waited for. Once the render clock, which the place is played on, has reached
   `tick`, nothing is waited for. A walk that ends later is what the actor did next (a monster
   walks on while its swing is under way): with the drawn point on its way along it, the actor
   is past the act's beginning, and an attack begun now would stop the walk. One whose first
   step is still to come leaves the actor where it swings from. */
static int aimed_when(struct GameMode *self, struct GameActor *actor, DWORD tick)
{
    DWORD now = session_get_server_time(&g_session1);
    int waited = (int)(now - tick) >= 0;
    const struct MoveRecord *record = drawn_record(&actor->place);
    struct MoveCell cells[MOVE_TRACK_STEPS + 1];
    int x, y, count;

    if (record && record->steps) {
        count = move_record_cells(record, cells);
        if ((int)(cells[count - 1].tick - tick) <= 0) {
            if ((int)(now - cells[count - 1].tick) < 0 || !game_actor_on_place(actor))
                return waited ? AIMED_NOW : AIMED_WAITS;
        } else if ((int)(now - cells[0].tick) >= 0 || !game_actor_on_place(actor)) {
            return AIMED_PAST;
        }
    }
    if (waited)
        return AIMED_NOW;
    game_mode_world_to_cell(self, actor->pos.x, actor->pos.z, &x, &y);
    return move_track_will_move(&actor->move_track, x, y, tick) ? AIMED_WAITS : AIMED_NOW;
}

/* Where the act of `gid` that waits for its actor to stand is, or -1. */
static int waiting_index(const struct GameMode *self, unsigned gid)
{
    int i;

    for (i = 0; i < self->aimed_waiting_count; i++)
        if (self->aimed_waiting[i].gid == gid)
            return i;
    return -1;
}

static void waiting_remove(struct GameMode *self, int at)
{
    for (at++; at < self->aimed_waiting_count; at++)
        self->aimed_waiting[at - 1] = self->aimed_waiting[at];
    self->aimed_waiting_count--;
}

/* Plays a swing or a cast of another actor at the player's own character at `now`, the own
   clock. That clock had passed the act's tick before its record could be here, and may have run
   on while the act waited: a swing is paced to the time it has left, so that it lands when the
   own clock reaches the tick it lands at, and a cast shows the time it still has (play_item).
   With `shown` 0 the actor is drawn on what it did next: the swing is open, so that its hit is
   its impact, and neither its animation nor a cast shows. */
static void play_aimed(struct GameMode *self, const struct ActRecord *act, DWORD now, int shown)
{
    struct TimedItem item;

    if (!shown) {
        if (act->kind == ACT_SWING && swing_actor(self, act->gid))
            swing_open(self, act->gid, act->target);
        return;
    }
    item.kind = TIMED_ACT;
    item.tick = act->at_tick;
    item.actor = act->gid;
    item.u.act = *act;
    play_item(self, &item, now);
}

/* What waited for its actor to arrive and has it there is played; what waited for an actor
   that is gone is forgotten. */
static void play_waiting(struct GameMode *self, DWORD own_now)
{
    int i = 0;

    while (i < self->aimed_waiting_count) {
        struct ActRecord act = self->aimed_waiting[i];
        struct GameActor *actor = swing_actor(self, act.gid);
        int when = actor ? aimed_when(self, actor, act.at_tick) : AIMED_NOW;

        if (when == AIMED_WAITS) {
            i++;
            continue;
        }
        waiting_remove(self, i);
        if (actor)
            play_aimed(self, &act, own_now, when == AIMED_NOW);
    }
}

/* An item leaves the queue and is played: an act or vitals record is the newest of its track
   that was, for whatever of that track comes later (world/act_track.h). A swing or a cast of
   another actor at the player's own character waits while that actor has yet to arrive
   (play_waiting); with as many waiting as there is room for, it is played as it comes. A newer
   act record of the actor takes the place of the one that waited, which then was never shown.
   The hit of a swing that still waits is that swing's impact: the swing begins first, without
   its animation when the actor is drawn walking, which the attack would stop. */
static void play_taken(struct GameMode *self, const struct TimedItem *item, DWORD now)
{
    struct GameActor *actor = item->kind != TIMED_HIT ? swing_actor(self, item->actor) : NULL;
    unsigned own = (unsigned)g_session1.ownAid;
    struct ViewUnit *unit;
    int at;

    if (item->kind != TIMED_HIT && (unit = view_floors_unit(&self->view_floors, item->actor)) != NULL)
        record_floors_played(&unit->floors, item);
    if (item->kind == TIMED_ACT && item->actor != own) {
        if ((at = waiting_index(self, item->actor)) >= 0)
            waiting_remove(self, at);
        if (item->u.act.kind != ACT_IDLE && item->u.act.target == own) {
            int room = self->aimed_waiting_count < (int)(sizeof self->aimed_waiting / sizeof self->aimed_waiting[0]);
            int when = actor ? aimed_when(self, actor, item->u.act.at_tick) : AIMED_NOW;

            if (when == AIMED_WAITS && room)
                self->aimed_waiting[self->aimed_waiting_count++] = item->u.act;
            else
                play_aimed(self, &item->u.act, now, when != AIMED_PAST);
            return;
        }
    } else if (item->kind == TIMED_HIT && (at = waiting_index(self, item->u.hit.source)) >= 0
               && self->aimed_waiting[at].kind == ACT_SWING && self->aimed_waiting[at].target == item->u.hit.target) {
        struct ActRecord swing = self->aimed_waiting[at];
        const struct GameActor *source = swing_actor(self, swing.gid);

        waiting_remove(self, at);
        play_aimed(self, &swing, now, !source || source->state != ACTOR_STATE_WALKING);
    }
    play_item(self, item, now);
}

/* The move track of the actor of `gid`, or NULL when it has none. */
static struct MoveTrack *track_of(void *context, uint32_t gid)
{
    struct GameActor *actor = swing_actor((struct GameMode *)context, gid);

    return actor ? &actor->move_track : NULL;
}

/* Where a unit's records go (world/presence.h): against the floors of its unit id, which
   outlive its actor. */
static struct Presence presence_of(struct GameMode *self)
{
    struct Presence presence;

    presence.view = &self->view_floors;
    presence.queue = &self->tracks;
    presence.wait = &self->record_wait;
    presence.own = (uint32_t)g_session1.ownAid;
    presence.track_of = track_of;
    presence.context = self;
    return presence;
}

/* Files an item in the queue, in whatever order items come. An act or vitals record whose unit
   is out of view or has no actor waits for it (game_mode_play_tracks): through the delivery
   piece a record can come before the packet that makes its actor known. An item the full queue
   hands back is played at once. */
static void file_timed(struct GameMode *self, const struct TimedItem *item)
{
    struct Presence presence = presence_of(self);
    struct TimedItem early;

    if (presence_file_timed(&presence, platform_ticks_ms(), item, &early) == TIMED_FILED_EARLY)
        play_taken(self, &early, item_clock(self, &early));
}

static void file_move(struct GameMode *self, unsigned gid, const struct MoveRecord *record)
{
    struct Presence presence = presence_of(self);

    presence_file_move(&presence, platform_ticks_ms(), gid, record);
}

/* ZC_TRACK_MOVE (plans/core-loop.md): where another actor is from a server tick on. The record
   waits on the actor until the render clock reaches its tick (game_actor_process_state), and
   the track takes it in whatever order records come (world/move_track.h) against its unit's floors (world/presence.h). One whose actor is
   not there yet waits for it (game_mode_play_tracks): through the delivery piece a record can
   come before the packet that makes its actor known. How long after its tick it came, by the
   server's clock, feeds the render delay; a record the server sent later than it made it says
   nothing about that. The player's own character has none: its walk is the predicted world's
   (game/own_body.c). */
void game_mode_zc_track_move(struct GameMode *self, const unsigned char *pkt)
{
    struct MoveRecord record;
    uint32_t gid;

    if (!move_record_read(pkt, (size_t)(pkt[2] | pkt[3] << 8), &gid, &record))
        return;
    game_mode_own_body_heard(gid, record.at_tick);
    if (game_mode_knows_clock(self) && !record.old)
        render_delay_sample(&self->render_delay, (int)(game_mode_true_time(self) - record.at_tick));
    if (gid == (uint32_t)g_session1.ownAid)
        return;
    file_move(self, gid, &record);
}

/* ZC_VIEW_STAMP (plans/presence.md): the unit `gid` comes into view or goes. The stamp belongs
   to the very next packet only, and is kept until then (game_mode_view_stamped). */
void game_mode_zc_view_stamp(struct GameMode *self, const unsigned char *pkt, unsigned size)
{
    self->view_stamp_waits = (unsigned char)view_stamp_read(pkt, size, &self->view_stamp);
    if (self->view_stamp_waits)
        game_mode_own_body_heard(self->view_stamp.gid, self->view_stamp.tick);
}

void game_mode_view_stamped(struct GameMode *self, int packet_id, const unsigned char *pkt, unsigned size)
{
    struct Presence presence = presence_of(self);
    const struct ViewStamp *stamp = &self->view_stamp;
    int at;

    if (!self->view_stamp_waits)
        return;
    self->view_stamp_waits = 0;
    if (!view_stamp_stamps(stamp, packet_id, pkt, size))
        return;
    /* its floors are set, what it holds is filed again against them, and the predicted world is
       told the going's tick */
    if (!presence_stamp(&presence, platform_ticks_ms(), stamp) || stamp->kind != VIEW_GOES)
        return;
    /* a swing at the own character that waited for its actor goes with it */
    if ((at = waiting_index(self, stamp->gid)) >= 0)
        waiting_remove(self, at);
    game_mode_own_body_gone(stamp->gid, stamp->tick);
}

/* Files what arrived. How long after its tick it came feeds the render delay; what was sent
   ahead of its tick does not (render_delay_sample), nor does a record sent later than it was
   made. */
static void file_item(struct GameMode *self, const struct TimedItem *item)
{
    if (game_mode_knows_clock(self) && !(item->kind == TIMED_ACT && item->u.act.old))
        render_delay_sample(&self->render_delay, (int)(game_mode_true_time(self) - item->tick));
    file_timed(self, item);
}

/* What waited for its unit and has it in view with an actor now is filed as if it had just
   come. */
static void file_waiting(struct GameMode *self)
{
    struct Presence presence = presence_of(self);
    struct TimedItem early;
    int filed;

    while (presence_file_waiting(&presence, platform_ticks_ms(), &early, &filed))
        if (filed == TIMED_FILED_EARLY)
            play_taken(self, &early, item_clock(self, &early));
}

/* The own body's swing as the predicted world has it, once a frame (game_mode_play_tracks): with
   `has` one in its windup at `target`, begun at at_tick, landing at `lands`, of a body whose
   attack motion is motion_ms. What is shown follows it (world/own_swing.h). A swing the
   world's body began is shown at once, on the own clock, its animation paced so that its hit
   meets the damage number, which comes with the server's notice (play_item). One the world has no
   more before it landed is taken back. The server's record of a swing that is shown is not
   played (game_mode_zc_track_act). A swing begins on a character that stands: while the
   character still walks the last of the way to the cell the body swings on (it is steered
   there after a correction), the swing waits, and begins when the character is there. */
void game_mode_own_swing(struct GameMode *self, int has, unsigned target, DWORD at_tick, DWORD lands,
                         unsigned motion_ms)
{
    DWORD own_now = game_mode_own_time(self);
    const struct GameActor *own = (const struct GameActor *)self->world->player;
    struct TimedItem item;
    int what;

    if (has && own && own->state == ACTOR_STATE_WALKING && !own_swing_is_shown(&self->own_swing, target, at_tick))
        return;
    what = own_swing_follow(&self->own_swing, has, target, at_tick, lands, own_now);

    if (what == OWN_SWING_TAKE_BACK || what == OWN_SWING_REPLACE)
        swing_cancel(self, (unsigned)g_session1.ownAid);
    if (what == OWN_SWING_SHOW || what == OWN_SWING_REPLACE) {
        own_swing_shown(&self->own_swing, (uint32_t)g_session1.ownAid, motion_ms ? motion_ms : DEFAULT_MOTION_TIME,
                        &item.u.act);
        item.kind = TIMED_ACT;
        item.tick = item.u.act.at_tick;
        item.actor = item.u.act.gid;
        play_item(self, &item, own_now);
    }
}

void game_mode_zc_track_act(struct GameMode *self, const unsigned char *pkt)
{
    struct TimedItem item;

    if (!act_record_read(pkt, &item.u.act))
        return;
    game_mode_own_body_heard(item.u.act.gid, item.u.act.at_tick);
    if (item.u.act.gid == (uint32_t)g_session1.ownAid) {
        /* the record of the swing that is already shown is not played again */
        if (item.u.act.kind == ACT_SWING && own_swing_record(&self->own_swing, &item.u.act))
            return;
    }
    item.kind = TIMED_ACT;
    item.tick = item.u.act.at_tick;
    item.actor = item.u.act.gid;
    /* one sent later than it was made, as with an entry, is played at once */
    if (item.u.act.old && game_mode_knows_clock(self) && (int32_t)(item.tick - item_clock(self, &item)) > 0)
        item.tick = item_clock(self, &item);
    file_item(self, &item);
}

void game_mode_zc_track_vitals(struct GameMode *self, const unsigned char *pkt)
{
    struct TimedItem item;

    vitals_record_read(pkt, &item.u.vitals);
    item.kind = TIMED_VITALS;
    item.tick = item.u.vitals.at_tick;
    item.actor = item.u.vitals.gid;
    file_item(self, &item);
}

void game_mode_zc_notice_hit(struct GameMode *self, const unsigned char *pkt)
{
    struct TimedItem item;

    hit_notice_read(pkt, &item.u.hit);
    item.kind = TIMED_HIT;
    item.tick = item.u.hit.at_tick;
    item.actor = item.u.hit.source;
    file_item(self, &item);
}

void game_mode_play_tracks(struct GameMode *self)
{
    DWORD render = session_get_server_time(&g_session1);
    DWORD own = own_clock(self);
    struct TimedItem item;
    unsigned target = 0, motion_ms = 0;
    DWORD at_tick = 0, lands = 0;
    uint32_t now;
    int has = game_mode_own_body_swing(&target, &at_tick, &lands, &motion_ms);

    game_mode_own_swing(self, has, target, at_tick, lands, motion_ms);
    file_waiting(self);
    play_waiting(self, own);
    while (timed_queue_take(&self->tracks, (uint32_t)g_session1.ownAid, own, render, &item, &now))
        play_taken(self, &item, now);
}

/* The server ended the player's intent on its own (plans/intents.md): the attack cursor state
   goes. An end of an older intent says nothing about the one that replaced it. */
static void intent_ended(struct GameMode *self, unsigned seq)
{
    if (seq != self->intent_seq)
        return;
    if (self->world->player)
        self->world->player->intent_hold = 0;
    mode_send_msg(self, MODE_MSG_SET_ATTACK_IN_PROGRESS, 0, 0, 0, 0);
}

/* ZC_OWN_INTENT (plans/core-loop.md): the end of one of the player's intents, by the intent's
   number, which is its stamp's (game_mode_send_intent). It clears the attack cursor state
   (intent_ended). What the intent makes of the character before that, its walk into reach and
   its swing, is the predicted world's to show, from the server's word of the body
   (game/own_body.c), and the server says nothing else of it. Its seq and state are read. */
#pragma pack(push, 1)
struct PACKET_ZC_OWN_INTENT {
    unsigned short id;
    unsigned short seq;
    unsigned char state;
    unsigned char reason;
    unsigned tick;
    unsigned short x, y;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_OWN_INTENT, 14);

/* The state the server sends: ended. */
enum { OWN_INTENT_ENDED = 3 };

void game_mode_zc_own_intent(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_OWN_INTENT *packet = (const struct PACKET_ZC_OWN_INTENT *)pkt;

    if (packet->state == OWN_INTENT_ENDED)
        intent_ended(self, packet->seq);
}

