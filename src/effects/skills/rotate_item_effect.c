#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/render_object.h"
#include "std/vec.h"

struct RotateItemEffect {
    struct Effect base;
    int trail_alpha;
    float trail_angle;
    struct Vec particles;
};

struct String;
void rotate_item_effect_init(struct Effect *self);
void rotate_item_effect_process(struct Effect *self);
void rotate_item_effect_render(struct Effect *self);
void rotate_item_effect_release(struct Effect *self);
static void rotate_item_effect_destroy(struct Effect *effect);

static const struct EffectOps rotate_item_effect_ops = {
    .set_submit = effect_set_submit,
    .init = rotate_item_effect_init,
    .update_range = effect_update_range,
    .process = rotate_item_effect_process,
    .render = rotate_item_effect_render,
    .release = rotate_item_effect_release,
    .is_alive = effect_is_within_duration,
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
    .destroy = rotate_item_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LIST_WORLD 5
#define TRAIL_COUNT 4
#define ORBIT_RADIUS 13.0f
#define ORBIT_HEIGHT -8.0f
#define TRAIL_GAP 5.0f
#define START_FRAME 5
#define NO_OWNER_TURN_TYPE 0x3d8

struct RotateItemParticle {
    float radius;
    float spin;
};
PARTICLE_EFFECT_DATA_FITS(RotateItemParticle);
static __inline struct RotateItemParticle *rotate_item_particle(struct Particle *p)
{
    return (struct RotateItemParticle *)p->effect_data;
}
#define particle_owner_type(p) ((p)->effect_type)
#define particle_tag(p)        ((p)->owner_tag)
#define particle_index(p)      ((p)->index)
#define particle_row(p)        ((p)->group)
#define particle_radius(p)     (rotate_item_particle(p)->radius)
#define particle_spin(p)       (rotate_item_particle(p)->spin)

#define ACTOR_TURN(actor) ((struct RenderObject *)(actor))->roty

#define EACH_PARTICLE(self, it) \
    for (it = (struct Particle **)(self)->particles.first; it != (struct Particle **)(self)->particles.last; it++)

struct RotateItemEffect *rotate_item_effect_construct(struct RotateItemEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &rotate_item_effect_ops;
    self->trail_alpha = 0;
    self->trail_angle = 0;
    self->particles.first = self->particles.last = self->particles.end = 0;
    return self;
}

struct Effect *rotate_item_effect_new(void)
{
    struct RotateItemEffect *self = mem_try_alloc(sizeof *self);

    return self ? &rotate_item_effect_construct(self)->base : NULL;
}

void rotate_item_effect_destruct(struct RotateItemEffect *self)
{
    self->base.ops = &rotate_item_effect_ops;
    vec_free(&self->particles);
    effect_destruct(&self->base);
}

static void rotate_item_effect_destroy(struct Effect *effect)
{
    struct RotateItemEffect *self = (struct RotateItemEffect *)effect;
    rotate_item_effect_destruct(self);
    mem_free(self);
}

void rotate_item_effect_release(struct Effect *effect)
{
    struct RotateItemEffect *self = (struct RotateItemEffect *)effect;
    struct Particle **it;

    EACH_PARTICLE(self, it)
        particle_free(*it);
    self->particles.last = self->particles.first;
}

void rotate_item_effect_init(struct Effect *effect)
{
    struct RotateItemEffect *self = (struct RotateItemEffect *)effect;
    static const char texture_name[] =
        "유저인터페이스\\item\\쿠나이_독.bmp";
    int i;

    for (i = 0; i < TRAIL_COUNT; i++) {
        struct Particle *p = particle_new(self->base.pos, LIST_WORLD);
        struct String texture;

        if (!p)
            continue;
        particle_owner_type(p) = self->base.type;
        particle_index(p) = i;
        particle_row(p) = 0;
        particle_tag(p) = 1;
        string_init(&texture);
        string_assign_cstr(&texture, texture_name);
        particle_set_texture(p, texture);
        p->scale = 2.5f;
        particle_set_color(p, 255, 200, 255);
        p->roll = 360.0f;
        particle_spin(p) = 0.4f;
        p->alpha = 255;
        particle_radius(p) = ORBIT_RADIUS;
        p->dest_blend = 6;
        vec_push(&self->particles, &p, sizeof(struct Particle *));
    }
}

void rotate_item_effect_update_particle(struct RotateItemEffect *self, struct Particle *p)
{
    float angle;
    int deg;

    if (!p || self->base.frame < START_FRAME)
        return;

    if (particle_index(p) == 0) {
        p->roll = (float)((int)(particle_spin(p) * 45.0f + p->roll) % 360);
        if (self->base.type != NO_OWNER_TURN_TYPE) {
            void *owner = effect_find_actor(self->base.owner_aid);
            if (owner)
                ACTOR_TURN(owner) = (float)((int)(particle_spin(p) * 45.0f + ACTOR_TURN(owner)) % 360);
        }
        angle = p->roll;
    } else {
        self->trail_alpha += particle_index(p) == 1 ? -150 : -25;
        p->alpha = self->trail_alpha;
        p->roll = self->trail_angle;
        angle = self->trail_angle;
    }

    deg = (int)angle;
    p->offset.x = g_cosTable[deg] * particle_radius(p);
    p->offset.y = ORBIT_HEIGHT;
    p->offset.z = g_sinTable[deg] * particle_radius(p);
    self->trail_angle = (float)((int)(p->roll - TRAIL_GAP) % 360);
    self->trail_alpha = p->alpha;

    if (p->alpha > 0) {
        float turn[12], scale[12], move[12], scaled[12], m[12];

        mat_set_rotate_x(turn, game_camera_pitch() / 180.0f * 3.141592f);
        mat_rotate_y(turn, (float)((int)(p->roll + 45.0f) % 360) / 180.0f * 3.141592f);
        mat_set_scale(scale, p->scale, p->scale, 1.0f);
        mat_set_translation(move, p->offset.x, p->offset.y, p->offset.z);
        mat_multiply(scaled, scale, turn);
        mat_multiply(m, scaled, move);
        memcpy(p->matrix, m, sizeof(m));
    }
}

void rotate_item_effect_process(struct Effect *effect)
{
    struct RotateItemEffect *self = (struct RotateItemEffect *)effect;
    struct Particle **it;

    EACH_PARTICLE(self, it) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        rotate_item_effect_update_particle(self, p);
        p->age++;
    }
}

void rotate_item_effect_render(struct Effect *effect)
{
    struct RotateItemEffect *self = (struct RotateItemEffect *)effect;
    struct Particle **it;

    EACH_PARTICLE(self, it)
        if (*it)
            particle_render(*it, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}
