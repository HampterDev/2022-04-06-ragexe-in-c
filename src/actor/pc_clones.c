#include "actor/actor_ops.h"
#include "session/state.h"
#include "actor/actor.h"
#include "actor/pc.h"
#include "actor/sprite_render.h"
#include "session/session_party.h"

#define PC_LAYERS 9

void pc_set_riding(struct GameActor * base_actor, unsigned char value)
{
    struct Pc * self = (struct Pc *)base_actor;

    self->mounted = value;
    actor_set_shield(self, self->shield);
}

void pc_set_body_style(struct GameActor * base_actor, int value, unsigned char apply)
{
    struct Pc * self = (struct Pc *)base_actor;

    self->body_style = value;
    if (apply) {
        actor_set_spr_job(self, game_actor_get_job((struct GameActor *)self));
        actor_set_body_pal_name(self);
    }
}

void pc_set_layer_spr_names(struct Pc *self, const struct Vec *names)
{
    int layer;

    for (layer = 0; layer < PC_LAYERS; layer++)
        pc_set_layer_spr(self, layer, string_cstr(&((const struct String *)names->first)[layer]));
}

void pc_set_layer_act_names(struct Pc *self, const struct Vec *names)
{
    int layer;

    for (layer = 0; layer < PC_LAYERS; layer++)
        pc_set_layer_act(self, layer, string_cstr(&((const struct String *)names->first)[layer]));
}

static void pc_copy_look_head(struct Pc *self, struct Pc *src)
{
    self->visible = 1;
    self->has_shadow = 0;
    self->start_tick = timeGetTime();
    game_actor_set_sex((struct GameActor *)self, src->sex);
    game_actor_set_job((struct GameActor *)self, game_actor_get_job((struct GameActor *)src));
    pc_set_riding((struct GameActor *)self, src->mounted);
    game_actor_set_transform_job_override((struct GameActor *)self, game_actor_get_transform_job((struct GameActor *)src));
    self->pos = src->pos;
    self->scale = src->scale;
    self->shift_x = src->shift_x;
    self->zoom = src->zoom;
    self->base_action = src->base_action;
    self->action = src->action;
    self->motion = src->motion;
    render_object_load_act_res((struct RenderObject *)self, render_object_get_act_name((struct RenderObject *)src));
    self->fixed_depth = src->fixed_depth;
    self->depth_layer = src->depth_layer;
    self->slant_depth = src->slant_depth;
    self->hit_flash = src->hit_flash;
    self->spr_angle = src->spr_angle;
    self->body_palette = 0;
    self->head_palette = src->head_palette;
    self->effect_state = game_actor_get_effect_state((struct RenderObject *)src);
}

static void pc_copy_look_tail(struct Pc *self, struct Pc *src)
{
    struct String copy;
    struct Vec names;

    string_copy(&copy, &src->head_pal_name);
    if (&self->head_pal_name != &copy) {
        string_free(&self->head_pal_name);
        string_move(&self->head_pal_name, &copy);
    }
    string_free(&copy);
    pc_set_layer_spr_names(self, pc_layer_spr_names((struct Pc *)src, &names));
    string_vec_free(&names);
    pc_set_layer_act_names(self, pc_layer_act_names((struct Pc *)src, &names));
    string_vec_free(&names);
    string_assign(&self->imf_name, string_cstr(&src->imf_name), src->imf_name.size);
    self->guild_id = game_actor_get_guild_id((struct GameActor *)src);
    self->aid = src->aid;
}

struct BlurPC *blur_pc_construct(struct BlurPC *self, struct Pc *master)
{
    pc_construct((struct Pc *)self);
    self->ops = &blur_pc_ops.base.base;
    self->owner = (struct GameActor *)master;
    pc_copy_look_head((struct Pc *)self, master);
    pc_copy_look_tail((struct Pc *)self, master);
    self->fade_param = 0;
    self->fade_step = 0x10000000;
    self->body_palette = master->body_palette;
    string_assign(&self->body_pal_name, string_cstr(&master->body_pal_name), master->body_pal_name.size);
    self->robe = 0;
    self->accessory[0] = 0;
    self->accessory[1] = 0;
    self->accessory[2] = 0;
    return self;
}

struct HalluPC2 *hallu_pc2_construct(struct HalluPC2 *self, struct Pc *master)
{
    pc_construct((struct Pc *)self);
    self->ops = &hallu_pc2_ops.base.base;
    self->owner = (struct GameActor *)master;
    pc_copy_look_head((struct Pc *)self, master);
    pc_copy_look_tail((struct Pc *)self, master);
    self->fade_float = 0;
    self->frames = 0;
    self->fade_step = 0x10000000;
    self->body_palette = master->body_palette;
    string_assign(&self->body_pal_name, string_cstr(&master->body_pal_name), master->body_pal_name.size);
    self->robe = 0;
    self->accessory[0] = 0;
    self->accessory[1] = 0;
    self->accessory[2] = 0;
    return self;
}

struct RenderObject *hallu_pc_construct(void *mem, struct RenderObject *actor)
{
    struct HalluPC *self = mem;
    struct Pc *master = (struct Pc *)actor;

    pc_construct((struct Pc *)self);
    self->ops = &hallu_pc_ops.base.base;
    self->owner = (struct GameActor *)master;
    pc_copy_look_head((struct Pc *)self, master);
    pc_copy_look_tail((struct Pc *)self, master);
    self->fade_float = 0;
    self->frames = 0;
    self->fade_step = 0x10000000;
    self->body_palette = master->body_palette;
    string_assign(&self->body_pal_name, string_cstr(&master->body_pal_name), master->body_pal_name.size);
    self->not_pickable = 1;
    pc_set_body_style((struct GameActor *)self, master->body_style, 1);
    self->robe = 0;
    self->accessory[0] = 0;
    self->accessory[1] = 0;
    self->accessory[2] = 0;
    return (struct RenderObject *)self;
}

struct Npc *npc_construct(struct Npc *self)
{
    game_actor_construct((struct GameActor *)self);
    self->ops = &npc_ops.base.base;
    self->player_gage = NULL;
    self->player_in_area = 0;
    self->last_effect_tick = timeGetTime();
    self->area_effect_launched = 0;
    self->mark = NULL;
    self->mark_effect = 9999;
    self->mark_icon = 0;
    return self;
}

static void npc_job_palette(struct Npc *self)
{
    char name[64];

    self->body_palette = 0;
    if (session_named_body_palettes_enabled(&g_session1)) {
        self->body_palette = 1;
        string_assign_cstr(&self->body_pal_name, name);
    }
}

struct HalluNPC *hallu_npc_construct(struct HalluNPC *self, struct GameActor *src)
{
    int sex;

    npc_construct((struct Npc *)self);
    self->ops = &hallu_npc_ops.base.base;
    self->visible = 1;
    self->has_shadow = 0;
    self->start_tick = timeGetTime();
    game_actor_set_sex((struct GameActor *)self, src->sex);
    game_actor_set_job((struct GameActor *)self, game_actor_get_job(src));
    self->mounted = src->mounted;
    game_actor_set_transform_job_override((struct GameActor *)self, game_actor_get_transform_job(src));
    self->pos = src->pos;
    self->scale = src->scale;
    self->shift_x = src->shift_x;
    self->zoom = src->zoom;
    self->base_action = src->base_action;
    self->action = src->action;
    self->motion = src->motion;
    render_object_load_act_res((struct RenderObject *)self, render_object_get_act_name((struct RenderObject *)src));
    self->fixed_depth = src->fixed_depth;
    self->depth_layer = src->depth_layer;
    self->slant_depth = src->slant_depth;
    self->hit_flash = src->hit_flash;
    self->spr_angle = src->spr_angle;
    self->body_palette = 0;
    self->effect_state = game_actor_get_effect_state((struct RenderObject *)src);
    sex = src->sex;
    game_actor_set_job((struct GameActor *)self, game_actor_get_job(src));
    game_actor_set_sex((struct GameActor *)self, sex);
    npc_job_palette((struct Npc *)self);
    self->guild_id = game_actor_get_guild_id(src);
    self->aid = 0;
    self->fade_level = 0;
    self->frames = 0;
    self->fade_step = 0x10000000;
    self->not_pickable = 1;
    return self;
}

struct HalluPlant *hallu_plant_construct(struct HalluPlant *self)
{
    int sex;

    npc_construct((struct Npc *)self);
    self->ops = &hallu_plant_ops.base.base;
    self->visible = 1;
    self->has_shadow = 0;
    self->start_tick = timeGetTime();
    game_actor_set_sex((struct GameActor *)self, 0);
    game_actor_set_job((struct GameActor *)self, 0x437);
    self->mounted = 0;
    game_actor_set_transform_job_override((struct GameActor *)self, -1);
    self->pos.x = 0.0f;
    self->pos.y = 0.0f;
    self->pos.z = 0.0f;
    self->scale = 1.0f;
    self->shift_x = 0;
    self->zoom = 1.0f;
    self->base_action = 0;
    self->action = 4;
    self->motion = 0;
    render_object_load_act_res((struct RenderObject *)self, "sprite\\몬스터\\blue_plant.act");
    sex = self->sex;
    self->fixed_depth = 0;
    self->depth_layer = 0;
    self->slant_depth = 0;
    self->hit_flash = 0;
    self->spr_angle = 0;
    self->body_palette = 0;
    self->effect_state = 0;
    game_actor_set_job((struct GameActor *)self, game_actor_get_job((struct GameActor *)self));
    game_actor_set_sex((struct GameActor *)self, sex);
    npc_job_palette((struct Npc *)self);
    self->guild_id = 0;
    self->aid = 0;
    self->frames = 0;
    self->not_pickable = 1;
    return self;
}

struct Corpse *corpse_construct(struct Corpse *self)
{
    render_object_construct((struct RenderObject *)self);
    self->ops = &corpse_ops.base;
    self->visible = 1;
    self->slant_depth = 1;
    self->extra_scale = 0.0f;
    self->start_tick = timeGetTime();
    return self;
}

struct SoundMaker *world_sound_construct(struct SoundMaker *self)
{
    render_object_construct((struct RenderObject *)self);
    self->ops = &sound_maker_ops.base;
    string_init(&self->wave);
    self->volume = 100.0f;
    self->range = 1.0f;
    self->area_w = 3;
    self->area_h = 3;
    self->area_reach = float_sqrt(18.0f);
    self->last_play = timeGetTime() - 4000;
    return self;
}
