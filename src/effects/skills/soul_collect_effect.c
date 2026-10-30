#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "render/render_api.h"
#include "std/list.h"

struct SoulCollectEffect {
    struct Effect base;
    struct List particles;
};

#define EFFECT_LEVEL(e) ((e)->level)
#define EFFECT_SPIN(e) ((e)->params.rise_angle)

#define PARTICLE_EFFECT_TYPE(p) ((p)->effect_type)
#define PARTICLE_OWNER_TAG(p) ((p)->owner_tag)
#define PARTICLE_SOUL(p) ((p)->index)
#define PARTICLE_CORE(p) ((p)->group)

#define TEXTURE_WIDTH(t) (((const struct Texture *)(t))->base.width)
#define TEXTURE_HEIGHT(t) (((const struct Texture *)(t))->base.height)
#define TEXTURE_IMAGE_WIDTH(t) ((unsigned)((const struct Texture *)(t))->reqWidth)
#define TEXTURE_IMAGE_HEIGHT(t) ((unsigned)((const struct Texture *)(t))->reqHeight)

#define SOULS_SHOWN 5

struct String;
void soul_collect_effect_init(struct Effect *self);
void soul_collect_effect_process(struct Effect *self);
void soul_collect_effect_render(struct Effect *self);
void soul_collect_effect_release(struct Effect *self);
static void soul_collect_effect_destroy(struct Effect *effect);

static const struct EffectOps soul_collect_effect_ops = {
    .set_submit = effect_set_submit,
    .init = soul_collect_effect_init,
    .update_range = effect_update_range,
    .process = soul_collect_effect_process,
    .render = soul_collect_effect_render,
    .release = soul_collect_effect_release,
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
    .destroy = soul_collect_effect_destroy,
    .fade_out = effect_fade_out,
};

static void free_last_particle(struct List *list)
{
    struct ListNode *node = list->head->prev;
    particle_free(*LIST_VALUE(node, struct Particle *));
    list_erase(list, node, NULL);
}

struct SoulCollectEffect *soul_collect_effect_construct(struct SoulCollectEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &soul_collect_effect_ops;
    list_init(&self->particles);
    EFFECT_SPIN(&self->base) = 0;
    return self;
}

struct Effect *soul_collect_effect_new(void)
{
    struct SoulCollectEffect *self = mem_try_alloc(sizeof *self);

    return self ? &soul_collect_effect_construct(self)->base : NULL;
}

void soul_collect_effect_destruct(struct SoulCollectEffect *self)
{
    self->base.ops = &soul_collect_effect_ops;
    list_free(&self->particles, NULL);
    effect_destruct(&self->base);
}

static void soul_collect_effect_destroy(struct Effect *effect)
{
    struct SoulCollectEffect *self = (struct SoulCollectEffect *)effect;
    soul_collect_effect_destruct(self);
    mem_free(self);
}

void soul_collect_effect_add_soul(struct SoulCollectEffect *self)
{
    int core;

    for (core = 0; core < 2; core++) {
        struct Particle *p = particle_new(self->base.pos, 5);
        unsigned soul;
        struct String name;

        if (!p)
            continue;
        soul = self->particles.size >> 1;
        string_init(&name);
        string_assign(&name, "effect\\soul_collect\\soul_collect.bmp",
                          sizeof("effect\\soul_collect\\soul_collect.bmp") - 1);
        particle_set_texture(p, name);
        PARTICLE_EFFECT_TYPE(p) = self->base.type;
        PARTICLE_SOUL(p) = (int)soul;
        PARTICLE_CORE(p) = core;
        PARTICLE_OWNER_TAG(p) = 1;
        p->alpha = 0;
        if (core == 0)
            p->depth_bias = 1e-5f;
        else if (core == 1)
            p->depth_bias = 2e-5f;
        particle_quad_unit_corners(p->corners);
        list_push_back(&self->particles, &p, sizeof p);
    }
}

void soul_collect_effect_remove_soul(struct SoulCollectEffect *self)
{
    int i;

    if (self->particles.size == 0)
        return;
    for (i = 0; i < 2; i++)
        free_last_particle(&self->particles);
}

void soul_collect_effect_init(struct Effect *effect)
{
    struct SoulCollectEffect *self = (struct SoulCollectEffect *)effect;
    int wanted = EFFECT_LEVEL(&self->base), have, i;

    if (wanted < 0)
        return;
    have = (int)(self->particles.size >> 1);
    if (have < wanted) {
        for (i = 0; i < EFFECT_LEVEL(&self->base) - have; i++)
            soul_collect_effect_add_soul(self);
    } else if (have > wanted) {
        for (i = 0; i < have - EFFECT_LEVEL(&self->base); i++)
            soul_collect_effect_remove_soul(self);
    }
}

void soul_collect_effect_process_particle(struct SoulCollectEffect *self, struct Particle *p)
{
    unsigned souls, shown, step;
    int soul, row, target_alpha = 255, alpha;
    float turn[12], scale[12], m[12], rad, ring_x, ring_z;

    if (!p)
        return;
    souls = self->particles.size >> 1;
    soul = PARTICLE_SOUL(p);
    if (soul < (int)(souls - SOULS_SHOWN)) {
        p->alpha = 0;
        return;
    }
    row = soul / 5;
    switch (row) {
    case 0:
        p->scale = 2.0f;
        particle_set_color(p, 0, 255, 0);
        target_alpha = 150;
        break;
    case 1:
        p->scale = 3.5f;
        particle_set_color(p, 0, 255, 0);
        target_alpha = 150;
        break;
    case 2:
        p->scale = 3.5f;
        particle_set_color(p, 255, 0, 0);
        break;
    case 3:
        p->scale = 5.5f;
        particle_set_color(p, 255, 0, 0);
        break;
    }
    if (PARTICLE_CORE(p) == 1) {
        p->scale = p->scale * 0.5f;
        particle_set_color(p, 255, 255, 255);
        target_alpha = 255;
    }

    mat_set_rotate_z(turn, 3.141592f);
    mat_rotate_x(turn, degrees_to_radians(game_camera_pitch()));
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_multiply(m, scale, turn);

    alpha = p->alpha + 2;
    p->alpha = target_alpha >= alpha ? alpha : target_alpha;
    memcpy(p->matrix, m, sizeof(m));

    shown = souls > SOULS_SHOWN ? SOULS_SHOWN : souls;
    step = 360u / shown;
    rad = degrees_to_radians((float)(int)(step * (unsigned)(soul - row * 5)) + EFFECT_SPIN(&self->base));
    ring_x = float_sin(rad) * 10.0f;
    ring_z = float_cos(rad) * 10.0f;
    p->pos.x = self->base.pos.x + ring_x;
    p->pos.y = self->base.pos.y - 10.0f;
    p->pos.z = ring_z + self->base.pos.z;
}

void soul_collect_effect_process(struct Effect *effect)
{
    struct SoulCollectEffect *self = (struct SoulCollectEffect *)effect;
    struct ListNode *node;
    float spin;

    for (node = self->particles.head->next; node != self->particles.head; node = node->next)
        soul_collect_effect_process_particle(self, *LIST_VALUE(node, struct Particle *));
    spin = EFFECT_SPIN(&self->base) + 0.5f;
    if (spin > 360.0f)
        spin = spin - 360.0f;
    EFFECT_SPIN(&self->base) = spin;
}

void soul_collect_effect_render_particle(struct Particle *p)
{
    const void *tex;
    float w, h;

    if (!p || p->alpha <= 0)
        return;
    tex = p->texture;
    w = (float)TEXTURE_WIDTH(tex);
    h = (float)TEXTURE_HEIGHT(tex);
    particle_render(p, 0, g_texelOffsetV / h, ((float)(double)TEXTURE_IMAGE_HEIGHT(tex) + g_texelOffsetV) / h,
                      g_texelOffsetU / w, ((float)(double)TEXTURE_IMAGE_WIDTH(tex) + g_texelOffsetU) / w, 1);
}

void soul_collect_effect_render(struct Effect *effect)
{
    struct SoulCollectEffect *self = (struct SoulCollectEffect *)effect;
    struct ListNode *node;
    for (node = self->particles.head->next; node != self->particles.head; node = node->next)
        soul_collect_effect_render_particle(*LIST_VALUE(node, struct Particle *));
}

void soul_collect_effect_release(struct Effect *effect)
{
    struct SoulCollectEffect *self = (struct SoulCollectEffect *)effect;
    struct ListNode *node;

    for (node = self->particles.head->next; node != self->particles.head; node = node->next)
        particle_free(*LIST_VALUE(node, struct Particle *));
    list_clear(&self->particles, NULL);
}
