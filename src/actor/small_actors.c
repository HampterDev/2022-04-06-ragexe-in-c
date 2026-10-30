#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "ui/window_access.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "actor/actor.h"
#include <math.h>
#include "actor/pc.h"
#include "actor/game_actor.h"
#include "actor/sprite_render.h"
#include "platform/language.h"
#include "core/msg_str.h"
#include "platform/clientinfo.h"
#include "session/session_skills.h"
#include "actor/clone_owner.h"

const struct RenderObjectOps sound_maker_ops = {
    .base = {
        .destroy = sound_maker_delete,
        .on_process = sound_maker_on_process,
        .send_msg = sound_maker_send_msg,
        .render = sound_maker_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

const struct GameActorOps npc_ops = {
    .base = {
        .base = {
            .destroy = npc_delete,
            .on_process = npc_on_process,
            .send_msg = npc_send_msg,
            .render = npc_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = game_actor_set_action,
        .update_motion = render_object_update_motion_base,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = game_actor_process_state,
    .set_state = npc_set_state,
    .set_unit_cell = npc_set_unit_cell,
    .clear_unit_cell = npc_clear_unit_cell,
    .set_job_model = npc_set_job_and_sex,
    .set_look = actor_default_look,
    .set_look_with_state = actor_default_look_state,
    .set_spr_job = npc_set_spr_job_,
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

const struct RenderObjectOps corpse_ops = {
    .base = {
        .destroy = corpse_delete,
        .on_process = corpse_on_process,
        .send_msg = corpse_send_msg,
        .render = corpse_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

const struct GameActorOps blur_pc_ops = {
    .base = {
        .base = {
            .destroy = blur_pc_delete,
            .on_process = blur_pc_on_process,
            .send_msg = blur_pc_send_msg,
            .render = blur_pc_render,
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

const struct GameActorOps hallu_pc_ops = {
    .base = {
        .base = {
            .destroy = hallu_pc_delete,
            .on_process = hallu_pc_on_process,
            .send_msg = hallu_pc_send_msg,
            .render = hallu_pc_render,
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

const struct GameActorOps hallu_pc2_ops = {
    .base = {
        .base = {
            .destroy = hallu_pc2_delete,
            .on_process = hallu_pc2_on_process,
            .send_msg = hallu_pc2_send_msg,
            .render = hallu_pc2_render,
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

const struct GameActorOps hallu_npc_ops = {
    .base = {
        .base = {
            .destroy = hallu_npc_delete,
            .on_process = hallu_npc_on_process,
            .send_msg = hallu_npc_send_msg,
            .render = hallu_npc_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = game_actor_set_action,
        .update_motion = render_object_update_motion_base,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = game_actor_process_state,
    .set_state = npc_set_state,
    .set_unit_cell = npc_set_unit_cell,
    .clear_unit_cell = npc_clear_unit_cell,
    .set_job_model = npc_set_job_and_sex,
    .set_look = actor_default_look,
    .set_look_with_state = actor_default_look_state,
    .set_spr_job = npc_set_spr_job_,
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

const struct GameActorOps hallu_plant_ops = {
    .base = {
        .base = {
            .destroy = hallu_plant_delete,
            .on_process = hallu_plant_on_process,
            .send_msg = hallu_plant_send_msg,
            .render = hallu_plant_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = game_actor_set_render_info,
        .set_action = game_actor_set_action,
        .update_motion = render_object_update_motion_base,
        .get_effect_state = game_actor_get_effect_state,
    },
    .process_state = game_actor_process_state,
    .set_state = npc_set_state,
    .set_unit_cell = npc_set_unit_cell,
    .clear_unit_cell = npc_clear_unit_cell,
    .set_job_model = npc_set_job_and_sex,
    .set_look = actor_default_look,
    .set_look_with_state = actor_default_look_state,
    .set_spr_job = npc_set_spr_job_,
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

void sound_maker_destruct(struct SoundMaker *self)
{
    self->ops = &sound_maker_ops.base;
    string_free(&self->wave);
    render_object_dc_render_object((struct RenderObject *)self);
}

void sound_maker_delete(struct GameObject *object)
{
    struct SoundMaker *self = (struct SoundMaker *)object;

    sound_maker_destruct(self);
    mem_free(self);
}

void npc_destruct(struct Npc *self)
{
    self->ops = &npc_ops.base.base;
    if (self->player_gage) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->player_gage);
        self->player_gage = NULL;
    }
    if (self->pc_gage) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->pc_gage);
        self->pc_gage = NULL;
    }
    game_actor_dc_game_actor((struct GameActor *)self);
}

void npc_delete(struct GameObject *object)
{
    struct Npc *self = (struct Npc *)object;

    npc_destruct(self);
    mem_free(self);
}

void corpse_destruct(struct Corpse *self)
{
    self->ops = &corpse_ops.base;
    render_object_dc_render_object((struct RenderObject *)self);
}

void corpse_delete(struct GameObject *object)
{
    struct Corpse *self = (struct Corpse *)object;

    corpse_destruct(self);
    mem_free(self);
}

void blur_pc_destruct(struct BlurPC *self)
{
    self->ops = &blur_pc_ops.base.base;
    pc_destruct((struct Pc *)self);
}

void blur_pc_delete(struct GameObject *object)
{
    struct BlurPC *self = (struct BlurPC *)object;

    blur_pc_destruct(self);
    mem_free(self);
}

void hallu_pc_destruct(struct HalluPC *self)
{
    self->ops = &hallu_pc_ops.base.base;
    pc_destruct((struct Pc *)self);
}

void hallu_pc_delete(struct GameObject *object)
{
    struct HalluPC *self = (struct HalluPC *)object;

    hallu_pc_destruct(self);
    mem_free(self);
}

void hallu_pc2_destruct(struct HalluPC2 *self)
{
    self->ops = &hallu_pc2_ops.base.base;
    pc_destruct((struct Pc *)self);
}

void hallu_pc2_delete(struct GameObject *object)
{
    struct HalluPC2 *self = (struct HalluPC2 *)object;

    hallu_pc2_destruct(self);
    mem_free(self);
}

void hallu_npc_destruct(struct HalluNPC *self)
{
    self->ops = &hallu_npc_ops.base.base;
    npc_destruct((struct Npc *)self);
}

void hallu_npc_delete(struct GameObject *object)
{
    struct HalluNPC *self = (struct HalluNPC *)object;

    hallu_npc_destruct(self);
    mem_free(self);
}

void hallu_plant_destruct(struct HalluPlant *self)
{
    self->ops = &hallu_plant_ops.base.base;
    npc_destruct((struct Npc *)self);
}

void hallu_plant_delete(struct GameObject *object)
{
    struct HalluPlant *self = (struct HalluPlant *)object;

    hallu_plant_destruct(self);
    mem_free(self);
}

static float frames_since_start(const struct RenderObject *self)
{
    unsigned elapsed = timeGetTime() - self->start_tick;

    return (float)(double)elapsed / 24.0f;
}

static void launch_plain(struct GameActor *self, int effect_id)
{
    Vec3 zero = { 0, 0, 0 };

    game_actor_launch_effect(self, effect_id, zero, 0.0f);
}

static void mirror_owner_flag(struct GameActor *self, struct GameActor **owner)
{
    if (!*owner)
        return;
    if (!actor_is_live(*owner)) {
        *owner = NULL;
        clone_owner_stale++;
        return;
    }
    if ((*owner)->overlay_flags & 0x10) {
        self->overlay_flags = 0x10;
        self->swell_step = 0x2d;
    }
}

unsigned char blur_pc_on_process(struct GameObject *object)
{
    struct BlurPC *self = (struct BlurPC *)object;

    mirror_owner_flag((struct GameActor *)self, &self->owner);
    if (self->argb < self->fade_step)
        return 0;
    self->argb -= self->fade_step;
    return 1;
}

unsigned char corpse_on_process(struct GameObject *object)
{
    struct Corpse *self = (struct Corpse *)object;

    float frames = frames_since_start((struct RenderObject *)self);
    int dir = game_object_dir_of((struct GameObject *)self, self->roty);

    self->action = dir + self->base_action;
    self->motion = actres_frame_count(render_object_get_act_res((struct RenderObject *)self),
                                      self->action) - 1;
    render_object_set_argb((struct RenderObject *)self, (int)(255.0f - frames), -1, -1, -1);
    return !(frames >= 255.0f);
}

unsigned char hallu_npc_on_process(struct GameObject *object)
{
    struct HalluNPC *self = (struct HalluNPC *)object;

    unsigned n = ++self->frames;

    if (self->argb < self->fade_step)
        return 0;
    if (n == 0x6e) {
        launch_plain((struct GameActor *)self, 0x319);
        n = self->frames;
    }
    if (n > 0x69 && !(n & 3))
        self->argb -= self->fade_step;
    return 1;
}

unsigned char hallu_pc2_on_process(struct GameObject *object)
{
    struct HalluPC2 *self = (struct HalluPC2 *)object;

    mirror_owner_flag((struct GameActor *)self, &self->owner);
    self->frames++;
    if (self->argb < self->fade_step)
        return 0;
    self->argb -= self->fade_step;
    return 1;
}

unsigned char hallu_pc_on_process(struct GameObject *object)
{
    struct HalluPC *self = (struct HalluPC *)object;

    unsigned n = ++self->frames;

    if (self->argb < self->fade_step)
        return 0;
    if (n == 0x6e) {
        launch_plain((struct GameActor *)self, 0x319);
        n = self->frames;
    }
    if (n > 0x69 && !(n & 3))
        self->argb -= self->fade_step;
    return 1;
}

unsigned char hallu_plant_on_process(struct GameObject *object)
{
    struct HalluPlant *self = (struct HalluPlant *)object;

    return ++self->frames <= 0x3c;
}

void hallu_pc_render(struct GameObject *object, const float *matrix)
{
    struct HalluPC *self = (struct HalluPC *)object;

    struct RenderObject *ro = (struct RenderObject *)self;

    mirror_owner_flag((struct GameActor *)self, &self->owner);
    if (is_effect_state_star2(self->effect_state) || effect_state_bit6(self->effect_state) || is_effect_state_burrow(self->effect_state))
        return;
    rand();
    rand();
    rand();
    rand();
    if (!self->visible) {
        render_object_render_projected(ro, matrix);
        return;
    }
    if (g_serviceType == 4 && clientinfo_is_admin_aid(self->aid) && self->aid != g_session1.ownAid)
        return;
    if (self->state != 6 && self->state != 3)
        render_object_render_shadow((struct RenderObject *)self, matrix, self->zoom);
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
    pc_render_body_layers((struct Pc *)self, matrix, 1, 0);
}

static float ss_max(float a, float b)
{
    return a > b ? a : b;
}

unsigned char sound_maker_on_process(struct GameObject *object)
{
    struct SoundMaker *self = (struct SoundMaker *)object;

    struct RenderObject *player;
    float dx, dz, sx, sz, slope, ox, oz, at_x, at_z;
    int ratio;

    if (self->area_w == 0)
        return 1;
    player = (struct RenderObject *)game_current_player();
    if (!player)
        return 1;
    if (render_object_calc_ground_dist((struct RenderObject *)self, player->pos.x, player->pos.z) -
            (self->area_reach + self->range) > 130.0f)
        return 1;
    dx = player->pos.x - self->pos.x;
    dz = player->pos.z - self->pos.z;
    sx = 1.0f;
    sz = 1.0f;
    if (!(dx >= 0.0f && dz >= 0.0f)) {
        if (0.0f > dx && dz >= 0.0f)
            sx = -1.0f;
        else if (0.0f > dx && 0.0f > dz)
            sx = sz = -1.0f;
        else if (dx >= 0.0f && 0.0f > dz)
            sz = -1.0f;
    }
    ratio = self->area_h / self->area_w;
    slope = dz / ss_max(0.001f, dx);
    if ((float)ratio >= (float)fabs(slope)) {
        ox = (float)self->area_w * sx * 0.5f;
        oz = ox * slope;
    } else {
        oz = (float)self->area_h * sz * 0.5f;
        ox = oz / ss_max(0.001f, slope);
    }
    at_x = self->pos.x + ox;
    at_z = self->pos.z + oz;
    if (render_object_calc_ground_dist(player, at_x, at_z) - self->range > 75.0f)
        return 1;
    if (self->wave.size == 0)
        return 1;
    if (timeGetTime() > self->interval + self->last_play) {
        self->last_play = timeGetTime();
        sound_play_wave(string_cstr(&self->wave), at_x - player->pos.x, 0.0f,
                        at_z - player->pos.z, (int)self->range, (int)(self->range / 6.0f), self->volume);
    }
    return 1;
}
