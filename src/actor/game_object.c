#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "ui/window_access.h"
#include "skills/skill_action_registry.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "game/context.h"
#include "actor/actor.h"
#include "std/std.h"
#include "actor/sprite_render.h"
#include "actor/game_actor.h"
#include "login/merc_ai.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "effects/rag/rag_effect.h"
#include "platform/clock.h"

static uint64_t g_gameObjectSerial;

const struct RenderObjectOps render_object_ops = {
    .base = {
        .destroy = render_object_delete,
        .on_process = render_object_on_process,
        .send_msg = render_object_send_msg,
        .render = render_object_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

const struct RenderObjectOps able_to_make_effect_ops = {
    .base = {
        .destroy = able_to_make_effect_delete,
        .on_process = render_object_on_process,
        .send_msg = able_to_make_effect_send_msg,
        .render = render_object_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

const struct GameActorOps game_actor_ops = {
    .base = {
        .base = {
            .destroy = game_actor_delete,
            .on_process = game_object_abstract_process,
            .send_msg = game_actor_send_msg,
            .render = render_object_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = game_actor_set_action,
        .update_motion = render_object_update_motion_base,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = game_actor_process_state,
    .set_state = game_actor_set_state,
    .set_unit_cell = game_actor_set_unit_cell,
    .clear_unit_cell = game_actor_clear_unit_cell,
    .set_job_model = actor_default_pair,
    .set_look = actor_default_look,
    .set_look_with_state = actor_default_look_state,
    .set_spr_job = actor_default_int,
    .set_head = actor_default_int,
    .set_weapon = actor_default_weapon,
    .set_weapon_look = actor_default_weapon,
    .set_accessory = actor_default_int,
    .set_accessory2 = actor_default_int,
    .set_accessory3 = actor_default_int,
    .set_shield = actor_default_int,
    .set_appearance_value = actor_default_int,
    .set_imf_name = actor_default_noop,
    .set_head_pal_name = actor_default_noop,
    .set_body_pal_name = actor_default_noop,
    .set_body_palette = actor_default_int,
    .set_head_palette = actor_default_int,
    .get_weapon = actor_default_no_weapon,
    .walk_motion = game_actor_walk_motion,
    .get_attack_motion = game_actor_get_attack_motion,
    .make_corpse = game_actor_make_corpse,
    .set_attack_speed = game_actor_set_attack_speed,
    .set_honor = actor_default_int,
    .set_honor_virtue = actor_default_int_flags,
    .get_char_id = actor_default_zero,
    .set_robe = actor_default_int_bool,
    .set_body_style = actor_default_int_bool,
    .set_mounted = game_actor_set_mounted,
    .get_sprite_variant = actor_default_zero,
    .set_sprite_variant = actor_default_int,
    .get_skip_arrow = actor_default_false,
    .set_skip_arrow = actor_default_bool,
    .refresh_accessory3 = actor_default_noop,
    .get_head_extent = game_actor_get_head_extent,
};

const struct RenderObjectOps mouse_pointer_ops = {
    .base = {
        .destroy = mouse_pointer_delete,
        .on_process = mouse_pointer_on_process,
        .send_msg = mouse_pointer_send_msg,
        .render = render_object_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

struct GameObject *game_object_construct(struct GameObject *self)
{
    self->ops = &game_object_ops;
    self->serial = ++g_gameObjectSerial;
    return self;
}

void game_object_destruct(struct GameObject *self)
{
    self->ops = &game_object_ops;
}

static void render_object_load_spr(struct RenderObject *self, const char *name)
{
    if (self->spr)
        res_unlock(self->spr);
    self->spr = res_mgr_get(g_resMgr(), name);
    if (self->spr)
        res_lock(self->spr);
}

static void render_object_load_act(struct RenderObject *self, const char *name)
{
    if (self->act)
        res_unlock(self->act);
    self->act = res_mgr_get(g_resMgr(), name);
    res_lock(self->act);
}

void render_object_on_init(struct RenderObject *self)
{
    char spr_name[0x80], act_name[0x80];

    self->pos.x = 0;
    self->pos.y = 0;
    self->pos.z = 0;
    self->base_action = 0;
    self->action = 0;
    self->sound_action = 0;
    self->motion = 0;
    self->sound_motion = 0;
    self->roty = 0;
    self->zoom = 1.0f;
    self->shadow_scale = 1.0f;
    self->play_mode = 0;
    self->motion_delay = 1.0f;
    self->state = 0;
    self->visible = 1;
    self->revealed = 1;
    self->argb = 0xffffffff;
    self->fixed_depth = 0;
    self->has_shadow = 0;
    self->pickable = 0;
    self->motion_finished = 0;
    self->motion_held = 0;
    self->shift_x = 0;
    self->spr_angle = 0;
    self->hit_tick = platform_ticks_ms();
    self->hit_flash = 0;
    self->slant_depth = 0;
    self->argb_locked = 0;
    self->hit_argb = 0xffff0000;
    self->depth_layer = 0;
    self->body_palette = 0;
    self->scale = 1.0f;
    self->plays = 1.0f;
    self->delay_factor = 1.0f;
    self->is_pc = 0;
    sprintf(spr_name, "NPC\\%s.spr", session_get_job_name(&g_session1, 0x2e, -1));
    sprintf(act_name, "NPC\\%s.act", session_get_job_name(&g_session1, 0x2e, -1));
    render_object_load_spr(self, spr_name);
    render_object_load_act(self, act_name);
    memset(self->saved_tint, 0, sizeof self->saved_tint);
    self->script_range_on = 0;
    self->script_steps_on = 0;
    self->script_pingpong_on = 0;
    self->script_step_frames = 10;
    self->drawn = 1;
    self->layer5_toggle = 0;
    self->pulse_phase = 0;
    self->ring_phase = 0;
    self->overlay_flags = 0;
    self->swell_step = 0;
    self->shrink_step = 0;
    self->outline_alpha = 0;
    self->overlay_step = 0;
    self->tint_fade_step = 0;
    self->hover_angle = 0;
    self->hover_rise = 0;
    self->balloon_font = 0;
    self->script_phase = 0;
    self->costume_body = 0;
    self->costume_frames = 0;
    self->aid = 0;
    self->pose_locked = 0;
    self->cell_lit = 1;
    tree_clear(&self->vars, NULL);
    self->prim_list = 0;
    self->hover_height = 0;
}

struct RenderObject *render_object_construct(struct RenderObject *self)
{
    game_object_construct((struct GameObject *)self);
    self->ops = &render_object_ops.base;
    self->pos.x = 0;
    self->pos.y = 0;
    self->pos.z = 0;
    string_init(&self->body_pal_name);
    tree_init(&self->vars);
    self->spr = NULL;
    self->act = NULL;
    render_object_on_init(self);
    return self;
}

void render_object_dc_render_object(struct RenderObject *self)
{
    self->ops = &render_object_ops.base;
    if (self->spr)
        res_unlock(self->spr);
    if (self->act)
        res_unlock(self->act);
    tree_free(&self->vars, NULL);
    string_free(&self->body_pal_name);
    self->ops = &game_object_ops;
}

void render_object_delete(struct GameObject *object)
{
    struct RenderObject *self = (struct RenderObject *)object;

    render_object_dc_render_object(self);
    mem_free(self);
}

const char *render_object_get_act_name(struct RenderObject *self)
{
    if (!self->act)
        return NULL;
    return res_get_name((struct Res *)self->act);
}

void *render_object_get_act_res(struct RenderObject *self)
{
    return self->act;
}

void *render_object_get_spr_res(struct RenderObject *self)
{
    return self->spr;
}

static void able_to_make_effect_reset(struct AbleToMakeEffect *self)
{
    self->effect_id = -1;
    self->effect_skill_level = 0;
    self->held_effect = 0;
    self->held_effect2 = 0;
    self->half_alpha = 0;
}

static void able_to_make_effect_init(struct AbleToMakeEffect *self)
{
    render_object_construct((struct RenderObject *)self);
    self->ops = &able_to_make_effect_ops.base;
    list_init(&self->effects);
    able_to_make_effect_reset(self);
}

struct AbleToMakeEffect *able_to_make_effect_construct(struct AbleToMakeEffect *self)
{
    able_to_make_effect_init(self);
    return self;
}

void able_to_make_effect_dc_able_to_make_effect(struct AbleToMakeEffect *self)
{
    struct ListNode *node;

    self->ops = &able_to_make_effect_ops.base;
    if (self->effects.size) {
        for (node = self->effects.head->next; node != self->effects.head; node = node->next) {
            struct RagEffect *effect = *LIST_VALUE(node, struct RagEffect *);

            if (effect)
                rag_effect_send_msg(effect, (struct RenderObject *)self, 0x18, 0, 0, 0);
        }
        list_clear(&self->effects, NULL);
    }
    self->held_effect = 0;
    self->held_effect2 = 0;
    list_free(&self->effects, NULL);
    render_object_dc_render_object((struct RenderObject *)self);
}

void able_to_make_effect_delete(struct GameObject *object)
{
    struct AbleToMakeEffect *self = (struct AbleToMakeEffect *)object;

    able_to_make_effect_dc_able_to_make_effect(self);
    mem_free(self);
}

unsigned char able_to_make_effect_is_effect_exist(void *actor, int effect_id)
{
    struct AbleToMakeEffect *self = actor;
    struct ListNode *head = self->effects.head, *node;

    for (node = head->next; node != head; node = node->next) {
        struct RagEffect *effect = *LIST_VALUE(node, struct RagEffect *);

        if (effect && effect->effect_id == effect_id)
            return 1;
    }
    return 0;
}

unsigned char able_to_make_effect_has_effect(struct GameActor *actor,
                                                    struct RagEffect *effect)
{
    struct AbleToMakeEffect *self = (struct AbleToMakeEffect *)actor;
    struct ListNode *head, *node;

    if (!self->effects.size)
        return 0;
    head = self->effects.head;
    for (node = head->next; node != head; node = node->next)
        if (*LIST_VALUE(node, struct RagEffect *) == effect)
            return 1;
    return 0;
}

void game_actor_on_init(struct GameActor *self)
{
    self->cursor_mark = 0;
    self->emotion_mark = 0;
    self->frozen_mark = 0;
    self->cart = 0;
    self->falcon = 0;
    self->stun_mark = 0;
    self->curse_mark = 0;
    self->sleep_mark = 0;
    self->msg_mark_2 = 0;
    self->emotion_mark_a = 0;
    self->msg_mark_1 = 0;
    self->emotion_mute = 0;
    self->lassitude_mark = 0;
    self->pos.x = 0;
    self->pos.y = 0;
    self->pos.z = 0;
    self->roty = 0;
    self->base_action = 0;
    self->action = 0;
    self->motion = 0;
    self->state = 0;
    self->play_mode = 0;
    self->motion_delay = 1.0f;
    self->has_shadow = 1;
    self->facing_vector.x = 0;
    self->facing_vector.y = 0;
    self->facing_vector.z = 0;
    self->walked = 0;
    self->aid = -1;
    self->process_server_time = session_get_server_time(&g_session1);
    self->process_tick = platform_ticks_ms();
    self->screen_x = 0;
    self->screen_y = 0;
    self->balloon_wnd = NULL;
    self->chatroom_title_wnd = NULL;
    self->shop_title_wnd = NULL;
    self->recharge_gage_wnd = NULL;
    self->balloon_width = 0;
    self->name_width = 0;
    self->motion_finished = 0;
    self->attack_target_aid = 0;
    self->attack_motion_count = -1.0f;
    self->path_cell = -1;
    self->pickable = 1;
    self->start_tick = platform_ticks_ms();
    self->pet_emotion_tick = 0;
    self->cast_end_tick = platform_ticks_ms() - 5000;
    self->efst_effect_frames = 3600;
    self->cast_start_tick = platform_ticks_ms() - 10000;
    self->argb = 0xffffffff;
    self->visible = 1;
    self->fixed_depth = 0;
    self->dies_after_hits = 0;
    self->body_state = 0;
    self->effect_state = 0;
    actor_set_mounted(self, 0);
    self->health_state = 0;
    self->pk_mode = 0;
    self->fade_until = 0;
    self->aura_level = 0;
    self->aura_on = 0;
    self->level = 0;
    self->casting = 0;
    self->state_locked = 0;
    self->wave_pending = 0;
    memset(self->wave_name, 0, sizeof self->wave_name);
    self->guild_id = 0;
    self->motion_hold_until = 0;
    self->model_kind = 0;
    self->siege_wnd = NULL;
    self->word_display_wnd = NULL;
    self->glide = 0;
    self->wave_extra_flag = 0;
    self->body_relocation = 0;
    self->attack_target_extra = 0;
    self->homun_ai = NULL;
    self->merc_ai = NULL;
    self->object_type = 4;
    self->buy_shop_title_wnd = NULL;
    self->transform_jobs[0] = -1;
    self->transform_jobs[1] = -1;
    self->transform_jobs[2] = -1;
    self->transform_jobs[3] = -1;
    self->sprite_height = 0;
    self->orb_kind = 0;
    self->orb_count = 0;
    self->effect_slots[0] = 0;
    self->effect_slots[1] = 0;
    self->effect_slots[2] = 0;
    self->effect_slots[3] = 0;
    self->effect_slots[4] = 0;
    self->cell_x = 0;
    self->name_masked = 0;
    self->mark_kind = 0;
    self->monster_gage_wnd = NULL;
    self->is_boss = 0;
    self->mount_look = 0;
    self->is_store_assistant = 0;
    self->not_pickable = 0;
    self->owner_aid = 0;
    self->misfortune_mark = 0;
    self->handicap_dots_mark = 0;
    self->sphere_count = 0;
}

struct GameActor *game_actor_construct(struct GameActor *self)
{
    able_to_make_effect_init((struct AbleToMakeEffect *)self);
    self->ops = &game_actor_ops.base.base;
    self->facing_vector.x = 0;
    self->facing_vector.y = 0;
    self->facing_vector.z = 0;
    path_info_init(&self->path);
    list_init(&self->extra_hit_queue);
    list_init(&self->hit_queue);
    self->miss_count = 0;
    self->own_hit_damage = 0;
    memset(self->requests, 0, sizeof self->requests);
    list_init(&self->attached);
    game_actor_on_init(self);
    return self;
}

void game_actor_release_attached(struct GameActor *self)
{
    struct ListNode *node;

    if (self->homun_ai && self->homun_ai->L) {
        merc_ai_close(self->homun_ai);
        if (self->homun_ai)
            mem_free(self->homun_ai);
        self->homun_ai = NULL;
    }
    if (self->merc_ai && self->merc_ai->L) {
        merc_ai_close(self->merc_ai);
        if (self->merc_ai)
            mem_free(self->merc_ai);
        self->merc_ai = NULL;
    }
    if (self->balloon_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->balloon_wnd);
        self->balloon_wnd = NULL;
    }
    if (self->chatroom_title_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->chatroom_title_wnd);
        self->chatroom_title_wnd = NULL;
    }
    if (self->shop_title_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->shop_title_wnd);
        self->shop_title_wnd = NULL;
    }
    if (self->recharge_gage_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->recharge_gage_wnd);
        self->recharge_gage_wnd = NULL;
    }
    if (self->buy_shop_title_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->buy_shop_title_wnd);
        self->buy_shop_title_wnd = NULL;
    }
    if (self->monster_gage_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->monster_gage_wnd);
        self->monster_gage_wnd = NULL;
    }
    for (node = self->attached.head->next; node != self->attached.head; node = node->next) {
        void *obj = *LIST_VALUE(node, void *);

        render_object_send(obj, NULL, 0x31, 0, 0, 0, 0);
    }
    list_clear(&self->attached, NULL);
    if (session_is_siege_mode(&g_session1) && self->siege_wnd) {
        game_mode_remove_listed(game_current_mode(), self->siege_wnd);
        ui_window_mgr_post_quit2(&g_windowMgr, self->siege_wnd);
        self->siege_wnd = NULL;
    }
    if (self->word_display_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->word_display_wnd);
        self->word_display_wnd = NULL;
    }
}

void game_actor_dc_game_actor(struct GameActor *self)
{
    self->ops = &game_actor_ops.base.base;
    game_actor_release_attached(self);
    list_free(&self->attached, NULL);
    list_free(&self->hit_queue, NULL);
    list_free(&self->extra_hit_queue, NULL);
    path_info_destroy(&self->path);
    able_to_make_effect_dc_able_to_make_effect((struct AbleToMakeEffect *)self);
}

void game_actor_delete(struct GameObject *object)
{
    struct GameActor *self = (struct GameActor *)object;

    game_actor_dc_game_actor(self);
    mem_free(self);
}

static void mouse_pointer_init(struct MousePointer *self)
{
    self->countdown = 0;
    self->fixed_depth = 1;
    self->visible = 0;
    self->start_tick = platform_ticks_ms();
    render_object_load_spr((struct RenderObject *)self, "cursors.spr");
    render_object_load_act((struct RenderObject *)self, "cursors.act");
    render_object_set_action_base((struct RenderObject *)self, 3, 0);
}

struct MousePointer *mouse_pointer_construct(struct MousePointer *self)
{
    render_object_construct((struct RenderObject *)self);
    self->ops = &mouse_pointer_ops.base;
    mouse_pointer_init(self);
    return self;
}

void mouse_pointer_delete(struct GameObject *object)
{
    struct MousePointer *self = (struct MousePointer *)object;

    self->ops = &mouse_pointer_ops.base;
    render_object_dc_render_object((struct RenderObject *)self);
    mem_free(self);
}

unsigned char mouse_pointer_on_process(struct GameObject *object)
{
    struct MousePointer *self = (struct MousePointer *)object;

    unsigned elapsed = platform_ticks_ms() - self->start_tick;
    float frames = (float)(double)elapsed / 24.0f;
    int count;

    self->action = self->base_action;
    count = actres_frame_count(self->act, self->base_action);
    self->motion = (int)(frames / self->motion_delay) % count;
    if (self->countdown) {
        if (--self->countdown == 0)
            self->visible = 0;
    }
    return 1;
}

static unsigned char argb_channel(int value)
{
    int v = value > 0 ? value : 0;

    return (unsigned char)(v < 0xff ? v : 0xff);
}

void render_object_modify_argb(struct RenderObject *self, int da, int dr, int dg,
                                         int db)
{
    unsigned char *argb = (unsigned char *)&self->argb;
    int a, r, g, b;

    if (self->argb_locked)
        return;
    a = argb[3] + da;
    r = argb[2] + dr;
    g = argb[1] + dg;
    b = argb[0] + db;
    argb[3] = argb_channel(a);
    argb[2] = argb_channel(r);
    argb[1] = argb_channel(g);
    argb[0] = argb_channel(b);
}

void mouse_pointer_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct MousePointer *self = (struct MousePointer *)object;

    if (msg == 1) {
        const float *at = (const float *)MSG_PTR(par1);
        struct C3dAttr *attr = game_current_map_attr();

        if (!attr) {
            self->visible = 0;
            return;
        }

        self->pos.x = at[0];
        self->pos.z = at[1];
        self->pos.y = c3d_attr_get_height(attr, self->pos.x, self->pos.z);
        self->visible = 1;
        self->countdown = 0;
        self->argb = par2 ? 0xffff0000 : 0xffffffff;
        return;
    }
    if (msg == 2) {
        if (par1)
            self->countdown = (int)par1;
        else
            self->visible = 0;
    }
}

void render_object_load_act_res(struct RenderObject *self, const char *name)
{
    if (self->act)
        res_unlock((struct Res *)self->act);
    self->act = res_mgr_get(g_resMgr(), name);
    res_lock((struct Res *)self->act);
}

void render_object_set_argb(struct RenderObject *self, int a, int r, int g, int b)
{
    unsigned char *argb = (unsigned char *)&self->argb;

    if (a != -1)
        argb[3] = (unsigned char)a;
    if (r != -1)
        argb[2] = (unsigned char)r;
    if (g != -1)
        argb[1] = (unsigned char)g;
    if (b != -1)
        argb[0] = (unsigned char)b;
}
