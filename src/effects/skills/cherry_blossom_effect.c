#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct CherryBlossomEffect;
struct Effect;
struct String;
void cherry_blossom_effect_init(struct Effect *self);
void cherry_blossom_effect_process(struct Effect *self);
void cherry_blossom_effect_render(struct Effect *self);
void cherry_blossom_effect_release(struct Effect *self);
static void cherry_blossom_effect_destroy(struct Effect *effect);

static const struct EffectOps cherry_blossom_effect_ops = {
    .set_submit = effect_set_submit,
    .init = cherry_blossom_effect_init,
    .update_range = effect_update_range,
    .process = cherry_blossom_effect_process,
    .render = cherry_blossom_effect_render,
    .release = cherry_blossom_effect_release,
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
    .destroy = cherry_blossom_effect_destroy,
    .fade_out = effect_fade_out,
};

#define PETAL_ROWS 4
#define PETAL_COLS 4

struct CherryBlossomEffect {
    struct Effect base;
    Vec3 petal_offset[PETAL_ROWS * PETAL_COLS];
    struct Vec petals;
};

struct CherryBlossomParticle {
    float sway[2];
    float sway_target[2];
    float petal_speed;
};
PARTICLE_EFFECT_DATA_FITS(CherryBlossomParticle);
static __inline struct CherryBlossomParticle *cherry_blossom_particle(const struct Particle *p)
{
    return (struct CherryBlossomParticle *)p->effect_data;
}

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void set_particle_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

static void petal_start(Vec3 *offset, struct Particle *p)
{
    struct CherryBlossomParticle *petal = cherry_blossom_particle(p);
    int i;
    offset->x = 0;
    offset->y = (float)rand_below(100);
    offset->z = 0;
    for (i = 0; i < 2; i++)
        petal->sway[i] = (float)rand_below(360);
    for (i = 0; i < 2; i++)
        petal->sway_target[i] = (float)rand_below(360);
}

struct CherryBlossomEffect *cherry_blossom_effect_construct(
    struct CherryBlossomEffect *self)
{
    int i;
    effect_construct(&self->base, 1013);
    self->base.ops = &cherry_blossom_effect_ops;
    for (i = 0; i < PETAL_ROWS * PETAL_COLS; i++)
        self->petal_offset[i].x = self->petal_offset[i].y = self->petal_offset[i].z = 0;
    self->petals.first = self->petals.last = self->petals.end = 0;
    return self;
}

struct Effect *cherry_blossom_effect_new(void)
{
    struct CherryBlossomEffect *self = mem_try_alloc(sizeof *self);

    return self ? &cherry_blossom_effect_construct(self)->base : NULL;
}

/* Effect types created as CherryBlossomEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow cherry_blossom_effect_type_rows[] = {
    EFFECT_TYPE(0x3f5, cherry_blossom_effect_new),
};
const struct EffectTypeTable cherry_blossom_effect_types = EFFECT_TYPE_TABLE(cherry_blossom_effect_type_rows);

void cherry_blossom_effect_destruct(struct CherryBlossomEffect *self)
{
    self->base.ops = &cherry_blossom_effect_ops;
    vec_free(&self->petals);
    effect_destruct(&self->base);
}

static void cherry_blossom_effect_destroy(struct Effect *effect)
{
    struct CherryBlossomEffect *self = (struct CherryBlossomEffect *)effect;
    cherry_blossom_effect_destruct(self);
    mem_free(self);
}

void cherry_blossom_effect_release(struct Effect *effect)
{
    struct CherryBlossomEffect *self = (struct CherryBlossomEffect *)effect;
    struct Particle **it;
    for (it = self->petals.first; it != (struct Particle **)self->petals.last; it++)
        particle_free(*it);
    self->petals.last = self->petals.first;
}

void cherry_blossom_effect_init(struct Effect *effect)
{
    struct CherryBlossomEffect *self = (struct CherryBlossomEffect *)effect;
    int row, col;

    for (row = 0; row < PETAL_ROWS; row++) {
        for (col = 0; col < PETAL_COLS; col++) {
            struct Particle *p = particle_new(self->base.pos, 5);
            if (!p)
                continue;
            set_particle_texture(p, "effect\\cherry_circle.bmp");
            p->effect_type = self->base.type;
            p->index = col;
            p->group = row;
            p->owner_tag = 2;
            cherry_blossom_particle(p)->petal_speed = 0.15f;
            p->scale = 0.55f;
            p->alpha = 0;
            petal_start(&self->petal_offset[row * PETAL_COLS + col], p);
            vec_push(&self->petals, &p, sizeof(struct Particle *));
        }
    }
}

static void sway_step(float *angle_f, float *target_f)
{
    int angle = (int)*angle_f;
    int diff = (int)*target_f - angle;

    *angle_f = (float)(angle + (diff > 0 ? 3 : -2));
    if (abs(diff) >= 3)
        return;
    if (diff > 0) {
        *target_f = (float)(angle ? rand_below(angle) : 0);
    } else {
        int r = angle ? rand_below(angle) : 0;
        *target_f = (float)(359 - r);
    }
}

void cherry_blossom_effect_step_petal(struct CherryBlossomEffect *self,
                                               struct Particle *p)
{
    Vec3 *offset;
    float speed;
    float turn[12], move[12], scale[12], tmp[12];

    if (!p)
        return;
    offset = &self->petal_offset[p->index + p->group * PETAL_COLS];
    speed = cherry_blossom_particle(p)->petal_speed;

    if (0.0f > offset->y) {
        if (-30.0f > offset->y) {
            petal_start(offset, p);
            p->alpha = 0;
        } else {
            struct CherryBlossomParticle *v = cherry_blossom_particle(p);
            sway_step(&v->sway[0], &v->sway_target[0]);
            sway_step(&v->sway[1], &v->sway_target[1]);
            offset->x = g_sinTable[(int)v->sway[0]] * speed + offset->x;
            offset->z = g_sinTable[(int)v->sway[1]] * speed + offset->z;
            if (-20.0f > offset->y) {
                int a = p->alpha - 4;
                p->alpha = a < 0 ? 0 : a;
            } else if (p->alpha != 0xff) {
                p->alpha = 0xff;
            }
        }
    }
    offset->y = offset->y - speed;

    mat_set_rotate_x(turn, 3.141592f);
    mat_rotate_x(turn, game_camera_pitch() / g_halfTurnDegrees * g_pi);
    mat_set_translation(move, offset->x, offset->y, offset->z);
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_multiply(tmp, scale, turn);
    mat_multiply(p->matrix, tmp, move);
}

void cherry_blossom_effect_process(struct Effect *effect)
{
    struct CherryBlossomEffect *self = (struct CherryBlossomEffect *)effect;
    struct Particle **it;
    for (it = self->petals.first; it != (struct Particle **)self->petals.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        cherry_blossom_effect_step_petal(self, p);
        p->age++;
    }
}

void cherry_blossom_effect_draw_petal(struct Particle *p)
{
    int i;
    if (!p)
        return;
    for (i = 0; i < 2; i++)
        particle_render(p, 0, 0, 1.0f, 0, 1.0f, 1);
}

void cherry_blossom_effect_render(struct Effect *effect)
{
    struct CherryBlossomEffect *self = (struct CherryBlossomEffect *)effect;
    struct Particle **it;
    for (it = self->petals.first; it != (struct Particle **)self->petals.last; it++)
        cherry_blossom_effect_draw_petal(*it);
}
