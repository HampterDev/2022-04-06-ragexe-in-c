#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include "effects/durations.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct Effect;
struct FriggSongEffect;
struct String;
void frigg_song_effect_init(struct Effect *self);
void frigg_song_effect_process(struct Effect *self);
void frigg_song_effect_render(struct Effect *self);
void frigg_song_effect_release(struct Effect *self);
void frigg_song_effect_after_init(struct Effect *self);
static void frigg_song_effect_destroy(struct Effect *effect);

static const struct EffectOps frigg_song_effect_ops = {
    .set_submit = effect_set_submit,
    .init = frigg_song_effect_init,
    .update_range = effect_update_range,
    .process = frigg_song_effect_process,
    .render = frigg_song_effect_render,
    .release = frigg_song_effect_release,
    .is_alive = effect_is_within_duration,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = frigg_song_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = frigg_song_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LIST_WORLD 5
#define SEGMENTS 21

struct FriggSongParticle {
    int elevation;
    int arc;
    float rise;
    float wall;
};
PARTICLE_EFFECT_DATA_FITS(FriggSongParticle);
static __inline struct FriggSongParticle *frigg_song_particle(struct Particle *p)
{
    return (struct FriggSongParticle *)p->effect_data;
}
#define particle_owner_type(p) ((p)->effect_type)
#define particle_tag(p)        ((p)->owner_tag)
#define particle_segment(p)    ((p)->index)
#define particle_ring(p)       ((p)->group)
#define particle_elevation(p)  (frigg_song_particle(p)->elevation)
#define particle_arc(p)        (frigg_song_particle(p)->arc)
#define particle_rise(p)       (frigg_song_particle(p)->rise)
#define particle_wall(p)       (frigg_song_particle(p)->wall)

struct FriggSongEffect {
    struct Effect base;
    Vec3 prev_foot;
    Vec3 prev_top;
    struct Vec particles;
};

#define EACH_PARTICLE(self, it) \
    for (it = (struct Particle **)(self)->particles.first; it != (struct Particle **)(self)->particles.last; it++)

static void set_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

struct FriggSongEffect *frigg_song_effect_construct(struct FriggSongEffect *self)
{
    effect_construct(&self->base, 0x423);
    self->base.ops = &frigg_song_effect_ops;
    self->prev_foot.x = self->prev_foot.y = self->prev_foot.z = 0;
    self->prev_top.x = self->prev_top.y = self->prev_top.z = 0;
    self->particles.first = self->particles.last = self->particles.end = 0;
    self->base.duration = g_effectDurations[0x423];
    return self;
}

struct Effect *frigg_song_effect_new(void)
{
    struct FriggSongEffect *self = mem_try_alloc(sizeof *self);

    return self ? &frigg_song_effect_construct(self)->base : NULL;
}

void frigg_song_effect_destruct(struct FriggSongEffect *self)
{
    self->base.ops = &frigg_song_effect_ops;
    vec_free(&self->particles);
    effect_destruct(&self->base);
}

static void frigg_song_effect_destroy(struct Effect *effect)
{
    struct FriggSongEffect *self = (struct FriggSongEffect *)effect;
    frigg_song_effect_destruct(self);
    mem_free(self);
}

void frigg_song_effect_shape_segment(struct FriggSongEffect *self, struct Particle *p)
{
    int segment, angle, elevation;
    float wall, radius, cos_a, sin_a, lean;
    Vec3 foot, top;

    if (!p)
        return;
    segment = particle_segment(p);
    wall = particle_wall(p);
    angle = (int)((float)((int)((float)particle_arc(p) / 20.0f) * segment) + p->roll) % 360;
    if (segment == 20)
        angle = (int)p->roll;
    elevation = particle_elevation(p);
    radius = p->scale;
    cos_a = g_cosTable[angle];
    sin_a = g_sinTable[angle];
    lean = g_cosTable[elevation] * wall;
    foot.x = radius * cos_a;
    foot.y = 0;
    foot.z = radius * sin_a;
    top.x = lean * cos_a + foot.x;
    top.y = 0.0f - g_sinTable[elevation] * wall;
    top.z = lean * sin_a + foot.z;
    if (segment > 0 && segment < SEGMENTS) {
        p->corners[1].x = top.x;  p->corners[1].y = top.y;  p->corners[1].z = top.z;
        p->corners[3].x = foot.x; p->corners[3].y = 0;      p->corners[3].z = foot.z;
        p->corners[0].x = self->prev_top.x;  p->corners[0].y = self->prev_top.y;  p->corners[0].z = self->prev_top.z;
        p->corners[2].x = self->prev_foot.x; p->corners[2].y = self->prev_foot.y; p->corners[2].z = self->prev_foot.z;
    }
    self->prev_foot = foot;
    self->prev_top = top;
}

void frigg_song_effect_setup_particle(struct FriggSongEffect *self, struct Particle *p)
{
    int ring;

    if (!p)
        return;
    p->alpha = 0x32;
    particle_set_color(p, 0x32, 0xc8, 0x32);
    ring = particle_ring(p);
    particle_arc(p) = 360;
    particle_wall(p) = 120.0f;
    particle_elevation(p) = 90;
    p->roll = (float)(ring * 10);
    p->scale = (float)ring * 0.5f + 2.5f;
    particle_rise(p) = 0;
    p->age = particle_ring(p);
    frigg_song_effect_shape_segment(self, p);
}

void frigg_song_effect_init(struct Effect *effect)
{
    struct FriggSongEffect *self = (struct FriggSongEffect *)effect;
    int segment;

    for (segment = 0; segment <= SEGMENTS; segment++) {
        struct Particle *p = particle_new(self->base.pos, LIST_WORLD);
        if (!p)
            continue;
        set_texture(p, "effect\\ring_blue.tga");
        particle_owner_type(p) = self->base.type;
        particle_segment(p) = segment;
        particle_ring(p) = 0;
        particle_tag(p) = 1;
        frigg_song_effect_setup_particle(self, p);
        vec_push(&self->particles, &p, sizeof(struct Particle *));
    }
}

void frigg_song_effect_after_init(struct Effect *effect)
{
    effect_add_sound_cue(effect, SOUND_CUE_WAVE, "effect\\wm_frigg_song.wav", 0, 0.0f);
}

void frigg_song_effect_release(struct Effect *effect)
{
    struct FriggSongEffect *self = (struct FriggSongEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it)
        particle_free(*it);
    self->particles.last = self->particles.first;
}

void frigg_song_raise_wall(struct Particle *p)
{
    float turn[12], stretch[12];
    int age, heading;
    float rise;

    if (!p)
        return;
    age = p->age;
    heading = (int)(p->roll - (float)age);
    if (age < 20) {
        rise = 1.0f;
    } else if (age <= 140) {
        float v = g_sinTable[age - 20] * particle_wall(p);
        rise = 0.0f > v ? 0.0f : v;
    } else {
        float wall = particle_wall(p);
        float v = (wall - 0.5f) - wall * 0.01f;
        rise = 0.0f > v ? 0.0f : v;
        particle_wall(p) = rise;
    }
    particle_rise(p) = rise;
    mat_set_rotate_y(turn, (float)heading / g_halfTurnDegrees * g_pi);
    mat_set_scale(stretch, 1.0f, particle_rise(p) / 100.0f * 1.1f, 1.0f);
    mat_multiply(p->matrix, turn, stretch);
}

void frigg_song_effect_process(struct Effect *effect)
{
    struct FriggSongEffect *self = (struct FriggSongEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        frigg_song_raise_wall(p);
        p->age++;
    }
}

void frigg_song_render_segment(struct Particle *p)
{
    struct ParticleRenderArgs args;
    int segment;

    if (!p)
        return;
    segment = particle_segment(p);
    args.facing_fixed = 0;
    args.v_top = 0;
    args.v_bottom = 1.0f;
    args.u_left = (float)(segment - 1) / 21.0f;
    args.u_right = (float)segment / 21.0f;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void frigg_song_effect_render(struct Effect *effect)
{
    struct FriggSongEffect *self = (struct FriggSongEffect *)effect;
    struct Particle **it;
    EACH_PARTICLE(self, it)
        frigg_song_render_segment(*it);
}
