#include "ui/window_types.h"
#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "game/ground_item_state.h"
#include "navi/navi_manager.h"
#include "contents/sound.h"
#include "game/client_state.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_dispatch.h"
#include "ui/window_access.h"
#include "game/context.h"
#include <math.h>
#include "actor/actor.h"
#include "actor/pc.h"
#include "actor/player.h"
#include "std/std.h"
#include "actor/sprite_render.h"
#include "game/gamemode.h"
#include "render/render_api.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "game/input.h"
#include "platform/clock.h"

void player_process_proceed(struct Player *self);
void player_process_costume_body(struct Player *self, unsigned char show);

static void player_send_owed_5b(struct Player *self)
{
    int a, b;

    if (!self->owed_walk || (self->state != 2 && self->state != 5 && self->state != 6))
        return;
    a = self->owed_walk_x;
    b = self->owed_walk_y;
    self->owed_walk = 0;
    self->owed_walk_x = -1;
    self->owed_walk_y = -1;
    render_object_send(self, NULL, 0x5b, a, b, 0, 0);
}

unsigned char player_on_process(struct GameObject *object)
{
    struct Player *self = (struct Player *)object;

    struct GameActor *actor = (struct GameActor *)self;
    struct ActorMove req;
    struct UIWindow *wnd;
    unsigned char result;
    struct GameMode *mode;

    game_actor_process_body_state(actor);
    game_actor_process_effect_state(actor);
    game_actor_process_will_be_attacked(actor);
    game_actor_process_chat_balloon_(actor);
    if (self->hit_flash && platform_ticks_ms() > self->hit_tick + 100) {
        self->hit_argb = 0xffff0000;
        self->hit_flash = 0;
    }
    if (self->argb != 0xffffffff)
        render_object_modify_argb((struct RenderObject *)self, 8, 8, 8, 8);
    mode = game_current_mode();
    if (self->move_request_pending && platform_ticks_ms() > self->move_request_tick + (g_session1.averagePing >> 1)) {
        game_mode_world_to_sub_cell(mode, self->pos.x, self->pos.z, &req.src_x, &req.src_y, &req.sub_x, &req.sub_y);
        req.start_tick = session_get_server_time(&g_session1);
        req.dst_x = self->requested_cell_x;
        req.dst_y = self->requested_cell_y;
        render_object_send(self, NULL, 0, MSG_ARG_PTR(&req), 0, 0, 0);
    }
    player_process_proceed(self);
    result = actor_process_state(self);
    render_object_process_sound((struct RenderObject *)self);
    if (self->wave_pending) {
        struct GameActor *player = (struct GameActor *)game_current_player();

        if (player)
            sound_play_wave((const char *)self->wave_name, self->pos.x - player->pos.x, 0.0f,
                            self->pos.z - player->pos.z, 0xfa, 0x28, 1.0f);
        self->wave_pending = 0;
    }
    player_process_costume_body(self, game_actor_get_transform_job(actor) > 0);
    if (g_session1.cameraInfoShown) {
        wnd = ui_window_find(0x121);
        if (wnd)
            ui_window_invalidate(wnd);
    }
    return result;
}

void player_set_state(struct GameActor * base_actor, int state, unsigned char snap)
{
    struct Player * self = (struct Player *)base_actor;

    if (self->state == 3)
        return;
    if (state != 1) {
        self->requested_cell_x = -1;
        self->requested_cell_y = -1;
    }
    player_send_owed_5b(self);
    if (state == 3) {
        struct GameMode *mode = game_current_mode();

        mode_send_message((struct Mode *)mode, 5, 0, 0, 0, 0);
        game_mode_launch_dummy_effect(mode, &self->pos, 0x174, 0x12c, 0);
        g_wobbleFramesLeft = 0;
    }
    pc_set_state((struct GameActor *)self, state, snap);
}

unsigned char player_process_state(struct GameActor * base_actor)
{
    struct Player * self = (struct Player *)base_actor;

    return pc_process_state((struct GameActor *)self);
}

#define MODE_SEND1(mode, a, b) \
    mode_send_message((struct Mode *)(mode), (a), (b), 0, 0, 0)

void player_shift_cell_by_target_radius(struct Player *self, struct GameActor *target, float x, float z,
                           int *a, int *b)
{
    int n = game_actor_get_npc_location_radius(target);
    float deg;

    if (target->model_kind == 1)
        n = 5;
    else if (!n)
        return;
    deg = (float)atan2((double)(x - self->pos.x), (double)-(z - self->pos.z)) / 3.141592f * 180.0f;
    if (deg >= 360.0f)
        deg -= 360.0f;
    if (0.0f > deg)
        deg += 360.0f;
    if (deg >= 0.0f && 25.0f > deg) {
        *a += n;
    } else if (deg >= 25.0f && 70.0f > deg) {
        *b += n;
        *a += n;
    } else if (deg >= 70.0f && 115.0f > deg) {
        *b += n;
    } else if (deg >= 115.0f && 160.0f > deg) {
        *b += n;
        *a -= n;
    } else if (deg >= 160.0f && 205.0f > deg) {
        *a -= n;
    } else if (deg >= 205.0f && 250.0f > deg) {
        *b -= n;
        *a += n;
    } else if (deg >= 250.0f && 295.0f > deg) {
        *b -= n;
    } else if (deg >= 295.0f && 340.0f > deg) {
        *b -= n;
        *a -= n;
    } else {
        *a += n;
    }
}

static float max0(float v)
{
    return 0.0f > v ? 0.0f : v;
}

float player_dist_beyond_target_radius(struct GameActor *target, float dist)
{
    if (target->model_kind == 1)
        return max0(dist - 20.0f);
    if (game_actor_get_npc_location_radius(target) > 0)
        return max0(dist - (float)(game_actor_get_npc_location_radius(target) + 1) * 4.0f);
    return max0(dist);
}

static void player_msg(struct Player *self, int msg)
{
    render_object_send(self, NULL, msg, 0, 0, 0, 0);
}

static void player_walk_msg(struct Player *self, int x, int y)
{
    render_object_send(self, NULL, 0x5b, x, y, 0, 0);
}

static int player_in_range(int range, float dist)
{
    return (float)(range * 5) + 5.5f >= dist;
}

static void player_attack_now(struct Player *self)
{
    if (self->attack_kind == 2)
        player_msg(self, 0xd);
    if (self->attack_sent)
        return;
    if (self->attack_tick + 0x4b0 >= platform_ticks_ms())
        return;
    self->attacks++;
    self->attack_tick = platform_ticks_ms();
    self->attack_sent = 1;
    player_msg(self, self->attack_kind == 2 ? 0x1d : 0x28);
}

static int player_skill_gate(const struct Player *self)
{
    int st = self->state;

    return st == 1 || st == 0 || st == 4 || st >= 0x10;
}

static int player_swing_holds(const struct Player *self)
{
    return self->state == 2;
}

static int player_proceed_attack(struct Player *self, struct GameMode *mode)
{
    struct GameActor *target = world_get_game_actor_by_aid(mode->world,
                                                          self->target_aid);
    int my_x, my_y, x, y, st, due, gate;
    unsigned interval;
    float dist, fx, fz;

    if (!target)
        return 0;
    if (target->state == 3)
        return 1;
    game_mode_world_to_cell(mode, self->pos.x, self->pos.z, &my_x, &my_y);

    st = self->state;
    if (game_actor_get_job((struct GameActor *)self) == 0x10
        || game_actor_get_job((struct GameActor *)self) == 0xfb1
        || game_actor_get_job((struct GameActor *)self) == 0xfc7)
        gate = st == 1 || st == 0 || st == 4 || st == 7 || st == 8;
    else if ((game_actor_get_job((struct GameActor *)self) >= 0xfce
              && game_actor_get_job((struct GameActor *)self) < 0x10eb)
             || game_actor_get_job((struct GameActor *)self) == 0x18
             || game_actor_get_job((struct GameActor *)self) == 0x19)
        gate = st == 1 || st == 0 || st == 4 || st == 7 || st == 8 || st >= 0x10;
    else
        gate = st == 1 || st == 0 || st == 4;
    if (!gate)
        return 1;

    dist = render_object_calc_dist((struct RenderObject *)target, self->pos.x, self->pos.y, self->pos.z);
    dist = player_dist_beyond_target_radius(target, dist);
    if ((self->requested_cell_x == my_x && self->requested_cell_y == my_y) || player_in_range(g_session1.attackRange, dist)) {
        player_attack_now(self);
        return 1;
    }

    interval = self->pending != 5 ? 0x1c2 : 0x4b0;
    due = platform_ticks_ms() > self->move_tick + interval;
    st = self->state;
    x = target->cell_x;
    y = target->cell_y;
    if (self->pending == 5) {
        fx = target->pos.x;
        fz = target->pos.z;
    } else {
        game_mode_cell_to_world(mode, x, y, &fx, &fz);
    }
    player_shift_cell_by_target_radius(self, target, fx, fz, &y, &x);
    game_mode_cell_along_line(mode, self->pos.x, self->pos.z, fx, fz, &x, &y, g_session1.attackRange);
    if (self->requested_cell_x == x && self->requested_cell_y == y)
        return 1;
    if (st == 2)
        return 1;
    if (self->attack_sent || !due)
        return 1;
    self->requested_cell_x = x;
    self->requested_cell_y = y;
    self->move_tick = platform_ticks_ms();
    player_walk_msg(self, x, y);
    return 1;
}

static int player_proceed_pickup(struct Player *self, struct GameMode *mode)
{
    struct Item *item = world_get_item_by_aid(mode->world, self->target_aid);
    int my_x, my_y, x, y, item_x, item_y;

    if (!item)
        return 0;
    game_mode_world_to_cell(mode, self->pos.x, self->pos.z, &my_x, &my_y);
    game_mode_world_to_cell(mode, item->base.pos.x, item->base.pos.z, &x, &y);
    item_x = x;
    item_y = y;
    game_mode_step_toward_walkable(mode, &my_x, &my_y, &x, &y);
    if (self->state != 0 && self->state < 0x10)
        return 1;
    if (game_mode_is_within_cells(&my_x, &my_y, &item_x, &item_y, 1)) {
        render_object_send(self, NULL, 0x10, item_x, item_y, 0, 0);
        if (!game_mode_is_pickup_requested(mode, self->target_aid)) {
            game_mode_add_pickup_request(mode, self->target_aid);
            MODE_SEND1(mode, 0x88, self->target_aid);
        }
        return 0;
    }
    if (self->requested_cell_x == x && self->requested_cell_y == y)
        return 1;
    self->requested_cell_x = x;
    self->requested_cell_y = y;
    player_walk_msg(self, x, y);
    return 1;
}

static int player_proceed_skill_actor(struct Player *self, struct GameMode *mode)
{
    struct GameActor *target;
    int my_x, my_y, x, y, range;
    float tx, tz, dist;

    if (self->target_aid == g_session1.ownAid)
        target = (struct GameActor *)self;
    else
        target = world_get_game_actor_by_aid(mode->world, self->target_aid);
    if (!target)
        return 0;
    game_mode_world_to_cell(mode, self->pos.x, self->pos.z, &my_x, &my_y);
    tx = target->pos.x;
    x = target->cell_x;
    tz = target->pos.z;
    y = target->cell_y;
    range = skill_attack_range(self->skill_id, self->skill_level);
    game_mode_cell_along_line(mode, self->pos.x, self->pos.z, tx, tz, &x, &y, range);
    if (!player_skill_gate(self))
        return 1;
    dist = render_object_calc_dist((struct RenderObject *)target, self->pos.x, self->pos.y, self->pos.z);
    dist = player_dist_beyond_target_radius(target, dist);
    if (player_in_range(range, dist) || (self->requested_cell_x == my_x && self->requested_cell_y == my_y)) {
        player_msg(self, 0x2a);
        return 0;
    }
    if (self->requested_cell_x == x && self->requested_cell_y == y)
        return 1;
    if (player_swing_holds(self))
        return 1;
    self->requested_cell_y = y;
    self->requested_cell_x = x;
    player_shift_cell_by_target_radius(self, target, tx, tz, &y, &x);
    player_walk_msg(self, x, y);
    return 1;
}

static int player_proceed_skill_ground(struct Player *self, struct GameMode *mode)
{
    int x = self->skill_cell_x, y = self->skill_cell_y, my_x, my_y, range;
    float fx, fz, dist;

    game_mode_world_to_cell(mode, self->pos.x, self->pos.z, &my_x, &my_y);
    range = skill_attack_range(self->skill_id, self->skill_level);
    game_mode_cell_to_world(mode, x, y, &fx, &fz);
    game_mode_cell_along_line(mode, self->pos.x, self->pos.z, fx, fz, &x, &y, range);
    if (!player_skill_gate(self))
        return 1;
    dist = render_object_calc_dist((struct RenderObject *)self, fx, self->pos.y, fz);
    if (player_in_range(range, dist) || (self->requested_cell_x == my_x && self->requested_cell_y == my_y)) {
        player_msg(self, 0x42);
        return 0;
    }
    if (self->requested_cell_x == x && self->requested_cell_y == y)
        return 1;
    if (player_swing_holds(self))
        return 1;
    self->requested_cell_y = y;
    self->requested_cell_x = x;
    player_walk_msg(self, x, y);
    return 1;
}

static int player_unit_attackable(struct GameActor *unit)
{
    int job = game_actor_get_job(unit);

    return job == 0x8d || job == 0x8f || job == 0x98 || job == 0xda || job == 0xde || job == 0xe6
           || (unsigned)(job - 0x5174) <= 3;
}

static int player_proceed_skill_unit(struct Player *self, struct GameMode *mode)
{
    struct GameActor *unit = (struct GameActor *)world_get_skill_by_aid(mode->world, self->target_aid);
    int my_x, my_y, x, y, range, job;
    float fx, fz, dist;

    if (!unit)
        return 0;
    game_mode_world_to_cell(mode, self->pos.x, self->pos.z, &my_x, &my_y);
    game_mode_world_to_cell(mode, unit->pos.x, unit->pos.z, &x, &y);
    range = skill_attack_range(self->skill_id, self->skill_level);
    if (!MODE_SEND1(mode, 0x5f, 0)) {
        if (range < 5 && self->skill_id != 0x7c)
            range = g_session1.attackRange > 5 ? g_session1.attackRange : 5;
    }
    game_mode_cell_to_world(mode, x, y, &fx, &fz);
    game_mode_cell_along_line(mode, self->pos.x, self->pos.z, fx, fz, &x, &y, range);
    if (!player_skill_gate(self))
        return 1;
    dist = render_object_calc_dist((struct RenderObject *)self, fx, self->pos.y, fz);
    if (player_in_range(range, dist) || (self->requested_cell_x == my_x && self->requested_cell_y == my_y)) {
        if (player_unit_attackable(unit) && !self->skill_id) {
            player_attack_now(self);
            return 1;
        }
        player_msg(self, 0x2a);
        return 0;
    }
    if (self->requested_cell_x == x && self->requested_cell_y == y)
        return 1;
    if (self->state == 2) {
        job = game_actor_get_job(unit);
        if (job != 0x8d && job != 0x8f && job != 0x98)
            return 1;
    }
    self->requested_cell_y = y;
    self->requested_cell_x = x;
    player_walk_msg(self, x, y);
    return 1;
}

void player_process_proceed(struct Player *self)
{
    struct GameMode *mode = game_current_mode();
    int keep;

    switch (self->pending) {
    case 1:
    case 5:
        keep = player_proceed_attack(self, mode);
        break;
    case 2:
        keep = player_proceed_pickup(self, mode);
        break;
    case 3:
        keep = player_proceed_skill_actor(self, mode);
        break;
    case 4:
        keep = player_proceed_skill_ground(self, mode);
        break;
    case 6:
        keep = player_proceed_skill_unit(self, mode);
        break;
    default:
        return;
    }
    if (!keep)
        player_msg(self, 0xd);
}

#define MODE_SEND(mode, msg, a, b, c) \
    mode_send_message((struct Mode *)(mode), (msg), (a), (b), (c), 0)

static int player_busy(const struct Player *self)
{
    return self->state == 6 || is_effect_state_burrow(self->effect_state);
}

static int player_burrow_skill(int skill, int hi, int with_250)
{
    if (hi)
        return 0;
    return skill == 0x89 || skill == 0xd4 || skill == 0x211 || skill == 0x212 || (with_250 && skill == 0x250);
}

static void player_msg_walk(struct Player *self, struct GameMode *mode, const struct ActorMove *move)
{
    int x, y, sub_x, sub_y;

    self->move_request_pending = 0;
    self->path_start = move->start_tick;
    if (game_actor_find_path((struct GameActor *)self, move->src_x, move->src_y, move->sub_x, move->sub_y,
                             move->dst_x, move->dst_y, g_session1.walkSpeed)) {
        self->cell_x = move->dst_x;
        self->cell_y = move->dst_y;
        actor_set_state(self, 1, 1);
        self->path_cell = 0;
    } else {
        game_mode_world_to_sub_cell(mode, self->pos.x, self->pos.z, &x, &y, &sub_x, &sub_y);
        if (game_actor_find_path((struct GameActor *)self, x, y, sub_x, sub_y, move->dst_x, move->dst_y,
                                 g_session1.walkSpeed)) {
            self->cell_x = move->dst_x;
            self->cell_y = move->dst_y;
            actor_set_state(self, 1, 1);
        }
    }
    navi_manager_check_arrival(&g_navi, self->cell_x, self->cell_y);
}

static void player_msg_turn(struct Player *self, struct GameMode *mode, int x, int y)
{
    float fx, fz, toward, roty;
    int deg, side, head, dir;

    if (self->state != 0 && self->state != 4 && self->state != 6)
        return;
    game_mode_cell_to_world(mode, x, y, &fx, &fz);
    toward = render_object_calc_dir(self->pos.x, self->pos.z, fx, fz);
    toward = (float)((game_mode_angle_to_direction((float)((double)toward + 22.5)) + 4) * 45);
    deg = (int)(self->roty - toward);
    if (deg < 0)
        deg += 360;
    side = deg / 45;
    if (side < 0)
        side += 8;
    else if (side >= 8)
        side -= 8;
    roty = toward;
    if (side == 0 || side == 4) {
        self->head_dir = 0;
        self->roty = roty;
    } else if (side == 1) {
        if (self->head_dir == 1) {
            self->head_dir = 0;
            self->roty = roty;
        } else {
            self->head_dir = 1;
        }
    } else if (side == 2 || side == 3) {
        self->head_dir = 1;
        self->roty = roty + 45.0f;
        if (self->roty >= 360.0f)
            self->roty -= 360.0f;
    } else if (side == 7) {
        if (self->head_dir == 2) {
            self->head_dir = 0;
            self->roty = roty;
        } else {
            self->head_dir = 2;
        }
    } else {
        roty -= 45.0f;
        self->head_dir = 2;
        self->roty = roty;
        if (0.0f > roty)
            self->roty = roty + 360.0f;
    }
    head = self->head_dir;
    self->motion = head;
    dir = game_mode_angle_to_direction(self->roty);
    self->roty = (float)((dir + 4) * 45);
    MODE_SEND(mode, 0x8b, game_mode_angle_to_direction(self->roty), head, 0);
}

static void player_msg_ground_cast(struct Player *self, struct GameMode *mode)
{
    struct GroundSkillUse cast;
    struct PathInfo path;
    struct UIWindow *wnd;
    int x, y, sub_x, sub_y, dir;

    cast.x = self->skill_cell_x;
    cast.skill_id = self->skill_id;
    cast.y = self->skill_cell_y;
    cast.level = self->skill_level;
    if (cast.skill_id == 0x7d || cast.skill_id == 0xdc) {
        wnd = ui_window_open(cast.skill_id == 0x7d ? 0x54 : 0x65);
        ui_window_send(wnd, NULL, 0x44, (int)&cast, 0, 0, 0);
        return;
    }
    if (cast.skill_id != 0x108 && cast.skill_id != 0xa04) {
        MODE_SEND(mode, 0x46, (int)&cast, 0, 0);
        return;
    }
    x = y = sub_x = sub_y = 0;
    game_mode_world_to_sub_cell(game_current_mode(), self->pos.x, self->pos.z, &x, &y, &sub_x,
                            &sub_y);
    path_info_init(&path);
    dir = 0;
    if (path_finder_find_path(&g_pathFinder, platform_ticks_ms(), x, y, sub_x, sub_y, self->skill_cell_x,
                              self->skill_cell_y, 0x96, &path, 0xf) == 1)
        dir = path_info_end_direction(&path);
    MODE_SEND(mode, 0x46, (int)&cast, dir, 0);
    path_info_destroy(&path);
}

void player_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct Player *self = (struct Player *)object;

    struct GameMode *mode = game_current_mode();
    int p1 = (int)par1, p1_hi = (int)(par1 >> 32), p2 = (int)par2, p2_hi = (int)(par2 >> 32), p3 = (int)par3;
    struct GameActor *actor;
    int st;

    if (msg < 0 || msg > 0x95)
        {
            pc_send_msg(object, source, msg, par1, par2, par3, par4);
            return;
        }
    switch ((int)msg) {
    case 0x0:
        player_msg_walk(self, mode, MSG_PTR(par1));
        break;
    case 0x8:
        actor_set_state(self, 0, 1);
        break;
    case 0x9:
        self->move_request_pending = 1;
        self->move_request_tick = platform_ticks_ms();
        self->requested_cell_x = p1;
        self->requested_cell_y = p2;
        break;
    case 0xa:
        if (player_busy(self))
            break;
        actor = world_get_game_actor_by_aid(mode->world, p1);
        if (!actor)
            break;
        self->target_aid = p1;
        self->attack_sent = 0;
        self->attack_kind = p2;
        self->attacks = 0;
        self->pending = (unsigned)(g_session1.walkSpeed * 4 / 3) < (unsigned)actor->walk_speed ? 5 : 1;
        break;
    case 0xd:
        self->attack_sent = 0;
        self->pending = 0;
        MODE_SEND(mode, 0x1a, 0, 0, 0);
        break;
    case 0xf:
        if (self->pending == 3) {
            MODE_SEND(mode, 0x1a, 0, 0, 0);
        } else if (self->pending == 1 || self->pending == 5 || self->pending == 6) {
            int attacks;

            if (!self->attack_kind)
                break;
            attacks = self->attacks;
            self->attack_kind = 2;
            if (!attacks)
                break;
            player_msg(self, 0xd);
            MODE_SEND(mode, 0x49, 0, 0, 0);
        }
        break;
    case 0x10:
        player_msg_turn(self, mode, p1, p2);
        break;
    case 0x11:
        player_msg(self, 0xd);
        st = self->state;
        if (st == 2 || st == 5 || st == 6) {
            self->owed_walk = 1;
            self->owed_walk_x = p1;
            self->owed_walk_y = p2;
        } else {
            self->owed_walk = 0;
            self->owed_walk_x = -1;
            self->owed_walk_y = -1;
            player_walk_msg(self, p1, p2);
        }
        break;
    case 0x12:
        if (player_busy(self))
            break;
        self->pending = 2;
        self->target_aid = p1;
        break;
    case 0x1d:
        MODE_SEND(mode, 0x89, 0, self->target_aid, 0);
        break;
    case 0x1e:
    case 0x1f:
        if (is_effect_state_burrow(self->effect_state))
            break;
        if (self->state == 0)
            MODE_SEND(mode, 0x89, 2, self->target_aid, 0);
        else if (self->state == 6)
            MODE_SEND(mode, 0x89, 3, self->target_aid, 0);
        break;
    case 0x28:
        actor = world_get_game_actor_by_aid(mode->world, self->target_aid);
        if (actor && actor->object_type == 7)
            break;
        MODE_SEND(mode, 0x89, 7, self->target_aid, 0);
        break;
    case 0x29:
        if (self->state == 6)
            break;
        if (is_effect_state_burrow(self->effect_state) && !player_burrow_skill(p2, p2_hi, 1))
            break;
        self->pending = 3;
        self->target_aid = p1;
        self->skill_id = p2;
        break;
    case 0x2a:
        MODE_SEND(mode, 0x45, self->skill_id, self->target_aid, self->skill_level);
        break;
    case 0x3c:
        if (player_busy(self))
            break;
        self->pending = self->pending == 1 ? 5 : 1;
        self->move_tick = platform_ticks_ms() - 0x4b0;
        self->target_aid = p1;
        self->attack_sent = 0;
        self->attacks = 0;
        self->requested_cell_x = -1;
        self->requested_cell_y = -1;
        break;
    case 0x41:
        if (self->state == 6)
            break;
        if (is_effect_state_burrow(self->effect_state) && !player_burrow_skill(p1, p1_hi, 0))
            break;
        self->pending = 4;
        self->skill_id = p1;
        self->skill_cell_x = p2;
        self->skill_cell_y = p3;
        self->target_aid = 0;
        break;
    case 0x42:
        player_msg_ground_cast(self, mode);
        break;
    case 0x5a:
        self->skill_level = p1;
        break;
    case 0x5b:
        if (platform_ticks_ms() > self->held_walk_tick + 0x15e) {
            self->held_walk_tick = platform_ticks_ms();
            MODE_SEND(mode, 0x8a, p1, p2, 0);
        }
        break;
    case 0x67:
        if (player_busy(self))
            break;
        self->pending = 6;
        self->skill_id = ((const struct SkillUse *)MSG_PTR(par2))->skill_id;
        self->skill_level = ((const struct SkillUse *)MSG_PTR(par2))->level;
        self->target_aid = p1;
        self->attack_kind = p3;
        self->attack_sent = 0;
        self->attacks = 0;
        break;
    case 0x95:
        path_info_clear(&self->path);
        break;
    default:
        pc_send_msg(object, source, msg, par1, par2, par3, par4);
        return;
    }
}

void player_process_costume_body(struct Player *self, unsigned char show)
{
    if (is_effect_state_star2(self->effect_state) || effect_state_bit6(self->effect_state) || is_effect_state_burrow(self->effect_state))
        show = 0;
    if (show) {
        if (!self->costume_body_obj) {
            struct GameObject *obj = game_mode_make_msg_effect(game_current_mode(), 0x2c, &self->pos,
                                                 self->aid, 1);

            list_push_back(&self->attached, &obj, sizeof obj);
            render_object_send(obj, self, 0x32, 0, 0, 0, 0);
            self->costume_body_obj = (int)obj;
        }
    } else if (self->costume_body_obj) {
        render_object_send((void *)self->costume_body_obj, NULL, 0x35, 0, 0, 0, 0);
        self->costume_body_obj = 0;
    }
}

const struct GameActorOps player_ops = {
    .base = {
        .base = {
            .destroy = player_deleting_destruct,
            .on_process = player_on_process,
            .send_msg = player_send_msg,
            .render = pc_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = game_actor_set_action,
        .update_motion = render_object_update_motion_base,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = player_process_state,
    .set_state = player_set_state,
    .set_unit_cell = actor_default_noop,
    .clear_unit_cell = actor_default_noop,
    .set_job_model = actor_default_pair,
    .set_look = pc_set_spr_act,
    .set_look_with_state = pc_set_spr_act_with_state,
    .set_spr_job = pc_set_spr_job,
    .set_head = pc_set_head,
    .set_weapon = pc_set_weapon,
    .set_weapon_look = pc_set_weapon_look,
    .set_accessory = pc_set_accessory,
    .set_accessory2 = pc_set_accessory2,
    .set_accessory3 = pc_set_accessory3,
    .set_shield = pc_set_shield,
    .set_appearance_value = pc_set_shoes,
    .set_imf_name = pc_set_imf_name,
    .set_head_pal_name = pc_set_head_pal_name,
    .set_body_pal_name = pc_set_body_pal_name,
    .set_body_palette = pc_set_body_palette,
    .set_head_palette = pc_set_head_palette,
    .get_weapon = pc_get_weapon,
    .walk_motion = game_actor_walk_motion,
    .get_attack_motion = game_actor_get_attack_motion,
    .make_corpse = game_actor_make_corpse,
    .set_attack_speed = pc_set_delay_factor,
    .set_honor = pc_set_honor,
    .set_honor_virtue = pc_set_honor_virtue,
    .get_char_id = pc_get_char_id,
    .set_robe = pc_set_robe,
    .set_body_style = pc_set_body_style,
    .set_mounted = pc_set_riding,
    .get_sprite_variant = pc_get_sprite_variant,
    .set_sprite_variant = pc_set_sprite_variant,
    .get_skip_arrow = pc_get_skip_arrow,
    .set_skip_arrow = pc_set_skip_arrow,
    .refresh_accessory3 = pc_refresh_accessory3,
    .get_head_extent = pc_get_head_extent,
};

static void player_init(struct Player *self, const float position[3], int cell_x, int cell_y)
{
    int saved_body_state, saved_effect_state, saved_health_state, sex, job;

    self->pos.x = position[0];
    self->pos.y = position[1];
    self->pos.z = position[2];
    self->requested_cell_x = cell_x;
    self->requested_cell_y = cell_y;
    self->skill_cell_x = 0;
    self->skill_cell_y = 0;
    self->roty = (float)((g_session1.playerDir + 4) * 45);
    self->aid = g_session1.ownAid;
    self->guild_id = g_guildInfo.guild_id;
    self->path_cell = -1;
    self->move_request_tick = 0;
    self->move_request_pending = 0;
    self->pending = 0;
    self->attack_kind = 0;
    self->attacks = 0;
    self->attack_sent = 0;
    self->target_aid = 0;
    self->pickable = 1;
    self->skill_id = 0;
    self->skill_level = 0;
    self->move_tick = platform_ticks_ms() - 1200;
    self->attack_tick = platform_ticks_ms() - 1200;
    self->screen_x = g_renderer->centreX;
    self->screen_y = g_renderer->centreY;
    self->owed_walk = 0;
    self->owed_walk_x = -1;
    self->owed_walk_y = -1;
    self->held_walk_tick = platform_ticks_ms() - 1200;
    actor_set_state(self, 0, 1);
    self->walk_speed = g_session1.walkSpeed;
    self->is_pc = 1;
    game_actor_set_sex((struct GameActor *)self, session_get_sex(&g_session1));

    saved_effect_state = self->effect_state;
    saved_body_state = self->body_state;
    saved_health_state = self->health_state;
    self->body_state = g_session1.bodyState;
    self->effect_state = g_session1.effectState;
    self->health_state = g_session1.healthState;
    actor_set_sprite_variant(self, g_session1.madogear);
    actor_set_body_style(self, g_session1.body, 0);
    sex = session_get_sex(&g_session1);
    job = session_get_drawn_job(&g_session1);
    {
        struct ActorLook look = { job, sex, g_session1.hairStyle, g_session1.weapon,
            { g_session1.headBottom, g_session1.headTop, g_session1.headMid },
            g_session1.shield, g_session1.hairColor, g_session1.clothesColor };
        actor_set_look_with_state(self, &look, g_session1.effectState);
    }
    actor_set_robe(self, g_session1.robe, 0);
    if (g_session1.ownDressUp)
        render_object_send(self, NULL, 0x6e, 0x358, 1, g_session1.ownDressUp, 0);
    self->effect_state = saved_effect_state;
    self->body_state = saved_body_state;
    self->health_state = saved_health_state;
    game_actor_set_attr_state((struct GameActor *)self, g_session1.bodyState, g_session1.effectState, g_session1.healthState, 0);

    render_object_send(self, NULL, 0x6d, 0xa4, 0, 0, 0);
    {
        Vec3 none = { 0.0f, 0.0f, 0.0f };

        self->status_icons = game_actor_launch_effect((struct GameActor *)self, 0xa4, none, 0);
    }
    if (!g_mapPropertyQuiet)
        player_refresh_status_effects(self);
    self->object_type = 0;
    self->costume_body_obj = 0;
}

struct Player *player_create(const float position[3], int cell_x, int cell_y)
{
    struct Player *self = mem_alloc(sizeof *self);
    if (!self)
        return NULL;
    pc_construct((struct Pc *)self);
    self->ops = &player_ops.base.base;
    player_init(self, position, cell_x, cell_y);
    return self;
}

void player_deleting_destruct(struct GameObject *object)
{
    struct Player *self = (struct Player *)object;

    self->ops = &player_ops.base.base;
    pc_destruct((struct Pc *)self);
    mem_free(self);
}
