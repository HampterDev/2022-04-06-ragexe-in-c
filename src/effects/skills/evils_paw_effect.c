#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdio.h>
#include <string.h>
#include "effects/skills/evils_paw_effect.h"
#include "effects/skills/skill_view.h"
#include "std/string_vec.h"

struct Effect;
struct EvilsPawEffect;
struct String;
void evils_paw_effect_init(struct Effect *self);
void evils_paw_effect_process(struct Effect *self);
void evils_paw_effect_render(struct Effect *self);
void evils_paw_effect_release(struct Effect *self);
unsigned char evils_paw_effect_is_alive(struct Effect *self);
void evils_paw_effect_restart(struct Effect *self);
static void evils_paw_effect_destroy(struct Effect *effect);

static const struct EffectOps evils_paw_effect_ops = {
    .set_submit = effect_set_submit,
    .init = evils_paw_effect_init,
    .update_range = effect_update_range,
    .process = evils_paw_effect_process,
    .render = evils_paw_effect_render,
    .release = evils_paw_effect_release,
    .is_alive = evils_paw_effect_is_alive,
    .restart = evils_paw_effect_restart,
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
    .destroy = evils_paw_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LIST_WORLD 5
#define PI_ROUGH 3.1415925026f

#define EACH_PARTICLE(self, it) \
    for (it = (struct Particle **)(self)->particles.first; it != (struct Particle **)(self)->particles.last; it++)

static void set_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

struct EvilsPawEffect *evils_paw_effect_construct(struct EvilsPawEffect *self)
{
    effect_construct(&self->base, 0x40f);
    self->base.ops = &evils_paw_effect_ops;
    self->particles.first = self->particles.last = self->particles.end = 0;
    self->frame_delay = 5;
    self->particle_scale = 50;
    self->frame_countdown = 5;
    self->offset.x = 0;
    self->offset.y = -40.0f;
    self->offset.z = 0;
    self->frames.first = self->frames.last = self->frames.end = 0;
    self->frame = 0;
    return self;
}

struct Effect *evils_paw_effect_new(void)
{
    struct EvilsPawEffect *self = mem_try_alloc(sizeof *self);

    return self ? &evils_paw_effect_construct(self)->base : NULL;
}

void evils_paw_effect_destruct(struct EvilsPawEffect *self)
{
    self->base.ops = &evils_paw_effect_ops;
    string_vec_free(&self->frames);
    vec_free(&self->particles);
    effect_destruct(&self->base);
}

static void evils_paw_effect_destroy(struct Effect *effect)
{
    struct EvilsPawEffect *self = (struct EvilsPawEffect *)effect;
    evils_paw_effect_destruct(self);
    mem_free(self);
}

void evils_paw_effect_reset_particle(struct EvilsPawEffect *self, struct Particle *p)
{
    particle_set_color(p, 0xff, 0xff, 0xff);
    p->alpha = 0xff;
    p->scale = (float)self->particle_scale;
    p->offset = self->offset;
}

void evils_paw_effect_add_particle(struct EvilsPawEffect *self)
{
    struct Particle *p = particle_new(self->base.pos, LIST_WORLD);
    if (!p)
        return;
    evils_paw_effect_reset_particle(self, p);
    p->src_blend = 5;
    p->dest_blend = 6;
    particle_quad_unit_corners(p->corners);
    vec_push(&self->particles, &p, sizeof(struct Particle *));
}

void evils_paw_effect_init(struct Effect *effect)
{
    struct EvilsPawEffect *self = (struct EvilsPawEffect *)effect;
    int set, picture;

    evils_paw_effect_add_particle(self);
    for (set = 0; set < 2; set++) {
        for (picture = 1; picture <= 10; picture++) {
            char name[64];
            sprintf(name, "effect\\%d-%d.tga", set + 1, picture);
            string_vec_push_text(&self->frames, name, strlen(name));
        }
    }
}

void evils_paw_effect_release(struct Effect *effect)
{
    struct EvilsPawEffect *self = (struct EvilsPawEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it)
        particle_free(*it);
    self->particles.last = self->particles.first;
}

void evils_paw_effect_restart(struct Effect *effect)
{
    struct EvilsPawEffect *self = (struct EvilsPawEffect *)effect;
    struct Particle **it;
    self->frame = 0;
    self->frame_countdown = self->frame_delay;
    EACH_PARTICLE(self, it)
        evils_paw_effect_reset_particle(self, *it);
}

unsigned char evils_paw_effect_is_alive(struct Effect *self)
{
    return 1;
}

void evils_paw_effect_show_frame(struct EvilsPawEffect *self, struct Particle *p)
{
    float turn[12], scale[12];

    if (!p)
        return;
    set_texture(p, string_cstr(&((struct String *)self->frames.first)[self->frame]));
    mat_set_rotate_z(turn, PI_ROUGH);
    mat_rotate_x(turn, game_camera_pitch() / g_halfTurnDegrees * g_pi);
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_multiply(p->matrix, scale, turn);
    p->pos.x = self->offset.x + self->base.pos.x;
    p->pos.y = self->offset.y + self->base.pos.y;
    p->pos.z = self->offset.z + self->base.pos.z;
}

void evils_paw_effect_process(struct Effect *effect)
{
    struct EvilsPawEffect *self = (struct EvilsPawEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        evils_paw_effect_show_frame(self, p);
        p->age++;
    }
    if (--self->frame_countdown < 0) {
        int count = (int)((struct String *)self->frames.last - (struct String *)self->frames.first);
        self->frame++;
        if ((unsigned)(count - 1) < (unsigned)self->frame)
            self->frame = 0;
        self->frame_countdown = self->frame_delay;
    }
}

void evils_paw_render_particle(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void evils_paw_effect_render(struct Effect *effect)
{
    struct EvilsPawEffect *self = (struct EvilsPawEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it)
        evils_paw_render_particle(*it);
}
