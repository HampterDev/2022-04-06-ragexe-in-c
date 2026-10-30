#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/skills/emitter_effect.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_math.h"
#include "std/vec.h"

#define EMITTER_TYPE 0x3ce
#define FADE_FRAMES 60.0f

#define VTBL(self) ((const struct EmitterEffectOps *)(self)->base.ops)
#define EMIT_PAUSED(self) ((self)->base.paused)

PARTICLE_EFFECT_DATA_FITS(EmitterParticle);
static __inline struct EmitterParticle *emitter_particle(const struct Particle *p)
{
    return (struct EmitterParticle *)p->effect_data;
}
#define PARTICLE_LIFE(p) ((p)->life_seconds)
#define PARTICLE_VELOCITY(p) ((p)->velocity)
#define PARTICLE_DRIFT(p) ((p)->offset)

struct Effect;
struct EmitterEffect;
struct Particle;
struct String;
static void emitter_effect_destroy(struct Effect *effect);

static const struct EmitterEffectOps emitter_effect_ops = {
    .base = {
        .set_submit = effect_set_submit,
        .init = emitter_effect_init,
        .update_range = emitter_effect_update_range,
        .process = emitter_effect_process,
        .render = emitter_effect_render,
        .release = emitter_effect_release,
        .is_alive = emitter_effect_is_alive,
        .restart = emitter_effect_restart,
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
        .destroy = emitter_effect_destroy,
        .fade_out = effect_fade_out,
    },
    .init_particle = emitter_effect_init_particle,
    .process_particle = emitter_effect_process_particle,
};

#define MATRIX_BYTES (12 * sizeof(float))

static void particle_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

struct EmitterEffect *emitter_effect_construct(struct EmitterEffect *self)
{
    effect_construct(&self->base, EMITTER_TYPE);
    self->base.ops = &emitter_effect_ops.base;
    self->velocity_min.x = -2.0f;
    self->velocity_min.y = 2.0f;
    self->velocity_min.z = -2.0f;
    self->velocity_max.x = 2.0f;
    self->velocity_max.y = -1.0f;
    self->velocity_max.z = 2.0f;
    self->acceleration.x = 0.7f;
    self->acceleration.y = -2.0f;
    self->acceleration.z = 1.0f;
    self->spawn_box.x = -1.0f;
    self->spawn_box.y = 2.0f;
    self->spawn_box.z = -1.0f;
    self->emit_carry = 0.0f;
    self->frame_time = 0.016666668f;
    self->size_min = 3.0f;
    self->size_max = 8.0f;
    self->life_min = 5.0f;
    self->life_max = 10.0f;
    self->rate_min = 20.0f;
    self->rate_max = 100.0f;
    self->scale_x = 1.0f;
    self->scale_y = 1.0f;
    self->r = 0xff;
    self->g = 0xff;
    self->b = 0xff;
    self->alpha = 0xff;
    string_init(&self->texture);
    string_assign_cstr(&self->texture, "smoke2.bmp");
    self->src_blend = 5;
    self->dest_blend = 2;
    self->count = 40;
    self->particle_speed = 1.0f;
    self->with_depth = 1;
    self->fixed_facing = 0;
    self->turn.x = self->turn.y = self->turn.z = 0;
    self->keep_particles = 0;
    self->particles.first = self->particles.last = self->particles.end = 0;
    particle_slots_construct(&self->slots);
    self->max_frames = -1;
    return self;
}

struct Effect *emitter_effect_new(void)
{
    struct EmitterEffect *self = mem_try_alloc(sizeof *self);

    return self ? &emitter_effect_construct(self)->base : NULL;
}

void emitter_effect_destruct(struct EmitterEffect *self)
{
    self->base.ops = &emitter_effect_ops.base;
    particle_slots_destroy(&self->slots);
    vec_free(&self->particles);
    string_free(&self->texture);
    effect_destruct(&self->base);
}

static void emitter_effect_destroy(struct Effect *effect)
{
    struct EmitterEffect *self = (struct EmitterEffect *)effect;
    emitter_effect_destruct(self);
    mem_free(self);
}

int emitter_effect_emit_count(struct EmitterEffect *self)
{
    float rate = effect_random_tenths(self->rate_min, self->rate_max);
    float want = self->frame_time * (float)(int)rate;
    int n = (int)want;
    float carry = want - (float)n + self->emit_carry;

    self->emit_carry = carry;
    if (carry > 1.0f) {
        int whole = (int)carry;
        n += whole;
        self->emit_carry = carry - (float)whole;
    }
    return n;
}

void emitter_effect_emit(struct EmitterEffect *self)
{
    int n = emitter_effect_emit_count(self);

    for (; n > 0; n--) {
        struct Particle *p = particle_slots_acquire(&self->slots);
        if (!p)
            continue;
        VTBL(self)->init_particle(self, p);
        if (self->keep_particles == 1)
            p->alpha = self->alpha;
    }
}

void emitter_effect_init_particle(struct EmitterEffect *self, struct Particle *p)
{
    float x, y, z;

    p->pos = self->base.pos;
    particle_texture(p, string_cstr(&self->texture));
    PARTICLE_LIFE(p) = effect_random_tenths(self->life_min, self->life_max);
    p->scale = effect_random_tenths(self->size_min, self->size_max);
    PARTICLE_VELOCITY(p).x = effect_random_tenths(self->velocity_min.x, self->velocity_max.x);
    PARTICLE_VELOCITY(p).y = effect_random_tenths(self->velocity_min.y, self->velocity_max.y);
    PARTICLE_VELOCITY(p).z = effect_random_tenths(self->velocity_min.z, self->velocity_max.z);
    x = effect_random_tenths(-self->spawn_box.x, self->spawn_box.x) + 0.0f;
    y = effect_random_tenths(-self->spawn_box.y, self->spawn_box.y) - 10.0f + 0.0f;
    z = effect_random_tenths(-self->spawn_box.z, self->spawn_box.z) + 0.0f;
    PARTICLE_DRIFT(p).x = x;
    PARTICLE_DRIFT(p).y = y;
    PARTICLE_DRIFT(p).z = z;
    p->alpha = 0;
    p->age = 0;
    if (self->src_blend != -1)
        p->src_blend = self->src_blend;
    if (self->dest_blend != -1)
        p->dest_blend = self->dest_blend;
}

void emitter_effect_update_particle(struct EmitterEffect *self, struct Particle *p)
{
    float m[12];
    float speed, dt;
    Vec3 was;
    int age;

    if (!p)
        return;
    age = p->age;
    particle_set_color(p, (short)self->r, (short)self->g, (short)self->b);
    if (self->keep_particles == 0) {
        float left = PARTICLE_LIFE(p) * FADE_FRAMES - (float)age;
        float step = self->frame_time * 255.0f;
        float alpha = (float)p->alpha;
        int a;
        if (FADE_FRAMES > left) {
            a = (int)(alpha - step);
            p->alpha = a < 0 ? 0 : a;
        } else {
            a = (int)(step + alpha);
            p->alpha = a > 0xff ? 0xff : a;
        }
    }

    speed = emitter_particle(p)->speed;
    dt = self->frame_time;
    was = p->pos;
    PARTICLE_DRIFT(p).y = PARTICLE_VELOCITY(p).y * speed * dt + PARTICLE_DRIFT(p).y;
    PARTICLE_DRIFT(p).z = PARTICLE_VELOCITY(p).z * speed * dt + PARTICLE_DRIFT(p).z;
    PARTICLE_DRIFT(p).x = speed * PARTICLE_VELOCITY(p).x * dt + PARTICLE_DRIFT(p).x;
    p->pos.y = PARTICLE_DRIFT(p).y + was.y;
    p->pos.z = PARTICLE_DRIFT(p).z + was.z;
    p->pos.x = PARTICLE_DRIFT(p).x + was.x;
    PARTICLE_VELOCITY(p).x = dt * self->acceleration.x + PARTICLE_VELOCITY(p).x;
    PARTICLE_VELOCITY(p).y = dt * self->acceleration.y + PARTICLE_VELOCITY(p).y;
    PARTICLE_VELOCITY(p).z = dt * self->acceleration.z + PARTICLE_VELOCITY(p).z;

    if (self->fixed_facing == 1) {
        Vec3 pos;
        mat_set_scale(m, p->scale * self->scale_x, p->scale * self->scale_y, 1.0f);
        mat_rotate_x(m, deg_to_rad((int)self->turn.x));
        mat_rotate_y(m, deg_to_rad((int)self->turn.y));
        mat_rotate_z(m, deg_to_rad((int)self->turn.z));
        pos = p->pos;
        mat_translate(m, &pos);
        memcpy(p->matrix, m, MATRIX_BYTES);
    } else {
        float tilt[12], scale[12];

        memcpy(tilt, g_identityMatrix.m, MATRIX_BYTES);
        mat_set_rotate_x(tilt, game_camera_pitch() / 180.0f * 3.141592f);
        memcpy(scale, g_identityMatrix.m, MATRIX_BYTES);
        mat_set_scale(scale, p->scale * self->scale_x, p->scale * self->scale_y, 1.0f);
        mat_multiply(m, scale, tilt);
        memcpy(p->matrix, m, MATRIX_BYTES);
    }
}

void emitter_effect_process_particle(struct EmitterEffect *self, struct Particle *p)
{
    p->pos = self->base.pos;
    if (particle_slots_is_used(&self->slots, p) == 1) {
        float life = PARTICLE_LIFE(p);
        if (!(0.0f == life) && !(life * FADE_FRAMES > (float)p->age) && self->keep_particles == 0) {
            particle_slots_release(&self->slots, p);
            p->age++;
            return;
        }
        emitter_effect_update_particle(self, p);
    }
    p->age++;
}

void emitter_effect_init(struct Effect *effect)
{
    struct EmitterEffect *self = (struct EmitterEffect *)effect;
    int i;
    for (i = 0; i < self->count; i++) {
        struct Particle *p = particle_new(self->base.pos, 5);
        if (!p)
            continue;
        emitter_particle(p)->speed = self->particle_speed;
        VTBL(self)->init_particle(self, p);
        particle_slots_add(&self->slots, p);
        vec_push(&self->particles, &p, sizeof p);
    }
}

void emitter_effect_update_range(struct Effect *effect)
{
    struct EmitterEffect *self = (struct EmitterEffect *)effect;
    unsigned char paused;

    if (effect_is_in_range(&self->base) == 1) {
        self->base.out_of_range = 0;
        paused = 0;
    } else {
        paused = 1;
    }
    EMIT_PAUSED(self) = paused;
    if (paused == 1 && self->slots.in_use == 0)
        self->base.out_of_range = paused;
}

void emitter_effect_process(struct Effect *effect)
{
    struct EmitterEffect *self = (struct EmitterEffect *)effect;
    struct Particle **it;

    if (!EMIT_PAUSED(self))
        emitter_effect_emit(self);
    for (it = self->particles.first; it != (struct Particle **)self->particles.last; it++)
        VTBL(self)->process_particle(self, *it);
}

void emitter_effect_render_particle(struct EmitterEffect *self, struct Particle *p)
{
    struct ParticleRenderArgs args;

    if (!p)
        return;
    args.facing_fixed = self->fixed_facing;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = 0.0f;
    args.u_right = 1.0f;
    args.map_chain_flag = 0;
    args.with_depth = self->with_depth;
    particle_render_with(p, &args);
}

void emitter_effect_render(struct Effect *effect)
{
    struct EmitterEffect *self = (struct EmitterEffect *)effect;
    struct Particle **it;
    for (it = self->particles.first; it != (struct Particle **)self->particles.last; it++)
        emitter_effect_render_particle(self, *it);
}

void emitter_effect_release(struct Effect *effect)
{
    struct EmitterEffect *self = (struct EmitterEffect *)effect;
    struct Particle **it;
    for (it = self->particles.first; it != (struct Particle **)self->particles.last; it++)
        particle_free(*it);
    self->particles.last = self->particles.first;
    particle_slots_clear(&self->slots);
}

unsigned char emitter_effect_is_alive(struct Effect *effect)
{
    struct EmitterEffect *self = (struct EmitterEffect *)effect;
    if (self->max_frames > 0 && self->base.frame > self->max_frames)
        return 0;
    return effect_is_owner_alive(&self->base);
}

void emitter_effect_restart(struct Effect *effect)
{
    struct EmitterEffect *self = (struct EmitterEffect *)effect;
    struct Particle **it;

    if (self->keep_particles == 1 && 0.0f == self->rate_min && 0.0f == self->rate_max)
        particle_slots_use_all(&self->slots);
    for (it = self->particles.first; it != (struct Particle **)self->particles.last; it++)
        VTBL(self)->init_particle(self, *it);
}

void emitter_effect_set_count(struct EmitterEffect *self, int count)
{
    if (self->count != count) {
        self->count = count;
        emitter_effect_release(&self->base);
        self->base.ops->init(&self->base);
    }
}

void emitter_effect_set_particle_speed(struct EmitterEffect *self, float speed)
{
    struct Particle **it;

    if (self->particle_speed == speed)
        return;
    self->particle_speed = speed;
    for (it = self->particles.first; it != (struct Particle **)self->particles.last; it++)
        emitter_particle(*it)->speed = self->particle_speed;
}
