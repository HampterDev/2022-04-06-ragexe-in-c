#include "effects/effect_allocator.h"
#include "effects/skills/cloud.h"
#include "render/math3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

struct String;
void cloud_init(struct Effect *self);
void cloud_process(struct Effect *self);
void cloud_render(struct Effect *self);
void cloud_release(struct Effect *self);
static void cloud_destroy(struct Effect *effect);

static const struct EffectOps cloud_ops = {
    .set_submit = effect_set_submit,
    .init = cloud_init,
    .update_range = effect_update_range,
    .process = cloud_process,
    .render = cloud_render,
    .release = cloud_release,
    .is_alive = effect_is_owner_alive,
    .restart = cloud_respawn,
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
    .destroy = cloud_destroy,
    .fade_out = effect_fade_out,
};

static int rand_below(int n)
{
    int r;
    if (n == 0)
        return 0;
    r = rand() % n;
    return r < 0 ? 0 : r;
}

struct Cloud *cloud_construct(struct Cloud *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &cloud_ops;
    self->drift_x = self->drift_x_goal = self->drift_z = self->drift_z_goal = self->pulse_phase = 0;
    self->size = 0;
    self->alpha = 0;
    self->fade_in_until = self->fade_out_after = 0;
    self->color[0] = self->color[1] = self->color[2] = 0xff;
    self->size_min = self->size_range = self->pulse = 0;
    self->fade_in_step = self->fade_out_step = 0;
    self->fade_in_min = self->fade_in_range = self->fade_out_min = self->fade_out_range = 0;
    self->spread_y = self->spread_x = self->spread_z = 0;
    return self;
}

struct Effect *cloud_new(void)
{
    struct Cloud *self = mem_try_alloc(sizeof *self);

    return self ? &cloud_construct(self)->base : NULL;
}

void cloud_destruct(struct Cloud *self)
{
    self->base.ops = &cloud_ops;
    effect_destruct(&self->base);
}

static void cloud_destroy(struct Effect *effect)
{
    struct Cloud *self = (struct Cloud *)effect;
    cloud_destruct(self);
    mem_free(self);
}

void cloud_release(struct Effect *effect)
{
    struct Cloud *self = (struct Cloud *)effect;
    if (self->puff) {
        particle_free(self->puff);
        self->puff = 0;
    }
}

void cloud_init(struct Effect *effect)
{
    struct Cloud *self = (struct Cloud *)effect;
    struct Particle *p = particle_new(self->base.pos, 1);
    self->puff = p;
    if (!p)
        return;
    p->effect_type = self->base.type;
    p->index = 0;
    p->group = 0;
    p->owner_tag = 1;
    self->puff->src_blend = 5;
    self->puff->dest_blend = 6;
}

void cloud_respawn(struct Effect *effect)
{
    struct Cloud *self = (struct Cloud *)effect;
    struct Particle *p;
    struct String texture;
    char name[32];
    int off_x, off_y, off_z;

    self->drift_x = rand_below(360);
    self->drift_x_goal = rand_below(360);
    self->drift_z = rand_below(360);
    self->drift_z_goal = rand_below(360);
    self->pulse_phase = rand_below(360);
    self->alpha = 0;
    self->size = (float)rand_below((int)self->size_range) + self->size_min;
    self->fade_in_until = self->fade_in_min + rand_below(self->fade_in_range);
    self->fade_out_after = self->fade_out_min + rand_below(self->fade_out_range);

    p = self->puff;
    if (!p)
        return;
    p->age = 0;
    off_z = rand_below((int)self->spread_z);
    off_y = rand_below((int)self->spread_y);
    off_x = rand_below((int)self->spread_x);
    p->offset.x = (float)off_x;
    p->offset.y = (float)off_y;
    p->offset.z = (float)off_z;
    particle_set_color(self->puff, self->color[0], self->color[1], self->color[2]);
    self->puff->alpha = 0;
    self->puff->scale = self->size;

    sprintf(name, "effect\\cloud%d.tga", rand_below(4) + 1);
    string_init(&texture);
    string_assign_cstr(&texture, name);
    particle_set_texture(self->puff, texture);
}

void cloud_fade(struct Cloud *self)
{
    struct Particle *p = self->puff;

    if (p->age < self->fade_in_until) {
        float a = self->fade_in_step + self->alpha;
        self->alpha = a < 255.0f ? a : 255.0f;
    } else if (p->age > self->fade_out_after) {
        self->alpha -= self->fade_out_step;
        if (self->alpha <= 0.0f) {
            cloud_respawn(&self->base);
            return;
        }
    }
    p->alpha = (int)self->alpha;
}

static void drift_step(int *angle, int *goal)
{
    int a = *angle, g = *goal;
    if (g > a) {
        a = a + 1 > 360 ? 360 : a + 1;
        *angle = a;
        if (g < a)
            g = rand_below(a);
    } else {
        a = a - 1 < 0 ? 0 : a - 1;
        *angle = a;
        if (!(g < a))
            g = 360 - rand_below(a);
    }
    *goal = g;
}

void cloud_drift(struct Cloud *self)
{
    struct Particle *p;
    Vec3 off, here;

    drift_step(&self->drift_x, &self->drift_x_goal);
    drift_step(&self->drift_z, &self->drift_z_goal);
    p = self->puff;
    off = p->offset;
    off.x = g_sinTable[self->drift_x] * 0.05f + off.x;
    off.z = g_sinTable[self->drift_z] * 0.05f + off.z;
    p->offset = off;
    here = self->base.pos;
    p->pos.x = off.x + here.x;
    p->pos.y = here.y + off.y;
    p->pos.z = off.z + here.z;
}

void cloud_pulse(struct Cloud *self)
{
    self->pulse_phase = (self->pulse_phase + 1) % 360;
    self->puff->scale = self->pulse * self->size * g_sinTable[self->pulse_phase] + self->size;
}

void cloud_orient(struct Cloud *self)
{
    float scale[12], tilt[12], m[12];
    float s = self->puff->scale;

    mat_set_scale(scale, s, s, 1.0f);
    mat_set_rotate_x(tilt, game_camera_pitch() / 180.0f * 3.141592f);
    mat_multiply(m, scale, tilt);
    memcpy(self->puff->matrix, m, sizeof m);
}

void cloud_process(struct Effect *effect)
{
    struct Cloud *self = (struct Cloud *)effect;
    if (!self->puff)
        return;
    cloud_fade(self);
    cloud_drift(self);
    cloud_pulse(self);
    cloud_orient(self);
    self->puff->age++;
}

void cloud_render(struct Effect *effect)
{
    struct Cloud *self = (struct Cloud *)effect;
    if (self->puff)
        particle_render(self->puff, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}
