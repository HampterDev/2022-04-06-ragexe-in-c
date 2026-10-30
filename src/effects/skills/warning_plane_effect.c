#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_controls.h"
#include "std/vec.h"

#define EFFECT_WARNING_PLANE 0x45a

struct WarningPlaneEffect {
    struct Effect base;
    struct Vec planes;
    unsigned char brightening;
    DWORD end_time;
};

struct String;
void warning_plane_effect_process(struct Effect *self);
void warning_plane_effect_render(struct Effect *self);
void warning_plane_effect_release(struct Effect *self);
unsigned char warning_plane_effect_is_alive(struct Effect *self);
static void warning_plane_effect_destroy(struct Effect *effect);

static const struct EffectOps warning_plane_effect_ops = {
    .set_submit = effect_set_submit,
    .init = effect_init,
    .update_range = effect_update_range,
    .process = warning_plane_effect_process,
    .render = warning_plane_effect_render,
    .release = warning_plane_effect_release,
    .is_alive = warning_plane_effect_is_alive,
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
    .destroy = warning_plane_effect_destroy,
    .fade_out = effect_fade_out,
};

struct WarningPlaneEffect *warning_plane_effect_construct(struct WarningPlaneEffect *self)
{
    effect_construct(&self->base, EFFECT_WARNING_PLANE);
    self->base.ops = &warning_plane_effect_ops;
    self->planes.first = self->planes.last = self->planes.end = 0;
    self->brightening = 0;
    self->end_time = 0;
    return self;
}

struct Effect *warning_plane_effect_new(void)
{
    struct WarningPlaneEffect *self = mem_try_alloc(sizeof *self);

    return self ? &warning_plane_effect_construct(self)->base : NULL;
}

void warning_plane_effect_destruct(struct WarningPlaneEffect *self)
{
    self->base.ops = &warning_plane_effect_ops;
    vec_free(&self->planes);
    effect_destruct(&self->base);
}

static void warning_plane_effect_destroy(struct Effect *effect)
{
    struct WarningPlaneEffect *self = (struct WarningPlaneEffect *)effect;
    warning_plane_effect_destruct(self);
    mem_free(self);
}

void warning_plane_effect_release(struct Effect *effect)
{
    struct WarningPlaneEffect *self = (struct WarningPlaneEffect *)effect;
    struct Particle **it;
    for (it = self->planes.first; it != (struct Particle **)self->planes.last; it++)
        particle_free(*it);
    self->planes.last = self->planes.first;
}

void warning_plane_effect_set_end_time(struct Effect *effect, DWORD end_time)
{
    ((struct WarningPlaneEffect *)effect)->end_time = end_time;
}

void warning_plane_effect_add_plane(struct Effect *effect, Vec3 pos)
{
    struct WarningPlaneEffect *self = (struct WarningPlaneEffect *)effect;
    struct Particle *p = particle_new(self->base.pos, 5);
    struct Particle *head = p;
    struct String texture;

    if (!p)
        return;
    string_init(&texture);
    string_assign_cstr(&texture, "warning_plane.tga");
    particle_set_texture(p, texture);
    head->effect_type = self->base.type;
    head->index = 0;
    head->group = 0;
    head->owner_tag = 1;
    p->scale = 2.5f;
    p->alpha = 150;
    particle_set_color(p, 255, 0, 0);
    p->pos = pos;
    particle_quad_unit_corners(p->corners);
    vec_push(&self->planes, &p, sizeof(struct Particle *));
}

unsigned char warning_plane_effect_is_alive(struct Effect *effect)
{
    struct WarningPlaneEffect *self = (struct WarningPlaneEffect *)effect;
    DWORD now = timeGetTime();
    if (self->end_time && now > self->end_time)
        return 0;
    return effect_is_owner_alive(&self->base);
}

void warning_plane_effect_animate_plane(struct WarningPlaneEffect *self, struct Particle *p)
{
    float turn[12], scaling[12], m[12];
    int alpha;

    if (!self->end_time || !p)
        return;
    memcpy(turn, g_identityMatrix.m, sizeof(turn));
    mat_set_rotate_x(turn, 1.5707964f);
    mat_rotate_y(turn, game_camera_heading() / -180.0f * g_pi);
    memcpy(scaling, g_identityMatrix.m, sizeof(scaling));
    mat_set_scale(scaling, p->scale, p->scale, 1.0f);
    mat_multiply(m, scaling, turn);
    memcpy(p->matrix, m, sizeof(m));
    alpha = p->alpha;
    if (self->brightening == 1) {
        alpha += 2;
        if (alpha >= 250)
            self->brightening = 0;
    } else {
        alpha -= 2;
        if (alpha <= 50)
            self->brightening = 1;
    }
    p->alpha = alpha;
}

void warning_plane_effect_process(struct Effect *effect)
{
    struct WarningPlaneEffect *self = (struct WarningPlaneEffect *)effect;
    struct Particle **it;
    for (it = self->planes.first; it != (struct Particle **)self->planes.last; it++) {
        warning_plane_effect_animate_plane(self, *it);
        (*it)->age++;
    }
}

void warning_plane_effect_render_plane(struct WarningPlaneEffect *self, struct Particle *p)
{
    if (self->end_time && p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void warning_plane_effect_render(struct Effect *effect)
{
    struct WarningPlaneEffect *self = (struct WarningPlaneEffect *)effect;
    struct Particle **it;
    for (it = self->planes.first; it != (struct Particle **)self->planes.last; it++)
        warning_plane_effect_render_plane(self, *it);
}
