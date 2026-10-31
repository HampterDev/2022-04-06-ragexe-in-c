#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "actor/appearance.h"
#include "render/rp_pool.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "actor/render_object.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "render/renderer.h"
#include "core/character_npc.h"
#include "ui/render/character_render.h"
#include "actor/game_actor.h"
#include "game/actor_entry.h"
#include "effects/rag/rag_effect.h"
#include "session/session_items.h"

const struct GameActorOps g_characterNpcVtbl_ops = {
    .base = {
        .base = {
            .destroy = character_npc_delete,
            .on_process = npc_on_process,
            .send_msg = character_npc_send_msg,
            .render = character_npc_render,
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

struct CharacterNpc *character_npc_construct(struct CharacterNpc *self)
{
    npc_construct((struct Npc *)self);
    self->ops = &g_characterNpcVtbl_ops.base.base;
    self->model = NULL;
    return self;
}

void character_npc_destruct(struct CharacterNpc *self)
{
    self->ops = &g_characterNpcVtbl_ops.base.base;
    if (self->model) {
        character_render_destroy(self->model);
        self->model = NULL;
    }
    npc_destruct((struct Npc *)self);
}

void character_npc_delete(struct GameObject *object)
{
    struct CharacterNpc *self = (struct CharacterNpc *)object;

    character_npc_destruct(self);
    mem_free(self);
}

static void npc_model_fill(struct CharacterNpc *self, const struct ActorLook *look, int robe, int body)
{
    struct CharacterRender *model = self->model;

    model->pos[0] = self->pos.x;
    model->pos[1] = self->pos.y;
    model->pos[2] = self->pos.z;
    self->model->job = self->job;
    self->model->sex = self->sex;
    self->model->head = self->entry_head;
    self->model->body_palette = look->body_palette;
    self->model->head_palette = look->head_palette;
    self->model->accessory_top = look->accessory[0];
    self->model->accessory_mid = look->accessory[1];
    self->model->accessory_bottom = look->accessory[2];
    character_render_set_robe(self->model, robe);
    self->model->body_style = body;
}

static void npc_model_fill_entry(struct CharacterNpc *self, const struct ActorEntryInfo *entry)
{
    struct ActorLook look = actor_look_from_entry(entry);

    npc_model_fill(self, &look, entry->robe, entry->body);
}

static void npc_model_fill_move(struct CharacterNpc *self, const struct ActorMoveEntryInfo *entry)
{
    struct ActorLook look = actor_look_from_move_entry(entry);

    npc_model_fill(self, &look, entry->robe, entry->body);
}

void character_npc_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct CharacterNpc *self = (struct CharacterNpc *)object;

    const struct ActorEntryInfo *entry = MSG_PTR(par1);
    const struct ActorMoveEntryInfo *walk = MSG_PTR(par1);

    switch (msg) {
    case 3:
        game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
        if (!self->model)
            return;
        npc_model_fill_entry(self, entry);
        return;
    case 4:
        game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
        self->visible = session_is_visible_npc(entry->job);
        game_actor_set_guild((struct GameActor *)self, entry->guild_id, entry->emblem_version);
        if (self->model)
            return;
        self->model = character_render_create(0, 0);
        npc_model_fill_entry(self, entry);
        return;
    case 6:
    case 0x7a:
        game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
        if (self->model)
            return;
        self->model = character_render_create(0, 0);
        npc_model_fill_move(self, walk);
        return;
    case 5:
    case 0x7b:
        game_actor_send_msg(object, source, msg, par1, par2, par3, par4);
        if (!self->model)
            return;
        npc_model_fill_move(self, walk);
        return;
    default:
        npc_send_msg(object, source, msg, par1, par2, par3, par4);
        return;
    }
}

void character_npc_render(struct GameObject *object, const float *view)
{
    struct CharacterNpc *self = (struct CharacterNpc *)object;

    struct List prims;
    struct ListNode *node;
    struct CharacterRender *model;
    struct ScreenBounds info;
    BYTE has_info;
    float screen_x, screen_y;

    if (!self->visible || !self->revealed)
        return;
    self->model->action = self->action;
    self->model->motion = self->motion;
    has_info = 0;
    character_render_collect_world_quads(self->model, &prims, self->pickable, &has_info);
    model = self->model;
    info = model->bbox;
    screen_y = model->screen_y;
    screen_x = model->screen_x;
    if (has_info == 1)
        game_actor_set_render_info((struct RenderObject *)self, &info, screen_x, screen_y);
    else
        render_object_set_screen_position((struct RenderObject *)self, screen_x, screen_y);
    for (node = prims.head->next; node != prims.head; node = node->next)
        renderer_add_prim(g_renderer, &LIST_VALUE(node, struct ChrQuadPair)->quad->base,
                          LIST_VALUE(node, struct ChrQuadPair)->flags);
    list_free(&prims, NULL);
}
