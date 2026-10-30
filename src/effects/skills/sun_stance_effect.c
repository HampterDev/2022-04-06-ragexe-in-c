#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/list.h"

enum SunStanceType {
    SUN_STANCE_SUN = 0x4be,
    SUN_STANCE_LUNAR = 0x4bf,
    SUN_STANCE_STAR = 0x4c0,
    SUN_STANCE_ALL = 0x4c1,
};

#define EMBLEM_COUNT 3
#define ORBIT_RADIUS 8.0f
#define ORBIT_HEIGHT 10.0f
#define ORBIT_SPEED 0.02f

struct SunStanceEffect {
    struct Effect base;
    struct List emblems;
};

struct SunStanceParticle {
    float emblem_angle;
};
PARTICLE_EFFECT_DATA_FITS(SunStanceParticle);
static __inline struct SunStanceParticle *sun_stance_particle(const struct Particle *p)
{
    return (struct SunStanceParticle *)p->effect_data;
}

struct String;
void sun_stance_effect_init(struct Effect *self);
void sun_stance_effect_process(struct Effect *self);
void sun_stance_effect_render(struct Effect *self);
void sun_stance_effect_release(struct Effect *self);
static void sun_stance_effect_destroy(struct Effect *effect);

static const struct EffectOps sun_stance_effect_ops = {
    .set_submit = effect_set_submit,
    .init = sun_stance_effect_init,
    .update_range = effect_update_range,
    .process = sun_stance_effect_process,
    .render = sun_stance_effect_render,
    .release = sun_stance_effect_release,
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
    .destroy = sun_stance_effect_destroy,
    .fade_out = effect_fade_out,
};

#define MATRIX_BYTES (12 * sizeof(float))

static void particle_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

struct SunStanceEffect *sun_stance_effect_construct(struct SunStanceEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &sun_stance_effect_ops;
    list_init(&self->emblems);
    return self;
}

struct Effect *sun_stance_effect_new(void)
{
    struct SunStanceEffect *self = mem_try_alloc(sizeof *self);

    return self ? &sun_stance_effect_construct(self)->base : NULL;
}

void sun_stance_effect_destruct(struct SunStanceEffect *self)
{
    self->base.ops = &sun_stance_effect_ops;
    list_free(&self->emblems, NULL);
    effect_destruct(&self->base);
}

static void sun_stance_effect_destroy(struct Effect *effect)
{
    struct SunStanceEffect *self = (struct SunStanceEffect *)effect;
    sun_stance_effect_destruct(self);
    mem_free(self);
}

void sun_stance_effect_init(struct Effect *effect)
{
    struct SunStanceEffect *self = (struct SunStanceEffect *)effect;
    static const char sun[] = "effect\\sun_stance\\sun_stance.tga";
    static const char lunar[] = "effect\\sun_stance\\lunar_stance.tga";
    static const char star[] = "effect\\sun_stance\\star_stance.tga";
    int i;

    for (i = 0; i < EMBLEM_COUNT; i++) {
        struct Particle *p = particle_new(self->base.pos, 5);
        const char *texture;

        if (!p)
            continue;
        switch (self->base.type) {
        case SUN_STANCE_SUN:   texture = sun;   break;
        case SUN_STANCE_LUNAR: texture = lunar; break;
        case SUN_STANCE_STAR:  texture = star;  break;
        case SUN_STANCE_ALL:   texture = i == 0 ? sun : i == 1 ? lunar : star; break;
        default:               texture = sun;   break;
        }
        particle_texture(p, texture);
        p->effect_type = self->base.type;
        p->index = 1;
        p->group = 0;
        p->owner_tag = 1;
        p->alpha = 0xff;
        p->scale = 3.0f;
        sun_stance_particle(p)->emblem_angle = (float)i * 2.0943947f;
        particle_quad_unit_corners(p->corners);
        list_push_back(&self->emblems, &p, sizeof p);
    }
}

void sun_stance_effect_update_emblem(struct SunStanceEffect *self, struct Particle *p)
{
    float tilt[12], scale[12], m[12], angle, x, z;

    if (!p)
        return;
    mat_set_rotate_x(tilt, DEG_TO_RAD(game_camera_pitch()));
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_multiply(m, scale, tilt);
    memcpy(p->matrix, m, MATRIX_BYTES);

    angle = sun_stance_particle(p)->emblem_angle + ORBIT_SPEED;
    sun_stance_particle(p)->emblem_angle = angle;
    x = float_sin(angle) * ORBIT_RADIUS;
    z = float_cos(angle) * ORBIT_RADIUS;
    p->pos.x = x + self->base.pos.x;
    p->pos.y = self->base.pos.y - ORBIT_HEIGHT;
    p->pos.z = z + self->base.pos.z;
}

void sun_stance_effect_process(struct Effect *effect)
{
    struct SunStanceEffect *self = (struct SunStanceEffect *)effect;
    struct ListNode *n;
    for (n = self->emblems.head->next; n != self->emblems.head; n = n->next)
        sun_stance_effect_update_emblem(self, *LIST_VALUE(n, struct Particle *));
}

void sun_stance_render_emblem(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void sun_stance_effect_render(struct Effect *effect)
{
    struct SunStanceEffect *self = (struct SunStanceEffect *)effect;
    struct ListNode *n;
    for (n = self->emblems.head->next; n != self->emblems.head; n = n->next)
        sun_stance_render_emblem(*LIST_VALUE(n, struct Particle *));
}

void sun_stance_effect_release(struct Effect *effect)
{
    struct SunStanceEffect *self = (struct SunStanceEffect *)effect;
    struct ListNode *n;

    for (n = self->emblems.head->next; n != self->emblems.head; n = n->next)
        particle_free(*LIST_VALUE(n, struct Particle *));
    list_clear(&self->emblems, NULL);
}
