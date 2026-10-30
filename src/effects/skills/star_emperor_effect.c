#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define STAR_EMPEROR_FIRE_PILLAR 0x4b3

#define BOOM_ROWS 6
#define BOOM_COLUMNS 12
#define RING_SEGMENTS 10
#define RING_SEGMENT_DEGREES 36

struct StarEmperorEffect {
    struct Effect base;
    struct Vec booms;
    struct Vec rings;
};

struct StarEmperorParticle {
    int ring_segment_span;
};
PARTICLE_EFFECT_DATA_FITS(StarEmperorParticle);
static __inline struct StarEmperorParticle *star_emperor_particle(const struct Particle *p)
{
    return (struct StarEmperorParticle *)p->effect_data;
}

struct String;
void star_emperor_effect_init(struct Effect *self);
void star_emperor_effect_process(struct Effect *self);
void star_emperor_effect_render(struct Effect *self);
void star_emperor_effect_release(struct Effect *self);
void star_emperor_effect_after_init(struct Effect *self);
static void star_emperor_effect_destroy(struct Effect *effect);

static const struct EffectOps star_emperor_effect_ops = {
    .set_submit = effect_set_submit,
    .init = star_emperor_effect_init,
    .update_range = effect_update_range,
    .process = star_emperor_effect_process,
    .render = star_emperor_effect_render,
    .release = star_emperor_effect_release,
    .is_alive = effect_is_within_duration,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = star_emperor_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = star_emperor_effect_destroy,
    .fade_out = effect_fade_out,
};

#define MATRIX_BYTES (12 * sizeof(float))

static void particle_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

static float clamp01(float x)
{
    if (0.0f > x)
        return 0.0f;
    if (x > 1.0f)
        return 1.0f;
    return x;
}

static void corner_on_sphere(struct TexDrawVertex *v, const float m[12])
{
    v->x = m[0] * 0.0f + m[3] * 0.0f - m[6] * 1.0f + m[9];
    v->y = m[4] * 0.0f + m[1] * 0.0f - m[7] * 1.0f + m[10];
    v->z = m[5] * 0.0f + m[2] * 0.0f - m[8] * 1.0f + m[11];
}

static int anim_step(const struct StarEmperorEffect *self, const struct Particle *p)
{
    int t = p->age + 1;
    if (self->base.type == STAR_EMPEROR_FIRE_PILLAR) {
        int half = self->base.duration / 2;
        t = half > p->age ? half - p->age : p->age - half;
    }
    return t;
}

static int anim_alpha(const struct StarEmperorEffect *self, const struct Particle *p, int t)
{
    int d = self->base.duration;
    if (self->base.type == STAR_EMPEROR_FIRE_PILLAR) {
        int quarter = d / 4;
        if (quarter <= t)
            t = d - t;
        return t * (255 / quarter);
    } else {
        int half = d / 2;
        int step = 255 / half;
        if (half > p->age)
            return t * step;
        return (d - t) * step;
    }
}

static float decelerated(const struct StarEmperorEffect *self, float speed, int t)
{
    float slow = speed / (float)self->base.duration * -0.5f;
    return (float)((t + 1) * t / 2) * slow + (float)t * speed;
}

struct StarEmperorEffect *star_emperor_effect_construct(struct StarEmperorEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &star_emperor_effect_ops;
    self->booms.first = self->booms.last = self->booms.end = 0;
    self->rings.first = self->rings.last = self->rings.end = 0;
    return self;
}

struct Effect *star_emperor_effect_new(void)
{
    struct StarEmperorEffect *self = mem_try_alloc(sizeof *self);

    return self ? &star_emperor_effect_construct(self)->base : NULL;
}

void star_emperor_effect_destruct(struct StarEmperorEffect *self)
{
    self->base.ops = &star_emperor_effect_ops;
    vec_free(&self->rings);
    vec_free(&self->booms);
    effect_destruct(&self->base);
}

static void star_emperor_effect_destroy(struct Effect *effect)
{
    struct StarEmperorEffect *self = (struct StarEmperorEffect *)effect;
    star_emperor_effect_destruct(self);
    mem_free(self);
}

void star_emperor_effect_make_rings(struct StarEmperorEffect *self)
{
    int i;
    for (i = 0; i < RING_SEGMENTS; i++) {
        struct Particle *p = particle_new(self->base.pos, 0x201);
        p->effect_type = self->base.type;
        p->index = i;
        p->group = 0;
        p->owner_tag = 1;
        particle_texture(p, "effect\\star_emperor\\ring_white.tga");
        star_emperor_particle(p)->ring_segment_span = RING_SEGMENT_DEGREES;
        vec_push(&self->rings, &p, sizeof p);
    }
}

void star_emperor_effect_make_booms(struct StarEmperorEffect *self)
{
    int row, col;
    for (row = 0; row < BOOM_ROWS; row++) {
        float v_top = (float)row * 0.16666667f;
        float v_bottom = (float)(row + 1) * 0.16666667f;
        float lat_top = DEG_TO_RAD((float)row * 30.0f - 90.0f);
        float lat_bottom = DEG_TO_RAD((float)(row + 1) * 30.0f - 90.0f);

        for (col = 0; col < BOOM_COLUMNS; col++) {
            struct Particle *p = particle_new(self->base.pos, 1);
            float u_left, u_right, lon_left, lon_right;
            float m[12];

            particle_texture(p, "effect\\star_emperor\\boom.tga");
            u_left = (float)col * 0.083333336f;
            u_right = (float)(col + 1) * 0.083333336f;
            p->corners[0].u = clamp01(u_left);
            p->corners[0].v = clamp01(v_top);
            p->corners[1].u = clamp01(u_right);
            p->corners[1].v = clamp01(v_top);
            p->corners[2].u = clamp01(u_left);
            p->corners[2].v = clamp01(v_bottom);
            p->corners[3].u = clamp01(u_right);
            p->corners[3].v = clamp01(v_bottom);

            lon_left = DEG_TO_RAD((float)col * -30.0f);
            lon_right = DEG_TO_RAD((float)(col + 1) * -30.0f);
            mat_set_rotate_x(m, lat_top);
            mat_rotate_y(m, lon_left);
            corner_on_sphere(&p->corners[0], m);
            mat_set_rotate_x(m, lat_top);
            mat_rotate_y(m, lon_right);
            corner_on_sphere(&p->corners[1], m);
            mat_set_rotate_x(m, lat_bottom);
            mat_rotate_y(m, lon_left);
            corner_on_sphere(&p->corners[2], m);
            mat_set_rotate_x(m, lat_bottom);
            mat_rotate_y(m, lon_right);
            corner_on_sphere(&p->corners[3], m);

            vec_push(&self->booms, &p, sizeof p);
        }
    }
}

void star_emperor_effect_update_ring(struct StarEmperorEffect *self, struct Particle *p)
{
    int from, to, t;
    float outer, inner = 0.0f;

    if (!p)
        return;
    from = star_emperor_particle(p)->ring_segment_span * p->index;
    to = (p->index + 1) * star_emperor_particle(p)->ring_segment_span;
    t = anim_step(self, p);
    outer = decelerated(self, 1.75f, t);
    if (outer > 12.0f)
        inner = outer - 12.0f;

    p->corners[0].x = g_cosTable[from] * outer;
    p->corners[0].z = 0.0f;
    p->corners[0].y = g_sinTable[from] * outer;
    p->corners[1].x = g_cosTable[to] * outer;
    p->corners[1].y = g_sinTable[to] * outer;
    p->corners[1].z = 0.0f;
    p->corners[2].x = g_cosTable[from] * inner;
    p->corners[2].y = g_sinTable[from] * inner;
    p->corners[2].z = 0.0f;
    p->corners[3].x = g_cosTable[to] * inner;
    p->corners[3].y = g_sinTable[to] * inner;
    p->corners[3].z = 0.0f;
    {
        float m[12];
        mat_set_rotate_x(m, 1.5707960f);
        memcpy(p->matrix, m, MATRIX_BYTES);
    }
    p->alpha = anim_alpha(self, p, t);
    p->pos = self->base.pos;
}

void star_emperor_effect_update_boom(struct StarEmperorEffect *self, struct Particle *p)
{
    float scale[12], spin[12], m[12], s;
    int t;

    if (!p)
        return;
    t = anim_step(self, p);
    s = decelerated(self, 1.15f, t);
    mat_set_scale(scale, s, s, s);
    mat_set_rotate_y(spin, DEG_TO_RAD((float)t * 3.0f));
    mat_multiply(m, scale, spin);
    memcpy(p->matrix, m, MATRIX_BYTES);
    p->alpha = anim_alpha(self, p, t);
    p->pos = self->base.pos;
}

void star_emperor_render_ring(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 0.25f, 1);
}

void star_emperor_render_boom(struct Particle *p)
{
    if (p)
        particle_render(p, 0, p->corners[0].v, p->corners[3].v, p->corners[0].u, p->corners[3].u, 1);
}

void star_emperor_effect_init(struct Effect *effect)
{
    struct StarEmperorEffect *self = (struct StarEmperorEffect *)effect;
    self->base.duration = self->base.type == STAR_EMPEROR_FIRE_PILLAR ? 60 : 30;
    star_emperor_effect_make_booms(self);
    star_emperor_effect_make_rings(self);
}

void star_emperor_effect_after_init(struct Effect *effect)
{
    if (effect->type != STAR_EMPEROR_FIRE_PILLAR)
        return;
    effect_add_sound_cue(effect, SOUND_CUE_WAVE, "effect\\wizard_fire_pillar_a.wav", 1, 0.0f);
    effect_add_sound_cue(effect, SOUND_CUE_WAVE, "effect\\wizard_fire_pillar_b.wav", 31, 0.0f);
}

void star_emperor_effect_process(struct Effect *effect)
{
    struct StarEmperorEffect *self = (struct StarEmperorEffect *)effect;
    struct Particle **it;
    for (it = self->booms.first; it != (struct Particle **)self->booms.last; it++) {
        star_emperor_effect_update_boom(self, *it);
        (*it)->age++;
    }
    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++) {
        star_emperor_effect_update_ring(self, *it);
        (*it)->age++;
    }
}

void star_emperor_effect_render(struct Effect *effect)
{
    struct StarEmperorEffect *self = (struct StarEmperorEffect *)effect;
    struct Particle **it;
    for (it = self->booms.first; it != (struct Particle **)self->booms.last; it++)
        star_emperor_render_boom(*it);
    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++)
        star_emperor_render_ring(*it);
}

void star_emperor_effect_release(struct Effect *effect)
{
    struct StarEmperorEffect *self = (struct StarEmperorEffect *)effect;
    struct Particle **it;
    for (it = self->booms.first; it != (struct Particle **)self->booms.last; it++)
        particle_free(*it);
    self->booms.last = self->booms.first;
    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++)
        particle_free(*it);
    self->rings.last = self->rings.first;
}
