#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "ui/window_access.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "actor/actor.h"
#include "actor/pc.h"
#include "std/std.h"
#include "actor/sprite_render.h"

const struct GameActorOps pc_ops = {
    .base = {
        .base = {
            .destroy = pc_deleting_destruct,
            .on_process = pc_on_process,
            .send_msg = pc_send_msg,
            .render = pc_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = game_actor_set_action,
        .update_motion = render_object_update_motion_base,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = pc_process_state,
    .set_state = pc_set_state,
    .set_unit_cell = game_actor_set_unit_cell,
    .clear_unit_cell = game_actor_clear_unit_cell,
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

#define PC_RES_COUNT 9

struct Pc *pc_construct(struct Pc *self)
{
    game_actor_construct((struct GameActor *)self);
    self->ops = &pc_ops.base.base;
    string_init(&self->imf_name);
    self->footprint_last.x = 0.0f;
    self->footprint_last.y = 0.0f;
    self->footprint_last.z = 0.0f;
    string_init(&self->head_pal_name);
    self->layer_sprs.first = self->layer_sprs.last = self->layer_sprs.end = NULL;
    self->layer_acts.first = self->layer_acts.last = self->layer_acts.end = NULL;
    vec_resize(&self->layer_sprs, PC_RES_COUNT, sizeof(void *));
    vec_resize(&self->layer_acts, PC_RES_COUNT, sizeof(void *));
    self->hp_gage_wnd = NULL;
    self->head_palette = 0;
    self->shield = 0;
    self->honor = 0;
    self->virtue = 0;
    self->guild_id = 0;
    self->head_dir = 0;
    self->shoes = 0;
    self->single_sprite = 0;
    self->pvp_rank = 0;
    self->pvp_total = 0;
    self->is_pc = 1;
    self->robe = -1;
    self->body_flags = -1;
    self->body_clip_x = -1;
    self->body_clip_y = -1;
    self->costume_body_obj = 0;
    self->char_id = 0;
    self->body_style = 0;
    self->look_job = 0;
    self->soul_attack = 0;
    self->helm_robe = 0;
    self->hat_flags = 0;
    self->madogear_type = 0;
    self->skip_arrow = 0;
    self->head_clip[0] = 0.0f;
    self->head_clip[1] = 0.0f;
    self->head_clip[2] = 0.0f;
    self->head_clip[3] = 0.0f;
    return self;
}

static void pc_destroy(struct Pc *self)
{
    int i;

    self->ops = &pc_ops.base.base;
    for (i = 0; i < PC_RES_COUNT; i++) {
        if (((struct Res **)self->layer_sprs.first)[i])
            res_unlock(((struct Res **)self->layer_sprs.first)[i]);
        if (((struct Res **)self->layer_acts.first)[i])
            res_unlock(((struct Res **)self->layer_acts.first)[i]);
    }
    if (self->hp_gage_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->hp_gage_wnd);
        self->hp_gage_wnd = NULL;
    }
    vec_free(&self->layer_acts);
    vec_free(&self->layer_sprs);
    string_free(&self->head_pal_name);
    string_free(&self->imf_name);
    game_actor_dc_game_actor((struct GameActor *)self);
}

void pc_destruct(struct Pc *self)
{
    pc_destroy(self);
}

void pc_deleting_destruct(struct GameObject *object)
{
    struct Pc *self = (struct Pc *)object;

    pc_destroy(self);
    mem_free(self);
}
