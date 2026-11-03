#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include "world/object_lookup.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct Effect;
struct RandomOptEffect;
struct String;
void random_opt_effect_init(struct Effect *self);
void random_opt_effect_process(struct Effect *self);
void random_opt_effect_render(struct Effect *self);
void random_opt_effect_release(struct Effect *self);
unsigned char random_opt_effect_is_alive(struct Effect *self);
void random_opt_effect_set_color(struct Effect *self, int r, int g, int b);
void random_opt_effect_set_texture(struct Effect *self, struct String name);
static void random_opt_effect_destroy(struct Effect *effect);

static const struct EffectOps random_opt_effect_ops = {
    .set_submit = effect_set_submit,
    .init = random_opt_effect_init,
    .update_range = effect_update_range,
    .process = random_opt_effect_process,
    .render = random_opt_effect_render,
    .release = random_opt_effect_release,
    .is_alive = random_opt_effect_is_alive,
    .restart = effect_restart,
    .set_color = random_opt_effect_set_color,
    .set_texture = random_opt_effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = random_opt_effect_destroy,
    .fade_out = effect_fade_out,
};

#define PARTICLE_SEGMENT(p) ((p)->index)
#define PARTICLE_RING(p) ((p)->group)
struct RandomOptParticle {
    int elevation;
    int turn;
    int segment_height;
    float height;
};
PARTICLE_EFFECT_DATA_FITS(RandomOptParticle);
static __inline struct RandomOptParticle *random_opt_particle(const struct Particle *p)
{
    return (struct RandomOptParticle *)p->effect_data;
}

#define EFFECT_RANDOM_OPT_FIRST 0x4a2
#define SEGMENTS 22
#define SEGMENT_SHEET 21.0f
#define PARTICLE_LIST 5

struct RandomOptEffect {
    struct Effect base;
    Vec3 prev_bottom;
    Vec3 prev_top;
    struct Vec segments;
};

struct RandomOptEffect *random_opt_effect_construct(struct RandomOptEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &random_opt_effect_ops;
    self->prev_bottom.x = self->prev_bottom.y = self->prev_bottom.z = 0;
    self->prev_top.x = self->prev_top.y = self->prev_top.z = 0;
    self->segments.first = self->segments.last = self->segments.end = 0;
    return self;
}

struct Effect *random_opt_effect_new(void)
{
    struct RandomOptEffect *self = mem_try_alloc(sizeof *self);

    return self ? &random_opt_effect_construct(self)->base : NULL;
}

/* Effect types created as RandomOptEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow random_opt_effect_type_rows[] = {
    EFFECT_TYPES(0x4a3, 0x4a4, random_opt_effect_new),
    EFFECT_TYPE(0x4a7, random_opt_effect_new),
};
const struct EffectTypeTable random_opt_effect_types = EFFECT_TYPE_TABLE(random_opt_effect_type_rows);

void random_opt_effect_destruct(struct RandomOptEffect *self)
{
    self->base.ops = &random_opt_effect_ops;
    vec_free(&self->segments);
    effect_destruct(&self->base);
}

static void random_opt_effect_destroy(struct Effect *effect)
{
    struct RandomOptEffect *self = (struct RandomOptEffect *)effect;
    random_opt_effect_destruct(self);
    mem_free(self);
}

void random_opt_effect_build_segment(struct RandomOptEffect *self, struct Particle *p)
{
    int segment, angle, elevation;
    float arch, reach, rise;
    Vec3 bottom, top;

    if (!p)
        return;
    segment = PARTICLE_SEGMENT(p);
    arch = g_sinTable[segment * 9] * random_opt_particle(p)->height;
    angle = (int)((float)((int)((float)random_opt_particle(p)->turn / 20.0f) * segment) + p->roll) % 360;
    elevation = random_opt_particle(p)->elevation;
    reach = g_cosTable[elevation] * arch;
    rise = g_sinTable[elevation] * arch;

    bottom.x = p->scale * g_cosTable[angle];
    bottom.y = 0;
    bottom.z = p->scale * g_sinTable[angle];
    top.x = reach * g_cosTable[angle] + bottom.x;
    top.y = 0.0f - rise;
    top.z = reach * g_sinTable[angle] + bottom.z;

    p->corners[1].x = top.x;
    p->corners[1].y = top.y;
    p->corners[1].z = top.z;
    p->corners[3].x = bottom.x;
    p->corners[3].y = bottom.y;
    p->corners[3].z = bottom.z;
    if (segment) {
        p->corners[0].x = self->prev_top.x;
        p->corners[0].y = self->prev_top.y;
        p->corners[0].z = self->prev_top.z;
        p->corners[2].x = self->prev_bottom.x;
        p->corners[2].y = self->prev_bottom.y;
        p->corners[2].z = self->prev_bottom.z;
    }
    self->prev_bottom = bottom;
    self->prev_top = top;
}

void random_opt_effect_init(struct Effect *effect)
{
    struct RandomOptEffect *self = (struct RandomOptEffect *)effect;
    int ring, alpha, start_angle, segment;

    for (ring = 0, alpha = 50, start_angle = 0; alpha > 20; ring++, alpha -= 10, start_angle += 120) {
        for (segment = 0; segment < SEGMENTS; segment++) {
            struct Particle *p = particle_new(self->base.pos, PARTICLE_LIST);
            struct String name;
            float ring_f;

            if (!p)
                continue;
            string_init(&name);
            string_assign_cstr(&name, "effect\\alpha_down.tga");
            particle_set_texture(p, name);
            p->index = segment;
            p->effect_type = self->base.type;
            p->group = ring;
            p->owner_tag = 3;
            p->src_blend = 5;
            p->dest_blend = 6;
            p->alpha = alpha;
            particle_set_color(p, 100, 100, 255);
            p->roll = (float)start_angle;
            ring_f = (float)ring;
            random_opt_particle(p)->turn = 360;
            random_opt_particle(p)->height = 100.0f - ring_f * 10.0f;
            p->scale = ring_f * 0.2f + 0.5f;
            random_opt_particle(p)->elevation = (int)(ring_f * 0.5f + 90.0f);
            random_opt_particle(p)->segment_height = 0;
            switch (self->base.type - EFFECT_RANDOM_OPT_FIRST) {
            case 0:
                particle_set_color(p, 255, 200, 255);
                random_opt_particle(p)->height = 120.0f - ring_f * 12.0f;
                p->scale = ring_f * 0.24f + 0.6f;
                break;
            case 1:
                particle_set_color(p, 192, 192, 192);
                break;
            case 2:
                particle_set_color(p, 0, 100, 150);
                break;
            case 3:
                particle_set_color(p, 255, 255, 128);
                break;
            case 4:
                particle_set_color(p, 128, 0, 255);
                break;
            case 5:
                particle_set_color(p, 255, 128, 0);
                break;
            }
            particle_quad_unit_corners(p->corners);
            vec_push(&self->segments, &p, sizeof(struct Particle *));
            random_opt_effect_build_segment(self, p);
        }
    }
}

unsigned char random_opt_effect_is_alive(struct Effect *effect)
{
    struct RandomOptEffect *self = (struct RandomOptEffect *)effect;
    void *world;

    if (!self->base.owner_aid)
        return 0;
    world = game_current_world();
    if (!world || !world_get_item_by_aid(world, self->base.owner_aid))
        return 0;
    return 1;
}

void random_opt_effect_spin(struct RandomOptEffect *self, struct Particle *p)
{
    float m[12];

    if (!p)
        return;
    p->roll = (float)(int)((float)PARTICLE_RING(p) + p->roll + 3.0f);
    memcpy(m, g_identityMatrix.m, sizeof(m));
    mat_set_rotate_y(m, p->roll / 180.0f * 3.141592f);
    memcpy(p->matrix, m, sizeof(m));
}

void random_opt_effect_process(struct Effect *effect)
{
    struct RandomOptEffect *self = (struct RandomOptEffect *)effect;
    struct Particle **it;

    for (it = self->segments.first; it != self->segments.last; it++) {
        (*it)->pos = self->base.pos;
        random_opt_effect_spin(self, *it);
        (*it)->age++;
    }
}

void random_opt_effect_draw_segment(struct RandomOptEffect *self, struct Particle *p)
{
    struct ParticleRenderArgs args;
    int segment;

    if (!p)
        return;
    segment = PARTICLE_SEGMENT(p);
    if (segment <= 0)
        return;
    args.facing_fixed = 0;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)(segment - 1) / SEGMENT_SHEET;
    args.u_right = (float)segment / SEGMENT_SHEET;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void random_opt_effect_render(struct Effect *effect)
{
    struct RandomOptEffect *self = (struct RandomOptEffect *)effect;
    struct Particle **it;

    for (it = self->segments.first; it != self->segments.last; it++)
        random_opt_effect_draw_segment(self, *it);
}

void random_opt_effect_release(struct Effect *effect)
{
    struct RandomOptEffect *self = (struct RandomOptEffect *)effect;
    struct Particle **it;

    for (it = self->segments.first; it != self->segments.last; it++)
        particle_free(*it);
    self->segments.last = self->segments.first;
}

void random_opt_effect_set_color(struct Effect *effect, int r, int g, int b)
{
    struct RandomOptEffect *self = (struct RandomOptEffect *)effect;
    struct Particle **it;

    for (it = self->segments.first; it != self->segments.last; it++)
        particle_set_color(*it, (short)r, (short)g, (short)b);
}

void random_opt_effect_set_texture(struct Effect *effect, struct String name)
{
    struct RandomOptEffect *self = (struct RandomOptEffect *)effect;
    struct Particle **it;

    for (it = self->segments.first; it != self->segments.last; it++) {
        struct String copy;
        string_copy(&copy, &name);
        particle_set_texture(*it, copy);
    }
    string_free(&name);
}
