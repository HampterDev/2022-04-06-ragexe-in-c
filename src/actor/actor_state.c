#include "actor/actor_ops.h"
#include "actor/render_ops.h"
#include "session/state.h"
#include "world/mode_dispatch.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/game_actor.h"
#include "game/gamemode.h"
#include "game/net_clock.h"
#include "core/msg_str.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "world/world.h"
#include "world/move_track.h"
#include "world/move_place.h"
#include "actor/actor_states.h"
#include <string.h>
#include "world/pathfinder_state.h"
#include <stdint.h>
#include "platform/clock.h"

#define GAME_MODE_SEND(mode, msg, p1, p2, p3, p4) \
    mode_send_message((struct Mode *)(mode), (msg), (p1), (p2), (p3), (p4))

void game_actor_report_guild_member_place(struct GameActor *self, float *x, float *z)
{
    struct World *world;
    float at[2];

    if (game_actor_get_guild_id(self) < 1)
        return;
    if (!session_is_player_job(&g_session1, game_actor_get_job(self)))
        return;
    world = game_current_world();
    if (!world)
        return;
    if (self->aid == g_session1.ownAid) {
        world_update_near_flag_at(world, x, z);
        return;
    }
    at[0] = *x;
    at[1] = *z;
    world_update_near_flag_for_id(world, self->aid, at);
}

static int fade(struct GameActor *self)
{
    unsigned now, left;

    if (!self->fade_until)
        return 1;
    now = platform_ticks_ms();
    if (now > self->fade_until)
        return 0;
    left = (self->fade_until - now) >> 1;
    ((unsigned char *)&self->argb)[3] = (unsigned char)(left > 0xff ? 0xff : left);
    return 1;
}

unsigned char game_actor_process_fade_out(struct GameActor *self)
{
    return (unsigned char)fade(self);
}

static void pet_idle_emotion(struct GameActor *self, struct GameMode *mode)
{
    if (self->object_type != 7 || self->aid != g_session1.aid || !self->pet_emotion_tick)
        return;
    if (platform_ticks_ms() - self->pet_emotion_tick <= 3000)
        return;
    if (rand() % 100 < 25) {
        int hunger = session_pet_hunger_level(g_session1.petFullness);
        int intimacy = session_pet_intimacy_level(g_session1.petIntimacy);
        int emotion = session_get_pet_emotion(&g_session1, hunger, intimacy, 4);

        GAME_MODE_SEND(mode, 0x94, emotion, 0, 0, 0);
    }
    self->pet_emotion_tick = 0;
}

/* A push is joined over the original's slide time (plans/drawn-place.md, part 4). */
#define FOLLOW_PUSH_MS      200

/* The world point of a place in 256ths of a cell, a cell's middle at a whole multiple of 256,
   and back. */
static void place_to_world(struct GameMode *mode, int32_t px, int32_t py, float *x, float *z)
{
    float zoom = (float)mode->world->attr->zoom, ox, oz;

    game_mode_cell_to_world(mode, 0, 0, &ox, &oz);
    *x = ox + zoom * (float)px / (float)MOVE_PLACE_CELL;
    *z = oz + zoom * (float)py / (float)MOVE_PLACE_CELL;
}

static void world_to_place(struct GameMode *mode, float x, float z, int32_t *px, int32_t *py)
{
    float zoom = (float)mode->world->attr->zoom, ox, oz, fx, fz;

    game_mode_cell_to_world(mode, 0, 0, &ox, &oz);
    fx = (x - ox) * (float)MOVE_PLACE_CELL / zoom;
    fz = (z - oz) * (float)MOVE_PLACE_CELL / zoom;
    *px = (int32_t)(fx < 0.0f ? fx - 0.5f : fx + 0.5f);
    *py = (int32_t)(fz < 0.0f ? fz - 0.5f : fz + 0.5f);
}

/* The actor's walk speed, ms a cell. */
static int walk_ms(const struct GameActor *self, const struct World *world)
{
    int own = self == (struct GameActor *)world->player ? g_session1.walkSpeed : self->walk_speed;

    return own > 0 ? own : 150;
}

void game_actor_set_place(struct GameActor *self, const struct MoveRecord *record)
{
    struct GameMode *mode = game_current_mode();
    struct MoveCell cells[MOVE_TRACK_STEPS + 1];
    int count = move_record_cells(record, cells);
    struct MovePlace now;
    int32_t px, py, dx, dy, gap;

    self->place = *record;
    self->has_place = 1;
    self->cell_x = cells[count - 1].x;
    self->cell_y = cells[count - 1].y;
    if (record->steps && record->speed_ms && record->cause == MOVE_CAUSE_WALKED)
        self->walk_speed = (int)record->speed_ms;
    /* how the drawn point joins it: a walk, a stop and a correction are walked off; a push slides
       over its own short time; a placing is taken at once */
    self->place_ms = walk_ms(self, mode->world);
    if (record->cause == MOVE_CAUSE_PLACED) {
        self->place_ms = 0;
    } else if (record->cause == MOVE_CAUSE_PUSHED) {
        move_place(record, game_actor_walk_time(self), &now);
        world_to_place(mode, self->pos.x, self->pos.z, &px, &py);
        dx = now.x - px;
        dy = now.y - py;
        dx = dx < 0 ? -dx : dx;
        dy = dy < 0 ? -dy : dy;
        gap = dx > dy ? dx : dy;
        /* ms a cell such that 13/10 of it covers the gap in the slide's time */
        self->place_ms = gap > MOVE_PLACE_CELL / 16
                             ? FOLLOW_PUSH_MS * MOVE_PLACE_CELL * MOVE_FOLLOW_RATE_NUM / (gap * MOVE_FOLLOW_RATE_DEN)
                             : FOLLOW_PUSH_MS;
        if (self->place_ms < 1)
            self->place_ms = 1;
    }
}

int game_actor_walk_record(struct GameActor *self, int src_x, int src_y, int sub_x, int sub_y, int dst_x, int dst_y,
                           int speed_ms, int cause, unsigned start)
{
    /* the server's direction codes: 0 north (y + 1), then counter-clockwise */
    static const signed char dir_x[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
    static const signed char dir_y[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    struct MoveRecord record;
    size_t i;
    int dir;

    self->path_start = start;
    if (!game_actor_find_path(self, src_x, src_y, sub_x, sub_y, dst_x, dst_y, speed_ms) || !self->path.count)
        return 0;
    memset(&record, 0, sizeof record);
    record.at_tick = self->path.cells[0].time;
    record.x = self->path.cells[0].x;
    record.y = self->path.cells[0].y;
    record.speed_ms = (unsigned)speed_ms;
    record.cause = (unsigned char)cause;
    for (i = 1; i < self->path.count && record.steps < MOVE_TRACK_STEPS; i++) {
        for (dir = 0; dir < 8; dir++)
            if (self->path.cells[i].x - self->path.cells[i - 1].x == dir_x[dir]
                && self->path.cells[i].y - self->path.cells[i - 1].y == dir_y[dir])
                break;
        if (dir == 8)
            break;
        record.path[record.steps++] = (unsigned char)dir;
    }
    if (record.steps) {
        record.land_ms = self->path.cells[1].time - self->path.cells[0].time;
        record.dir = record.path[0];
    }
    game_actor_set_place(self, &record);
    return 1;
}

unsigned game_actor_walk_time(struct GameActor *self)
{
    struct GameMode *mode = game_current_mode();

    /* The player's own walk is drawn where the server will have it (plans/netcode.md), and
       its own records are played on that clock. */
    if (self == (struct GameActor *)mode->world->player && game_mode_knows_clock(mode))
        return game_mode_own_time(mode);
    return session_get_server_time(&g_session1);
}

int game_actor_on_place(struct GameActor *self)
{
    struct GameMode *mode = game_current_mode();
    struct MovePlace place;
    int32_t px, py;

    if (!self->has_place)
        return 1;
    move_place(&self->place, game_actor_walk_time(self), &place);
    world_to_place(mode, self->pos.x, self->pos.z, &px, &py);
    return px == place.x && py == place.y;
}

int game_actor_play_place(struct GameActor *self)
{
    struct GameMode *mode = game_current_mode();
    struct World *world = mode->world;
    struct MovePlace place;
    Vec3 from = self->pos;
    unsigned real = platform_ticks_ms();
    int since = (int)(real - self->place_real), moved, cell_was_x, cell_was_y, cell_x, cell_y;
    int32_t px, py;
    float dx, dz;

    self->place_real = real;
    if (self->move_track.count) {
        struct ViewUnit *unit = view_floors_unit(&mode->view_floors, (uint32_t)self->aid);
        struct MoveRecord record;

        if (unit && move_track_take(&self->move_track, &unit->floors, game_actor_walk_time(self), &record))
            game_actor_set_place(self, &record);
    }
    if (!self->has_place)
        return 0;
    move_place(&self->place, game_actor_walk_time(self), &place);
    world_to_place(mode, self->pos.x, self->pos.z, &px, &py);
    cell_was_x = (px + MOVE_PLACE_CELL / 2) >> 8;
    cell_was_y = (py + MOVE_PLACE_CELL / 2) >> 8;
    moved = move_follow(&px, &py, place.x, place.y, since < 0 ? 0u : (uint32_t)since, (uint32_t)self->place_ms);
    if (moved) {
        place_to_world(mode, px, py, &self->pos.x, &self->pos.z);
        self->pos.y = c3d_attr_get_height(world->attr, self->pos.x, self->pos.z);
        dx = self->pos.x - from.x;
        dz = self->pos.z - from.z;
        self->walked = self->walked + float_sqrt(dx * dx + dz * dz);
        cell_x = (px + MOVE_PLACE_CELL / 2) >> 8;
        cell_y = (py + MOVE_PLACE_CELL / 2) >> 8;
        if (cell_x != cell_was_x || cell_y != cell_was_y)
            game_actor_report_guild_member_place(self, &self->pos.x, &self->pos.z);
    } else {
        dx = dz = 0.0f;
    }
    /* the state chooses the animation from the place and the drawn point (part 3) */
    if (self->state == ACTOR_STATE_STAND && moved && !self->glide) {
        actor_set_state(self, ACTOR_STATE_WALKING);
    } else if (self->state == ACTOR_STATE_WALKING && !place.moving && px == place.x && py == place.y) {
        actor_set_state(self, ACTOR_STATE_STAND);
        self->glide = 0;
        self->motion_held = 0;
        if (self->body_relocation) {
            self->body_relocation = 0;
            render_object_play_motion_range((struct RenderObject *)self, 0x60, 2, 2, 0x60, 2, 1, 0xbb8, 0);
        }
        follow_set_times(0, 0);
    }
    if (self->state == ACTOR_STATE_WALKING && !self->glide) {
        if (moved) {
            double fx = (double)dx, fz = (double)-dz;
            float roty = (float)atan2(fx, fz) / 3.141592f * 180.0f;

            if (roty >= 360.0f)
                roty -= 360.0f;
            if (0.0f > roty)
                roty += 360.0f;
            self->roty = roty;
        }
        self->action = game_actor_dir_of((struct GameObject *)self, self->roty) + self->base_action;
        actor_walk_motion(self);
    }
    return moved;
}

unsigned char game_actor_process_state(struct GameActor * self)
{
    struct GameMode *mode;
    struct World *world;
    int moved;

    if (self->motion_hold_until && platform_ticks_ms() > self->motion_hold_until) {
        self->motion_held = 0;
        self->motion_hold_until = 0;
    }
    if (!fade(self))
        return 0;
    mode = game_current_mode();
    world = mode->world;
    moved = game_actor_play_place(self);

    switch (self->state) {
    case 0x00:
        if (self->play_mode == 1 && self->motion_finished)
            actor_set_state(self, 0);
        else
            pet_idle_emotion(self, mode);
        render_object_update_motion((struct RenderObject *)self);
        break;

    case 0x01:
        /* the walk is drawn by the place it follows (game_actor_play_place) */
        break;

    case 0x05: case 0x07: case 0x2d:
        self->casting = 0;
    case 0x02: case 0x09: case 0x0c: case 0x34: case 0x35: case 0x36: case 0x38: case 0x39: case 0x3a:
    case 0x3b:
        render_object_update_motion((struct RenderObject *)self);
        if (self->motion_finished)
            actor_set_state(self, 0);
        break;

    case 0x03:
        self->script_range_on = 0;
        self->script_steps_on = 0;
        self->script_pingpong_on = 0;
        self->motion_held = 0;
        self->casting = 0;
        render_object_update_motion((struct RenderObject *)self);
        if (!self->motion_finished || self->hit_queue.size)
            break;
        GAME_MODE_SEND(mode, 0xd, self->aid, 0, 0, 0);
        {
            int job = game_actor_get_job(self);

            if ((job > 0x1f && job < 0x3e8) || job_is_monster(job) || self->object_type == 9
                || session_is_homun_or_merc_job(&g_session1, job)) {
                actor_make_corpse(self);
                return 0;
            }
        }
        break;

    case 0x04:
        /* a flinch holds nothing: the place moves the actor under it, a push included */
        self->casting = 0;
        render_object_update_motion((struct RenderObject *)self);
        if (self->motion_finished)
            actor_set_state(self, 0);
        break;

    case 0x08: {
        int frames;

        self->casting = 0;
        render_object_update_motion((struct RenderObject *)self);
        frames = (int)((float)(platform_ticks_ms() - self->start_tick) / 24.0f);
        if (self->attack_target_aid && frames % 34 == 0) {
            struct RenderObject *other = (struct RenderObject *)game_find_actor(self->attack_target_aid);

            if (other)
                render_object_turn_toward((struct RenderObject *)self, other->pos.x, other->pos.z);
        }
        break;
    }

    case 0x0a: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
    case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1e: case 0x1f: case 0x20: case 0x21:
    case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x28: case 0x29: case 0x2a:
    case 0x2b: case 0x2c: case 0x2f: case 0x30: case 0x31: case 0x32: case 0x33: case 0x37:
        self->casting = 0;
        render_object_update_motion((struct RenderObject *)self);
        break;

    case 0x0d: {
        unsigned now;

        if (!self->casting) {
            self->casting = 1;
            if (session_is_player_job(&g_session1, game_actor_get_job(self))) {
                int sex = self->sex;
                uint64_t weapon =
                    ((uint64_t)actor_get_weapon(self));

                if (session_job_accepts_weapon(&g_session1, game_actor_get_job(self), sex, weapon))
                    render_object_set_action((struct RenderObject *)self, 0x58, 1);
                else
                    render_object_set_action((struct RenderObject *)self, 0x50, 1);
            } else {
                render_object_set_action((struct RenderObject *)self, 0x10, 1);
            }
            self->motion_held = 1;
        }
        platform_ticks_ms();
        now = platform_ticks_ms();
        if (now >= self->cast_end_tick) {
            self->casting = 0;
            self->motion_held = 0;
            actor_set_state(self, 0);
            render_object_send(self, NULL, 0x53, 0, 0, 0, 0);
        }
        render_object_update_motion((struct RenderObject *)self);
        break;
    }
    }

    self->process_tick = platform_ticks_ms();
    if (moved)
        self->pos.y = c3d_attr_get_height(world->attr, self->pos.x, self->pos.z);
    return 1;
}

#define ACTOR_SEND0(actor, msg, p2) render_object_send((actor), NULL, (msg), (p2), 0, 0, 0)

static void launch_plain(struct GameActor *self, int effect_id)
{
    Vec3 zero = { 0, 0, 0 };

    game_actor_launch_effect(self, effect_id, zero, 0.0f);
}

static void stop_effects(struct GameActor *self, int effect_id)
{
    while (able_to_make_effect_is_effect_exist(self, effect_id))
        ACTOR_SEND0(self, 0x6d, effect_id);
}

static void script3(struct GameActor *self, unsigned char frames, int action, int motion, int last,
                    int close_motion, int duration, int loop)
{
    self->script_step_frames = frames;
    render_object_play_motion_range((struct RenderObject *)self, (unsigned char)action, (unsigned char)motion,
               (unsigned char)last, (unsigned char)action, (unsigned char)close_motion, 1, duration, loop);
}

static void script1(struct GameActor *self, int action, int m0, int m1, int m2, int m3, int m4, int duration)
{
    unsigned char a = (unsigned char)action;

    render_object_play_motion_steps((struct RenderObject *)self, a, (unsigned char)m0, a, (unsigned char)m1, a,
               (unsigned char)m2, a, (unsigned char)m3, a, (unsigned char)m4, 1, duration, 0);
}

static void face_target(struct GameActor *self, struct GameMode *mode)
{
    struct World *world = mode->world;
    struct GameActor *target;

    if (self->attack_target_aid == g_session1.ownAid) {
        target = (struct GameActor *)world->player;
        if (self->aid == self->attack_target_aid)
            return;
    } else {
        target = world_get_game_actor_by_aid(world, self->attack_target_aid);
        if (!target)
            return;
    }
    render_object_turn_toward((struct RenderObject *)self, target->pos.x, target->pos.z);
}

void game_actor_set_state(struct GameActor * self, int state)
{
    struct GameMode *mode;
    unsigned char member;
    int aid, base, old;

    g_session1.homun_ai_state[0] = 0;
    g_session1.mercOrder = 0;
    if (self->state == 3 || self->state_locked || state == -1)
        return;
    mode = game_current_mode();
    aid = self->aid;
    member = g_session1.ownAid != aid && game_actor_get_guild_id(self) >= 1
             && session_is_player_job(&g_session1, game_actor_get_job(self));
    if (state != self->state && self->state == 1 && member) {
        float at[2];

        at[0] = self->pos.x;
        at[1] = self->pos.z;
        world_place_id_at_cell(mode->world, aid, at);
    }
    if (game_actor_get_transform_job(self) > 0)
        base = game_actor_transform_action_of(state);
    else
        base = game_actor_action_of(self, state);

    switch (state) {
    case 0x00:
        render_object_set_action((struct RenderObject *)self, base, 0);
        self->motion_finished = 1;
        actor_set_unit_cell(self);
        break;
    case 0x01:
        if (!self->body_relocation)
            render_object_set_action((struct RenderObject *)self, base, 0);
        actor_clear_unit_cell(self);
        break;
    case 0x07:
        if (self->state == 6)
            return;
    case 0x02: case 0x09:
        render_object_set_action((struct RenderObject *)self, base, 1);
        face_target(self, mode);
        if (game_actor_get_job(self) == 0x505)
            game_actor_launch_ranged_attack(self, self->attack_target_aid, 28.0f);
        self->attack_motion_count = (float)actor_get_attack_motion(self);
        game_actor_pace_attack(self);
        actor_set_unit_cell(self);
        break;
    case 0x03:
        self->script_range_on = 0;
        self->script_steps_on = 0;
        self->script_pingpong_on = 0;
        self->motion_held = 0;
        if (self->body_state == 2) {
            ACTOR_SEND0(self, 0x3b, 0);
            ACTOR_SEND0(self, 0x3e, 0);
        }
        self->slant_depth = 1;
        self->motion_finished = 0;
        self->motion_held = 0;
        render_object_set_action((struct RenderObject *)self, base, 1);
        break;
    case 0x04: {
        struct World *world;
        struct GameActor *target;
        int job;

        if (self->script_range_on || self->script_steps_on || self->script_pingpong_on)
            return;
        world = mode->world;
        if (self->attack_target_aid == g_session1.ownAid)
            target = (struct GameActor *)world->player;
        else
            target = world_get_game_actor_by_aid(world, self->attack_target_aid);
        if (target)
            ACTOR_SEND0(target, 0x57, 0);
        render_object_set_action((struct RenderObject *)self, base, 1);
        job = game_actor_get_job(self);
        if (job != 0xe6 && job != 0x8d)
            actor_set_unit_cell(self);
        break;
    }
    case 0x05:
        self->casting = 0;
        self->motion_held = 0;
    case 0x0b:
        render_object_set_action((struct RenderObject *)self, base, 1);
        break;
    case 0x0c:
        render_object_set_action((struct RenderObject *)self, base, 1);
        actor_set_unit_cell(self);
        break;
    case 0x0d:
        if (session_is_player_job(&g_session1, game_actor_get_job(self))) {
            int sex = self->sex;
            uint64_t weapon =
                ((uint64_t)actor_get_weapon(self));

            session_job_accepts_weapon(&g_session1, game_actor_get_job(self), sex, weapon);
            render_object_set_action((struct RenderObject *)self, base, 1);
        } else {
            render_object_set_action((struct RenderObject *)self, 0x10, 1);
        }
        self->motion_held = 1;
        break;
    case 0x10:
        self->script_step_frames = 10;
        render_object_play_motion_range((struct RenderObject *)self, (unsigned char)base, 1, 3, 0, 0, 1, 99999999, 1);
        break;
    case 0x11:
        script3(self, 10, base, 1, 3, 1, 99999999, 1);
        break;
    case 0x12:
        script3(self, 10, base, 0, 0, 0, 3000, 0);
        break;
    case 0x13:
        self->script_step_frames = 10;
        script1(self, base, 0, 0, 0, 1, 1, 3000);
        stop_effects(self, 0x1be);
        stop_effects(self, 0x1bd);
        launch_plain(self, 0x1bd);
        launch_plain(self, 0x19b);
        break;
    case 0x14:
        self->script_step_frames = 10;
        render_object_play_motion_steps((struct RenderObject *)self, (unsigned char)base, 1, (unsigned char)base, 1,
                   (unsigned char)base, 1, (unsigned char)base, 1, 8, 5, 1, 3000, 0);
        break;
    case 0x15:
        script3(self, 10, base, 0, 0, 0, 2000, 0);
        launch_plain(self, 0x1c1);
        break;
    case 0x16:
        script3(self, 10, base, 2, 2, 2, 2000, 0);
        launch_plain(self, 0x1c1);
        break;
    case 0x17:
        script3(self, 10, base, 3, 3, 3, 2000, 0);
        launch_plain(self, 0x1c1);
        break;
    case 0x18:
        script3(self, 10, base, 4, 4, 4, 2000, 0);
        launch_plain(self, 0x1c1);
        break;
    case 0x19:
        script3(self, 10, base, 1, 1, 1, 2000, 0);
        launch_plain(self, 0x1c1);
        break;
    case 0x1a: {
        int job;

        self->script_step_frames = 4;
        job = game_actor_get_job(self);
        if (job == 0xfd1 || game_actor_get_job(self) == 0x1083)
            script1(self, base, 4, 5, 6, 7, 2, 500);
        else
            script1(self, base, 2, 3, 4, 4, 4, 500);
        break;
    }
    case 0x1b:
        script3(self, 10, base, 5, 5, 5, 3000, 0);
        break;
    case 0x1c:
        self->script_step_frames = 10;
        script1(self, base, 0, 1, 2, 3, 4, 1000);
        break;
    case 0x1e:
        script3(self, 10, base, 1, 1, 1, 2000, 0);
        break;
    case 0x1f:
        self->script_step_frames = 5;
        script1(self, 0x50, 3, 5, 7, 8, 2, 500);
        break;
    case 0x20:
        self->script_step_frames = 5;
        script1(self, base, 4, 5, 6, 7, 2, 500);
        break;
    case 0x21:
        script3(self, 10, base, 2, 2, 2, 1000, 0);
        break;
    case 0x22:
        script3(self, 10, base, 3, 3, 3, 1000, 0);
        break;
    case 0x38:
        self->motion_finished = 0;
    case 0x23:
        script3(self, 6, base, 0, 1, 1, 500, 0);
        break;
    case 0x24:
        script3(self, 6, base, 2, 5, 5, 1000, 0);
        break;
    case 0x25: {
        int m = rand() % 2 > 0 ? 3 : 2;

        script3(self, 10, base, m, m, m, 500, 0);
        break;
    }
    case 0x26:
        self->script_step_frames = 6;
        if (rand() % 2 > 0)
            script1(self, base, 3, 2, 1, 0, 0, 1000);
        else
            render_object_play_motion_range((struct RenderObject *)self, (unsigned char)base, 0, 3, (unsigned char)base, 3, 1,
                       1000, 0);
        break;
    case 0x27: case 0x30:
        script3(self, 6, base, 4, 5, 5, 500, 0);
        break;
    case 0x28:
        script3(self, 6, base, 0, 2, 2, 500, 0);
        break;
    case 0x29: {
        int m = rand() % 2 > 0 ? 2 : 1;

        script3(self, 10, base, m, m, m, 500, 0);
        break;
    }
    case 0x2a:
        script3(self, 10, base, 0, 0, 0, 500, 0);
        break;
    case 0x2b:
        script3(self, 10, base, 5, 5, 5, 500, 0);
        break;
    case 0x2c:
        script3(self, 10, base, 4, 4, 4, 500, 0);
        break;
    case 0x2d: case 0x34: case 0x35: case 0x36: case 0x3a: case 0x3b:
        render_object_set_action((struct RenderObject *)self, base, 1);
        break;
    case 0x2e:
        self->script_step_frames = 0x14;
        script1(self, base, 0, 0, 0, 1, 1, 3000);
        stop_effects(self, 0x1be);
        stop_effects(self, 0x430);
        launch_plain(self, 0x430);
        launch_plain(self, 0x19b);
        break;
    case 0x2f:
        script3(self, 6, base, 0, 3, 3, 9999999, 1);
        break;
    case 0x31:
        script3(self, 6, base, 0, 7, 7, 1000, 0);
        self->motion_finished = 0;
        break;
    case 0x32:
        script3(self, 10, base, 0, 0, 0, 9999999, 0);
        break;
    case 0x33:
        self->script_step_frames = 4;
        script1(self, base, 2, 3, 4, 4, 4, 500);
        self->motion_finished = 0;
        break;
    case 0x37:
        script3(self, 6, base, 2, 4, 4, 500, 0);
        break;
    case 0x39:
        self->motion_finished = 0;
        script3(self, 6, base, 5, 5, 5, 500, 0);
        break;
    }

    self->action = game_actor_dir_of((struct GameObject *)self, self->roty)
                   + self->base_action;
    old = self->state;
    if (old == 0 || old == 6)
        self->resume_state = old;
    self->state = state;
    if (state == 7 || state == 0x31)
        self->state = 7;
    self->start_tick = platform_ticks_ms();
}
