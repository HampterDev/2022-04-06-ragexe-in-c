#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_DIR "이팩트\\"
#define EFFECT_CN_POWDERING 0x450

struct PowderingEffect {
    struct Effect base;
    struct Vec smoke;
    struct Vec sparks;
    int side;
};

enum FadeState { FADE_IN = 0, FADE_OUT = 1, FADE_DONE = 2 };
struct PowderingParticle {
    int peak_alpha;
    int fade_frames;
    float alpha_step;
    unsigned char fade_state;
    float scale_step;
};
PARTICLE_EFFECT_DATA_FITS(PowderingParticle);
static __inline struct PowderingParticle *powdering_particle(const void *p)
{
    return (struct PowderingParticle *)((const struct Particle *)p)->effect_data;
}
#define SMOKE_OFFSET(p)  (&(p)->offset)
#define SPRITE_OFFSET(sp) (&(sp)->sprite_offset)

struct String;
void powdering_effect_init(struct Effect *self);
void powdering_effect_process(struct Effect *self);
void powdering_effect_render(struct Effect *self);
void powdering_effect_release(struct Effect *self);
unsigned char powdering_effect_is_alive(struct Effect *self);
void powdering_effect_after_init(struct Effect *self);
static void powdering_effect_destroy(struct Effect *effect);

static const struct EffectOps powdering_effect_ops = {
    .set_submit = effect_set_submit,
    .init = powdering_effect_init,
    .update_range = effect_update_range,
    .process = powdering_effect_process,
    .render = powdering_effect_render,
    .release = powdering_effect_release,
    .is_alive = powdering_effect_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = powdering_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = powdering_effect_destroy,
    .fade_out = effect_fade_out,
};

static void particle_tag(void *p, int effect_type, int index)
{
    struct Particle *head = p;
    head->effect_type = effect_type;
    head->index = index;
    head->group = 0;
    head->owner_tag = 1;
}

static int random_below(int range)
{
    int r;
    if (range == 0)
        return 0;
    r = rand() % range;
    return r < 0 ? 0 : r;
}

static int random_spot(const struct PowderingEffect *self)
{
    int r = random_below(self->side * 5);
    return r - self->side * 5 / 2 + 1;
}

struct PowderingEffect *powdering_effect_construct(struct PowderingEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &powdering_effect_ops;
    self->smoke.first = self->smoke.last = self->smoke.end = 0;
    self->sparks.first = self->sparks.last = self->sparks.end = 0;
    self->side = 0;
    return self;
}

struct Effect *powdering_effect_new(void)
{
    struct PowderingEffect *self = mem_try_alloc(sizeof *self);

    return self ? &powdering_effect_construct(self)->base : NULL;
}

void powdering_effect_destruct(struct PowderingEffect *self)
{
    self->base.ops = &powdering_effect_ops;
    vec_free(&self->sparks);
    vec_free(&self->smoke);
    effect_destruct(&self->base);
}

static void powdering_effect_destroy(struct Effect *effect)
{
    struct PowderingEffect *self = (struct PowderingEffect *)effect;
    powdering_effect_destruct(self);
    mem_free(self);
}

void powdering_effect_release(struct Effect *effect)
{
    struct PowderingEffect *self = (struct PowderingEffect *)effect;
    struct Particle **it;
    for (it = self->smoke.first; it != (struct Particle **)self->smoke.last; it++)
        particle_free(*it);
    self->smoke.last = self->smoke.first;
    for (it = self->sparks.first; it != (struct Particle **)self->sparks.last; it++)
        particle_free(*it);
    self->sparks.last = self->sparks.first;
}

void powdering_effect_reset_spark(struct PowderingEffect *self,
                                             struct SpriteParticle *sp)
{
    int x = random_spot(self);
    int height = random_below(7) + 7;
    int z = random_spot(self);
    int size;

    SPRITE_OFFSET(sp)->y = -(float)height;
    SPRITE_OFFSET(sp)->x = (float)x;
    SPRITE_OFFSET(sp)->z = (float)z;
    size = random_below(3) + 1;
    sp->base.alpha = 0;
    sp->base.scale = 0.5f - (float)size * 0.1f;
    powdering_particle(sp)->fade_state = FADE_IN;
}

void powdering_effect_reset_smoke(struct Particle *p)
{
    int grow = random_below(50);
    int fade, peak, blue, red;

    p->scale = 0;
    fade = random_below(21) + 30;
    p->alpha = 0;
    powdering_particle(p)->fade_frames = fade;
    powdering_particle(p)->scale_step = ((float)(grow + 1) * 0.1f + 6.0f) / (float)(fade * 2);
    peak = random_below(100) + 50;
    powdering_particle(p)->peak_alpha = peak;
    powdering_particle(p)->alpha_step = (float)(peak / fade);
    blue = random_below(100);
    red = random_below(50);
    particle_set_color(p, (short)(115 - red), 255, (short)(150 - blue));
    powdering_particle(p)->fade_state = FADE_IN;
}

void powdering_effect_spawn_smoke(struct PowderingEffect *self)
{
    int i;
    for (i = 0; i < self->side * self->side; i++) {
        struct Particle *p = particle_new(self->base.pos, 5);
        struct String texture;
        int x, z;

        if (!p)
            continue;
        particle_tag(p, self->base.type, i);
        string_init(&texture);
        string_assign_cstr(&texture, "smoke2.bmp");
        particle_set_texture(p, texture);
        x = random_spot(self);
        z = random_spot(self);
        SMOKE_OFFSET(p)->y = 0;
        SMOKE_OFFSET(p)->x = (float)x;
        SMOKE_OFFSET(p)->z = (float)z;
        powdering_effect_reset_smoke(p);
        particle_quad_unit_corners(p->corners);
        vec_push(&self->smoke, &p, sizeof p);
    }
}

void powdering_effect_spawn_sparks(struct PowderingEffect *self)
{
    int i;
    for (i = 0; i < self->side * self->side; i++) {
        struct SpriteParticle *sp = sprite_particle_new(self->base.pos, 1);

        if (!sp)
            continue;
        particle_tag(sp, self->base.type, i);
        sprite_particle_set_spr(sp, EFFECT_DIR "particle2.spr");
        sprite_particle_set_act(sp, EFFECT_DIR "particle2.act");
        powdering_effect_reset_spark(self, sp);
        particle_quad_unit_corners(sp->base.corners);
        vec_push(&self->sparks, &sp, sizeof sp);
    }
}

void powdering_effect_init(struct Effect *effect)
{
    struct PowderingEffect *self = (struct PowderingEffect *)effect;
    switch (self->base.level) {
    case 1: case 2: self->side = 3; break;
    case 3: case 4: self->side = 5; break;
    case 5: self->side = 7; break;
    }
    powdering_effect_spawn_smoke(self);
    powdering_effect_spawn_sparks(self);
}

void powdering_effect_after_init(struct Effect *effect)
{
    if (effect->type == EFFECT_CN_POWDERING)
        effect_add_sound_cue(effect, SOUND_CUE_WAVE, "effect\\su_cn_powdering.wav", 0, 0.0f);
}

void powdering_effect_step_smoke(struct PowderingEffect *self, struct Particle *p)
{
    unsigned char state;
    float scale, angle;
    float scaling[12], turn[12], m[12];
    int alpha, step;
    Vec3 here;

    if (!p || powdering_particle(p)->fade_state == FADE_DONE)
        return;
    state = powdering_particle(p)->fade_state;
    scale = powdering_particle(p)->scale_step + p->scale;
    alpha = p->alpha;
    step = powdering_particle(p)->peak_alpha / powdering_particle(p)->fade_frames;
    if (state == FADE_IN) {
        alpha += step;
        if (!(powdering_particle(p)->peak_alpha > alpha)) {
            alpha = powdering_particle(p)->peak_alpha;
            powdering_particle(p)->fade_state = FADE_OUT;
        }
    } else if (state == FADE_OUT) {
        alpha -= step;
        if (alpha < 0) {
            if (effect_is_owner_alive(&self->base)) {
                powdering_particle(p)->fade_state = FADE_IN;
                powdering_effect_reset_smoke(p);
                return;
            }
            alpha = 0;
            powdering_particle(p)->fade_state = FADE_DONE;
        }
    }
    p->alpha = alpha;
    p->scale = scale;
    mat_set_scale(scaling, scale, scale, 1.0f);
    angle = game_camera_pitch();
    mat_set_rotate_x(turn, angle / g_halfTurnDegrees * g_pi);
    mat_multiply(m, scaling, turn);
    memcpy(p->matrix, m, sizeof(m));
    here = self->base.pos;
    p->pos.x = here.x + 0.0f + SMOKE_OFFSET(p)->x;
    p->pos.y = here.y - 10.0f + SMOKE_OFFSET(p)->y;
    p->pos.z = here.z + 0.0f + SMOKE_OFFSET(p)->z;
    p->age++;
}

void powdering_effect_step_spark(struct PowderingEffect *self, struct SpriteParticle *sp)
{
    unsigned char state;
    int alpha, drift;
    Vec3 off, here;

    if (!sp || powdering_particle(sp)->fade_state == FADE_DONE)
        return;
    state = powdering_particle(sp)->fade_state;
    alpha = sp->base.alpha;
    if (state == FADE_IN) {
        alpha += random_below(30);
        if (alpha >= 255) {
            alpha = 254;
            powdering_particle(sp)->fade_state = FADE_OUT;
        }
    } else if (state == FADE_OUT) {
        alpha -= random_below(30);
        if (alpha < 0) {
            if (effect_is_owner_alive(&self->base)) {
                powdering_particle(sp)->fade_state = FADE_IN;
                powdering_effect_reset_spark(self, sp);
                return;
            }
            alpha = 0;
            powdering_particle(sp)->fade_state = FADE_DONE;
        }
    }
    sp->base.alpha = alpha;
    off = *SPRITE_OFFSET(sp);
    drift = random_below(10);
    SPRITE_OFFSET(sp)->x = off.x;
    SPRITE_OFFSET(sp)->z = off.z;
    SPRITE_OFFSET(sp)->y = (float)drift * 0.01f + off.y;
    here = self->base.pos;
    sp->base.pos.y = SPRITE_OFFSET(sp)->y + here.y;
    sp->base.pos.x = here.x + off.x;
    sp->base.pos.z = here.z + off.z;
    sp->base.age++;
}

void powdering_effect_process(struct Effect *effect)
{
    struct PowderingEffect *self = (struct PowderingEffect *)effect;
    struct Particle **p;
    struct SpriteParticle **sp;
    for (p = self->smoke.first; p != (struct Particle **)self->smoke.last; p++)
        powdering_effect_step_smoke(self, *p);
    for (sp = self->sparks.first; sp != (struct SpriteParticle **)self->sparks.last; sp++)
        powdering_effect_step_spark(self, *sp);
}

unsigned char powdering_effect_is_alive(struct Effect *effect)
{
    struct PowderingEffect *self = (struct PowderingEffect *)effect;
    struct Particle **it;
    for (it = self->smoke.first; it != (struct Particle **)self->smoke.last; it++)
        if (powdering_particle(*it)->fade_state != FADE_DONE)
            return 1;
    for (it = self->sparks.first; it != (struct Particle **)self->sparks.last; it++)
        if (powdering_particle(*it)->fade_state != FADE_DONE)
            return 1;
    return 0;
}

void powdering_effect_render_smoke(struct Particle *p)
{
    if (p && powdering_particle(p)->fade_state != FADE_DONE)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void powdering_effect_render_spark(struct SpriteParticle *sp)
{
    if (sp && powdering_particle(sp)->fade_state != FADE_DONE)
        sprite_particle_render(sp);
}

void powdering_effect_render(struct Effect *effect)
{
    struct PowderingEffect *self = (struct PowderingEffect *)effect;
    struct Particle **p;
    struct SpriteParticle **sp;
    for (p = self->smoke.first; p != (struct Particle **)self->smoke.last; p++)
        powdering_effect_render_smoke(*p);
    for (sp = self->sparks.first; sp != (struct SpriteParticle **)self->sparks.last; sp++)
        powdering_effect_render_spark(*sp);
}
