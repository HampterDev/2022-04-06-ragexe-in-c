#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define LIGHT_SPHERE_PINK 0x4ad
#define LIGHT_SPHERE_YELLOW 0x4ae
#define LIGHT_SPHERE_SKY 0x4af

#define CORES 4
#define RAY_ROWS 4
#define RAYS_PER_ROW 80
#define LIGHT_SPHERE_LIST 5

struct LightSphereEffect {
    struct Effect base;
    struct Vec cores;
    struct Vec rays;
};

struct String;
void light_sphere_effect_init(struct Effect *self);
void light_sphere_effect_process(struct Effect *self);
void light_sphere_effect_render(struct Effect *self);
void light_sphere_effect_release(struct Effect *self);
unsigned char light_sphere_effect_is_alive(struct Effect *self);
static void light_sphere_effect_destroy(struct Effect *effect);

static const struct EffectOps light_sphere_effect_ops = {
    .set_submit = effect_set_submit,
    .init = light_sphere_effect_init,
    .update_range = effect_update_range,
    .process = light_sphere_effect_process,
    .render = light_sphere_effect_render,
    .release = light_sphere_effect_release,
    .is_alive = light_sphere_effect_is_alive,
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
    .destroy = light_sphere_effect_destroy,
    .fade_out = effect_fade_out,
};
#define SIN_DEG(d) (g_sinTable[d])
#define COS_DEG(d) (g_cosTable[d])
#define MATRIX_BYTES (12 * sizeof(float))

#define PART_EFFECT_TYPE(p) ((p)->effect_type)
struct LightSphereParticle {
    int            ray_elevation;
    int            ray_phase;
    union {
        float      core_y;
        float      ray_lift;
    };
    union {
        float      core_phase;
        float      ray_set;
    };
};
PARTICLE_EFFECT_DATA_FITS(LightSphereParticle);
static __inline struct LightSphereParticle *light_sphere_particle(const struct Particle *p)
{
    return (struct LightSphereParticle *)p->effect_data;
}
#define PART_INDEX(p)       ((p)->index)
#define RAY_AZIMUTH(p)      ((p)->roll)

static void set_texture(struct Particle *p, const char *name)
{
    struct String texture;
    string_init(&texture);
    string_assign_cstr(&texture, name);
    particle_set_texture(p, texture);
}

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

struct LightSphereEffect *light_sphere_effect_construct(struct LightSphereEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &light_sphere_effect_ops;
    self->cores.first = self->cores.last = self->cores.end = 0;
    self->rays.first = self->rays.last = self->rays.end = 0;
    return self;
}

struct Effect *light_sphere_effect_new(void)
{
    struct LightSphereEffect *self = mem_try_alloc(sizeof *self);

    return self ? &light_sphere_effect_construct(self)->base : NULL;
}

/* Effect types created as LightSphereEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow light_sphere_effect_type_rows[] = {
    EFFECT_TYPES(0x4ad, 0x4af, light_sphere_effect_new),
};
const struct EffectTypeTable light_sphere_effect_types = EFFECT_TYPE_TABLE(light_sphere_effect_type_rows);

void light_sphere_effect_destruct(struct LightSphereEffect *self)
{
    self->base.ops = &light_sphere_effect_ops;
    vec_free(&self->rays);
    vec_free(&self->cores);
    effect_destruct(&self->base);
}

static void light_sphere_effect_destroy(struct Effect *effect)
{
    struct LightSphereEffect *self = (struct LightSphereEffect *)effect;
    light_sphere_effect_destruct(self);
    mem_free(self);
}

void light_sphere_effect_spawn_rays(struct LightSphereEffect *self, int set)
{
    int row, i;

    for (row = 0; row < RAY_ROWS; row++) {
        for (i = 0; i < RAYS_PER_ROW; i++) {
            struct Particle *p = particle_new(self->base.pos, LIGHT_SPHERE_LIST);
            int first_shade;

            if (!p)
                continue;
            set_texture(p, "effect\\white02.bmp");
            PART_EFFECT_TYPE(p) = self->base.type;
            PART_INDEX(p) = i;
            p->group = row;
            p->owner_tag = 1;
            p->alpha = 40;
            p->scale = 1.0f;
            light_sphere_particle(p)->ray_elevation = rand_below(360);
            RAY_AZIMUTH(p) = (float)rand_below(360);
            light_sphere_particle(p)->ray_phase = rand_below(360);
            light_sphere_particle(p)->ray_lift = 10.0f;
            light_sphere_particle(p)->ray_set = (float)set;

            first_shade = rand_below(2) == 0;
            switch (self->base.type) {
            case LIGHT_SPHERE_PINK:
                if (first_shade)
                    particle_set_color(p, 255, 124, 128);
                else
                    particle_set_color(p, 255, 74, 78);
                break;
            case LIGHT_SPHERE_YELLOW:
                if (first_shade)
                    particle_set_color(p, 255, 255, 155);
                else
                    particle_set_color(p, 255, 255, 55);
                break;
            case LIGHT_SPHERE_SKY:
                if (first_shade)
                    particle_set_color(p, 59, 204, 255);
                else
                    particle_set_color(p, 109, 154, 255);
                break;
            default:
                if (first_shade)
                    particle_set_color(p, 105, 105, 255);
                else
                    particle_set_color(p, 55, 55, 255);
                break;
            }
            particle_quad_unit_corners(p->corners);
            vec_push(&self->rays, &p, sizeof p);
        }
    }
}

void light_sphere_effect_spawn_cores(struct LightSphereEffect *self)
{
    int i;

    for (i = 0; i < CORES; i++) {
        struct Particle *p = particle_new(self->base.pos, LIGHT_SPHERE_LIST);
        if (!p)
            continue;
        set_texture(p, "effect\\thunder_center.bmp");
        PART_EFFECT_TYPE(p) = self->base.type;
        PART_INDEX(p) = i;
        p->group = 0;
        p->owner_tag = 1;
        p->scale = 3.5f;
        p->alpha = 1;
        light_sphere_particle(p)->core_y = -10.0f;
        light_sphere_particle(p)->core_phase = 0.0f;
        switch (self->base.type) {
        case LIGHT_SPHERE_PINK:
            particle_set_color(p, 255, 124, 128);
            break;
        case LIGHT_SPHERE_YELLOW:
            particle_set_color(p, 255, 255, 55);
            break;
        case LIGHT_SPHERE_SKY:
            particle_set_color(p, 59, 204, 255);
            break;
        default:
            break;
        }
        particle_quad_unit_corners(p->corners);
        vec_push(&self->cores, &p, sizeof p);
    }
}

void light_sphere_shape_ray(struct Particle *p)
{
    float radius, across, tip_x, tip_y, tip_z, base_y;
    int elevation, azimuth;

    if (!p)
        return;
    elevation = light_sphere_particle(p)->ray_elevation;
    radius = SIN_DEG(light_sphere_particle(p)->ray_phase) * p->scale + p->scale;
    azimuth = (int)RAY_AZIMUTH(p);
    across = COS_DEG(elevation) * radius;
    tip_x = COS_DEG(azimuth) * across;
    tip_z = SIN_DEG(azimuth) * across;
    tip_y = SIN_DEG(elevation) * radius - light_sphere_particle(p)->ray_lift;
    base_y = -light_sphere_particle(p)->ray_lift;

    p->corners[0].x = tip_x;
    p->corners[0].y = tip_y;
    p->corners[0].z = tip_z;
    p->corners[1].x = tip_x;
    p->corners[1].y = tip_y;
    p->corners[1].z = tip_z;
    if (light_sphere_particle(p)->ray_set == 0.0f) {
        p->corners[2].x = -0.5f;
        p->corners[2].z = 0.0f;
        p->corners[3].x = 0.5f;
        p->corners[3].z = 0.0f;
    } else {
        p->corners[2].x = 0.0f;
        p->corners[2].z = -0.5f;
        p->corners[3].x = 0.0f;
        p->corners[3].z = 0.5f;
    }
    p->corners[2].y = base_y;
    p->corners[3].y = base_y;
}

void light_sphere_update_ray(struct Particle *p)
{
    float scale;

    if (!p)
        return;
    scale = p->scale;
    if (10.0f > scale)
        scale = scale * 1.02f;
    p->scale = scale;
    if (p->age > 500) {
        int alpha = p->alpha - 1;
        p->alpha = alpha < 0 ? 0 : alpha;
    }
    light_sphere_particle(p)->ray_phase = (light_sphere_particle(p)->ray_phase + 1 + rand_below(2)) % 360;
    light_sphere_shape_ray(p);
}

void light_sphere_update_core(struct Particle *p)
{
    float tilt[12], size[12], sized[12], move[12], m[12];
    float s;
    int alpha, phase;

    if (!p)
        return;
    alpha = p->alpha;
    if (p->age >= 300) {
        alpha -= 1;
        if (alpha < 0)
            alpha = 0;
    } else {
        alpha += 1;
        if (alpha >= 255)
            alpha = 255;
    }
    p->alpha = alpha;
    phase = ((int)light_sphere_particle(p)->core_phase + 2) % 360;
    light_sphere_particle(p)->core_phase = (float)phase;
    s = SIN_DEG(phase) * p->scale * 0.05f + p->scale;

    memcpy(tilt, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_rotate_x(tilt, DEG_TO_RAD(game_camera_pitch()));
    memcpy(size, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_scale(size, s, s, 1.0f);
    mat_multiply(sized, size, tilt);
    memcpy(move, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_translation(move, 0.0f, light_sphere_particle(p)->core_y, 0.0f);
    mat_multiply(m, sized, move);
    memcpy(p->matrix, m, MATRIX_BYTES);
}

void light_sphere_render(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void light_sphere_effect_init(struct Effect *effect)
{
    struct LightSphereEffect *self = (struct LightSphereEffect *)effect;
    light_sphere_effect_spawn_cores(self);
    light_sphere_effect_spawn_rays(self, 0);
    light_sphere_effect_spawn_rays(self, 1);
}

void light_sphere_effect_process(struct Effect *effect)
{
    struct LightSphereEffect *self = (struct LightSphereEffect *)effect;
    struct Particle **it;

    for (it = self->cores.first; it != (struct Particle **)self->cores.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        light_sphere_update_core(p);
        p->age++;
    }
    for (it = self->rays.first; it != (struct Particle **)self->rays.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        light_sphere_update_ray(p);
        p->age++;
    }
}

void light_sphere_effect_render(struct Effect *effect)
{
    struct LightSphereEffect *self = (struct LightSphereEffect *)effect;
    struct Particle **it;
    for (it = self->cores.first; it != (struct Particle **)self->cores.last; it++)
        light_sphere_render(*it);
    for (it = self->rays.first; it != (struct Particle **)self->rays.last; it++)
        light_sphere_render(*it);
}

void light_sphere_effect_release(struct Effect *effect)
{
    struct LightSphereEffect *self = (struct LightSphereEffect *)effect;
    struct Particle **it;
    for (it = self->cores.first; it != (struct Particle **)self->cores.last; it++)
        particle_free(*it);
    self->cores.last = self->cores.first;
    for (it = self->rays.first; it != (struct Particle **)self->rays.last; it++)
        particle_free(*it);
    self->rays.last = self->rays.first;
}

unsigned char light_sphere_effect_is_alive(struct Effect *effect)
{
    struct LightSphereEffect *self = (struct LightSphereEffect *)effect;
    struct Particle **core = self->cores.first, **ray = self->rays.first;

    while (core != (struct Particle **)self->cores.last && !((*core)->alpha > 0))
        core++;
    while (ray != (struct Particle **)self->rays.last && !((*ray)->alpha > 0))
        ray++;
    return !(core == (struct Particle **)self->cores.last && ray == (struct Particle **)self->rays.last);
}
