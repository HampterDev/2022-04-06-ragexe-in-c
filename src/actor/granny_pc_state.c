#include "ui/window_types.h"
#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "ui/window_access.h"
#include "render/renderer.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include <string.h>
#include "actor/actor.h"
#include "actor/pc.h"
#include "std/std.h"
#include "actor/weather.h"
#include "actor/sprite_render.h"
#include "actor/granny_pc.h"
#include "game/actor_entry.h"
#include "game/gamemode.h"
#include "resources/attr_types.h"
#include "session/session_jobs.h"
#include "ui/character/gauge.h"
#include "ui/window.h"
#include "ui/wndmgr/wndmgr.h"
#include "session/session_skills.h"

unsigned char granny_pc_on_process(struct GameObject *object)
{
    struct GrannyPc *self = (struct GrannyPc *)object;

    struct GameActor *actor = (struct GameActor *)self;
    unsigned char result;
    float roty, facing;

    game_actor_process_effect_state(actor);
    game_actor_process_will_be_attacked(actor);
    game_actor_process_chat_balloon_(actor);
    if (self->hp_gage_wnd) {
        int raise = g_renderer->height * 12 / 480;

        ui_window_move(self->hp_gage_wnd, self->screen_x - 30, (int)((float)raise * self->scale + (float)self->screen_y));
    }
    game_actor_place_hp_gage(actor);
    if (self->hit_flash && timeGetTime() > self->hit_tick + 100) {
        self->hit_argb = 0xffff0000;
        self->hit_flash = 0;
    }
    if (self->argb != 0xffffffff)
        render_object_modify_argb((struct RenderObject *)self, 8, 8, 8, 8);
    result = granny_pc_process_state((struct GameActor *)self);
    render_object_process_sound((struct RenderObject *)self);
    if (self->wave_pending) {
        struct GameActor *player = (struct GameActor *)game_current_player();

        if (player)
            sound_play_wave((const char *)self->wave_name, self->pos.x - player->pos.x, 0.0f,
                            self->pos.z - player->pos.z, 0xfa, 0x28, 1.0f);
        self->wave_pending = 0;
    }

    roty = self->roty;
    if (roty > 360.0f) {
        roty = (float)((int)roty % 360);
        self->roty = roty;
    }
    if (self->first_frame == 1) {
        facing = 180.0f - roty;
        self->first_frame = 0;
        self->facing = facing;
    } else {
        int target = (int)(180.0f - (float)(int)roty);
        int diff, ahead;

        facing = self->facing;
        diff = (int)(facing - (float)target);
        if (diff > 360 || diff < -360)
            diff %= 360;
        ahead = diff < 0 ? diff + 360 : diff;
        if (ahead > 16) {
            facing = ahead < 180 ? facing - 16.0f : facing + 16.0f;
            self->facing = facing;
        }
    }
    if (facing > 360.0f) {
        facing = (float)((int)facing % 360);
        self->facing = facing;
    }
    if (0.0f > facing) {
        facing = facing + 360.0f;
        self->facing = facing;
    }
    return result;
}

unsigned char granny_pc_process_state(struct GameActor * base_actor)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    struct GameActor *actor = (struct GameActor *)self;

    if (self->motion_hold_until && timeGetTime() > self->motion_hold_until) {
        self->motion_held = 0;
        self->motion_hold_until = 0;
    }
    if (!game_actor_process_fade_out(actor))
        return 0;
    switch (self->state) {
    case 2: case 7:
        granny_pc_update_motion((struct RenderObject *)self);
        if (self->motion_finished)
            granny_pc_set_state((struct GameActor *)self, 0, 1);
        return 1;
    case 3:
        self->script_range_on = 0;
        self->script_steps_on = 0;
        self->script_pingpong_on = 0;
        self->motion_held = 0;
        granny_pc_update_motion((struct RenderObject *)self);
        return 1;
    case 6:
        granny_pc_update_motion((struct RenderObject *)self);
        return 1;
    case 8: {
        int frame;

        granny_pc_update_motion((struct RenderObject *)self);
        frame = (int)((float)(timeGetTime() - self->start_tick) / 24.0f);
        if (self->attack_target_aid && frame % 34 == 0) {
            struct GameActor *target = game_find_actor(self->attack_target_aid);

            if (target)
                render_object_turn_toward((struct RenderObject *)self, target->pos.x, target->pos.z);
        }
        return 1;
    }
    default:
        return game_actor_process_state(actor);
    }
}

static unsigned char granny_pc_attack_state(struct GrannyPc *self, unsigned char snap)
{
    unsigned char accepts;

    self->casting = 0;
    if (self->body_state != 1 && self->body_state != 2)
        self->motion_held = 0;
    accepts = session_job_accepts_weapon(&g_session1, game_actor_get_job((struct GameActor *)self),
                                        self->sex, (unsigned __int64)self->weapon);
    game_actor_set_state((struct GameActor *)self, accepts ? 9 : 2, snap);
    return accepts;
}

static void granny_pc_shoot(struct GrannyPc *self, float height)
{
    game_mode_launch_arrow(game_current_mode(), &self->pos, self->attack_target_aid, height, 0);
    self->attack_motion_count = 8.0f / self->motion_delay + self->attack_motion_count;
}

void granny_pc_set_state(struct GameActor * base_actor, int state, unsigned char snap)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    struct GameActor *actor = (struct GameActor *)self;
    int old = self->state;

    if (old == 3 || self->state_locked)
        return;
    if (old == 4 && snap == 1) {
        self->pos.x = self->slide_x;
        self->pos.z = self->slide_z;
    }
    switch (state) {
    case 0:
        game_actor_set_state(actor, 0, snap);
        self->motion = self->head_dir;
        if (old == 4 || old == 2 || self->pk_mode || old == 8)
            granny_pc_set_action((struct RenderObject *)self, 0x20, 0);
        game_complete_item_use();
        break;
    case 1:
        self->casting = 0;
        if (self->body_state != 1 && self->body_state != 2)
            self->motion_held = 0;
        self->head_dir = 0;
        game_actor_set_state(actor, 1, snap);
        break;
    case 2: {
        unsigned char accepts = granny_pc_attack_state(self, snap), arrow;

        self->attack_motion_count = session_get_job_float(&g_session1, game_actor_get_job(actor), self->sex,
                                             (int)self->weapon, accepts);
        arrow = 0;

        client_lua_call(g_session1.luaState, "IsItemUsingArrow", "d>b", (int)self->weapon, &arrow);
        if (arrow == 1)
            granny_pc_shoot(self, self->attack_motion_count * self->motion_delay);
        break;
    }
    case 6:
        game_actor_set_unit_cell((struct GameActor *)self);
        granny_pc_set_action((struct RenderObject *)self, 0x10, 1);
        self->motion_finished = 1;
        self->motion = self->head_dir;
        break;
    case 7: {
        int job;

        if (old == 6)
            return;
        job = game_actor_get_job(actor);
        if (job == 0x13 || job == 0x14 || job == 0xe || job == 0xfb4 || job == 0xfb5 || job == 0xfaf
            || job == 0xfca || job == 0xfcb || job == 0xfc5 || job == 0xfe4 || job == 0xfeb || job == 0xfe5
            || job == 0xfec || job == 0xfe9 || job == 0xfe2) {
            granny_pc_set_action((struct RenderObject *)self, 0x20, 1);
        } else if (job == 0xf || job == 0xfb0 || job == 0xfc6 || job == 0xfe6 || job == 0xfed || job == 0x100a) {
            granny_pc_set_action((struct RenderObject *)self, 0x60, 1);
            render_object_set_script_step_frames((struct RenderObject *)self, 4);
            render_object_play_motion_range((struct RenderObject *)self, 0x60, 0, 0, 0x60, 0, 1, 0x190, 0);
        } else {
            granny_pc_set_action((struct RenderObject *)self, 0x60, 1);
        }
        self->attack_motion_count = 6.0f;
        break;
    }
    case 8:
        game_actor_set_unit_cell((struct GameActor *)self);
        granny_pc_set_action((struct RenderObject *)self, 0x60, 1);
        self->motion_finished = 1;
        break;
    case 9: {
        unsigned char accepts = granny_pc_attack_state(self, snap);
        float t = session_get_job_float(&g_session1, game_actor_get_job(actor), self->sex,
                                       (int)self->weapon, accepts);

        self->attack_motion_count = t;
        if (self->weapon == 0xe || self->weapon == 0xd)
            granny_pc_shoot(self, self->motion_delay * t);
        break;
    }
    default:
        game_actor_set_state(actor, state, snap);
        if (self->state == 3)
            granny_pc_play_state_anim(self, 1);
        return;
    }
    self->model_action = game_actor_dir_of((struct GameObject *)self, self->roty)
                    + self->model_base_action;
    self->state = state;
    self->start_tick = timeGetTime();
}

static int granny_pc_packet_state(int v)
{
    return v == 1 ? 3 : v == 2 ? 6 : 0;
}

static void granny_pc_packet_look(struct GrannyPc *self, struct ActorMoveEntryInfo *entry)
{
    struct ActorLook look;
    self->head_dir = 0;
    entry->job = game_actor_state_job(entry->job, entry->effect_state);
    look = actor_look_from_move_entry(entry);
    granny_pc_set_spr_act((struct GameActor *)self, &look);
    game_actor_set_attr_state((struct GameActor *)self, entry->body_state, entry->effect_state, entry->health_state, 0);
    game_actor_set_pk_mode((struct GameActor *)self, entry->is_pk_mode);
    game_actor_set_guild((struct GameActor *)self, entry->guild_id, entry->emblem_version);
    granny_pc_set_honor_virtue((struct GameActor *)self, entry->honor, entry->virtue);
}

static void granny_pc_packet_stand(struct GrannyPc *self, struct GameMode *mode, const struct ActorEntryInfo *entry)
{
    game_mode_cell_to_world(mode, entry->x, entry->y, &self->pos.x, &self->pos.z);
    self->pos.y = c3d_attr_get_height(mode->world->attr, self->pos.x, self->pos.z);
    self->roty = (float)((entry->dir + 4) * 45);
}

static void granny_pc_packet_stand_look(struct GrannyPc *self, struct ActorEntryInfo *entry)
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
    granny_pc_set_spr_act((struct GameActor *)self, &look);
    game_actor_set_attr_state((struct GameActor *)self, entry->body_state, entry->effect_state, entry->health_state, 0);
    game_actor_set_pk_mode((struct GameActor *)self, entry->is_pk_mode);
    game_actor_set_guild((struct GameActor *)self, entry->guild_id, entry->emblem_version);
    granny_pc_set_honor_virtue((struct GameActor *)self, entry->honor, entry->virtue);
    granny_pc_set_state((struct GameActor *)self, granny_pc_packet_state(entry->state), 1);
}

static void granny_pc_packet_walk(struct GrannyPc *self, const struct ActorMoveEntryInfo *entry)
{
    if (game_actor_find_path((struct GameActor *)self, entry->src_x, entry->src_y, entry->sub_x, entry->sub_y,
                            entry->dst_x, entry->dst_y, entry->speed)) {
        self->cell_x = entry->dst_x;
        self->cell_y = entry->dst_y;
        granny_pc_set_state((struct GameActor *)self, 1, 1);
        self->path_cell = 0;
    } else {
        granny_pc_set_state((struct GameActor *)self, 0, 1);
    }
}

void granny_pc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct GrannyPc *self = (struct GrannyPc *)object;

    struct GameMode *mode = game_current_mode();
    struct ActorEntryInfo *entry = MSG_PTR(par1);
    struct ActorMoveEntryInfo *walk = MSG_PTR(par1);

    if (msg < 3 || msg > 0x7c)
        {
            game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
            return;
        }
    switch ((int)msg) {
    case 3: case 0x7c:
        granny_pc_packet_stand(self, mode, entry);
        granny_pc_packet_stand_look(self, entry);
        return;
    case 4: case 0x79:
        self->aid = entry->aid;
        granny_pc_packet_stand(self, game_current_mode(), entry);
        self->walk_speed = entry->speed;
        granny_pc_packet_stand_look(self, entry);
        if (par2 == 2 && !effect_state_bit6(self->effect_state)) {
            Vec3 zero = { 0.0f, 0.0f, 0.0f };

            game_actor_launch_effect((struct GameActor *)self, 0x158, zero, 0.0f);
        }
        game_actor_show_party_mark((struct GameActor *)self, entry->aid);
        return;
    case 6: case 0x7a:
        self->aid = walk->aid;
        self->walk_speed = walk->speed;
        self->path_start = walk->move_start_time;
        if (msg == 0x7a) {
            self->effect_state = walk->effect_state;
            self->health_state = walk->health_state;
            self->body_state = walk->body_state;
        }
        granny_pc_packet_look(self, walk);
        game_mode_cell_to_world(mode, walk->src_x, walk->src_y, &self->pos.x, &self->pos.z);
        self->pos.x = (float)mode->world->attr->zoom * 0.0625f * (float)(walk->sub_x - 8) + self->pos.x;
        self->pos.z = (float)mode->world->attr->zoom * 0.0625f * (float)(walk->sub_y - 8) + self->pos.z;
        self->pos.y = c3d_attr_get_height(mode->world->attr, self->pos.x, self->pos.z);
        granny_pc_packet_walk(self, walk);
        game_actor_show_party_mark((struct GameActor *)self, walk->aid);
        return;
    case 5: case 0x7b:
        self->aid = walk->aid;
        self->walk_speed = walk->speed;
        self->path_start = walk->move_start_time;
        granny_pc_packet_look(self, walk);
        if (game_actor_find_path((struct GameActor *)self, walk->src_x, walk->src_y, walk->sub_x, walk->sub_y,
                                walk->dst_x, walk->dst_y, walk->speed)) {
            self->cell_x = walk->dst_x;
            self->cell_y = walk->dst_y;
            granny_pc_set_state((struct GameActor *)self, 1, 1);
            self->path_cell = 0;
        }
        game_actor_show_party_mark((struct GameActor *)self, walk->aid);
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
    case 0x64:
        if (self->state == 0 || self->state == 6)
            self->motion = (int)par1;
        return;
    case 0x69: case 0x6a: {
        long long v;

        granny_pc_set_spr_job((struct GameActor *)self, game_actor_get_job((struct GameActor *)self));
        granny_pc_set_imf_name((struct GameActor *)self);
        v = granny_pc_get_weapon((struct GameActor *)self);
        granny_pc_set_weapon((struct GameActor *)self, v);
        granny_pc_set_shield((struct GameActor *)self, self->shield);
        granny_pc_set_head_pal_name((struct GameActor *)self);
        return;
    }
    case 0x72:
        self->pvp_rank = (int)par2;
        self->pvp_total = (int)par1;
        return;
    case 0x4d:
        return;
    default:
        game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
        return;
    }
}
