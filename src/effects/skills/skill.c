#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "ui/window_access.h"
#include "actor/appearance.h"
#include "game/world_objects.h"
#include "world/model_types.h"
#include "resources/model_res.h"
#include "render/projection.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "resources/attr_queries.h"
#include "contents/sound.h"
#include "world/pick_node.h"
#include "effects/durations.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "effects/effect_mgr.h"
#include "effects/skills/skill_view.h"
#include "effects/skills/skill.h"
#include <stdlib.h>
#include "render/render_api.h"
#include "actor/actor_common.h"
#include "actor/game_actor.h"
#include "world/world.h"

#define UNIT_JOB 0x7e
#define NEW_FRAMES 36000
#define MSG_CELL_GONE 0x18
#define LOOP_EXTEND 0x15

struct GameActor;
struct RenderObject;
struct Skill;

const struct GameActorOps skill_ops = {
    .base = {
        .base = {
            .destroy = skill_delete,
            .on_process = skill_on_process,
            .send_msg = skill_send_msg,
            .render = skill_render,
            .get_direction = game_actor_dir_of,
        },
        .render_body = skill_render_body,
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

#include "world/scene_objects.h"
#include "effects/rag/rag_effect_types.h"
#include "effects/rag/rag_effect.h"

static void skill_reset(struct Skill *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;

    game_actor_set_job((struct GameActor *)me, UNIT_JOB);
    self->frames = NEW_FRAMES;
    me->pickable = 0;
    me->revealed = 0;
    self->base.balloon_wnd = 0;
    self->model = 0;
    self->loop_effect = 0;
    self->creator_aid = 0;
    self->range = 0;
    self->launched = 0;
    self->cells.last = self->cells.first;
}

struct Skill *skill_construct(struct Skill *self, int unit_aid)
{
    game_actor_construct(&self->base);
    self->base.ops = &skill_ops.base.base;
    self->cells.first = 0;
    self->cells.last = 0;
    self->cells.end = 0;
    self->unit_aid = unit_aid;
    skill_reset(self);
    return self;
}

void skill_release_model(struct Skill *self)
{
    void *list;

    if (!self->model)
        return;
    list = game_current_scene_root();
    if (list)
        scene_graph_node_remove(list, self->model, 0);
    if (self->model)
        self->model->ops->destroy(self->model);
    self->model = 0;
    if (self->loop_effect)
        rag_effect_send_msg(self->loop_effect, 0, MSG_CELL_GONE, 0, 0, 0);
    self->loop_effect = 0;
    if (self->launched) {
        effect_remove_if_dead(effect_mgr_find_effect(g_effectMgr, self->unit_aid, self->effect_id));
        self->launched = 0;
    }
}

static void skill_release(struct Skill *self)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    struct SkillCell *cell;

    if (self->base.balloon_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, self->base.balloon_wnd);
        self->base.balloon_wnd = 0;
    }
    skill_release_model(self);
    for (cell = self->cells.first; cell != self->cells.last; cell++) {
        struct World *world = game_current_world();
        int x = cell->x, y = cell->y;
        if (!world)
            break;
        world_send_msg_to_object(world, me, x, y, MSG_CELL_GONE, 0, 0, 0, 0);
    }
    self->cells.last = self->cells.first;
}

void skill_destruct(struct Skill *self)
{
    self->base.ops = &skill_ops.base.base;
    skill_release(self);
    vec_free(&self->cells);
    game_actor_dc_game_actor(&self->base);
}

void skill_delete(struct GameObject *object)
{
    struct Skill *self = (struct Skill *)object;

    skill_destruct(self);
    mem_free(self);
}

const char *skill_trap_model(struct Skill *self)
{
#define TRAP_DIR "외부소품\\트랩"
#define TRAP3_DIR "event\\3차트랩_"
    int job = game_actor_get_job((struct GameActor *)&self->base);

    switch (job) {
    case 0x8f: case 0xda:
        return TRAP_DIR "03_3.rsm";
    case 0x90:
        return TRAP_DIR "02.rsm";
    case 0x91:
        return TRAP_DIR "01.rsm";
    case 0x93:
        return TRAP_DIR "03.rsm";
    case 0x94:
        return TRAP_DIR "03_6.rsm";
    case 0x95:
        return TRAP_DIR "03_4.rsm";
    case 0x96:
        return TRAP_DIR "03_5.rsm";
    case 0x97:
        return TRAP_DIR "03_2.rsm";
    case 0x98:
        return TRAP_DIR "04.rsm";
    case 0x99:
        return TRAP_DIR "05.rsm";
    }
    return 0;
#undef TRAP_DIR
#undef TRAP3_DIR
}

void skill_load_model(struct Skill *self, const char *name)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    float colors[3][3] = {
        { 0.5f, 0.7f, 0.5f },
        { 1.0f, 1.0f, 1.0f },
        { 0.5f, 0.5f, 0.5f },
    };
    struct C3dModelRes *res;
    struct C3dActor *model;
    struct SceneGraphNode *scene;
    Vec3 pos;

    if (!name || !me->revealed)
        return;
    skill_release_model(self);
    scene = game_current_scene_root();
    if (!scene)
        return;
    res = res_mgr_get(g_resMgr(), name);
    if (!res)
        return;
    self->model = model_actor_create(res->version);
    if (!self->model)
        return;
    if (!c3d_actor_add_volume_boxes(self->model, res->blocks, res->blockCount)) {
        skill_release_model(self); return;
    }
    self->model->ops->set_speed(self->model, 1.0f);
    pos = me->pos;
    model = self->model;
    model->pos[0] = pos.x;
    model->pos[1] = pos.y;
    model->pos[2] = pos.z;
    model->dirty = 1;
    if (!self->model->ops->create(self->model, res)) {
        skill_release_model(self); return;
    }
    self->model->ops->animate(self->model, 3.0);
    self->model->pickable = 0;
    self->model->animType = 0;
    self->model->ops->light(self->model, colors[0], colors[1], colors[2]);
    if (scene != game_current_scene_root()) {
        skill_release_model(self);
        return;
    }
    if (!scene_graph_node_insert(scene, self->model, 0))
        skill_release_model(self);
}

static struct RagEffect *launch(struct Skill *self, int effect_id)
{
    Vec3 here;

    here.x = 0.0f;
    here.y = 0.0f;
    here.z = 0.0f;
    return game_actor_launch_effect(&self->base, effect_id, here, 0);
}

static struct Effect *launch_by_mgr(int aid, int effect_id, Vec3 pos, int b)
{
    return effect_mgr_spawn(g_effectMgr, aid, effect_id, pos, 0, b, 0, 0);
}

static Vec3 vec_zero(void)
{
    Vec3 v;
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    return v;
}

static struct RagEffect *keep_looping(struct Skill *self, int effect_id)
{
    struct RagEffect *effect = self->loop_effect;

    if (!effect) {
        effect = launch(self, effect_id);
        self->loop_effect = effect;
        if (!effect)
            return 0;
    }
    rag_effect_set_duration(self->loop_effect, rag_effect_get_duration(effect) + LOOP_EXTEND);
    return effect;
}

static int due(struct Skill *self, int period)
{
    if (self->frames < (unsigned)period)
        return 0;
    self->frames = 0;
    return 1;
}

static void unit_sound(const char *wave)
{
    sound_play_wave(wave, 0.0f, 0.0f, 0.0f, 250, 40, 1.0f);
}

void skill_launch_area_effect(struct Skill *self, int effect_id, int period)
{
    struct RenderObject *me = (struct RenderObject *)&self->base;
    int range, i, j;

    if (effect_mgr_is_registered(g_effectMgr, effect_id))
        effect_mgr_spawn(g_effectMgr, me->aid, effect_id, vec_zero(), 0, -1, 1, 0);
    if (period <= 0)
        period = g_effectDurations[effect_id];
    period -= 20;
    if (self->frames < (unsigned)period)
        return;
    self->frames = 0;
    range = self->range < 0 ? 0 : self->range;
    for (i = -range; i <= range; i++) {
        for (j = -range; j <= range; j++) {
            Vec3 at;
            float x, z, y;
            void *attr = game_current_map_attr();
            struct GameMode *mode = game_current_mode();

            if (!attr || !mode)
                return;

            x = (float)(6 * i) + me->pos.x + (float)(rand() % 11) * 0.2f - 1.0f;
            z = (float)(6 * j) + me->pos.z + (float)(rand() % 11) * 0.2f - 1.0f;
            y = c3d_attr_get_height(attr, x, z);
            if (effect_id == 0x3bf || effect_id == 0x3cf)
                y = y + 20.0f;
            at.y = y - (float)(rand() % 11) * 0.4f;
            at.x = x;
            at.z = z;
            game_mode_launch_dummy_effect(mode, &at, effect_id, period + 20, 0);
        }
    }
}

unsigned char skill_on_process(struct GameObject *object)
{
    struct Skill *self = (struct Skill *)object;

    struct RenderObject *me = (struct RenderObject *)&self->base;
    const int *dur = g_effectDurations;

    game_actor_process_chat_balloon_((struct GameActor *)me);
    game_actor_process_will_be_attacked((struct GameActor *)me);

    switch (game_actor_get_job((struct GameActor *)me)) {
    case 0x83: keep_looping(self, 0x13f); break;
    case 0x84: keep_looping(self, 0x13e); break;
    case 0x87: case 0xc5: keep_looping(self, 0x8a); break;
    case 0x8d: case 0xc3: case 0xc4: keep_looping(self, 0x4a); break;
    case 0x92: keep_looping(self, 0xab); break;
    case 0x9a: keep_looping(self, 0xef); break;
    case 0x9b: keep_looping(self, 0xf0); break;
    case 0x9c: keep_looping(self, 0xf1); break;
    case 0x9d: keep_looping(self, 0xf2); break;
    case 0xb1: keep_looping(self, 0x12e); break;
    case 0xb3: keep_looping(self, 0x172); break;
    case 0xb4: keep_looping(self, 0x176); break;
    case 0xb6: keep_looping(self, 0x195); break;
    case 0xb7: keep_looping(self, 0x194); break;
    case 0xb8: keep_looping(self, 0x20a); break;
    case 0xb9: keep_looping(self, 0x205); break;
    case 0xbb: keep_looping(self, 0x26c); break;
    case 0xbc: keep_looping(self, 0x277); break;
    case 0xc8: keep_looping(self, 0x29f); break;
    case 0xc9: keep_looping(self, 0x2a0); break;
    case 0x7e: if (due(self, dur[0x13b] - 7)) launch(self, 0x13b); break;
    case 0x7f: if (due(self, dur[0x19] - 7)) launch(self, 0x19); break;
    case 0x80: if (due(self, dur[0x13d] - 8)) launch(self, 0x13d); break;
    case 0x81: if (due(self, dur[0x13c] - 8)) launch(self, 0x13c); break;
    case 0x82: if (due(self, dur[0x5b] - 8)) launch(self, 0x5b); break;
    case 0x88: if (due(self, dur[0x61] - 8)) launch(self, 0x61); break;
    case 0x8e:
        if (due(self, dur[0x5f] - 20)) {
            if (g_session1.map_rule_flags[0])
                launch_by_mgr(0, 0x489, me->pos, -1);
            else
                launch(self, 0x5f);
        }
        break;
    case 0xb2: if (due(self, dur[0x155] - 8)) launch(self, 0x155); break;
    case 0xbd: if (due(self, dur[0x27a] - 1)) launch(self, 0x27a); break;
    case 0xbe: if (due(self, dur[0x27e] - 1)) launch(self, 0x27e); break;
    case 0xbf: if (due(self, dur[0x27f] - 1)) launch(self, 0x27f); break;
    case 0xc0: if (due(self, dur[0x280] - 1)) launch(self, 0x280); break;
    case 0xc1: if (due(self, dur[0x281] - 1)) launch(self, 0x281); break;
    case 0xc2: if (due(self, dur[0x282] - 1)) launch(self, 0x282); break;
    case 0xda:
        if (due(self, dur[0x358] - 8)) {
            launch(self, 0x358);
            unit_sound("effect\\REVERBERATION.wav");
        }
        break;
    case 0xdc: if (due(self, dur[0x398] - 7)) launch(self, 0x398); break;
    case 0xdd: if (due(self, dur[0x39e] - 7)) launch(self, 0x39e); break;
    case 0xfe: if (due(self, dur[0x402] - 1)) launch(self, 0x402); break;
    case 0xff: case 0x100: if (due(self, dur[0x19] - 1)) launch(self, 0x19); break;
    case 0x101: if (due(self, dur[0x41a] - 1)) launch(self, 0x41a); break;
    case 0xc7:
        if (!self->launched && launch_by_mgr(self->unit_aid, 0x613, me->pos, self->range * 2))
            self->launched = 1;
        break;
    case 0xe0:
        if (!self->launched && launch_by_mgr(self->unit_aid, 0x438, me->pos, -1)) {
            self->launched = 1;
            self->effect_id = 0x438;
        }
        break;
    case 0x10c:
        if (!self->launched) {
            self->launched = 1;
            launch(self, self->effect_id);
            self->effect_id = 0x520;
        }
        break;

    case 0xfd: skill_launch_area_effect(self, 0x3fc, 0); break;
    }
    self->frames++;
    return 1;
}

void skill_render(struct GameObject *object, const float *view)
{
    struct Skill *self = (struct Skill *)object;

    struct RenderObject *me = (struct RenderObject *)&self->base;
    struct Renderer *r;
    float sx, sy, rhw, scale;
    struct ScreenBounds rect;

    if (!me->revealed) {
        render_object_render_projected(me, view);
        return;
    }
    renderer_project_point(g_renderer, &me->pos, view, &sx, &sy, &rhw);
    r = g_renderer;
    scale = renderer_screen_scale(r, rhw) / g_spriteScaleDivisor;
    me->scale = scale;
    rect.left = sx - (r->scaleX + r->scaleX) * scale;
    rect.top = sy - (r->scaleY + r->scaleY) * scale;
    rect.right = (r->scaleX + r->scaleX) * scale + sx;
    rect.bottom = sy;
    rect.z = rhw;
    skill_render_body(me, &rect, sx, sy);
}

void skill_render_body(struct RenderObject *object, const struct ScreenBounds *rect, float x, float y)
{
    struct Skill *self = (struct Skill *)object;
    struct RenderObject *me = (struct RenderObject *)&self->base;

    if (me->pickable) {
        struct PickInfo *pick;
        int least, w, h;

        pick = mem_alloc(sizeof *pick);
        if (pick) {
            pick->left = 0.0f;
            pick->top = 0.0f;
            pick->near_z = 0.0f;
            pick->right = 0.0f;
            pick->bottom = 0.0f;
            pick->far_z = 0.0f;
        }
        pick->left = rect->left < rect->right ? rect->left : rect->right;
        pick->top = rect->top;
        pick->near_z = rect->z + 350.0f;
        pick->right = rect->left > rect->right ? rect->left : rect->right;
        pick->bottom = rect->bottom;
        pick->far_z = rect->z + 350.0f;
        pick->aid = self->unit_aid;
        pick->job = game_actor_get_job((struct GameActor *)me);
        pick->attackable_as_pvp = 0;
        pick->kind = 2;
        least = (int)(renderer_scale_x(g_renderer) * 34.0f);
        w = (int)(pick->right - pick->left);
        if (w < least) {
            float grow = (float)((least - w) / 2);
            pick->right = pick->right + grow;
            pick->left = pick->left - grow;
        }
        h = (int)(pick->bottom - pick->top);
        if (h < least) {
            float grow = (float)((least - h) / 2);
            pick->bottom = pick->bottom + grow;
            pick->top = pick->top - grow;
        }
        actor_pick_node_insert(&g_actorPickNode, pick);
    }
    render_object_set_screen_position(me, x, y);
}
