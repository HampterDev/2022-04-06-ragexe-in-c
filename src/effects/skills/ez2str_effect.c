#include "effects/effect_allocator.h"
#include "actor/actor_ops.h"
#include "contents/client_lua_calls.h"
#include "contents/sound.h"
#include "effects/durations.h"
#include "effects/effect_mgr.h"
#include "session/state.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effects/skills/ez2str_effect.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_math.h"
#include "effects/render_object.h"
#include "render/render_api.h"
#include "actor/game_actor.h"
#include "render/screen_draw.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"

struct Effect;

static void ez2_str_effect_destroy(struct Effect *effect);

static const struct EffectOps ez2_str_effect_ops = {
    .set_submit = ez2_str_effect_set_submit,
    .init = ez2_str_effect_init,
    .update_range = effect_update_range,
    .process = ez2_str_effect_process,
    .render = ez2_str_effect_render,
    .release = ez2_str_effect_release,
    .is_alive = ez2_str_effect_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = ez2_str_effect_after_init,
    .skip = ez2_str_effect_skip,
    .set_stop_frame = ez2_str_effect_set_stop_frame,
    .set_start_delay = ez2_str_effect_set_start_delay,
    .set_spin = ez2_str_effect_set_spin,
    .set_str_name = ez2_str_effect_set_str_name,
    .set_list_flags = ez2_str_effect_set_list_flags,
    .set_zoom = ez2_str_effect_set_zoom,
    .set_alpha_scale = ez2_str_effect_set_alpha_scale,
    .destroy = ez2_str_effect_destroy,
    .fade_out = effect_fade_out,
};

#define CEZ2STR_DEFAULT_STR "빨간포션.str"
#define HAT_FIRST 0x950

#define RENDERER_WIDTH(r)  ((r)->width)
#define RENDERER_HEIGHT(r) ((r)->height)
#define ACTOR_STATE_BITS(a) ((const struct RenderObject *)(a))->overlay_flags
#define ACTOR_RIDING(a)    ((const struct GameActor *)(a))->mounted
#define ACTOR_VISIBLE(a) ((const struct RenderObject *)(a))->visible
#define ACTOR_REVEALED(a) ((const struct RenderObject *)(a))->revealed
#define ACTOR_LIFT(a)      ((const struct RenderObject *)(a))->hover_height
#define PARTICLE_LIFETIME(p) ((p)->base.life_seconds)

struct TypeRange { short first, last; };

static int type_in(const struct TypeRange *set, int count, int type)
{
    int i;
    for (i = 0; i < count; i++)
        if (type >= set[i].first && type <= set[i].last)
            return 1;
    return 0;
}
#define TYPE_IN(set, type) type_in((set), (int)(sizeof(set) / sizeof((set)[0])), (type))

static const struct TypeRange HEADGEAR_TYPES[] = {
    {0x3f4, 0x3f4}, {0x40a, 0x40c}, {0x433, 0x433}, {0x437, 0x437}, {0x43b, 0x43f}, {0x441, 0x441},
    {0x443, 0x444}, {0x447, 0x447},
};

static const struct TypeRange HEAD_TYPES[] = {
    {0x425, 0x425}, {0x42c, 0x42c}, {0x442, 0x442}, {0x446, 0x446}, {0x44a, 0x44a}, {0x44e, 0x44f},
    {0x451, 0x451}, {0x453, 0x455}, {0x45c, 0x45c}, {0x45f, 0x45f}, {0x463, 0x464}, {0x467, 0x467},
    {0x46d, 0x46e}, {0x470, 0x470}, {0x472, 0x473}, {0x475, 0x476}, {0x4c2, 0x4c5}, {0x4cc, 0x4ce},
    {0x4d6, 0x4d7}, {0x4db, 0x4db}, {0x4ea, 0x4fb}, {0x4fd, 0x4fd}, {0x522, 0x522}, {0x543, 0x54a},
    {0x552, 0x552}, {0x590, 0x593}, {0x5c5, 0x5c5}, {0x5ce, 0x5d1}, {0x62d, 0x62f}, {0x631, 0x639},
    {0x63b, 0x63f}, {0x64b, 0x650}, {0x657, 0x660}, {0x663, 0x676}, {0x6a2, 0x6a9}, {0x6ac, 0x6ac},
    {0x6c2, 0x6c3}, {0x6cd, 0x6cd}, {0x900, 0x900}, {0x904, 0x904}, {0x910, 0x921}, {0x923, 0x924},
    {0x926, 0x926}, {0x943, 0x944},
};

static const struct TypeRange OWNER_BOUND_TYPES[] = {
    {0x08d, 0x08d}, {0x3e6, 0x3e6}, {0x45f, 0x45f}, {0x470, 0x470}, {0x473, 0x473}, {0x4c5, 0x4c5},
    {0x4ee, 0x4f1}, {0x514, 0x51d}, {0x552, 0x552}, {0x5bb, 0x5c4}, {0x63f, 0x63f}, {0x64b, 0x650},
    {0x676, 0x676}, {0x6cd, 0x6cd}, {0x8fe, 0x8fe},
};

static const struct TypeRange LOOPING_TYPES[] = {
    {0x3e6, 0x3e6}, {0x442, 0x442}, {0x45f, 0x45f}, {0x470, 0x470}, {0x473, 0x473}, {0x4c5, 0x4c5},
    {0x4dc, 0x4e5}, {0x4ee, 0x4f1},
};

static const struct TypeRange KEYED_LOOPING_TYPES[] = {
    {0x552, 0x552}, {0x584, 0x584}, {0x5a6, 0x5a6}, {0x607, 0x607}, {0x63f, 0x63f}, {0x64b, 0x650},
    {0x676, 0x676}, {0x6cd, 0x6cd}, {0x8fe, 0x8fe},
};

static const struct TypeRange BEFORE_ACTORS_TYPES[] = {
    {0x4e7, 0x4e7}, {0x4eb, 0x4eb}, {0x4f6, 0x4f9}, {0x4fb, 0x4fb}, {0x4fd, 0x4fd}, {0x503, 0x504},
    {0x507, 0x507}, {0x509, 0x50a}, {0x511, 0x511}, {0x51f, 0x51f}, {0x521, 0x521}, {0x544, 0x544},
    {0x54a, 0x54a}, {0x54c, 0x54c}, {0x54f, 0x54f}, {0x551, 0x551}, {0x556, 0x556}, {0x558, 0x558},
    {0x55a, 0x55a}, {0x55c, 0x55c}, {0x563, 0x563}, {0x569, 0x569}, {0x56b, 0x56b}, {0x571, 0x571},
    {0x575, 0x575}, {0x586, 0x587}, {0x589, 0x589}, {0x595, 0x595}, {0x59c, 0x5a0}, {0x5a2, 0x5a2},
    {0x5ab, 0x5ad}, {0x5af, 0x5b0}, {0x5b4, 0x5b4}, {0x5b6, 0x5b6}, {0x5b8, 0x5b8}, {0x5ce, 0x5ce},
    {0x5d1, 0x5d1}, {0x5d3, 0x5d3}, {0x5d5, 0x5d5}, {0x5d9, 0x5da}, {0x5de, 0x5de}, {0x5e0, 0x5e0},
    {0x5e2, 0x5e2}, {0x5ff, 0x600}, {0x602, 0x602}, {0x60c, 0x60c}, {0x60f, 0x60f}, {0x612, 0x612},
    {0x616, 0x616}, {0x61a, 0x61a}, {0x61c, 0x61c}, {0x623, 0x623}, {0x629, 0x629}, {0x62b, 0x62b},
    {0x62e, 0x62e}, {0x632, 0x632}, {0x634, 0x634}, {0x636, 0x636}, {0x63c, 0x63c}, {0x63e, 0x63e},
    {0x655, 0x656}, {0x659, 0x659}, {0x65c, 0x65c}, {0x65e, 0x65e}, {0x660, 0x660}, {0x662, 0x662},
    {0x664, 0x665}, {0x668, 0x668}, {0x66b, 0x66b}, {0x675, 0x675}, {0x67b, 0x67b}, {0x682, 0x682},
    {0x684, 0x684}, {0x68c, 0x68c}, {0x698, 0x698}, {0x69b, 0x69c}, {0x6a8, 0x6a8}, {0x6ab, 0x6ab},
    {0x6c3, 0x6c3}, {0x8fe, 0x8fe}, {0x911, 0x911}, {0x914, 0x914}, {0x917, 0x917}, {0x91c, 0x91d},
    {0x923, 0x923},
};

struct TypeShift { short first, last; signed char shift; };
static const struct TypeShift HEAD_HEIGHT_SHIFTS[] = {
    {0x40b, 0x40b, 5}, {0x45f, 0x45f, 5},
    {0x425, 0x425, -6}, {0x46d, 0x46d, -6}, {0x476, 0x476, -6}, {0x4c2, 0x4c2, -6}, {0x4c5, 0x4c5, -6},
    {0x4cd, 0x4ce, -6}, {0x4d6, 0x4d6, -6}, {0x545, 0x548, -6}, {0x5cf, 0x5d0, -6}, {0x63f, 0x63f, -6},
    {0x64b, 0x650, -6},
    {0x42c, 0x42c, -8}, {0x43b, 0x43b, -8}, {0x442, 0x442, -8}, {0x44a, 0x44a, -8}, {0x44e, 0x44f, -8},
    {0x451, 0x451, -8}, {0x453, 0x455, -8}, {0x45c, 0x45c, -8}, {0x463, 0x463, -8}, {0x46e, 0x46e, -8},
    {0x470, 0x470, -8}, {0x472, 0x473, -8},
    {0x43c, 0x43c, -7},
    {0x43d, 0x43d, -5}, {0x441, 0x441, -5}, {0x443, 0x443, -5}, {0x447, 0x447, -5}, {0x4ed, 0x4ed, -5},
    {0x552, 0x552, -5},
    {0x464, 0x464, 8}, {0x467, 0x467, 8},
    {0x475, 0x475, -4}, {0x4c3, 0x4c3, -4}, {0x522, 0x522, -4}, {0x543, 0x544, -4}, {0x549, 0x54a, -4},
    {0x5c5, 0x5c5, -4}, {0x5ce, 0x5ce, -4}, {0x5d1, 0x5d1, -4}, {0x900, 0x900, -4}, {0x904, 0x904, -4},
    {0x913, 0x921, -4}, {0x923, 0x924, -4}, {0x926, 0x926, -4}, {0x943, 0x944, -4},
    {0x4c4, 0x4c4, -10}, {0x4d7, 0x4d7, -10}, {0x4db, 0x4db, -10}, {0x4ee, 0x4f1, -10},
    {0x4cc, 0x4cc, -16},
    {0x4ec, 0x4ec, -2}, {0x4f2, 0x4f4, -2}, {0x4f6, 0x4f9, -2}, {0x4fd, 0x4fd, -2}, {0x63b, 0x63e, -2},
    {0x6c2, 0x6c3, -2}, {0x6cd, 0x6cd, -2},
    {0x639, 0x639, -3}, {0x657, 0x657, -3}, {0x672, 0x673, -3}, {0x676, 0x676, -3}, {0x6a7, 0x6a9, -3},
};

static void lua_ask_hat(const char *func, const char *fmt, int hat, void *out)
{
    client_lua_call(g_session1.luaState, func, fmt, hat, out);
}

static void cue(struct Ez2StrEffect *self, const char *wave, int frame)
{
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, wave, frame, 0.0f);
}

static void cue_message(struct Ez2StrEffect *self, int frame)
{
    effect_add_sound_cue(&self->base, SOUND_CUE_GAME_MESSAGE, 0, frame, 0.0f);
}

static void set_list(struct Ez2StrEffect *self, int list_flags)
{
    self->particle->base.list_flags = list_flags;
}

struct Ez2StrEffect *ez2_str_effect_construct(struct Ez2StrEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &ez2_str_effect_ops;
    self->particle = 0;
    string_init(&self->str_name);
    string_assign(&self->str_name, CEZ2STR_DEFAULT_STR, 12);
    self->adjust_position_when_shrunk = 0;
    self->screen_shade = 0;
    self->turn_with_camera = 0;
    self->camera_angle = 0.0;
    self->attached_to_head = 0;
    return self;
}

struct Effect *ez2_str_effect_new(void)
{
    struct Ez2StrEffect *self = mem_try_alloc(sizeof *self);

    return self ? &ez2_str_effect_construct(self)->base : NULL;
}

/* Effect types created as Ez2StrEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow ez2_str_effect_type_rows[] = {
    EFFECT_TYPES(0x0cc, 0x0d3, ez2_str_effect_new),
    EFFECT_TYPES(0x3e4, 0x3e7, ez2_str_effect_new),
    EFFECT_TYPES(0x3f0, 0x3f2, ez2_str_effect_new),
    EFFECT_TYPE(0x3e9, ez2_str_effect_new),
    EFFECT_TYPE(0x3f3, ez2_str_effect_new),
    EFFECT_TYPE(0x3e0, ez2_str_effect_new),
    EFFECT_TYPE(0x3e3, ez2_str_effect_new),
    EFFECT_TYPE(0x3e2, ez2_str_effect_new),
    EFFECT_TYPE(0x3e1, ez2_str_effect_new),
    EFFECT_TYPE(0x3f4, ez2_str_effect_new),
    EFFECT_TYPE(0x40a, ez2_str_effect_new),
    EFFECT_TYPE(0x437, ez2_str_effect_new),
    EFFECT_TYPE(0x43f, ez2_str_effect_new),
    EFFECT_TYPES(0x40b, 0x40c, ez2_str_effect_new),
    EFFECT_TYPE(0x426, ez2_str_effect_new),
    EFFECT_TYPE(0x428, ez2_str_effect_new),
    EFFECT_TYPE(0x427, ez2_str_effect_new),
    EFFECT_TYPES(0x42a, 0x42c, ez2_str_effect_new),
    EFFECT_TYPE(0x432, ez2_str_effect_new),
    EFFECT_TYPE(0x425, ez2_str_effect_new),
    EFFECT_TYPES(0x433, 0x436, ez2_str_effect_new),
    EFFECT_TYPES(0x43b, 0x43e, ez2_str_effect_new),
    EFFECT_TYPES(0x441, 0x442, ez2_str_effect_new),
    EFFECT_TYPES(0x443, 0x444, ez2_str_effect_new),
    EFFECT_TYPES(0x446, 0x447, ez2_str_effect_new),
    EFFECT_TYPE(0x44a, ez2_str_effect_new),
    EFFECT_TYPES(0x44d, 0x44f, ez2_str_effect_new),
    EFFECT_TYPE(0x451, ez2_str_effect_new),
    EFFECT_TYPES(0x453, 0x455, ez2_str_effect_new),
    EFFECT_TYPE(0x45c, ez2_str_effect_new),
    EFFECT_TYPES(0x48a, 0x48b, ez2_str_effect_new),
    EFFECT_TYPE(0x489, ez2_str_effect_new),
    EFFECT_TYPE(0x45f, ez2_str_effect_new),
    EFFECT_TYPE(0x461, ez2_str_effect_new),
    EFFECT_TYPES(0x463, 0x464, ez2_str_effect_new),
    EFFECT_TYPES(0x466, 0x467, ez2_str_effect_new),
    EFFECT_TYPE(0x469, ez2_str_effect_new),
    EFFECT_TYPE(0x08d, ez2_str_effect_new),
    EFFECT_TYPES(0x46d, 0x46e, ez2_str_effect_new),
    EFFECT_TYPES(0x470, 0x473, ez2_str_effect_new),
    EFFECT_TYPES(0x475, 0x476, ez2_str_effect_new),
    EFFECT_TYPES(0x4c2, 0x4c5, ez2_str_effect_new),
    EFFECT_TYPES(0x4cc, 0x4ce, ez2_str_effect_new),
    EFFECT_TYPES(0x4d6, 0x4d7, ez2_str_effect_new),
    EFFECT_TYPES(0x4db, 0x4e7, ez2_str_effect_new),
    EFFECT_TYPES(0x4ea, 0x4ed, ez2_str_effect_new),
    EFFECT_TYPES(0x4f2, 0x4f9, ez2_str_effect_new),
    EFFECT_TYPES(0x4fd, 0x501, ez2_str_effect_new),
    EFFECT_TYPES(0x4fa, 0x4fb, ez2_str_effect_new),
    EFFECT_TYPE(0x512, ez2_str_effect_new),
    EFFECT_TYPE(0x505, ez2_str_effect_new),
    EFFECT_TYPES(0x502, 0x504, ez2_str_effect_new),
    EFFECT_TYPE(0x507, ez2_str_effect_new),
    EFFECT_TYPE(0x50f, ez2_str_effect_new),
    EFFECT_TYPES(0x508, 0x50a, ez2_str_effect_new),
    EFFECT_TYPES(0x50d, 0x50e, ez2_str_effect_new),
    EFFECT_TYPES(0x510, 0x511, ez2_str_effect_new),
    EFFECT_TYPES(0x51e, 0x52a, ez2_str_effect_new),
    EFFECT_TYPES(0x513, 0x51d, ez2_str_effect_new),
    EFFECT_TYPE(0x52b, ez2_str_effect_new),
    EFFECT_TYPE(0x544, ez2_str_effect_new),
    EFFECT_TYPE(0x543, ez2_str_effect_new),
    EFFECT_TYPE(0x546, ez2_str_effect_new),
    EFFECT_TYPE(0x545, ez2_str_effect_new),
    EFFECT_TYPE(0x548, ez2_str_effect_new),
    EFFECT_TYPE(0x547, ez2_str_effect_new),
    EFFECT_TYPE(0x54a, ez2_str_effect_new),
    EFFECT_TYPE(0x549, ez2_str_effect_new),
    EFFECT_TYPES(0x54b, 0x54c, ez2_str_effect_new),
    EFFECT_TYPES(0x568, 0x569, ez2_str_effect_new),
    EFFECT_TYPES(0x54e, 0x551, ez2_str_effect_new),
    EFFECT_TYPES(0x58e, 0x58f, ez2_str_effect_new),
    EFFECT_TYPES(0x552, 0x560, ez2_str_effect_new),
    EFFECT_TYPES(0x562, 0x567, ez2_str_effect_new),
    EFFECT_TYPES(0x56a, 0x575, ez2_str_effect_new),
    EFFECT_TYPES(0x578, 0x580, ez2_str_effect_new),
    EFFECT_TYPES(0x582, 0x589, ez2_str_effect_new),
    EFFECT_TYPE(0x58d, ez2_str_effect_new),
    EFFECT_TYPES(0x590, 0x5a7, ez2_str_effect_new),
    EFFECT_TYPES(0x5aa, 0x5b9, ez2_str_effect_new),
    EFFECT_TYPES(0x656, 0x657, ez2_str_effect_new),
    EFFECT_TYPES(0x5ba, 0x5f4, ez2_str_effect_new),
    EFFECT_TYPES(0x5f6, 0x5fa, ez2_str_effect_new),
    EFFECT_TYPES(0x5fc, 0x608, ez2_str_effect_new),
    EFFECT_TYPES(0x60a, 0x612, ez2_str_effect_new),
    EFFECT_TYPES(0x615, 0x617, ez2_str_effect_new),
    EFFECT_TYPES(0x619, 0x61c, ez2_str_effect_new),
    EFFECT_TYPES(0x61e, 0x61f, ez2_str_effect_new),
    EFFECT_TYPE(0x639, ez2_str_effect_new),
    EFFECT_TYPES(0x622, 0x626, ez2_str_effect_new),
    EFFECT_TYPES(0x628, 0x62f, ez2_str_effect_new),
    EFFECT_TYPES(0x631, 0x638, ez2_str_effect_new),
    EFFECT_TYPES(0x63b, 0x63f, ez2_str_effect_new),
    EFFECT_TYPES(0x64b, 0x650, ez2_str_effect_new),
    EFFECT_TYPES(0x654, 0x655, ez2_str_effect_new),
    EFFECT_TYPES(0x658, 0x676, ez2_str_effect_new),
    EFFECT_TYPES(0x67a, 0x67c, ez2_str_effect_new),
    EFFECT_TYPES(0x681, 0x685, ez2_str_effect_new),
    EFFECT_TYPES(0x68b, 0x68c, ez2_str_effect_new),
    EFFECT_TYPES(0x68f, 0x69d, ez2_str_effect_new),
    EFFECT_TYPES(0x6a2, 0x6ac, ez2_str_effect_new),
    EFFECT_TYPES(0x6c2, 0x6c3, ez2_str_effect_new),
    EFFECT_TYPE(0x6cd, ez2_str_effect_new),
    EFFECT_TYPES(0x8fe, 0x900, ez2_str_effect_new),
    EFFECT_TYPE(0x904, ez2_str_effect_new),
    EFFECT_TYPES(0x907, 0x921, ez2_str_effect_new),
    EFFECT_TYPES(0x923, 0x924, ez2_str_effect_new),
    EFFECT_TYPE(0x926, ez2_str_effect_new),
};
const struct EffectTypeTable ez2_str_effect_types = EFFECT_TYPE_TABLE(ez2_str_effect_type_rows);

/* Effect types created as Ez2StrEffect that effects/effect_registry.c registers after the
   EZ2STR file types (see g_effectTypes there). A type missing here is not created. */
static const struct EffectTypeRow ez2_str_effect_late_type_rows[] = {
    EFFECT_TYPES(0x943, 0x944, ez2_str_effect_new),
};
const struct EffectTypeTable ez2_str_effect_late_types = EFFECT_TYPE_TABLE(ez2_str_effect_late_type_rows);

/* The headgear effects, created as Ez2StrEffect: one type per row of the Lua table
   hatEffectTable, HAT_FIRST + 1 up to HAT_FIRST + its size, read once when the effect
   manager is created. A type that is already registered stops the registration (returns 0). */
unsigned char ez2_str_effect_add_hat_types(struct EffectMgr *mgr)
{
    int hat_count, i;

    hat_count = 0;

    client_lua_call(g_session1.luaState, "GetTableSizeForC", "s>d", "hatEffectTable", &hat_count);
    for (i = 1; i <= hat_count; i++)
        if (!effect_mgr_register_factory(mgr, HAT_FIRST + i, ez2_str_effect_new))
            return 0;
    return 1;
}

void ez2_str_effect_destruct(struct Ez2StrEffect *self)
{
    self->base.ops = &ez2_str_effect_ops;
    string_free(&self->str_name);
    effect_destruct(&self->base);
}

static void ez2_str_effect_destroy(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    ez2_str_effect_destruct(self);
    mem_free(self);
}

unsigned char ez2_str_effect_is_headgear_type(struct Ez2StrEffect *self)
{
    int type = self->base.type;
    if (TYPE_IN(HEADGEAR_TYPES, type))
        return 1;
    return type >= HAT_FIRST;
}

unsigned char ez2_str_effect_is_head_type(struct Ez2StrEffect *self)
{
    return (unsigned char)TYPE_IN(HEAD_TYPES, self->base.type);
}

float ez2str_head_x_offset(int type, const char *str_path)
{
    float x = 0.0f;
    float result;

    switch (type) {
    case 0x40b: case 0x40c:
        x = 5.0f;
        break;
    case 0x43e:
        x = 8.0f;
        break;
    case 0x4c3:
        x = 0.5f;
        break;
    case 0x4cd: case 0x4ce: case 0x543: case 0x544: case 0x545: case 0x546: case 0x5ce: case 0x5cf:
        x = 1.0f;
        break;
    case 0x4f2: case 0x4f5:
        x = -0.5f;
        break;
    case 0x547: case 0x548: case 0x549: case 0x54a: case 0x5d0: case 0x5d1:
        x = -1.0f;
        break;
    }
    if (strstr(str_path, "strangelights"))
        x = 5.0f;
    else if (strstr(str_path, "superstar"))
        x = 5.0f;
    else if (strstr(str_path, "ljosalfar"))
        x = 8.0f;

    result = x;
    if (type >= HAT_FIRST) {
        double hat_x = 0.0;
        lua_ask_hat("GetHatEfPosX", "d>l", type - HAT_FIRST, &hat_x);
        result = (float)((double)x + hat_x);
    }
    return result;
}

int ez2str_head_height(int type, void *actor)
{
    unsigned char ignore_riding = 0;
    int height = 0;
    int i;

    if (type >= HAT_FIRST)
        lua_ask_hat("IsIgnoredRidingState_Include_AttachedToHead", "d>b", type - HAT_FIRST, &ignore_riding);

    if (actor) {
        if (session_is_baby_job(&g_session1, game_actor_get_job(actor))) {
            if (ignore_riding == 1)
                height = 16;
            else if (ACTOR_RIDING(actor) != 0)
                height = 21;
            else
                height = game_actor_get_mount_height(actor) + 16;
        } else {
            if (ignore_riding == 1)
                height = 18;
            else if (ACTOR_RIDING(actor) != 0)
                height = 23;
            else
                height = game_actor_get_mount_height(actor) + 18;
        }
    }

    for (i = 0; i < (int)(sizeof(HEAD_HEIGHT_SHIFTS) / sizeof(HEAD_HEIGHT_SHIFTS[0])); i++)
        if (type >= HEAD_HEIGHT_SHIFTS[i].first && type <= HEAD_HEIGHT_SHIFTS[i].last) {
            height += HEAD_HEIGHT_SHIFTS[i].shift;
            break;
        }

    if (type >= HAT_FIRST) {
        int hat_height = 0;
        lua_ask_hat("GetHatEfPos", "d>d", type - HAT_FIRST, &hat_height);
        height += hat_height;
    }
    return height - 7;
}

unsigned char ez2_str_effect_place_at_head(struct Ez2StrEffect *self,
                                                    struct Ez2StrParticle *p)
{
    void *actor;
    float x_offset, height, scale, x_shift, y_shift;
    float view[12], from_view[12];
    float x, y, z, a, b, c;

    if (ez2_str_effect_is_headgear_type(self) != 1 && ez2_str_effect_is_head_type(self) != 1)
        return 0;
    actor = effect_find_actor(self->base.owner_aid);
    if (!actor)
        return 0;

    x_offset = ez2str_head_x_offset(self->base.type, string_cstr(&p->path));
    height = (float)ez2str_head_height(self->base.type, actor);
    scale = session_get_baby_job_scale(&g_session1, game_actor_get_job(actor));
    if (((ACTOR_STATE_BITS(actor) >> 4) & 1) && self->adjust_position_when_shrunk == 1) {
        float own = 1.0f;
        render_object_apply_shrink_scale(actor, &own);
        scale = own * scale;
    }
    x_shift = scale * x_offset;
    y_shift = scale * height;
    if (!(x_shift >= 0.0f) && !(y_shift >= 0.0f))
        return 0;

    x = p->base.pos.x;
    y = p->base.pos.y;
    z = p->base.pos.z;
    if (!game_copy_view_matrix(view))
        return 0;
    mat_invert(from_view, view);

    a = (view[3] * y + view[0] * x) + view[6] * z + view[9] + x_shift;
    b = (view[4] * y + view[1] * x) + view[7] * z + view[10] - y_shift;
    c = (view[5] * y + view[2] * x) + view[8] * z + view[11];
    p->base.pos.x = (b * from_view[3] + a * from_view[0]) + c * from_view[6] + from_view[9];
    p->base.pos.y = (b * from_view[4] + a * from_view[1]) + c * from_view[7] + from_view[10];
    p->base.pos.z = (a * from_view[2] + from_view[5] * b) + from_view[8] * c + from_view[11];
    return 1;
}

void ez2_str_effect_set_submit(struct Effect *effect, ParticleSubmitFn fn)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    self->particle->base.submit = fn;
}

void ez2_str_effect_init(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, EZ2STR_LIST_DEFAULT);
    int type;

    if (!p)
        return;
    p->actor_aid = self->base.owner_aid;

    if (ez2_str_effect_is_headgear_type(self) == 1) {
        unsigned char adjust_size = 0, adjust_position = 0;
        self->base.has_instance_key = 1;
        p->loop_forever = 1;
        lua_ask_hat("IsAdjustSizeWhenShrinkState", "d>b", self->base.type - HAT_FIRST, &adjust_size);
        p->scale_by_actor = adjust_size;
        lua_ask_hat("IsAdjustPositionWhenShrinkState", "d>b", self->base.type - HAT_FIRST, &adjust_position);
        self->adjust_position_when_shrunk = adjust_position;
    }

    type = self->base.type;
    if (TYPE_IN(LOOPING_TYPES, type)) {
        p->loop_forever = 1;
    } else if (TYPE_IN(KEYED_LOOPING_TYPES, type)) {
        self->base.has_instance_key = 1;
        p->loop_forever = 1;
    }
    if (self->base.instance_key > 0)
        p->loop_forever = 1;

    particle_quad_unit_corners(p->base.corners);
    self->particle = p;
    ez2_str_effect_open_for_type(self, p, self->base.type);
    self->camera_angle = (double)game_camera_heading();
    if (self->base.type >= HAT_FIRST)
        lua_ask_hat("IsAttachedToHead", "d>b", self->base.type - HAT_FIRST, &self->attached_to_head);
}

void ez2_str_effect_follow_camera(struct Ez2StrEffect *self)
{
    double now, spin;

    if (!self->particle)
        return;
    now = (double)game_camera_heading();
    if (now == self->camera_angle)
        return;
    spin = (self->camera_angle - now) + (double)self->particle->spin_degrees;
    if (0.0 > spin)
        spin += 360.0;
    else if (spin > 360.0)
        spin -= 360.0;
    self->particle->spin_degrees = (float)spin;
    self->camera_angle = now;
}

void ez2_str_effect_step_particle(struct Ez2StrEffect *self, struct Ez2StrParticle *p)
{
    Vec3 offset;

    if (!p)
        return;
    offset.x = 0.0f;
    offset.z = 0.0f;
    if (!ez2_str_effect_place_at_head(self, p) && self->base.type != 0x584)
        offset.y = -80.0f;
    else
        offset.y = 0.0f;
    ez2_str_particle_process(p, offset);
}

void ez2_str_effect_process(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    Vec3 pos;
    void *actor;

    if (self->turn_with_camera == 1 && self->particle)
        ez2_str_effect_follow_camera(self);

    pos = self->base.pos;
    self->particle->base.pos = pos;
    ez2_str_effect_step_particle(self, self->particle);
    self->particle->base.age++;

    actor = effect_find_actor(self->base.owner_aid);
    if (!actor)
        return;
    if (self->attached_to_head == 1 && game_actor_get_transform_job(actor) <= 0) {
        float scale;
        struct ActorHeadExtent extent;

        scale = session_get_baby_job_scale(&g_session1, game_actor_get_job(actor));
        if (self->adjust_position_when_shrunk == 1)
            render_object_apply_shrink_scale(actor, &scale);
        extent = actor_get_head_extent(actor);
        self->particle->screen_dx = extent.width * scale;
        self->particle->screen_dy = extent.height * scale;
    }
    self->particle->lift = ACTOR_LIFT(actor);
}

void ez2_str_effect_draw_particle(struct Ez2StrEffect *self, struct Ez2StrParticle *p)
{
    int type;

    if (!p)
        return;
    type = self->base.type;
    if (type == 0x907 || type == 0x90a || type == 0x90d) {
        self->screen_shade += 0x2000000;
        if (self->screen_shade >= 0xaa000000u)
            self->screen_shade = 0xaa000000u;
        screen_fill_rect(0, 0, RENDERER_WIDTH(g_renderer), RENDERER_HEIGHT(g_renderer), self->screen_shade);
    }
    ez2_str_particle_render_current(p);
}

void ez2_str_effect_render(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    unsigned char *actor = effect_find_actor(self->base.owner_aid);

    if (actor) {
        if (ez2_str_effect_is_headgear_type(self)) {
            if (is_effect_state_star2(actor_get_effect_state(actor)))
                return;
        }
        if (ez2_str_effect_is_headgear_type(self) && !ACTOR_REVEALED(actor))
            return;
        if (ez2_str_effect_is_headgear_type(self) && !ACTOR_VISIBLE(actor))
            return;
    }
    if (!self->particle)
        return;
    ez2_str_effect_draw_particle(self, self->particle);
}

void ez2_str_effect_release(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    particle_free(&self->particle->base);
}

unsigned char ez2_str_effect_is_alive(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    float limit;

    if (ez2_str_effect_is_headgear_type(self) == 1 || TYPE_IN(OWNER_BOUND_TYPES, self->base.type))
        return effect_is_owner_alive(&self->base);
    limit = PARTICLE_LIFETIME(self->particle);
    if (limit == 0.0f)
        return 1;
    return limit * 60.0f > (float)self->particle->base.age;
}

static void hat_render_order(struct Ez2StrEffect *self)
{
    unsigned char before = 0;

    if (self->base.type < HAT_FIRST)
        return;
    lua_ask_hat("IsRenderBeforeCharacter", "d>b", self->base.type - HAT_FIRST, &before);
    if (before)
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
}

#define WAVE_BEGIN_SPELL "effect\\EF_BeginSpell.wav"

void ez2_str_effect_after_init(struct Effect *effect)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    int type = self->base.type;

    switch (type) {
    case 0xcc: case 0xcd: case 0xce: case 0xcf: case 0xd2:
        cue(self, "_heal_effect.wav", 0);
        break;
    case 0xd0: case 0xd1: case 0xd3:
        cue(self, "effect\\흡기.wav", 0);
        break;
    case 0x426:
        cue(self, "effect\\RL_HAMMER_OF_GOD.wav", 0);
        break;
    case 0x44d: cue(self, "effect\\su_stemspear.wav", 0); break;
    case 0x44e: cue(self, "effect\\su_tunabelly.wav", 0); break;
    case 0x44f: cue(self, "effect\\su_scratch.wav", 0); break;
    case 0x453: cue(self, "effect\\su_scaroftarou.wav", 0); break;
    case 0x455: cue(self, "effect\\su_pickypeck.wav", 0); break;
    case 0x45c: cue(self, "effect\\su_brunchofshrimp.wav", 0); break;
    case 0x463: cue(self, "effect\\su_grooming.wav", 0); break;
    case 0x464: cue(self, "effect\\su_chattering.wav", 0); break;
    case 0x466: cue(self, "effect\\su_hiss.wav", 0); break;
    case 0x467: cue(self, "effect\\su_powerofflock.wav", 0); break;
    case 0x469: cue(self, "effect\\su_svg_spirit.wav", 0); break;
    case 0x489:
        if (rand() % 12 == 0)
            effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\wizard_quagmire.wav", 0, -100.0f);
        break;
    case 0x48a:
        cue(self, "effect\\wizard_meteo.wav", 0);
        cue(self, "effect\\hunter_blastmine.wav", 0);
        cue(self, "effect\\hunter_blastmine.wav", 10);
        cue(self, "effect\\wizard_meteo.wav", 20);
        cue(self, "effect\\wizard_meteo.wav", 50);
        cue(self, "effect\\wizard_meteo.wav", 80);
        cue(self, "effect\\wizard_meteo.wav", 100);
        cue(self, "effect\\wizard_meteo.wav", 130);
        cue(self, "effect\\wizard_meteo.wav", 140);
        cue(self, "effect\\wizard_meteo.wav", 180);
        cue(self, "effect\\wizard_meteo.wav", 200);
        cue_message(self, 50);
        cue_message(self, 100);
        cue_message(self, 150);
        cue_message(self, 200);
        cue_message(self, 250);
        break;
    case 0x48b:
        cue(self, "effect\\wizard_meteor.wav", 0);
        cue_message(self, 40);
        break;
    case 0x554: case 0x5d7:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "ice_titan_attack.wav", 5);
        break;
    case 0x573:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\EF_Bash.wav", 0);
        cue(self, "effect\\rog_intimidate.wav", 35);
        break;
    case 0x580: case 0x5ef:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\EF_hit2.wav", 10);
        cue(self, "effect\\EF_hit4.wav", 20);
        break;
    case 0x583:
        cue(self, "effect\\EF_hit2.wav", 0);
        break;
    case 0x585: case 0x5fe:
        cue(self, "effect\\g크레이지위드.wav", 0);
        break;
    case 0x590:
        cue(self, "effect\\mon_맹룡과강.wav", 5);
        cue(self, "effect\\mon_맹룡과강.wav", 5);
        cue(self, "effect\\mon_맹룡과강.wav", 5);
        break;
    case 0x60a: case 0x60b: case 0x64b: case 0x64c: case 0x64d: case 0x64e: case 0x64f: case 0x650:
        set_list(self, EZ2STR_LIST_DEFAULT);
        break;
    case 0x60d:
        cue(self, "effect\\EF_Bash.wav", 0);
        break;
    case 0x60e:
        cue(self, "hell_in.wav", 0);
        break;
    case 0x615:
        cue(self, "effect\\샤프슈팅.wav", 0);
        cue(self, "effect\\흡기.wav", 0);
        break;
    case 0x619:
        cue(self, "effect\\프레셔.wav", 3);
        cue(self, "effect\\S바레티르스피어.wav", 22);
        cue_message(self, 22);
        break;
    case 0x61e:
        cue(self, "effect\\EF_StoneCurse.wav", 0);
        break;
    case 0x638:
        cue(self, "effect\\매직 크래쉬.wav", 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;
    case 0x907: case 0x908: case 0x90a: case 0x90b: case 0x90d: case 0x90e:
        set_list(self, EZ2STR_LIST_SCREEN_FILL);
        break;
    case 0x666:
        cue(self, "chainlight.wav", 0);
        break;
    case 0x66c:
        cue(self, "effect\\priest_lexaeterna.wav", 0);
        break;
    case 0x676:
        cue(self, "effect\\EF_GlassWall.wav", 30);
        break;
    case 0x69a:
        cue(self, "effect\\wl_siennaexecrate.wav", 0);
        break;
    case 0x6a6:
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        cue(self, "effect\\wizard_fire_pillar_b.wav", 15);
        cue_message(self, 20);
        break;
    case 0x6aa:
        cue(self, "effect\\wizard_earthspike.wav", 20);
        cue_message(self, 20);
        break;
    case 0x909: case 0x90c: case 0x90f:
        set_list(self, EZ2STR_LIST_SCREEN_FILL);
        cue_message(self, 0);
        break;
    case 0x943: cue(self, "effect\\drop_effect_1.wav", 0); break;
    case 0x944: cue(self, "effect\\drop_effect_2.wav", 0); break;

    case 0x625: case 0x62a: case 0x62d: case 0x633: case 0x654: case 0x65d: case 0x663: case 0x669:
    case 0x66a: case 0x681: case 0x683: case 0x685:
        cue(self, WAVE_BEGIN_SPELL, 0);
        break;
    case 0x58f: case 0x592: case 0x5dc: case 0x610: case 0x671:
        cue(self, WAVE_BEGIN_SPELL, 0);
        set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        break;

    default:
        if (TYPE_IN(BEFORE_ACTORS_TYPES, type))
            set_list(self, EZ2STR_LIST_BEFORE_ACTORS);
        else
            hat_render_order(self);
        break;
    }
}

void ez2_str_effect_skip(struct Effect *effect, float fraction)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        ez2_str_particle_skip(self->particle, fraction);
}

void ez2_str_effect_set_stop_frame(struct Effect *effect, float frame)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->stop_frame = (int)frame;
}

void ez2_str_effect_set_start_delay(struct Effect *effect, int ms)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->start_delay = ms;
}

void ez2_str_effect_set_spin(struct Effect *effect, float degrees)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->spin_degrees = degrees;
}

void ez2_str_effect_set_str_name(struct Effect *effect, const char *name)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    string_assign_cstr(&self->str_name, name);
    particle_free(&self->particle->base);
    ez2_str_effect_init(&self->base);
}

void ez2_str_effect_set_list_flags(struct Effect *effect, int list_flags)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    self->particle->base.list_flags = list_flags;
}

void ez2_str_effect_set_zoom(struct Effect *effect, float zoom)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->zoom = zoom;
}

void ez2_str_effect_set_alpha_scale(struct Effect *effect, float scale)
{
    struct Ez2StrEffect *self = (struct Ez2StrEffect *)effect;
    if (self->particle)
        self->particle->alpha_scale = scale;
}
