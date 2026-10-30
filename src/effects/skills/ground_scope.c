#include "skills/skill_action.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "render/texture_cache.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "world/ground_api.h"
#include "skills/skill_action_registry.h"
#include "session/state.h"
#include "effects/render_object.h"
#include "effects/skills/skill_view.h"
#include "actor/game_actor.h"
#include "game/mode_callbacks.h"

#define MSG_SET_CELL 0xe
#define MSG_SET_CASTER 0x16
#define MSG_SET_END 0x73
#define MSG_SET_SKILL 0x75

#define SKILL_POISON_SMOKE 0x7f0
#define SKILL_COMET 0x8a5
#define SKILL_COMPETENTIA 0x149e

struct GroundScope {
    struct AbleToMakeEffect base;
    int caster_aid;
    int cell_x, cell_y;
    unsigned end_tick;
    int size;
    float angle;
    float spin;
    int skill_id;
};

struct GameObject;

const struct RenderObjectOps ground_scope_ops = {
    .base = {
        .destroy = ground_scope_delete,
        .on_process = ground_scope_on_process,
        .send_msg = ground_scope_send_msg,
        .render = ground_scope_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

static struct GroundScope *ground_scope_construct(struct GroundScope *self)
{
    able_to_make_effect_construct(&self->base);
    self->base.ops = &ground_scope_ops.base;
    self->end_tick = 0;
    self->size = 1;
    self->angle = 0.0f;
    self->skill_id = -1;
    return self;
}

struct GroundScope *ground_scope_new(void)
{
    struct GroundScope *self = mem_try_alloc(sizeof *self);

    return self ? ground_scope_construct(self) : NULL;
}

void ground_scope_delete(struct GameObject *object)
{
    struct GroundScope *self = (struct GroundScope *)object;

    able_to_make_effect_dc_able_to_make_effect(&self->base);
    mem_free(self);
}

unsigned char ground_scope_on_process(struct GameObject *object)
{
    struct GroundScope *self = (struct GroundScope *)object;

    return self->end_tick >= timeGetTime();
}

static int ground_scope_area_cells(int skill_id)
{
    switch (skill_id) {
    case 0x2e0: case 0x98f: case 0x991:
        return -2;
    case 0xb: case 0x19: case 0x2f: case 0x45: case 0x50: case 0x56: case 0x58: case 0x6e: case 0x81:
    case 0x82: case 0x89: case 0x8c: case 0x17d: case 0x1e3: case 0x8d5:
        return 3;
    case 0x11: case 0x15: case 0x46: case 0x5b: case 0x5c: case 0x1e4: case 0x302: case 0x7d5: case 0x7f0:
    case 0x7fc:
        return 5;
    case 0x4f: case 0x1de: case 0x2dd: case 0x2e3: case 0x8bd: case 0x992: case 0x994: case 0x995: case 0x996:
    case 0x138e: case 0x1395: case 0x13a4: case 0x13b2: case 0x146b: case 0x146d: case 0x146e: case 0x149f:
        return 7;
    case 0x53: case 0x59: case 0x2d3: case 0x2db: case 0x301: case 0x7d8: case 0x8d4: case 0x98e: case 0x138c:
    case 0x1462: case 0x1466: case 0x1493: case 0x14c7: case 0x14f9: case 0x14fa: case 0x14fb: case 0x14fc:
    case 0x14fd:
        return 9;
    case 0x55: case 0x1461:
        return 11;
    case 0x2c4: case 0x8a5:
        return 18;
    case 0x149e:
        return 19;
    }
    return skill_action_mgr_get_skill_area(skill_action_mgr_instance(), skill_id);
}

void ground_scope_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct GroundScope *self = (struct GroundScope *)object;

    switch (msg) {
    case MSG_SET_CELL:
        self->cell_x = (int)par1;
        self->cell_y = (int)par2;
        return;
    case MSG_SET_CASTER:
        self->caster_aid = (int)par1;
        return;
    case MSG_SET_END:
        self->end_tick = (unsigned)par1;
        return;
    case MSG_SET_SKILL: {
        int skill_id = (int)par1;
        self->size = ground_scope_area_cells(skill_id) + 2;
        self->skill_id = skill_id;
        self->spin = self->size ? 0.5f / (float)self->size : 0.0f;
        return;
    }
    }
    able_to_make_effect_send_msg((struct GameObject *)&self->base, source, msg, par1, par2, par3, par4);
}

void ground_scope_render(struct GameObject *object, const float *view)
{
    struct GroundScope *self = (struct GroundScope *)object;

    int x0, y0, row, col;
    float half, s, c;
    unsigned color;
    struct C3dGround15 *ground;

    ground = game_current_ground();
    if (!ground)
        return;
    half = (float)self->size * 0.5f;
    x0 = self->cell_x - self->size / 2;
    y0 = self->cell_y - self->size / 2;
    s = float_sin(self->angle);
    c = float_cos(self->angle);
    color = ((unsigned)(s * 40.0f) << 24) - 0x7f000001u;

    for (row = 0; row < self->size; row++) {
        float r0 = (float)row - half, r1 = (float)(row + 1) - half;
        float r0s = r0 * s, r0c = r0 * c, r1s = r1 * s, r1c = r1 * c;

        for (col = 0; col < self->size; col++) {
            float a = (float)col - half, b = (float)(col + 1) - half;
            float as = a * s, ac = a * c, bs = b * s, bc = b * c;
            float n = (float)self->size;
            float uv[8];
            const char *name;
            unsigned cell_color = color;

            uv[0] = (r0s + ac + half) / n;
            uv[1] = (r0c - as + half) / n;
            uv[2] = (bc + r0s + half) / n;
            uv[3] = (r0c - bs + half) / n;
            uv[4] = (r1s + ac + half) / n;
            uv[5] = (r1c - as + half) / n;
            uv[6] = (r1s + bc + half) / n;
            uv[7] = (r1c - bs + half) / n;

            if (self->skill_id == SKILL_POISON_SMOKE) {
                name = g_session1.map_rule_flags[0] == 1
                           ? "effect\\mineffect\\new_poisonsmoke\\new_poisonsmoke_cast\\poi_magic_circle_1.tga"
                           : "effect\\new_poisonsmoke\\new_poisonsmoke_cast\\poi_magic_circle_1.tga";
            } else if (self->skill_id == SKILL_COMET) {
                name = "effect\\new_comet\\new_comet_cast_magicrune\\new_comet_cast_magicrune.tga";
            } else if (self->skill_id == SKILL_COMPETENTIA) {
                name = "effect\\competentia\\competentia_cast_bottom\\com_rune_01.tga";
                cell_color = (unsigned)(unsigned char)(int)((float)(double)(color >> 24) * 0.6f) << 24 | 0xffffff;
            } else {
                name = "effect\\magic_target.tga";
            }
            c3d_ground15_draw_attr_quad(ground, view, x0 + col, y0 + row, cell_color,
                       texture_cache_get(&g_textureCache, name, 0, 0), uv);
        }
    }
    if (g_isSkipFrame)
        self->angle = self->spin + self->angle;
}
