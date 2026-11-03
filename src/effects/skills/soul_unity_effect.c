#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/render_object.h"
#include "render/render_api.h"
#include "std/list.h"

#define EFFECT_SOUL_UNITY 0x4c6

struct SoulUnityEffect {
    struct Effect base;
    struct List particles;
};

#define PARTICLE_EFFECT_TYPE(p) ((p)->effect_type)
#define PARTICLE_OWNER_TAG(p) ((p)->owner_tag)
#define PARTICLE_INDEX(p) ((p)->index)
#define PARTICLE_GROUP(p) ((p)->group)

#define TEXTURE_WIDTH(t) (((const struct Texture *)(t))->base.width)
#define TEXTURE_HEIGHT(t) (((const struct Texture *)(t))->base.height)
#define TEXTURE_IMAGE_WIDTH(t) ((unsigned)((const struct Texture *)(t))->reqWidth)
#define TEXTURE_IMAGE_HEIGHT(t) ((unsigned)((const struct Texture *)(t))->reqHeight)

#define ACTOR_MOUNTED(a) ((const struct GameActor *)(a))->mounted

struct String;
void soul_unity_effect_init(struct Effect *self);
void soul_unity_effect_process(struct Effect *self);
void soul_unity_effect_render(struct Effect *self);
void soul_unity_effect_release(struct Effect *self);
static void soul_unity_effect_destroy(struct Effect *effect);

static const struct EffectOps soul_unity_effect_ops = {
    .set_submit = effect_set_submit,
    .init = soul_unity_effect_init,
    .update_range = effect_update_range,
    .process = soul_unity_effect_process,
    .render = soul_unity_effect_render,
    .release = soul_unity_effect_release,
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
    .destroy = soul_unity_effect_destroy,
    .fade_out = effect_fade_out,
};

static float camera_tilt(void)
{
    return game_camera_pitch();
}
static float camera_turn(void)
{
    return game_camera_heading();
}

static void camera_rotation(float m[12])
{
    mat_set_rotate_x(m, degrees_to_radians(camera_tilt()));
    mat_rotate_y(m, degrees_to_radians(camera_turn()));
}

static Vec3 transform(const float *m, Vec3 v)
{
    Vec3 out;
    out.x = v.x * m[0] + v.y * m[3] + v.z * m[6] + m[9];
    out.y = v.x * m[1] + v.y * m[4] + v.z * m[7] + m[10];
    out.z = v.x * m[2] + v.y * m[5] + v.z * m[8] + m[11];
    return out;
}

struct SoulUnityEffect *soul_unity_effect_construct(struct SoulUnityEffect *self)
{
    effect_construct(&self->base, EFFECT_SOUL_UNITY);
    self->base.ops = &soul_unity_effect_ops;
    list_init(&self->particles);
    return self;
}

struct Effect *soul_unity_effect_new(void)
{
    struct SoulUnityEffect *self = mem_try_alloc(sizeof *self);

    return self ? &soul_unity_effect_construct(self)->base : NULL;
}

/* Effect types created as SoulUnityEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow soul_unity_effect_type_rows[] = {
    EFFECT_TYPE(0x4c6, soul_unity_effect_new),
};
const struct EffectTypeTable soul_unity_effect_types = EFFECT_TYPE_TABLE(soul_unity_effect_type_rows);

void soul_unity_effect_destruct(struct SoulUnityEffect *self)
{
    self->base.ops = &soul_unity_effect_ops;
    list_free(&self->particles, NULL);
    effect_destruct(&self->base);
}

static void soul_unity_effect_destroy(struct Effect *effect)
{
    struct SoulUnityEffect *self = (struct SoulUnityEffect *)effect;
    soul_unity_effect_destruct(self);
    mem_free(self);
}

void soul_unity_effect_init(struct Effect *effect)
{
    struct SoulUnityEffect *self = (struct SoulUnityEffect *)effect;
    struct Particle *p = particle_new(self->base.pos, 0);
    struct String name;

    if (!p)
        return;
    string_init(&name);
    string_assign(&name, "effect\\soul_unity\\soul_unity.bmp", sizeof("effect\\soul_unity\\soul_unity.bmp") - 1);
    particle_set_texture(p, name);
    PARTICLE_EFFECT_TYPE(p) = self->base.type;
    PARTICLE_INDEX(p) = 1;
    PARTICLE_GROUP(p) = 0;
    PARTICLE_OWNER_TAG(p) = 1;
    p->scale = 2.5f;
    p->depth_bias = 0.035f;
    particle_quad_unit_corners(p->corners);
    list_push_back(&self->particles, &p, sizeof p);
}

void soul_unity_effect_process_particle(struct SoulUnityEffect *self, struct Particle *p)
{
    float rot[12], scale[12], turn[12], move[12], tmp[12], world[12];
    Vec3 up, at;
    void *owner;

    if (!p)
        return;
    up.x = 0.0f;
    up.y = -15.0f;
    up.z = 0.0f;
    owner = effect_find_actor(self->base.owner_aid);
    if (owner && ACTOR_MOUNTED(owner))
        up.y = -20.0f;
    camera_rotation(rot);
    up = transform(rot, up);
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    camera_rotation(turn);
    at = self->base.pos;
    mat_set_translation(move, at.x + up.x, at.y + up.y, at.z + up.z);
    mat_multiply(tmp, scale, turn);
    mat_multiply(world, tmp, move);
    memcpy(p->matrix, world, sizeof(world));
}

void soul_unity_effect_process(struct Effect *effect)
{
    struct SoulUnityEffect *self = (struct SoulUnityEffect *)effect;
    struct ListNode *node;
    for (node = self->particles.head->next; node != self->particles.head; node = node->next)
        soul_unity_effect_process_particle(self, *LIST_VALUE(node, struct Particle *));
}

void soul_unity_effect_render_particle(struct Particle *p)
{
    const void *tex;
    float w, h;

    if (!p || !p->texture)
        return;
    tex = p->texture;
    w = (float)TEXTURE_WIDTH(tex);
    h = (float)TEXTURE_HEIGHT(tex);
    particle_render(p, 1, ((float)(double)TEXTURE_IMAGE_HEIGHT(tex) + g_texelOffsetV) / h, g_texelOffsetV / h,
                      g_texelOffsetU / w, ((float)(double)TEXTURE_IMAGE_WIDTH(tex) + g_texelOffsetU) / w, 1);
}

void soul_unity_effect_render(struct Effect *effect)
{
    struct SoulUnityEffect *self = (struct SoulUnityEffect *)effect;
    struct ListNode *node;
    for (node = self->particles.head->next; node != self->particles.head; node = node->next)
        soul_unity_effect_render_particle(*LIST_VALUE(node, struct Particle *));
}

void soul_unity_effect_release(struct Effect *effect)
{
    struct SoulUnityEffect *self = (struct SoulUnityEffect *)effect;
    struct ListNode *node;

    for (node = self->particles.head->next; node != self->particles.head; node = node->next)
        particle_free(*LIST_VALUE(node, struct Particle *));
    list_clear(&self->particles, NULL);
}
