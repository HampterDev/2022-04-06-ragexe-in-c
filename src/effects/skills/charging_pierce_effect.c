#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "render/texture_types.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_controls.h"
#include "render/render_api.h"
#include "actor/game_actor.h"
#include <stdint.h>

struct ChargingPierceEffect;
struct Effect;
struct String;
void charging_pierce_effect_init(struct Effect *self);
void charging_pierce_effect_process(struct Effect *self);
void charging_pierce_effect_render(struct Effect *self);
void charging_pierce_effect_release(struct Effect *self);
static void charging_pierce_effect_destroy(struct Effect *effect);

static const struct EffectOps charging_pierce_effect_ops = {
    .set_submit = effect_set_submit,
    .init = charging_pierce_effect_init,
    .update_range = effect_update_range,
    .process = charging_pierce_effect_process,
    .render = charging_pierce_effect_render,
    .release = charging_pierce_effect_release,
    .is_alive = effect_is_owner_alive,
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
    .destroy = charging_pierce_effect_destroy,
    .fade_out = effect_fade_out,
};

#define MAX_CHARGES 10
#define LIST_WORLD 5
#define LIST_ORB 0x101
#define CAST_STR "new_chargingpierce\\new_chargingpierce_cast\\new_chargingpierce_cast.str"
#define ORB_TEXTURE "effect\\new_chargingpierce\\cha_orb.bmp"

#define particle_owner_type(p) ((p)->effect_type)
#define particle_tag(p)        ((p)->owner_tag)
#define particle_col(p)        ((p)->index)
#define particle_row(p)        ((p)->group)

struct ChargingPierceEffect {
    struct Effect base;
    Vec3 slots[MAX_CHARGES];
    struct Particle *orbs[MAX_CHARGES];
    struct Ez2StrParticle *casts[MAX_CHARGES];
};

static Vec3 transform(const float *m, float x, float y, float z)
{
    Vec3 r;
    r.x = x * m[0] + y * m[3] + z * m[6] + m[9];
    r.y = x * m[1] + y * m[4] + z * m[7] + m[10];
    r.z = x * m[2] + y * m[5] + z * m[8] + m[11];
    return r;
}

static float marker_height(const struct ChargingPierceEffect *self, float base)
{
    float height = base;
    void *owner = effect_find_actor(self->base.owner_aid);
    if (owner)
        height = base - (float)(game_actor_get_mount_height(owner) ? 3 : 0);
    return height;
}

struct ChargingPierceEffect *charging_pierce_effect_construct(
    struct ChargingPierceEffect *self)
{
    int i;
    effect_construct(&self->base, -1);
    self->base.ops = &charging_pierce_effect_ops;
    for (i = 0; i < MAX_CHARGES; i++)
        self->slots[i].x = self->slots[i].y = self->slots[i].z = 0;
    return self;
}

struct Effect *charging_pierce_effect_new(void)
{
    struct ChargingPierceEffect *self = mem_try_alloc(sizeof *self);

    return self ? &charging_pierce_effect_construct(self)->base : NULL;
}

void charging_pierce_effect_destruct(struct ChargingPierceEffect *self)
{
    self->base.ops = &charging_pierce_effect_ops;
    effect_destruct(&self->base);
}

static void charging_pierce_effect_destroy(struct Effect *effect)
{
    struct ChargingPierceEffect *self = (struct ChargingPierceEffect *)effect;
    charging_pierce_effect_destruct(self);
    mem_free(self);
}

int charging_pierce_effect_count_charges(struct ChargingPierceEffect *self)
{
    int i, count = 0;
    for (i = 0; i < MAX_CHARGES; i++)
        if (self->orbs[i] || self->casts[i])
            count++;
    return count;
}

void charging_pierce_effect_add_orb(struct ChargingPierceEffect *self, int i)
{
    struct Particle *p;
    struct String name;

    if (self->orbs[i])
        return;
    p = particle_new(self->base.pos, LIST_ORB);
    particle_owner_type(p) = self->base.type;
    particle_col(p) = 1;
    particle_row(p) = 0;
    particle_tag(p) = 1;
    string_init(&name);
    string_assign_cstr(&name, ORB_TEXTURE);
    particle_set_texture(p, name);
    p->scale = 1.5f;
    p->depth_bias = 0.035f;
    particle_quad_unit_corners(p->corners);
    self->orbs[i] = p;
}

void charging_pierce_effect_set_charges(struct Effect *effect,
                                                  uint64_t count)
{
    struct ChargingPierceEffect *self = (struct ChargingPierceEffect *)effect;
    int i;

    if (count > MAX_CHARGES)
        return;
    if ((uint64_t)(int64_t)charging_pierce_effect_count_charges(self) > count)
        charging_pierce_effect_release(&self->base);
    i = charging_pierce_effect_count_charges(self);
    if ((uint64_t)(int64_t)i >= count)
        return;
    for (; (uint64_t)(int64_t)i < count; i++) {
        if (!self->orbs[i] && !self->casts[i]) {
            struct Ez2StrParticle *cast = ez2str_particle_new(self->base.pos, LIST_WORLD);
            ez2_str_particle_load(cast, CAST_STR);
            self->casts[i] = cast;
        }
    }
}

void charging_pierce_effect_init(struct Effect *effect)
{
    struct ChargingPierceEffect *self = (struct ChargingPierceEffect *)effect;
    static const float row_x[5] = { -5.0f, -2.5f, -0.0f, 2.5f, 5.0f };
    int i;

    self->base.has_instance_key = 1;
    for (i = 0; i < 5; i++) {
        self->slots[i].x = row_x[i];
        self->slots[i].y = -2.5f;
        self->slots[i].z = 0;
    }
    for (i = 0; i < 5; i++) {
        self->slots[5 + i].x = i == 2 ? 0.0f : row_x[i];
        self->slots[5 + i].y = 0;
        self->slots[5 + i].z = 0;
    }
    for (i = 0; i < MAX_CHARGES; i++) {
        self->orbs[i] = 0;
        self->casts[i] = 0;
    }
}

void charging_pierce_effect_release(struct Effect *effect)
{
    struct ChargingPierceEffect *self = (struct ChargingPierceEffect *)effect;
    int i;
    for (i = 0; i < MAX_CHARGES; i++) {
        if (self->orbs[i]) {
            particle_free(self->orbs[i]);
            self->orbs[i] = 0;
        }
        if (self->casts[i]) {
            particle_free(&self->casts[i]->base);
            self->casts[i] = 0;
        }
    }
}

void charging_pierce_effect_place_cast(struct ChargingPierceEffect *self,
                                                 struct Ez2StrParticle *cast, int i)
{
    float view[12], to_world[12];
    float height;
    Vec3 pos, eye, zero;

    if (!cast)
        return;
    height = marker_height(self, -13.0f);
    pos = self->base.pos;
    if (!game_copy_view_matrix(view))
        return;
    mat_invert(to_world, view);
    eye = transform(view, pos.x, pos.y, pos.z);
    eye.x = eye.x + self->slots[i].x;
    eye.y = eye.y + (self->slots[i].y + height);
    cast->base.pos = transform(to_world, eye.x, eye.y, eye.z);
    zero.x = zero.y = zero.z = 0;
    ez2_str_particle_process(cast, zero);
    cast->base.age++;
}

void charging_pierce_effect_place_orb(struct ChargingPierceEffect *self,
                                                struct Particle *orb, int i)
{
    float turn[12], scale[12], orb_turn[12], move[12], scaled[12];
    float height;
    Vec3 offset;

    if (!orb)
        return;
    height = marker_height(self, -10.0f);
    mat_set_rotate_x(turn, degrees_to_radians(game_camera_pitch()));
    mat_rotate_y(turn, degrees_to_radians(game_camera_heading()));
    offset = transform(turn, self->slots[i].x + 0.0f, self->slots[i].y + height, self->slots[i].z + 0.0f);
    mat_set_scale(scale, orb->scale, orb->scale, 1.0f);
    mat_set_rotate_x(orb_turn, degrees_to_radians(game_camera_pitch()));
    mat_rotate_y(orb_turn, degrees_to_radians(game_camera_heading()));
    mat_set_translation(move, self->base.pos.x + offset.x, self->base.pos.y + offset.y, self->base.pos.z + offset.z);
    mat_multiply(scaled, scale, orb_turn);
    mat_multiply(orb->matrix, scaled, move);
}

void charging_pierce_effect_process(struct Effect *effect)
{
    struct ChargingPierceEffect *self = (struct ChargingPierceEffect *)effect;
    int i;

    for (i = 0; i < MAX_CHARGES; i++) {
        struct Ez2StrParticle *cast;
        float length;

        if (self->orbs[i])
            charging_pierce_effect_place_orb(self, self->orbs[i], i);
        cast = self->casts[i];
        if (!cast)
            continue;
        length = cast->base.life_seconds;
        if (length != 0.0f && !(length * 60.0f > (float)cast->base.age)) {
            particle_free(&cast->base);
            self->casts[i] = 0;
            if (!self->orbs[i])
                charging_pierce_effect_add_orb(self, i);
            continue;
        }
        charging_pierce_effect_place_cast(self, cast, i);
    }
}

void charging_pierce_render_cast(struct Ez2StrParticle *cast)
{
    if (!cast)
        return;
    ez2_str_particle_render_current(cast);
}

void charging_pierce_render_orb(struct Particle *orb)
{
    const struct Texture *tex;
    float u_left, u_right, v_top, v_bottom;

    if (!orb)
        return;
    tex = orb->texture;
    u_right = ((float)(unsigned)tex->reqWidth + g_texelOffsetU) / (float)tex->base.width;
    u_left = g_texelOffsetU / (float)tex->base.width;
    v_bottom = g_texelOffsetV / (float)tex->base.height;
    v_top = ((float)(unsigned)tex->reqHeight + g_texelOffsetV) / (float)tex->base.height;
    particle_render(orb, 1, v_top, v_bottom, u_left, u_right, 1);
}

void charging_pierce_effect_render(struct Effect *effect)
{
    struct ChargingPierceEffect *self = (struct ChargingPierceEffect *)effect;
    int i;
    for (i = 0; i < MAX_CHARGES; i++) {
        charging_pierce_render_orb(self->orbs[i]);
        charging_pierce_render_cast(self->casts[i]);
    }
}
