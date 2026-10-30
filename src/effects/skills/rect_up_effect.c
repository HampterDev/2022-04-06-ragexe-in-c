#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_RECT_UP 0x440

struct RectUpEffect {
    struct Effect base;
    Vec3 last_bottom;
    Vec3 last_top;
    int fade_after;
    struct Vec quads;
};

#define P_INDEX(p) ((p)->index)

struct RectUpParticle {
    int tint_base;
    int tint_phase;
    int height_phase;
    float height;
    float max_height;
};
PARTICLE_EFFECT_DATA_FITS(RectUpParticle);
static __inline struct RectUpParticle *rect_up_particle(const struct Particle *p)
{
    return (struct RectUpParticle *)p->effect_data;
}
#define P_OFFSET(p)       (&(p)->offset)

struct String;
void rect_up_effect_init(struct Effect *self);
void rect_up_effect_process(struct Effect *self);
void rect_up_effect_render(struct Effect *self);
void rect_up_effect_release(struct Effect *self);
void rect_up_effect_after_init(struct Effect *self);
static void rect_up_effect_destroy(struct Effect *effect);
static void rect_up_effect_tint(struct RectUpEffect *self, struct Particle *p);
static void rect_up_effect_set_texture(struct RectUpEffect *self, struct Particle *p);

static const struct EffectOps rect_up_effect_ops = {
    .set_submit = effect_set_submit,
    .init = rect_up_effect_init,
    .update_range = effect_update_range,
    .process = rect_up_effect_process,
    .render = rect_up_effect_render,
    .release = rect_up_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = rect_up_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = rect_up_effect_destroy,
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
    int r = rand() % range;
    return r < 0 ? 0 : r;
}

static int next_degree(int deg)
{
    return deg + (deg + 1 < 360 ? 1 : -359);
}

struct RectUpEffect *rect_up_effect_construct(struct RectUpEffect *self)
{
    effect_construct(&self->base, EFFECT_RECT_UP);
    self->base.ops = &rect_up_effect_ops;
    self->last_bottom.x = self->last_bottom.y = self->last_bottom.z = 0;
    self->last_top.x = self->last_top.y = self->last_top.z = 0;
    self->quads.first = self->quads.last = self->quads.end = 0;
    return self;
}

struct Effect *rect_up_effect_new(void)
{
    struct RectUpEffect *self = mem_try_alloc(sizeof *self);

    return self ? &rect_up_effect_construct(self)->base : NULL;
}

void rect_up_effect_destruct(struct RectUpEffect *self)
{
    self->base.ops = &rect_up_effect_ops;
    vec_free(&self->quads);
    effect_destruct(&self->base);
}

static void rect_up_effect_destroy(struct Effect *effect)
{
    struct RectUpEffect *self = (struct RectUpEffect *)effect;
    rect_up_effect_destruct(self);
    mem_free(self);
}

void rect_up_effect_release(struct Effect *effect)
{
    struct RectUpEffect *self = (struct RectUpEffect *)effect;
    struct Particle **it;
    for (it = self->quads.first; it != (struct Particle **)self->quads.last; it++)
        particle_free(*it);
    self->quads.last = self->quads.first;
}

void rect_up_effect_add_wall(struct RectUpEffect *self, Vec3 centre)
{
    int index = 0, angle;

    for (angle = 45; angle <= 405; angle += 90, index++) {
        struct Particle *p = particle_new(self->base.pos, 5);

        if (!p)
            continue;
        *P_OFFSET(p) = centre;
        particle_tag(p, self->base.type, index);
        rect_up_particle(p)->height_phase = random_below(360);
        p->roll = (float)angle;
        rect_up_particle(p)->height = 0;
        if (self->base.type == EFFECT_RECT_UP) {
            rect_up_particle(p)->max_height = 16.0f;
            p->alpha = 120;
            p->scale = 3.5f;
            rect_up_particle(p)->tint_base = random_below(50) + 100;
            rect_up_particle(p)->tint_phase = 0;
        }
        rect_up_effect_set_texture(self, p);
        particle_quad_unit_corners(p->corners);
        vec_push(&self->quads, &p, sizeof(struct Particle *));
    }
}

void rect_up_effect_init(struct Effect *effect)
{
    struct RectUpEffect *self = (struct RectUpEffect *)effect;
    int x, z;

    if (self->base.type != EFFECT_RECT_UP)
        return;
    self->fade_after = 1880;
    for (z = -5; z < 10; z += 5)
        for (x = -5; x < 10; x += 5) {
            Vec3 centre;
            centre.x = (float)x;
            centre.y = 0;
            centre.z = (float)z;
            rect_up_effect_add_wall(self, centre);
        }
}

void rect_up_effect_after_init(struct Effect *effect)
{
    if (effect->type == EFFECT_RECT_UP)
        effect_add_sound_cue(effect, SOUND_CUE_WAVE, "effect\\EF_FireWall.wav", 0, 0.0f);
}

static void rect_up_effect_set_texture(struct RectUpEffect *self, struct Particle *p)
{
    struct String texture;

    if (!p || self->base.type != EFFECT_RECT_UP)
        return;
    string_init(&texture);
    string_assign_cstr(&texture, "effect\\ring_yellow.tga");
    particle_set_texture(p, texture);
}

static void rect_up_effect_tint(struct RectUpEffect *self, struct Particle *p)
{
    int phase;

    if (!p || self->base.type != EFFECT_RECT_UP)
        return;
    phase = next_degree(rect_up_particle(p)->tint_phase);
    rect_up_particle(p)->tint_phase = phase;
    particle_set_color(p, 255, 255,
                        (short)(int)(g_sinTable[phase] * 100.0f + (float)rect_up_particle(p)->tint_base));
}

void rect_up_effect_animate(struct RectUpEffect *self, struct Particle *p)
{
    int phase, age;
    float max_height, height, m[12];

    if (!p)
        return;
    phase = next_degree(rect_up_particle(p)->height_phase);
    age = p->age;
    rect_up_particle(p)->height_phase = phase;
    if (age > self->fade_after) {
        int alpha = p->alpha - 2;
        p->alpha = alpha < 0 ? 0 : alpha;
    }
    max_height = rect_up_particle(p)->max_height;
    height = g_sinTable[phase] * max_height * 0.35f + max_height * 0.65f;
    if (age < 90)
        height *= g_sinTable[age];
    rect_up_particle(p)->height = height;
    rect_up_effect_tint(self, p);
    memcpy(m, g_identityMatrix.m, sizeof(m));
    mat_rotate_y(m, game_camera_heading() / -180.0f * g_pi);
    memcpy(p->matrix, m, sizeof(m));
}

void rect_up_effect_process(struct Effect *effect)
{
    struct RectUpEffect *self = (struct RectUpEffect *)effect;
    struct Particle **it;
    for (it = self->quads.first; it != (struct Particle **)self->quads.last; it++) {
        struct Particle *p = *it;
        Vec3 here = self->base.pos;
        p->pos.x = P_OFFSET(p)->x + here.x;
        p->pos.y = P_OFFSET(p)->y + here.y;
        p->pos.z = P_OFFSET(p)->z + here.z;
        rect_up_effect_animate(self, p);
        p->age++;
    }
}

void rect_up_effect_render_corner(struct RectUpEffect *self, struct Particle *p)
{
    int deg, index;
    float top, x, z;

    if (!p)
        return;
    deg = (int)p->roll % 360;
    index = P_INDEX(p);
    top = -rect_up_particle(p)->height;
    x = g_cosTable[deg] * p->scale;
    z = g_sinTable[deg] * p->scale;
    p->corners[1].y = top;
    p->corners[1].x = x;
    p->corners[1].z = z;
    p->corners[3].x = x;
    p->corners[3].y = 0;
    p->corners[3].z = z;
    if (index) {
        p->corners[0].x = self->last_top.x;
        p->corners[0].y = self->last_top.y;
        p->corners[0].z = self->last_top.z;
        p->corners[2].x = self->last_bottom.x;
        p->corners[2].y = self->last_bottom.y;
        p->corners[2].z = self->last_bottom.z;
    }
    self->last_bottom.x = x;
    self->last_bottom.y = 0;
    self->last_bottom.z = z;
    self->last_top.x = x;
    self->last_top.y = top;
    self->last_top.z = z;
    if (index)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void rect_up_effect_render(struct Effect *effect)
{
    struct RectUpEffect *self = (struct RectUpEffect *)effect;
    struct Particle **it;
    for (it = self->quads.first; it != (struct Particle **)self->quads.last; it++)
        rect_up_effect_render_corner(self, *it);
}
