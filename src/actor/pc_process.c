#include "ui/window_types.h"
#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "ui/window_access.h"
#include "render/renderer.h"
#include "contents/client_lua_calls.h"
#include "game/own_states.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "actor/sprite_render.h"
#include "std/std.h"
#include "game/actor_entry.h"
#include "game/gamemode.h"
#include "resources/attr_types.h"
#include "core/msg_str.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "ui/character/gauge.h"
#include "ui/window.h"
#include "ui/wndmgr/wndmgr.h"
#include "game/mouse_camera.h"
#include <stdint.h>
#include "platform/clock.h"

unsigned char pc_on_process(struct GameObject *object)
{
    struct Pc *self = (struct Pc *)object;

    struct GameActor *actor = (struct GameActor *)self;
    unsigned char result, show;

    game_actor_process_effect_state(actor);
    game_actor_process_will_be_attacked(actor);
    game_actor_process_chat_balloon_(actor);
    if (self->hp_gage_wnd) {
        int raise = g_renderer->height * 12 / 480;

        ui_window_move(self->hp_gage_wnd, self->screen_x - 30, (int)((float)raise * self->scale + (float)self->screen_y));
    }
    game_actor_place_hp_gage(actor);
    if (self->hit_flash && platform_ticks_ms() > self->hit_tick + 100) {
        self->hit_argb = 0xffff0000;
        self->hit_flash = 0;
    }
    if (self->argb != 0xffffffff)
        render_object_modify_argb((struct RenderObject *)self, 8, 8, 8, 8);
    result = actor_process_state(self);
    render_object_process_sound((struct RenderObject *)self);
    if (self->wave_pending) {
        struct GameActor *player = (struct GameActor *)game_current_player();

        if (player)
            sound_play_wave((const char *)self->wave_name, self->pos.x - player->pos.x, 0.0f,
                            self->pos.z - player->pos.z, 0xfa, 0x28, 1.0f);
        self->wave_pending = 0;
    }
    show = game_actor_get_transform_job(actor) > 0;
    if (is_effect_state_star2(self->effect_state) || effect_state_bit6(self->effect_state) || is_effect_state_burrow(self->effect_state))
        show = 0;
    if (show) {
        if (!self->costume_body_obj) {
            struct GameObject *obj = game_mode_make_msg_effect(game_current_mode(), 0x2c, &self->pos,
                                                 self->aid, 1);

            list_push_back(&self->attached, &obj, sizeof obj);
            render_object_send(obj, self, 0x32, 0, 0, 0, 0);
            self->costume_body_obj = obj;
        }
    } else if (self->costume_body_obj) {
        render_object_send(self->costume_body_obj, NULL, 0x35, 0, 0, 0, 0);
        self->costume_body_obj = 0;
    }
    return result;
}

static int pc_packet_state(int v)
{
    return v == 1 ? 3 : v == 2 ? 6 : 0;
}

static void pc_stand_place(struct Pc *self, struct GameMode *mode, const struct ActorEntryInfo *entry)
{
    game_mode_cell_to_world(mode, entry->x, entry->y, &self->pos.x, &self->pos.z);
    self->pos.y = c3d_attr_get_height(mode->world->attr, self->pos.x, self->pos.z);
    self->roty = (float)((entry->dir + 4) * 45);
}

static void pc_stand_look(struct Pc *self, struct ActorEntryInfo *entry, int twin)
{
    struct ActorLook look;
    self->xsize = entry->x_size;
    self->ysize = entry->y_size;
    self->cell_x = entry->x;
    self->cell_y = entry->y;
    self->head_dir = entry->head_dir;
    self->entry_head = entry->head;
    entry->job = game_actor_state_job(entry->job, entry->effect_state);
    look = actor_look_from_entry(entry);
    if (!twin) {
        actor_set_look(self, &look);
    } else {
        actor_set_look_with_state(self, &look, entry->effect_state);
        actor_set_robe(self, entry->robe, 0);
    }
    game_actor_set_attr_state((struct GameActor *)self, entry->body_state, entry->effect_state, entry->health_state, 0);
    game_actor_set_pk_mode((struct GameActor *)self, entry->is_pk_mode);
    game_actor_set_guild((struct GameActor *)self, entry->guild_id, entry->emblem_version);
    actor_set_honor_virtue(self, entry->honor, entry->virtue);
}

static void pc_walk_look(struct Pc *self, struct ActorMoveEntryInfo *entry, int twin)
{
    struct ActorLook look;
    self->aid = entry->aid;
    self->walk_speed = entry->speed;
    self->path_start = entry->move_start_time;
    self->head_dir = 0;
    entry->job = game_actor_state_job(entry->job, entry->effect_state);
    look = actor_look_from_move_entry(entry);
    if (!twin) {
        actor_set_look(self, &look);
    } else {
        actor_set_look_with_state(self, &look, entry->effect_state);
        actor_set_robe(self, entry->robe, 0);
    }
    game_actor_set_attr_state((struct GameActor *)self, entry->body_state, entry->effect_state, entry->health_state, 0);
    game_actor_set_pk_mode((struct GameActor *)self, entry->is_pk_mode);
    game_actor_set_guild((struct GameActor *)self, entry->guild_id, entry->emblem_version);
    actor_set_honor_virtue(self, entry->honor, entry->virtue);
}

static void pc_walk_place(struct Pc *self, struct GameMode *mode, const struct ActorMoveEntryInfo *entry)
{
    game_mode_cell_to_world(mode, entry->src_x, entry->src_y, &self->pos.x, &self->pos.z);
    self->pos.x = (float)mode->world->attr->zoom * 0.0625f * (float)(entry->sub_x - 8) + self->pos.x;
    self->pos.z = (float)mode->world->attr->zoom * 0.0625f * (float)(entry->sub_y - 8) + self->pos.z;
    self->pos.y = c3d_attr_get_height(mode->world->attr, self->pos.x, self->pos.z);
}

static void pc_walk(struct Pc *self, const struct ActorMoveEntryInfo *entry, int stand_else)
{
    if (game_actor_find_path((struct GameActor *)self, entry->src_x, entry->src_y, entry->sub_x, entry->sub_y,
                            entry->dst_x, entry->dst_y, entry->speed)) {
        self->cell_x = entry->dst_x;
        self->cell_y = entry->dst_y;
        actor_set_state(self, 1, 1);
        self->path_cell = 0;
    } else if (stand_else) {
        actor_set_state(self, 0, 1);
    }
}

static void pc_appear_effect(struct Pc *self, long long par2)
{
    if (par2 == 2 && !effect_state_bit6(self->effect_state)) {
        Vec3 zero = { 0.0f, 0.0f, 0.0f };

        game_actor_launch_effect((struct GameActor *)self, 0x158, zero, 0.0f);
    }
}

void pc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct Pc *self = (struct Pc *)object;

    struct GameMode *mode = game_current_mode();
    struct ActorEntryInfo *entry = MSG_PTR(par1);
    struct ActorMoveEntryInfo *walk = MSG_PTR(par1);

    if (msg < 3 || msg > 0x88)
        {
            game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
            return;
        }
    switch ((int)msg) {
    case 3:
        pc_stand_place(self, mode, entry);
        pc_stand_look(self, entry, 0);
        actor_set_state(self, pc_packet_state(entry->state), 1);
        return;
    case 0x7c:
        self->body_style = entry->body;
        pc_stand_place(self, mode, entry);
        pc_stand_look(self, entry, 1);
        self->char_id = entry->char_id;
        actor_set_state(self, pc_packet_state(entry->state), 1);
        return;
    case 4:
        self->aid = entry->aid;
        pc_stand_place(self, game_current_mode(), entry);
        self->walk_speed = entry->speed;
        pc_stand_look(self, entry, 0);
        actor_set_state(self, pc_packet_state(entry->state), 1);
        pc_appear_effect(self, par2);
        game_actor_show_party_mark((struct GameActor *)self, entry->aid);
        return;
    case 0x79:
        self->body_style = entry->body;
        self->aid = entry->aid;
        pc_stand_place(self, game_current_mode(), entry);
        self->walk_speed = entry->speed;
        pc_stand_look(self, entry, 1);
        actor_set_state(self, pc_packet_state(entry->state), 1);
        pc_appear_effect(self, par2);
        game_actor_show_party_mark((struct GameActor *)self, entry->aid);
        self->char_id = entry->char_id;
        return;
    case 6:
        pc_walk_look(self, walk, 0);
        pc_walk_place(self, mode, walk);
        pc_walk(self, walk, 1);
        game_actor_show_party_mark((struct GameActor *)self, walk->aid);
        return;
    case 0x7a:
        self->body_style = walk->body;
        pc_walk_look(self, walk, 1);
        pc_walk_place(self, mode, walk);
        pc_walk(self, walk, 1);
        game_actor_show_party_mark((struct GameActor *)self, walk->aid);
        self->char_id = walk->char_id;
        return;
    case 5:
        self->body_style = walk->body;
        pc_walk_look(self, walk, 0);
        pc_walk(self, walk, 0);
        game_actor_show_party_mark((struct GameActor *)self, walk->aid);
        self->char_id = walk->char_id;
        return;
    case 0x7b:
        self->body_style = walk->body;
        pc_walk_look(self, walk, 1);
        pc_walk(self, walk, 0);
        game_actor_show_party_mark((struct GameActor *)self, walk->aid);
        self->char_id = walk->char_id;
        self->body_style = walk->body;
        return;
    case 0x22:
        if (!self->hp_gage_wnd) {
            self->hp_gage_wnd = (struct UIWindow *)ui_pc_gage_new();
            ui_window_create(self->hp_gage_wnd, 0x3c, 5);
            ui_window_mgr_add_to_window_list(&g_windowMgr, self->hp_gage_wnd);
        }
        ui_pc_gage_set_hp((struct UIPcGage *)self->hp_gage_wnd, (int)par1, (int)par2);
        ui_pc_gage_set_sp((struct UIPcGage *)self->hp_gage_wnd, 0, 0);
        ui_window_move(self->hp_gage_wnd, self->screen_x - 30, scale_y_from480(12) + self->screen_y);
        return;
    case 0x23:
        if (self->hp_gage_wnd) {
            ui_window_mgr_post_quit2(&g_windowMgr, self->hp_gage_wnd);
            self->hp_gage_wnd = NULL;
        }
        return;
    case 0x4d:
        return;
    case 0x64:
        if (self->state == 0 || self->state == 6)
            self->motion = (int)par1;
        return;
    case 0x69: case 0x6a: case 0x88: {
        long long v;

        actor_set_spr_job(self, game_actor_get_job((struct GameActor *)self));
        actor_set_imf_name(self);
        v = actor_get_weapon(self);
        if ((unsigned long long)(v - 1) <= 0x67)
            actor_set_weapon(self, v);
        else
            actor_set_weapon_look(self, v);
        actor_set_shield(self, self->shield);
        actor_set_head_pal_name(self);
        actor_set_body_pal_name(self);
        return;
    }
    case 0x72:
        self->pvp_rank = (int)par2;
        self->pvp_total = (int)par1;
        return;
    default:
        game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
        return;
    }
}

unsigned char pc_process_state(struct GameActor * base_actor)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct GameActor *actor = (struct GameActor *)self;

    if (self->motion_hold_until && platform_ticks_ms() > self->motion_hold_until) {
        self->motion_held = 0;
        self->motion_hold_until = 0;
    }
    if (!game_actor_process_fade_out(actor))
        return 0;
    switch (self->state) {
    case 2: case 7: case 45:
        render_object_update_motion((struct RenderObject *)self);
        if (self->motion_finished)
            actor_set_state(self, 0, 1);
        return 1;
    case 3:
        self->script_range_on = 0;
        self->script_steps_on = 0;
        self->script_pingpong_on = 0;
        self->motion_held = 0;
        render_object_update_motion((struct RenderObject *)self);
        return 1;
    case 6:
        render_object_update_motion((struct RenderObject *)self);
        return 1;
    case 8: {
        int frame;

        render_object_update_motion((struct RenderObject *)self);
        frame = (int)((float)(platform_ticks_ms() - self->start_tick) / 24.0f);
        if (self->attack_target_aid && frame % 34 == 0) {
            struct GameActor *target = (struct GameActor *)game_find_actor(self->attack_target_aid);

            if (target)
                render_object_turn_toward((struct RenderObject *)self, target->pos.x, target->pos.z);
        }
        return 1;
    }
    case 51:
        render_object_update_motion((struct RenderObject *)self);
        if (self->motion_finished == 1)
            self->state = 2;
        return 1;
    default:
        return game_actor_process_state(actor);
    }
}

void pc_render(struct GameObject *object, const float *matrix)
{
    struct Pc *self = (struct Pc *)object;

    struct RenderObject *ro = (struct RenderObject *)self;
    unsigned char masked = 0;

    if (effect_state_bit14(self->effect_state))
        actor_footprints(ro, 1);
    if (self->overlay_flags & 0x8000)
        actor_footprints(ro, 2);
    masked = own_states_find(0xb8) != NULL;
    self->half_alpha = 0;
    if (g_session1.hideOtherPlayers == 1 && self->aid != g_session1.ownAid)
        self->half_alpha = 1;
    if (masked == 1 && (!self->visible || !self->revealed) && !effect_state_bit6(self->effect_state)) {
        render_object_set_argb(ro, -1, 0, 0, 0);
        pc_render_body_layers((struct Pc *)self, matrix, 0, 0);
        return;
    }
    if (!self->visible) {
        render_object_render_projected(ro, matrix);
        return;
    }
    if (self->state != 6 && self->state != 3)
        render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
    if (self->state == 6) {
        int job = game_actor_get_job((struct GameActor *)self);

        if (job == 0xfd0 || job == 0x108e || job == 0x1093 || job == 0x1094 || job == 0x10dc)
            render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
    }
    if (!self->revealed) {
        render_object_render_projected(ro, matrix);
        return;
    }
    if (is_event_aid(self->aid)) {
        pc_render_event_account((struct Pc *)self, matrix);
        return;
    }
    if (self->single_sprite) {
        render_object_render_sprite((struct RenderObject *)ro, matrix, 0, 0.0f);
        return;
    }
    pc_render_body_layers((struct Pc *)self, matrix, 0, 0);
}

static int pc_hit_action(int job)
{
    switch (job) {
    case 0xe: case 0x13: case 0x14: case 0x18:
    case 0xfc5: case 0xfca: case 0xfcb: case 0xfce: case 0xfcf: case 0xfd0: case 0x1081: case 0x1082:
    case 0x1084: case 0x108e: case 0x108f: case 0x1091: case 0x1093: case 0x1094: case 0x10ce: case 0x10dc:
        return 0x20;
    case 0x107a: case 0x107b: case 0x107c: case 0x107d: case 0x10d4: case 0x10db:
        return 0x50;
    default:
        return 0x60;
    }
}

static void pc_cap_zoom(struct Pc *self)
{
    if (self->aid == g_session1.ownAid)
        game_camera_limit_distance(g_maxViewDistance);
}

static void pc_attack_reset(struct Pc *self)
{
    self->casting = 0;
    if (self->body_state != 1 && self->body_state != 2)
        self->motion_held = 0;
}

static void pc_shoot(struct Pc *self, float height, int job)
{
    game_mode_launch_arrow(game_current_mode(), &self->pos, self->attack_target_aid, height, job);
    self->attack_motion_count = 8.0f / self->motion_delay + self->attack_motion_count;
}

void pc_set_state(struct GameActor * base_actor, int state, unsigned char snap)
{
    struct Pc * self = (struct Pc *)base_actor;

    struct GameActor *actor = (struct GameActor *)self;
    int old = self->state;
    int action;

    if (old == 3 || self->state_locked || state == -1)
        return;
    if (old == 4 && snap == 1) {
        self->pos.x = self->slide_x;
        self->pos.z = self->slide_z;
    }
    if (game_actor_get_transform_job(actor) > 0) {
        action = game_actor_transform_action_of(state);
    } else {
        action = 0;
        switch (state) {
        case 0: action = 0x20; break;
        case 6: action = 0x10; break;
        case 7: action = pc_hit_action(game_actor_get_job(actor)); break;
        case 8: {
            int job = game_actor_get_job(actor);

            action = job == 0x107a || job == 0x107c || job == 0x10d4 ? 0x28 : 0x60;
            break;
        }
        }
    }
    switch (state) {
    case 0: {
        int job;

        game_actor_set_state(actor, 0, snap);
        if (game_actor_get_job(actor) == 0xfcf)
            pc_cap_zoom(self);
        if (game_actor_get_job(actor) == 0x1082)
            pc_cap_zoom(self);
        job = game_actor_get_job(actor);
        if (job == 0x108f || game_actor_get_job(actor) == 0x1091)
            pc_cap_zoom(self);
        self->motion = self->head_dir;
        if (old == 2 || old == 4 || old == 8 || old == 52 || old == 53 || old == 54 || old == 58 || old == 59
            || self->pk_mode)
            render_object_set_action((struct RenderObject *)self, action, 0);
        game_complete_item_use();
        break;
    }
    case 1:
        pc_attack_reset(self);
        self->head_dir = 0;
        game_actor_set_state(actor, 1, snap);
        break;
    case 2: {
        unsigned char accepts, arrow;
        int job, kind;

        pc_attack_reset(self);
        accepts = session_job_accepts_weapon(&g_session1, game_actor_get_job(actor), self->sex,
                                            (uint64_t)self->weapon);
        switch (game_actor_get_job(actor)) {
        case 0xfce: case 0xfcf: case 0xfd0: case 0x1081: case 0x1082: case 0x108e: case 0x108f: case 0x1091:
        case 0x1093: case 0x1094: case 0x10ce: case 0x10dc: {
            int r = rand() % 10;

            if (r < 0)
                r = 0;
            accepts = r < 7;
            break;
        }
        }
        game_actor_set_state(actor, accepts ? 9 : 2, snap);
        self->attack_motion_count = session_get_job_float(&g_session1, game_actor_get_job(actor), self->sex,
                                             (int)self->weapon, accepts);
        kind = (int)self->weapon;
        if (kind > 0x69)
            kind = session_get_weapon_view_id(kind);
        arrow = 0;

        client_lua_call(g_session1.luaState, "IsItemUsingArrow", "d>b", kind, &arrow);
        if (arrow == 1) {
            if (actor_get_skip_arrow(self) == 1)
                actor_set_skip_arrow(self, 0);
            else
                game_mode_launch_arrow(game_current_mode(), &self->pos, self->attack_target_aid,
                           self->motion_delay * self->attack_motion_count, 0);
            self->attack_motion_count = 8.0f / self->motion_delay + self->attack_motion_count;
        }
        if (session_is_doram_job(&g_session1, game_actor_get_job(actor)) == 1 && self->soul_attack == 1) {
            job = game_actor_get_job(actor);
            pc_shoot(self, self->motion_delay * self->attack_motion_count, job);
        }
        break;
    }
    case 6:
        actor_set_unit_cell(self);
        render_object_set_action((struct RenderObject *)self, action, 1);
        self->motion_finished = 1;
        self->motion = self->head_dir;
        break;
    case 7: {
        int job;

        if (self->pose_locked == 1 || self->state == 6)
            return;
        render_object_set_action((struct RenderObject *)self, action, 1);
        job = game_actor_get_job(actor);
        switch (job) {
        case 0xfb4: case 0xfb5: case 0xfaf: case 0xfe2: case 0xfe9: case 0xf: case 0xfb0: case 0xfc6:
        case 0xfe6: case 0xfed: case 0x100a: case 0x10a6:
            render_object_set_script_step_frames((struct RenderObject *)self, 4);
            render_object_play_motion_range((struct RenderObject *)self, 0x60, 0, 0, 0x60, 0, 1, 0x190, 0);
            break;
        case 0x19: case 0x107e:
            render_object_set_script_step_frames((struct RenderObject *)self, 4);
            render_object_play_motion_range((struct RenderObject *)self, 0x60, 2, 5, 0x60, 5, 1, 0x3e8, 0);
            break;
        case 0x18: case 0x1084:
            render_object_set_script_step_frames((struct RenderObject *)self, 4);
            render_object_play_motion_range((struct RenderObject *)self, 0x60, 0, 3, 0x60, 3, 1, 0x3e8, 0);
            break;
        }
        self->attack_motion_count = 6.0f;
        break;
    }
    case 8:
        actor_set_unit_cell(self);
        render_object_set_action((struct RenderObject *)self, action, 1);
        self->motion_finished = 1;
        break;
    case 9: {
        unsigned char accepts;
        float t;

        pc_attack_reset(self);
        accepts = session_job_accepts_weapon(&g_session1, game_actor_get_job(actor), self->sex,
                                            (uint64_t)self->weapon);
        game_actor_set_state(actor, accepts ? 9 : 2, snap);
        t = session_get_job_float(&g_session1, game_actor_get_job(actor), self->sex,
                                 (int)self->weapon, accepts);
        self->attack_motion_count = t;
        if (self->weapon == 0xe || self->weapon == 0xd)
            pc_shoot(self, t * self->motion_delay, 0);
        break;
    }
    default:
        game_actor_set_state(actor, state, snap);
        return;
    }
    self->action = game_actor_dir_of((struct GameObject *)self, self->roty)
                   + self->base_action;
    self->state = state;
    self->start_tick = platform_ticks_ms();
}
