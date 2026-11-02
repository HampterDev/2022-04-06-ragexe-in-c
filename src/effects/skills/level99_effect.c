#include "effects/effect_allocator.h"
#include "actor/actor_ops.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "session/session_skills.h"

#define LEVEL99_TYPE 200
#define LEVEL99_LIST 13
#define FLARES 2
#define RING_QUADS 22
#define RING_SWEEP 315

struct Level99Effect {
    struct Effect base;
    Vec3 ring_ground;
    Vec3 ring_top;
    struct Vec flares;
    struct Vec rings;
};

struct String;
void level99_effect_init(struct Effect *self);
void level99_effect_process(struct Effect *self);
void level99_effect_render(struct Effect *self);
void level99_effect_release(struct Effect *self);
void level99_effect_set_color(struct Effect *self, int r, int g, int b);
void level99_effect_set_texture(struct Effect *self, struct String texture);
static void level99_effect_destroy(struct Effect *effect);

static const struct EffectOps level99_effect_ops = {
    .set_submit = effect_set_submit,
    .init = level99_effect_init,
    .update_range = effect_update_range,
    .process = level99_effect_process,
    .render = level99_effect_render,
    .release = level99_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = effect_restart,
    .set_color = level99_effect_set_color,
    .set_texture = level99_effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = level99_effect_destroy,
    .fade_out = effect_fade_out,
};
#define SIN_DEG(d) (g_sinTable[d])
#define COS_DEG(d) (g_cosTable[d])
#define MATRIX_BYTES (12 * sizeof(float))

#define PART_EFFECT_TYPE(p) ((p)->effect_type)
struct Level99Particle {
    union {
        int        flare_phase;
        int        ring_tilt;
    };
    int            ring_sweep_of;
    int            ring_lift;
    float          ring_height;
};
PARTICLE_EFFECT_DATA_FITS(Level99Particle);
static __inline struct Level99Particle *level99_particle(const struct Particle *p)
{
    return (struct Level99Particle *)p->effect_data;
}
#define PART_INDEX(p)       ((p)->index)
#define RING_SPIN(p)        ((p)->roll)

static void set_texture(struct Particle *p, const char *name)
{
    struct String texture;
    string_init(&texture);
    string_assign_cstr(&texture, name);
    particle_set_texture(p, texture);
}

struct Level99Effect *level99_effect_construct(struct Level99Effect *self)
{
    effect_construct(&self->base, LEVEL99_TYPE);
    self->base.ops = &level99_effect_ops;
    self->ring_ground.x = self->ring_ground.y = self->ring_ground.z = 0.0f;
    self->ring_top.x = self->ring_top.y = self->ring_top.z = 0.0f;
    self->flares.first = self->flares.last = self->flares.end = 0;
    self->rings.first = self->rings.last = self->rings.end = 0;
    return self;
}

struct Effect *level99_effect_new(void)
{
    struct Level99Effect *self = mem_try_alloc(sizeof *self);

    return self ? &level99_effect_construct(self)->base : NULL;
}

/* Effect types created as Level99Effect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow level99_effect_type_rows[] = {
    EFFECT_TYPE(0x0c8, level99_effect_new),
};
const struct EffectTypeTable level99_effect_types = EFFECT_TYPE_TABLE(level99_effect_type_rows);

void level99_effect_destruct(struct Level99Effect *self)
{
    self->base.ops = &level99_effect_ops;
    vec_free(&self->rings);
    vec_free(&self->flares);
    effect_destruct(&self->base);
}

static void level99_effect_destroy(struct Effect *effect)
{
    struct Level99Effect *self = (struct Level99Effect *)effect;
    level99_effect_destruct(self);
    mem_free(self);
}

void level99_effect_shape_ring(struct Level99Effect *self, struct Particle *p)
{
    int index, angle, tilt;
    float arch, scale, c, s, ground_x, ground_z, along, top_x, top_y, top_z;

    if (!p)
        return;
    index = PART_INDEX(p);
    arch = SIN_DEG(index * 9) * level99_particle(p)->ring_height;
    scale = p->scale;
    angle = (int)((float)((int)((float)level99_particle(p)->ring_sweep_of / 20.0f) * index) + RING_SPIN(p)) % 360;
    tilt = level99_particle(p)->ring_tilt;
    c = COS_DEG(angle);
    s = SIN_DEG(angle);
    ground_x = scale * c;
    ground_z = scale * s;
    along = COS_DEG(tilt) * arch;
    top_x = along * c + ground_x;
    top_y = 0.0f - SIN_DEG(tilt) * arch;
    top_z = along * s + ground_z;

    p->corners[1].x = top_x;
    p->corners[1].y = top_y;
    p->corners[1].z = top_z;
    p->corners[3].x = ground_x;
    p->corners[3].y = 0.0f;
    p->corners[3].z = ground_z;
    if (index != 0) {
        p->corners[0].x = self->ring_top.x;
        p->corners[0].y = self->ring_top.y;
        p->corners[0].z = self->ring_top.z;
        p->corners[2].x = self->ring_ground.x;
        p->corners[2].y = self->ring_ground.y;
        p->corners[2].z = self->ring_ground.z;
    }
    self->ring_ground.x = ground_x;
    self->ring_ground.y = 0.0f;
    self->ring_ground.z = ground_z;
    self->ring_top.x = top_x;
    self->ring_top.y = top_y;
    self->ring_top.z = top_z;
}

void level99_effect_spawn_rings(struct Level99Effect *self)
{
    int ring = 0, start = 0, tilt = 55;

    do {
        int quad;
        for (quad = 0; quad <= RING_QUADS - 1; quad++) {
            struct Particle *p = particle_new(self->base.pos, LEVEL99_LIST);
            float n;

            if (!p)
                continue;
            set_texture(p, "effect\\ring_blue.tga");
            PART_INDEX(p) = quad;
            PART_EFFECT_TYPE(p) = self->base.type;
            p->group = ring;
            p->owner_tag = 3;
            p->alpha = 120;
            particle_set_color(p, 100, 100, 255);
            n = (float)ring;
            RING_SPIN(p) = (float)start;
            level99_particle(p)->ring_sweep_of = RING_SWEEP;
            level99_particle(p)->ring_height = 15.0f - (n + n);
            p->scale = n * 0.2f + 3.9f;
            level99_particle(p)->ring_tilt = tilt;
            level99_particle(p)->ring_lift = 0;
            particle_quad_unit_corners(p->corners);
            vec_push(&self->rings, &p, sizeof p);
            level99_effect_shape_ring(self, p);
        }
        ring++;
        tilt -= 5;
        start += 90;
    } while (tilt > 40);
}

void level99_effect_spin_ring(struct Particle *p)
{
    float m[12];
    float spin;

    if (!p)
        return;
    spin = (float)(int)((float)p->group + RING_SPIN(p) + 3.0f);
    RING_SPIN(p) = spin;
    memcpy(m, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_rotate_y(m, DEG_TO_RAD(spin));
    memcpy(p->matrix, m, MATRIX_BYTES);
}

void level99_effect_spawn_flares(struct Level99Effect *self)
{
    int i;

    for (i = 0; i < FLARES; i++) {
        struct Particle *p = particle_new(self->base.pos, LEVEL99_LIST);
        if (!p)
            continue;
        set_texture(p, "effect\\pikapika2.bmp");
        PART_EFFECT_TYPE(p) = self->base.type;
        PART_INDEX(p) = i;
        p->group = 0;
        p->owner_tag = 1;
        p->alpha = 200;
        p->roll = (float)(i * 22);
        level99_particle(p)->flare_phase = i * 180;
        p->scale = 9.0f;
        particle_quad_unit_corners(p->corners);
        vec_push(&self->flares, &p, sizeof p);
    }
}

void level99_effect_update_flare(struct Particle *p)
{
    float turn[12], size[12], m[12];
    float s;
    int phase;

    if (!p)
        return;
    memcpy(turn, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_rotate_z(turn, DEG_TO_RAD(p->roll));
    mat_rotate_x(turn, 1.5707960f);
    phase = level99_particle(p)->flare_phase;
    phase += phase + 3 < 360 ? 3 : -357;
    level99_particle(p)->flare_phase = phase;
    memcpy(size, g_identityMatrix.m, MATRIX_BYTES);
    s = (SIN_DEG(phase) + 1.0f) * (p->scale * 0.1f) + p->scale;
    mat_set_scale(size, s, s, 1.0f);
    mat_multiply(m, size, turn);
    memcpy(p->matrix, m, MATRIX_BYTES);
}

void level99_render_ring_quad(struct Particle *p)
{
    struct ParticleRenderArgs args;
    int index;

    if (!p)
        return;
    index = PART_INDEX(p);
    if (index <= 0)
        return;
    args.facing_fixed = 0;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)(index - 1) / 21.0f;
    args.u_right = (float)index / 21.0f;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void level99_render_flare(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void level99_effect_init(struct Effect *effect)
{
    struct Level99Effect *self = (struct Level99Effect *)effect;
    level99_effect_spawn_flares(self);
    level99_effect_spawn_rings(self);
}

void level99_effect_process(struct Effect *effect)
{
    struct Level99Effect *self = (struct Level99Effect *)effect;
    struct Particle **it;

    for (it = self->flares.first; it != (struct Particle **)self->flares.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        level99_effect_update_flare(p);
        p->age++;
    }
    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        level99_effect_spin_ring(p);
        p->age++;
    }
}

void level99_effect_render(struct Effect *effect)
{
    struct Level99Effect *self = (struct Level99Effect *)effect;
    struct RenderObject *owner = effect_find_actor(self->base.owner_aid);
    struct Particle **it;

    if (owner && self->base.type == LEVEL99_TYPE) {
        if (is_effect_state_cloaking(actor_get_effect_state(owner)) || !owner->revealed || !owner->visible)
            return;
    }
    for (it = self->flares.first; it != (struct Particle **)self->flares.last; it++)
        level99_render_flare(*it);
    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++)
        level99_render_ring_quad(*it);
}

void level99_effect_release(struct Effect *effect)
{
    struct Level99Effect *self = (struct Level99Effect *)effect;
    struct Particle **it;
    for (it = self->flares.first; it != (struct Particle **)self->flares.last; it++)
        particle_free(*it);
    self->flares.last = self->flares.first;
    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++)
        particle_free(*it);
    self->rings.last = self->rings.first;
}

void level99_effect_set_color(struct Effect *effect, int r, int g, int b)
{
    struct Level99Effect *self = (struct Level99Effect *)effect;
    struct Particle **it;
    for (it = self->flares.first; it != (struct Particle **)self->flares.last; it++)
        particle_set_color(*it, (short)r, (short)g, (short)b);
    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++)
        particle_set_color(*it, (short)r, (short)g, (short)b);
}

void level99_effect_set_texture(struct Effect *effect, struct String texture)
{
    struct Level99Effect *self = (struct Level99Effect *)effect;
    struct Particle **it;
    for (it = self->flares.first; it != (struct Particle **)self->flares.last; it++) {
        struct String copy;
        string_copy(&copy, &texture);
        particle_set_texture(*it, copy);
    }
    for (it = self->rings.first; it != (struct Particle **)self->rings.last; it++) {
        struct String copy;
        string_copy(&copy, &texture);
        particle_set_texture(*it, copy);
    }
    string_free(&texture);
}
