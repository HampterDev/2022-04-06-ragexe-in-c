#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/render_object.h"
#include "std/vec.h"

enum {
    STORM_KICK = 0x1b3,
    STORM_KICK_SPIN = 0x1cb,
    STORM_KICK_ICE = 0x1cc,
    STORM_KICK_MILD_WIND = 0x1cd,
    STORM_KICK_DODGE = 0x1ce,
    STORM_KICK_DODGE_POSE = 0x1cf,
    STORM_KICK_DOUBLE = 0x1d0,
    STORM_KICK_LASTING = 0x1d1,
    STORM_KICK_SMALL = 0x4c7,
};

#define STORM_LAYERS 10
#define CLOUD_SEGMENTS 21
#define KICK_LIST 5

struct StormKickEffect {
    struct Effect base;
    Vec3 prev_bottom;
    Vec3 prev_top;
    struct Vec storms;
    struct Vec clouds;
};

struct String;
void storm_kick_effect_init(struct Effect *self);
void storm_kick_effect_process(struct Effect *self);
void storm_kick_effect_render(struct Effect *self);
void storm_kick_effect_release(struct Effect *self);
void storm_kick_effect_after_init(struct Effect *self);
static void storm_kick_effect_destroy(struct Effect *effect);

static const struct EffectOps storm_kick_effect_ops = {
    .set_submit = effect_set_submit,
    .init = storm_kick_effect_init,
    .update_range = effect_update_range,
    .process = storm_kick_effect_process,
    .render = storm_kick_effect_render,
    .release = storm_kick_effect_release,
    .is_alive = effect_is_within_duration,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = storm_kick_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = storm_kick_effect_destroy,
    .fade_out = effect_fade_out,
};
#define SIN_DEG(d) (g_sinTable[d])
#define COS_DEG(d) (g_cosTable[d])

#define ACTOR_HEADING(actor) ((struct RenderObject *)(actor))->roty

struct StormKickParticle {
    int            cloud_tilt;
    int            cloud_twist;
    union {
        float      storm_depth;
        float      cloud_width;
    };
    float          storm_rise;
    float          storm_which;
    union {
        float      storm_radius;
        float      cloud_height;
    };
};
PARTICLE_EFFECT_DATA_FITS(StormKickParticle);
static __inline struct StormKickParticle *storm_kick_particle(const struct Particle *p)
{
    return (struct StormKickParticle *)p->effect_data;
}

#define MATRIX_BYTES (12 * sizeof(float))

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void set_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

static void draw_whole(struct Particle *p)
{
    particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

struct StormKickEffect *storm_kick_effect_construct(struct StormKickEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &storm_kick_effect_ops;
    self->prev_bottom.x = self->prev_bottom.y = self->prev_bottom.z = 0.0f;
    self->prev_top.x = self->prev_top.y = self->prev_top.z = 0.0f;
    self->storms.first = self->storms.last = self->storms.end = 0;
    self->clouds.first = self->clouds.last = self->clouds.end = 0;
    return self;
}

struct Effect *storm_kick_effect_new(void)
{
    struct StormKickEffect *self = mem_try_alloc(sizeof *self);

    return self ? &storm_kick_effect_construct(self)->base : NULL;
}

/* Effect types created as StormKickEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow storm_kick_effect_type_rows[] = {
    EFFECT_TYPE(0x1b3, storm_kick_effect_new),
    EFFECT_TYPE(0x4c7, storm_kick_effect_new),
};
const struct EffectTypeTable storm_kick_effect_types = EFFECT_TYPE_TABLE(storm_kick_effect_type_rows);

void storm_kick_effect_destruct(struct StormKickEffect *self)
{
    self->base.ops = &storm_kick_effect_ops;
    vec_free(&self->clouds);
    vec_free(&self->storms);
    effect_destruct(&self->base);
}

static void storm_kick_effect_destroy(struct Effect *effect)
{
    struct StormKickEffect *self = (struct StormKickEffect *)effect;
    storm_kick_effect_destruct(self);
    mem_free(self);
}

void storm_kick_tint(struct Particle *p, int type)
{
    switch (type) {
    case STORM_KICK:
        particle_set_color(p, 50, 50, 255);
        break;
    case STORM_KICK_SPIN:
        particle_set_color(p, 164, 136, 55);
        break;
    case STORM_KICK_ICE:
        particle_set_color(p, 255, 255, 255);
        break;
    case STORM_KICK_MILD_WIND:
        particle_set_color(p, 130, 130, 255);
        break;
    case STORM_KICK_DOUBLE:
        if (storm_kick_particle(p)->storm_which == 0.0f)
            particle_set_color(p, 255, 255, 255);
        else if (storm_kick_particle(p)->storm_which == 1.0f)
            particle_set_color(p, 255, 55, 55);
        break;
    case STORM_KICK_LASTING:
        particle_set_color(p, 155, 255, 155);
        break;
    case STORM_KICK_SMALL:
        particle_set_color(p, 255, 255, 50);
        break;
    }
}

void storm_kick_effect_shape_storm(struct Particle *p)
{
    int layer, deg, d1, d2, d3, d4, i;
    float height, radius;

    if (!p)
        return;
    layer = p->index;
    radius = storm_kick_particle(p)->storm_radius;
    height = (float)layer * storm_kick_particle(p)->storm_rise + -storm_kick_particle(p)->storm_depth;
    deg = (int)((float)(layer * 45) + p->roll) % 360;
    for (i = 1; i <= layer; i++)
        radius = radius - (2.2f - (float)i * 0.2f) * p->scale;
    height = height - 1.0f;

    d1 = (deg + 90) % 360;
    d2 = (d1 + 90) % 360;
    d3 = (d2 + 90) % 360;
    d4 = (d3 + 90) % 360;
    p->corners[0].x = COS_DEG(d3) * radius;
    p->corners[0].y = height;
    p->corners[0].z = SIN_DEG(d3) * radius;
    p->corners[1].x = COS_DEG(d2) * radius;
    p->corners[1].y = height;
    p->corners[1].z = SIN_DEG(d2) * radius;
    p->corners[2].x = COS_DEG(d4) * radius;
    p->corners[2].y = height;
    p->corners[2].z = SIN_DEG(d4) * radius;
    p->corners[3].x = COS_DEG(d1) * radius;
    p->corners[3].y = height;
    p->corners[3].z = SIN_DEG(d1) * radius;
}

void storm_kick_effect_spawn_storm(struct StormKickEffect *self, int which)
{
    int i;

    for (i = 0; i < STORM_LAYERS; i++) {
        struct Particle *p = particle_new(self->base.pos, KICK_LIST);
        if (!p)
            continue;
        set_texture(p, "effect\\storm2.tga");
        p->effect_type = self->base.type;
        p->index = i;
        p->group = 0;
        p->owner_tag = 1;
        storm_kick_particle(p)->storm_radius = 40.0f;
        p->alpha = 0;
        p->roll = (float)rand_below(360);
        p->scale = 2.0f;
        storm_kick_particle(p)->storm_depth = 50.0f;
        storm_kick_particle(p)->storm_rise = 5.0f;
        storm_kick_particle(p)->storm_which = (float)which;
        storm_kick_tint(p, self->base.type);
        particle_quad_unit_corners(p->corners);
        vec_push(&self->storms, &p, sizeof p);
        storm_kick_effect_shape_storm(p);
    }
}

void storm_kick_effect_step_storm(struct StormKickEffect *self, struct Particle *p)
{
    float m[12];
    int alpha, spin;

    if (!p)
        return;
    alpha = p->alpha;
    if (p->age > 60 && self->base.type != STORM_KICK_LASTING)
        alpha = alpha - 1 < 0 ? 0 : alpha - 1;
    p->alpha = p->age > 40 ? alpha : alpha + 1;

    spin = (int)(p->roll - (float)(rand_below(3) + 3)) % 360;
    p->roll = (float)spin;
    memcpy(m, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_rotate_y(m, DEG_TO_RAD((float)spin));
    memcpy(p->matrix, m, MATRIX_BYTES);
}

void storm_kick_effect_draw_storm(struct StormKickEffect *self, struct Particle *p)
{
    int alpha = p->alpha;

    switch (self->base.type) {
    case STORM_KICK_SPIN:
    case STORM_KICK_ICE:
    case STORM_KICK_MILD_WIND:
    case STORM_KICK_LASTING:
        p->dest_blend = 6;
        draw_whole(p);
        break;
    case STORM_KICK_DOUBLE:
        if (storm_kick_particle(p)->storm_which == 0.0f) {
            p->dest_blend = 6;
            p->alpha = alpha / 3;
            draw_whole(p);
            p->alpha = alpha;
        } else if (storm_kick_particle(p)->storm_which == 1.0f) {
            p->dest_blend = 2;
            draw_whole(p);
        }
        break;
    default:
        p->dest_blend = 2;
        draw_whole(p);
        break;
    }
}

void storm_kick_effect_spawn_clouds(struct StormKickEffect *self, int band)
{
    int ribbon = 0, bearing, segment;

    for (bearing = 0; bearing < 360; bearing += 90, ribbon++) {
        for (segment = 0; segment < CLOUD_SEGMENTS; segment++) {
            struct Particle *p = particle_new(self->base.pos, KICK_LIST);
            float r;

            if (!p)
                continue;
            set_texture(p, "effect\\cloud11.tga");
            p->effect_type = self->base.type;
            p->index = segment;
            p->group = ribbon;
            p->owner_tag = 1;
            p->alpha = 0;
            storm_kick_particle(p)->cloud_twist = 30;
            p->roll = (float)bearing;
            storm_kick_particle(p)->cloud_tilt = rand_below(21) + 80;
            storm_kick_particle(p)->cloud_width = 1.0f;
            r = (float)ribbon;
            if (self->base.type != STORM_KICK_SMALL) {
                if (band == 0) {
                    storm_kick_particle(p)->cloud_height = 48.0f - r * 5.0f;
                    p->scale = 14.0f - (r + r);
                } else if (band == 1) {
                    storm_kick_particle(p)->cloud_height = 28.0f - r * 5.0f;
                    p->scale = 6.0f - r;
                }
            } else {
                if (band == 0) {
                    storm_kick_particle(p)->cloud_height = 28.0f - r * 5.0f;
                    p->scale = 10.0f - (r + r);
                } else if (band == 1) {
                    storm_kick_particle(p)->cloud_height = 18.0f - r * 5.0f;
                    p->scale = 5.0f - r;
                }
            }
            particle_quad_unit_corners(p->corners);
            vec_push(&self->clouds, &p, sizeof p);
        }
    }
}

void storm_kick_effect_step_cloud(struct StormKickEffect *self, struct Particle *p)
{
    float roll, radius, low, cos_tilt, sin_tilt;
    Vec3 bottom, top;
    int alpha, twist, age, deg;

    if (!p)
        return;
    roll = (float)((int)(p->roll + 5.0f) % 360);
    alpha = p->alpha;
    twist = storm_kick_particle(p)->cloud_twist + 3;
    if (twist > 180)
        twist = 180;
    storm_kick_particle(p)->cloud_twist = twist;
    p->roll = roll;
    age = p->age;
    if (self->base.type == STORM_KICK_SMALL) {
        if (age > 30)
            alpha = alpha - 3 < 0 ? 0 : alpha - 3;
        else
            alpha += 3;
    } else {
        if (age > 50)
            alpha = alpha - 1 < 0 ? 0 : alpha - 1;
        if (age < 12)
            alpha++;
    }
    radius = p->scale;
    p->alpha = alpha;
    if (age > 12)
        radius += 0.1f;
    p->scale = radius;

    deg = (int)((float)((int)((float)twist / 20.0f) * p->index) + roll) % 360;
    low = -storm_kick_particle(p)->cloud_height;
    cos_tilt = COS_DEG(storm_kick_particle(p)->cloud_tilt) * storm_kick_particle(p)->cloud_width;
    sin_tilt = storm_kick_particle(p)->cloud_width * SIN_DEG(storm_kick_particle(p)->cloud_tilt);
    bottom.x = COS_DEG(deg) * radius;
    bottom.y = low;
    bottom.z = SIN_DEG(deg) * radius;
    top.x = COS_DEG(deg) * cos_tilt + bottom.x;
    top.y = low - sin_tilt;
    top.z = SIN_DEG(deg) * cos_tilt + bottom.z;

    p->corners[1].x = top.x;
    p->corners[1].y = top.y;
    p->corners[1].z = top.z;
    p->corners[3].x = bottom.x;
    p->corners[3].y = bottom.y;
    p->corners[3].z = bottom.z;
    if (p->index != 0) {
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

void storm_kick_draw_cloud(struct Particle *p)
{
    struct ParticleRenderArgs args;
    int segment;

    if (!p)
        return;
    segment = p->index;
    if (segment <= 0)
        return;
    args.facing_fixed = 0;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)(segment - 1) / 21.0f;
    args.u_right = (float)segment / 21.0f;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void storm_kick_effect_spin_owner(struct StormKickEffect *self)
{
    void *actor = effect_find_actor(self->base.owner_aid);
    float heading;

    if (!actor)
        return;
    switch (self->base.type) {
    case STORM_KICK:
    case STORM_KICK_SMALL:
        if (self->base.frame > 47)
            return;
        heading = ACTOR_HEADING(actor) + 30.0f;
        break;
    case STORM_KICK_SPIN:
        if (self->base.frame > 50)
            return;
        heading = ACTOR_HEADING(actor) + 14.0f;
        break;
    default:
        return;
    }
    ACTOR_HEADING(actor) = heading;
    if (heading > 360.0f)
        ACTOR_HEADING(actor) = heading - 360.0f;
}

void storm_kick_effect_set_duration(struct StormKickEffect *self)
{
    switch (self->base.type) {
    case STORM_KICK:
    case STORM_KICK_SPIN:
    case STORM_KICK_ICE:
    case STORM_KICK_MILD_WIND:
    case STORM_KICK_DOUBLE:
        self->base.duration = 100;
        break;
    case STORM_KICK_LASTING:
        self->base.duration = 1000;
        break;
    case STORM_KICK_SMALL:
        self->base.duration = 60;
        break;
    }
}

void storm_kick_effect_init(struct Effect *effect)
{
    struct StormKickEffect *self = (struct StormKickEffect *)effect;
    switch (self->base.type) {
    case STORM_KICK:
    case STORM_KICK_SPIN:
    case STORM_KICK_ICE:
    case STORM_KICK_MILD_WIND:
    case STORM_KICK_LASTING:
        storm_kick_effect_spawn_storm(self, 0);
        storm_kick_effect_spawn_clouds(self, 0);
        storm_kick_effect_spawn_clouds(self, 1);
        break;
    case STORM_KICK_DOUBLE:
        storm_kick_effect_spawn_storm(self, 0);
        storm_kick_effect_spawn_storm(self, 1);
        storm_kick_effect_spawn_clouds(self, 0);
        storm_kick_effect_spawn_clouds(self, 1);
        break;
    case STORM_KICK_SMALL:
        storm_kick_effect_spawn_clouds(self, 0);
        storm_kick_effect_spawn_clouds(self, 1);
        break;
    }
    storm_kick_effect_set_duration(self);
}

void storm_kick_effect_after_init(struct Effect *effect)
{
    switch (effect->type) {
    case STORM_KICK:
        effect_add_sound_cue(effect, SOUND_CUE_WAVE,
                             "effect\\T_회오리차기.wav", 5, 0.0f);
        break;
    case STORM_KICK_ICE:
        effect_add_sound_cue(effect, SOUND_CUE_WAVE, "ice_titan_attack.wav", 5, 0.0f);
        break;
    case STORM_KICK_DOUBLE:
        effect_add_sound_cue(effect, SOUND_CUE_WAVE, "effect\\REVERBERATION.wav", 0, 0.0f);
        break;
    }
}

void storm_kick_effect_process(struct Effect *effect)
{
    struct StormKickEffect *self = (struct StormKickEffect *)effect;
    struct Particle **it;

    for (it = self->storms.first; it != (struct Particle **)self->storms.last; it++) {
        (*it)->pos = self->base.pos;
        storm_kick_effect_step_storm(self, *it);
        (*it)->age++;
    }
    for (it = self->clouds.first; it != (struct Particle **)self->clouds.last; it++) {
        (*it)->pos = self->base.pos;
        storm_kick_effect_step_cloud(self, *it);
        (*it)->age++;
    }
    storm_kick_effect_spin_owner(self);
}

void storm_kick_effect_render(struct Effect *effect)
{
    struct StormKickEffect *self = (struct StormKickEffect *)effect;
    struct Particle **it;

    for (it = self->storms.first; it != (struct Particle **)self->storms.last; it++)
        storm_kick_effect_draw_storm(self, *it);
    for (it = self->clouds.first; it != (struct Particle **)self->clouds.last; it++)
        storm_kick_draw_cloud(*it);
}

void storm_kick_effect_release(struct Effect *effect)
{
    struct StormKickEffect *self = (struct StormKickEffect *)effect;
    struct Particle **it;

    for (it = self->storms.first; it != (struct Particle **)self->storms.last; it++)
        particle_free(*it);
    self->storms.last = self->storms.first;
    for (it = self->clouds.first; it != (struct Particle **)self->clouds.last; it++)
        particle_free(*it);
    self->clouds.last = self->clouds.first;
}
