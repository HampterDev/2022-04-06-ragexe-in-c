#include "effects/effect_allocator.h"
#include "actor/actor_ops.h"
#include "render/math3d.h"
#include <stdio.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "session/session_skills.h"

struct Effect;
struct Level150Effect;
struct String;
void level150_effect_init(struct Effect *self);
void level150_effect_process(struct Effect *self);
void level150_effect_render(struct Effect *self);
void level150_effect_release(struct Effect *self);
void level150_effect_set_color(struct Effect *self, int r, int g, int b);
static void level150_effect_destroy(struct Effect *effect);

static const struct EffectOps level150_effect_ops = {
    .set_submit = effect_set_submit,
    .init = level150_effect_init,
    .update_range = effect_update_range,
    .process = level150_effect_process,
    .render = level150_effect_render,
    .release = level150_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = effect_restart,
    .set_color = level150_effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = level150_effect_destroy,
    .fade_out = effect_fade_out,
};

enum {
    LEVEL150_BLUE = 978,
    LEVEL150_YELLOW = 1022,
    LEVEL150_RED = 1226,
    LEVEL150_ORANGE = 2275,
};

#define BUBBLE_SEGMENTS 22
#define BUBBLE_FRAMES 27
#define SHOCK_CIR0002 3

struct Level150Effect {
    struct Effect base;
    Vec3 last_bottom;
    Vec3 last_top;
    struct Vec bubbles;
    struct Vec shocks;
};

struct BubbleVars {
    int tilt;
    int sweep;
    float segment_height;
    float radius;
};
PARTICLE_EFFECT_DATA_FITS(BubbleVars);
#define BUBBLE_VARS(p) ((struct BubbleVars *)(p)->effect_data)

struct ShockVars {
    int pulse;
    float lift;
    float size;
};
PARTICLE_EFFECT_DATA_FITS(ShockVars);
#define SHOCK_VARS(p) ((struct ShockVars *)(p)->effect_data)

static void set_particle_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

struct Level150Effect *level150_effect_construct(struct Level150Effect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &level150_effect_ops;
    self->last_bottom.x = self->last_bottom.y = self->last_bottom.z = 0;
    self->last_top.x = self->last_top.y = self->last_top.z = 0;
    self->bubbles.first = self->bubbles.last = self->bubbles.end = 0;
    self->shocks.first = self->shocks.last = self->shocks.end = 0;
    return self;
}

struct Effect *level150_effect_new(void)
{
    struct Level150Effect *self = mem_try_alloc(sizeof *self);

    return self ? &level150_effect_construct(self)->base : NULL;
}

void level150_effect_destruct(struct Level150Effect *self)
{
    self->base.ops = &level150_effect_ops;
    vec_free(&self->shocks);
    vec_free(&self->bubbles);
    effect_destruct(&self->base);
}

static void level150_effect_destroy(struct Effect *effect)
{
    struct Level150Effect *self = (struct Level150Effect *)effect;
    level150_effect_destruct(self);
    mem_free(self);
}

void level150_effect_set_color(struct Effect *effect, int r, int g, int b)
{
    struct Level150Effect *self = (struct Level150Effect *)effect;
    struct Particle **it;
    for (it = self->bubbles.first; it != (struct Particle **)self->bubbles.last; it++)
        particle_set_color(*it, (short)r, (short)g, (short)b);
    for (it = self->shocks.first; it != (struct Particle **)self->shocks.last; it++)
        particle_set_color(*it, (short)r, (short)g, (short)b);
}

void level150_effect_release(struct Effect *effect)
{
    struct Level150Effect *self = (struct Level150Effect *)effect;
    struct Particle **it;
    for (it = self->bubbles.first; it != (struct Particle **)self->bubbles.last; it++)
        particle_free(*it);
    self->bubbles.last = self->bubbles.first;
    for (it = self->shocks.first; it != (struct Particle **)self->shocks.last; it++)
        particle_free(*it);
    self->shocks.last = self->shocks.first;
}

void level150_effect_shape_bubble(struct Level150Effect *self, struct Particle *p)
{
    struct BubbleVars *v;
    float base_radius, scale, radius, ct, st, c, s, lean;
    int index, angle;

    if (!p)
        return;
    v = BUBBLE_VARS(p);
    base_radius = v->radius;
    index = p->index;
    scale = p->scale;
    radius = g_sinTable[index * 9] * base_radius * 0.15f + base_radius;
    angle = (int)((float)((int)((float)v->sweep / 20.0f) * index) + p->roll) % 360;
    ct = g_cosTable[v->tilt];
    st = g_sinTable[v->tilt];
    c = g_cosTable[angle];
    s = g_sinTable[angle];
    lean = ct * radius;

    p->corners[1].x = lean * c + scale * c;
    p->corners[1].y = 5.0f - st * radius;
    p->corners[1].z = lean * s + scale * s;
    p->corners[3].x = scale * c;
    p->corners[3].y = 5.0f;
    p->corners[3].z = scale * s;
    if (index != 0) {
        p->corners[0].x = self->last_top.x;
        p->corners[0].y = self->last_top.y;
        p->corners[0].z = self->last_top.z;
        p->corners[2].x = self->last_bottom.x;
        p->corners[2].y = self->last_bottom.y;
        p->corners[2].z = self->last_bottom.z;
    }
    self->last_bottom.x = scale * c;
    self->last_bottom.y = 5.0f;
    self->last_bottom.z = scale * s;
    self->last_top.x = p->corners[1].x;
    self->last_top.y = p->corners[1].y;
    self->last_top.z = p->corners[1].z;
}

static void tint_bubble(struct Particle *p, int type)
{
    switch (type) {
    case LEVEL150_BLUE:   particle_set_color(p, 0x64, 0x64, 0xff); break;
    case LEVEL150_YELLOW: particle_set_color(p, 0xff, 0xff, 0x64); break;
    case LEVEL150_RED:    particle_set_color(p, 0xff, 0x9b, 0x9b); break;
    case LEVEL150_ORANGE: particle_set_color(p, 0xff, 0x9b, 0x00); break;
    }
}

void level150_effect_add_bubble_band(struct Level150Effect *self, int alpha_bonus)
{
    int type = self->base.type;
    int ring = 0, start = 180, alpha = alpha_bonus + 135;
    char name[0x1000];

    for (; start < 540; start += 90, ring++, alpha -= 45) {
        int i;
        for (i = 0; i < BUBBLE_SEGMENTS; i++) {
            struct Particle *p = particle_new(self->base.pos, 13);
            struct BubbleVars *v;
            if (!p)
                continue;
            v = BUBBLE_VARS(p);
            sprintf(name, "effect\\W_bubble%02d.tga", i + 1);
            set_particle_texture(p, name);
            p->effect_type = self->base.type;
            p->group = ring;
            p->index = i;
            p->owner_tag = 4;
            p->alpha = alpha;
            tint_bubble(p, type);
            p->roll = (float)(start % 360);
            v->sweep = 360;
            v->radius = 30.0f - (float)ring;
            p->scale = 3.5f;
            v->tilt = 80;
            v->segment_height = 0;
            p->age = 0;
            level150_effect_shape_bubble(self, p);
            vec_push(&self->bubbles, &p, sizeof p);
        }
    }
}

void level150_effect_add_shock_rings(struct Level150Effect *self, int kind)
{
    int type = self->base.type;
    int i, pulse;

    for (i = 0, pulse = 0; pulse < 360; i++, pulse += 180) {
        struct Particle *p = particle_new(self->base.pos, 13);
        struct ShockVars *v;
        if (!p)
            continue;
        v = SHOCK_VARS(p);
        p->effect_type = self->base.type;
        p->index = i;
        p->group = 0;
        p->owner_tag = 1;
        v->pulse = pulse;
        p->alpha = 200;
        v->lift = -1.0f;
        p->age = 0;
        if (kind == SHOCK_CIR0002) {
            set_particle_texture(p, "effect\\cir0002.tga");
            v->size = 18.0f;
        } else {
            set_particle_texture(p, "effect\\emp shock.tga");
            v->size = 11.0f;
        }
        vec_push(&self->shocks, &p, sizeof p);
        switch (type) {
        case LEVEL150_BLUE:
            if (kind == SHOCK_CIR0002)
                particle_set_color(p, 0x23, 0x23, 0xff);
            else
                particle_set_color(p, 0x69, 0x69, 0xff);
            break;
        case LEVEL150_YELLOW:
            if (kind == SHOCK_CIR0002)
                particle_set_color(p, 0xff, 0xff, 0x23);
            else
                particle_set_color(p, 0xff, 0xff, 0x69);
            break;
        case LEVEL150_RED:
            if (kind == SHOCK_CIR0002)
                particle_set_color(p, 0xff, 0x37, 0x37);
            else
                particle_set_color(p, 0xff, 0x9b, 0x9b);
            break;
        case LEVEL150_ORANGE:
            particle_set_color(p, 0xff, 0x9b, 0x00);
            break;
        }
    }
}

void level150_effect_init(struct Effect *effect)
{
    struct Level150Effect *self = (struct Level150Effect *)effect;
    level150_effect_add_bubble_band(self, 45);
    level150_effect_add_bubble_band(self, 25);
    level150_effect_add_shock_rings(self, 2);
    level150_effect_add_shock_rings(self, SHOCK_CIR0002);
}

void level150_effect_step_bubble(struct Particle *p)
{
    struct BubbleVars *v;
    int tilt, alpha;
    float scale, base_radius, radius;

    if (!p)
        return;
    v = BUBBLE_VARS(p);
    tilt = v->tilt - 1;
    scale = p->scale + 0.07f;
    alpha = tilt >= 10 ? p->alpha : 0;
    if (tilt < 10)
        tilt = 10;
    if (!(scale >= 3.5f)) {
        alpha += 10;
    } else {
        int faded = alpha - 3;
        if (faded <= 0) {
            scale = 2.87f;
            tilt = 74;
        }
        alpha = faded > 0 ? faded : 0;
    }
    p->alpha = alpha;
    p->scale = scale;
    v->tilt = tilt;
    base_radius = v->radius;
    radius = g_sinTable[p->index * 9] * base_radius * 0.15f + base_radius;
    mat_set_scale(p->matrix, scale * 0.5f, g_sinTable[tilt] * base_radius / radius, scale * 0.5f);
}

void level150_effect_step_shock(struct Particle *p)
{
    struct ShockVars *v;
    int pulse, index;
    float size, scale, camera_angle;
    float turn[12], grow[12];

    if (!p)
        return;
    v = SHOCK_VARS(p);
    pulse = v->pulse;
    pulse += pulse + 3 < 360 ? 3 : -357;
    index = p->index;
    v->pulse = pulse;
    size = v->size;
    scale = size * 0.1f * g_sinTable[pulse] + 1.0f + size * 0.8f;
    camera_angle = game_camera_heading();
    mat_set_rotate_x(turn, 1.570796f);
    mat_rotate_y(turn, degrees_to_radians((float)(index * 23)) - 2.7925262f
                           - degrees_to_radians((float)(int)camera_angle));
    mat_set_scale(grow, scale, scale, 1.0f);
    mat_multiply(p->matrix, grow, turn);
}

void level150_effect_process(struct Effect *effect)
{
    struct Level150Effect *self = (struct Level150Effect *)effect;
    struct Particle **it;
    for (it = self->bubbles.first; it != (struct Particle **)self->bubbles.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        level150_effect_step_bubble(p);
        p->age++;
    }
    for (it = self->shocks.first; it != (struct Particle **)self->shocks.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        level150_effect_step_shock(p);
        p->age++;
    }
}

void level150_effect_draw_bubble(struct Particle *p)
{
    struct ParticleRenderArgs args;
    char name[0x1000];
    int index;

    if (!p)
        return;
    index = p->index;
    if (index <= 0)
        return;
    args.facing_fixed = 0;
    args.v_top = 0;
    args.v_bottom = 1.0f;
    args.u_left = (float)index / 21.0f;
    args.u_right = (float)(index + 1) / 21.0f;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    sprintf(name, "effect\\W_bubble%02d.tga", p->age % BUBBLE_FRAMES + 1);
    set_particle_texture(p, name);
    particle_render_with(p, &args);
}

void level150_effect_draw_shock(struct Particle *p)
{
    particle_render(p, 0, 0, 1.0f, 0, 1.0f, 1);
}

void level150_effect_render(struct Effect *effect)
{
    struct Level150Effect *self = (struct Level150Effect *)effect;
    struct RenderObject *owner = effect_find_actor(self->base.owner_aid);
    struct Particle **it;

    if (owner && self->base.type == LEVEL150_ORANGE) {
        if (is_effect_state_cloaking(actor_get_effect_state(owner)))
            return;
        if (!owner->revealed || !owner->visible)
            return;
    }
    for (it = self->bubbles.first; it != (struct Particle **)self->bubbles.last; it++)
        level150_effect_draw_bubble(*it);
    for (it = self->shocks.first; it != (struct Particle **)self->shocks.last; it++)
        level150_effect_draw_shock(*it);
}
