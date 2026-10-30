#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "ui/window_access.h"
#include "render/texture.h"
#include "render/texture_cache.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "actor/actor.h"
#include "actor/pc.h"
#include "std/std.h"
#include "actor/weather.h"
#include "actor/sprite_render.h"
#include "actor/granny_pc.h"
#include "session/session_lua.h"
#include "session/session_names.h"

const struct GameActorOps granny_pc_ops = {
    .base = {
        .base = {
            .destroy = granny_pc_deleting_destruct,
            .on_process = granny_pc_on_process,
            .send_msg = granny_pc_send_msg,
            .render = granny_pc_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = granny_pc_set_action,
        .update_motion = granny_pc_update_motion,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = granny_pc_process_state,
    .set_state = granny_pc_set_state,
    .set_unit_cell = game_actor_set_unit_cell,
    .clear_unit_cell = game_actor_clear_unit_cell,
    .set_job_model = actor_default_pair,
    .set_look = granny_pc_set_spr_act,
    .set_look_with_state = granny_pc_set_spr_act_with_state,
    .set_spr_job = granny_pc_set_spr_job,
    .set_head = granny_pc_set_head,
    .set_weapon = granny_pc_set_weapon,
    .set_weapon_look = actor_default_weapon,
    .set_accessory = granny_pc_set_accessory,
    .set_accessory2 = granny_pc_set_accessory2,
    .set_accessory3 = granny_pc_set_accessory3,
    .set_shield = granny_pc_set_shield,
    .set_appearance_value = actor_default_int,
    .set_imf_name = granny_pc_set_imf_name,
    .set_head_pal_name = granny_pc_set_head_pal_name,
    .set_body_pal_name = actor_default_noop,
    .set_body_palette = actor_default_int,
    .set_head_palette = actor_default_int,
    .get_weapon = granny_pc_get_weapon,
    .walk_motion = game_actor_walk_motion,
    .get_attack_motion = game_actor_get_attack_motion,
    .make_corpse = game_actor_make_corpse,
    .set_attack_speed = granny_pc_set_delay_factor,
    .set_honor = granny_pc_set_honor,
    .set_honor_virtue = granny_pc_set_honor_virtue,
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

static void granny_pc_destroy(struct GrannyPc *self)
{
    self->ops = &granny_pc_ops.base.base;
    granny_pc_free_res(self);
    if (self->hp_gage_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->hp_gage_wnd);
        self->hp_gage_wnd = NULL;
    }
    if (self->shadow_tex)
        texture_destroy(self->shadow_tex);
    vec_free(&self->shadow_points);
    string_free(&self->head_pal_name);
    string_free(&self->imf_name);
    game_actor_dc_game_actor((struct GameActor *)self);
}

void granny_pc_deleting_destruct(struct GameObject *object)
{
    struct GrannyPc *self = (struct GrannyPc *)object;

    granny_pc_destroy(self);
    mem_free(self);
}

int64_t granny_pc_get_weapon(struct GameActor * base_actor)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    return self->weapon;
}

void granny_pc_set_spr_job(struct GameActor * base_actor, int job)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    self->shadow_scale = session_get_npc_location_radius(&g_session1, job, 0);
}

void granny_pc_set_head(struct GameActor * base_actor, int value)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    self->head = value;
}

void granny_pc_set_weapon(struct GameActor * base_actor, int64_t value)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    self->weapon = value;
}

void granny_pc_set_accessory(struct GameActor * base_actor, int value)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    self->accessory = value;
}

void granny_pc_set_accessory2(struct GameActor * base_actor, int value)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    self->accessory2 = value;
}

void granny_pc_set_accessory3(struct GameActor * base_actor, int value)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    self->accessory3 = value;
}

void granny_pc_set_shield(struct GameActor * base_actor, int unused)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    (void)unused;
    (void)game_actor_get_job((struct GameActor *)self);
}

void granny_pc_set_delay_factor(struct GameActor * base_actor, int ms)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    if (!(self->weapon == 11) && ms > g_maxAttackDelayMs)
        ms = g_maxAttackDelayMs;
    self->delay_factor = (float)ms / 432.0f;
}

void granny_pc_set_honor(struct GameActor * base_actor, int value)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    if (value < 0)
        granny_pc_send_msg((struct GameObject *)self, NULL, 0x15, 0x22, 0, 0, 0);
    else
        granny_pc_send_msg((struct GameObject *)self, NULL, 0x76, 0, 0, 0, 0);
    self->honor = value;
}

void granny_pc_set_honor_virtue(struct GameActor * base_actor, int value, unsigned int virtue)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    granny_pc_set_honor((struct GameActor *)self, value);
    self->virtue = virtue;
}

void granny_pc_set_head_pal_name(struct GameActor * base_actor)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    struct String name;

    if (!self->head_palette)
        return;
    session_get_head_pal_name(&name, self->head,
                            game_actor_get_job((struct GameActor *)self), self->sex, self->head_palette);
    string_free(&self->head_pal_name);
    string_move(&self->head_pal_name, &name);
}

void granny_pc_set_imf_name(struct GameActor * base_actor)
{
    struct GrannyPc * self = (struct GrannyPc *)base_actor;

    struct String name;

    session_get_imf_name(&name, game_actor_get_job((struct GameActor *)self), self->sex);
    string_free(&self->imf_name);
    string_move(&self->imf_name, &name);
}
