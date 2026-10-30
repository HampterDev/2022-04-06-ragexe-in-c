#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/string_vec.h"

struct Effect;
struct JupitelThunderHit;
struct String;
void jupitel_thunder_hit_init(struct Effect *self);
void jupitel_thunder_hit_process(struct Effect *self);
void jupitel_thunder_hit_render(struct Effect *self);
void jupitel_thunder_hit_release(struct Effect *self);
unsigned char jupitel_thunder_hit_is_alive(struct Effect *self);
static void jupitel_thunder_hit_destroy(struct Effect *effect);

static const struct EffectOps jupitel_thunder_hit_ops = {
    .set_submit = effect_set_submit,
    .init = jupitel_thunder_hit_init,
    .update_range = effect_update_range,
    .process = jupitel_thunder_hit_process,
    .render = jupitel_thunder_hit_render,
    .release = jupitel_thunder_hit_release,
    .is_alive = jupitel_thunder_hit_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
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
    .destroy = jupitel_thunder_hit_destroy,
    .fade_out = effect_fade_out,
};

struct JupitelThunderHitParticle {
    signed char texture_index;
    float growth;
};
PARTICLE_EFFECT_DATA_FITS(JupitelThunderHitParticle);
static __inline struct JupitelThunderHitParticle *jupitel_thunder_hit_particle(const void *p)
{
    return (struct JupitelThunderHitParticle *)((const struct Particle *)p)->effect_data;
}

#define EFFECT_JUPITEL_HIT 0x5e
#define EFFECT_DARK_JUPITEL_HIT 0x1c4
#define EFFECT_CREATING_STAR 0x4b0
#define PARTICLE_LIST 0x203
#define RING_FRAMES 10

struct JupitelThunderHit {
    struct Effect base;
    struct Vec rings;
    struct Vec blasts;
    struct Vec blast_textures;
};

static float camera_pitch(void)
{
    return game_camera_pitch() / 180.0f * 3.141592f;
}

static void push_texture_name(struct Vec *v, const char *name)
{
    string_vec_push_text(v, name, strlen(name));
}

static void place_particle(struct JupitelThunderHit *self, struct Particle *p, float size)
{
    float pitch[12], scale[12], m[12];
    Vec3 at;

    mat_set_rotate_x(pitch, camera_pitch());
    mat_set_scale(scale, size, size, 1.0f);
    mat_multiply(m, scale, pitch);
    memcpy(p->matrix, m, sizeof(m));
    at = self->base.pos;
    p->pos.x = at.x + 0.0f;
    p->pos.y = at.y - 5.0f;
    p->pos.z = at.z + 0.0f;
}

struct JupitelThunderHit *jupitel_thunder_hit_construct(struct JupitelThunderHit *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &jupitel_thunder_hit_ops;
    self->rings.first = self->rings.last = self->rings.end = 0;
    self->blasts.first = self->blasts.last = self->blasts.end = 0;
    self->blast_textures.first = self->blast_textures.last = self->blast_textures.end = 0;
    self->base.duration = 250;
    return self;
}

struct Effect *jupitel_thunder_hit_new(void)
{
    struct JupitelThunderHit *self = mem_try_alloc(sizeof *self);

    return self ? &jupitel_thunder_hit_construct(self)->base : NULL;
}

void jupitel_thunder_hit_destruct(struct JupitelThunderHit *self)
{
    self->base.ops = &jupitel_thunder_hit_ops;
    string_vec_free(&self->blast_textures);
    vec_free(&self->blasts);
    vec_free(&self->rings);
    effect_destruct(&self->base);
}

static void jupitel_thunder_hit_destroy(struct Effect *effect)
{
    struct JupitelThunderHit *self = (struct JupitelThunderHit *)effect;
    jupitel_thunder_hit_destruct(self);
    mem_free(self);
}

void jupitel_thunder_hit_add_blast(struct JupitelThunderHit *self)
{
    struct Particle *p;
    struct String name;

    switch (self->base.type) {
    case EFFECT_JUPITEL_HIT:
    case EFFECT_CREATING_STAR:
        push_texture_name(&self->blast_textures, "effect\\thunder_plazma_blast_a.bmp");
        push_texture_name(&self->blast_textures, "effect\\thunder_plazma_blast_b.bmp");
        push_texture_name(&self->blast_textures, "effect\\thunder_ball_d.bmp");
        push_texture_name(&self->blast_textures, "effect\\thunder_ball_e.bmp");
        push_texture_name(&self->blast_textures, "effect\\thunder_ball_f.bmp");
        break;
    case EFFECT_DARK_JUPITEL_HIT:
        push_texture_name(&self->blast_textures, "effect\\twirl_soft.bmp");
        push_texture_name(&self->blast_textures, "effect\\thunder_ball_b.bmp");
        push_texture_name(&self->blast_textures, "effect\\twirl_soft.bmp");
        push_texture_name(&self->blast_textures, "effect\\thunder_ball_c.bmp");
        push_texture_name(&self->blast_textures, "effect\\twirl_soft.bmp");
        break;
    }

    p = particle_new(self->base.pos, PARTICLE_LIST);
    p->age = -10;
    p->scale = 7.5f;
    jupitel_thunder_hit_particle(p)->texture_index = 0;
    string_init(&name);
    string_assign_cstr(&name, "effect\\thunder_plazma_blast_a.bmp");
    particle_set_texture(p, name);
    vec_push(&self->blasts, &p, sizeof p);
}

void jupitel_thunder_hit_add_ring(struct JupitelThunderHit *self)
{
    struct String texture, copy;
    struct Particle *p;

    string_init(&texture);
    string_assign_cstr(&texture, "effect\\thunder_pang.bmp");
    if (self->base.type == EFFECT_JUPITEL_HIT)
        string_assign_cstr(&texture, "effect\\thunder_pang.bmp");
    else if (self->base.type == EFFECT_DARK_JUPITEL_HIT)
        string_assign_cstr(&texture, "effect\\pokjuk_d.bmp");

    p = particle_new(self->base.pos, PARTICLE_LIST);
    string_copy(&copy, &texture);
    particle_set_texture(p, copy);
    jupitel_thunder_hit_particle(p)->growth = 2.5f;
    vec_push(&self->rings, &p, sizeof p);
    string_free(&texture);
}

void jupitel_thunder_hit_init(struct Effect *effect)
{
    struct JupitelThunderHit *self = (struct JupitelThunderHit *)effect;
    if (self->base.type != EFFECT_CREATING_STAR)
        jupitel_thunder_hit_add_ring(self);
    jupitel_thunder_hit_add_blast(self);
}

unsigned char jupitel_thunder_hit_is_alive(struct Effect *effect)
{
    struct JupitelThunderHit *self = (struct JupitelThunderHit *)effect;
    if (self->base.type == EFFECT_CREATING_STAR)
        return effect_is_owner_alive(&self->base);
    if (!self->base.owner_aid)
        return 1;
    return !effect_is_finished(&self->base);
}

void jupitel_thunder_hit_step_ring(struct JupitelThunderHit *self, struct Particle *p)
{
    float size;

    if (!p)
        return;
    if (p->age == RING_FRAMES) {
        p->age = -RING_FRAMES;
        return;
    }
    size = (float)p->age * jupitel_thunder_hit_particle(p)->growth;
    place_particle(self, p, size);
    p->scale = size;
}

void jupitel_thunder_hit_step_blast(struct JupitelThunderHit *self, struct Particle *p)
{
    unsigned next;
    struct String copy;

    if (!p)
        return;
    next = (unsigned)(jupitel_thunder_hit_particle(p)->texture_index + 1);
    if (!(next < VEC_COUNT(&self->blast_textures, struct String)))
        next = 0;
    string_copy(&copy, (struct String *)self->blast_textures.first + next);
    particle_set_texture(p, copy);
    jupitel_thunder_hit_particle(p)->texture_index = (signed char)next;
    place_particle(self, p, p->scale);
}

void jupitel_thunder_hit_process(struct Effect *effect)
{
    struct JupitelThunderHit *self = (struct JupitelThunderHit *)effect;
    struct Particle **it;

    for (it = self->rings.first; it != self->rings.last; it++) {
        (*it)->age++;
        if ((*it)->age >= 0 && *it)
            jupitel_thunder_hit_step_ring(self, *it);
    }
    for (it = self->blasts.first; it != self->blasts.last; it++) {
        (*it)->age++;
        if ((*it)->age >= 0 && *it)
            jupitel_thunder_hit_step_blast(self, *it);
    }
}

void jupitel_thunder_hit_draw_particle(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void jupitel_thunder_hit_render(struct Effect *effect)
{
    struct JupitelThunderHit *self = (struct JupitelThunderHit *)effect;
    struct Particle **it;

    for (it = self->rings.first; it != self->rings.last; it++)
        if ((*it)->age >= 0 && *it)
            jupitel_thunder_hit_draw_particle(*it);
    for (it = self->blasts.first; it != self->blasts.last; it++)
        if ((*it)->age >= 0 && *it)
            jupitel_thunder_hit_draw_particle(*it);
}

void jupitel_thunder_hit_release(struct Effect *effect)
{
    struct JupitelThunderHit *self = (struct JupitelThunderHit *)effect;
    struct Particle **it;

    for (it = self->rings.first; it != self->rings.last; it++)
        particle_free(*it);
    self->rings.last = self->rings.first;
    for (it = self->blasts.first; it != self->blasts.last; it++)
        particle_free(*it);
    self->blasts.last = self->blasts.first;
}
