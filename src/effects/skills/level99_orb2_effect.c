#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define ORB2_TYPE 0x3d1
#define ORB_ROWS 4
#define ORB_COLUMNS 4
#define ORBS (ORB_ROWS * ORB_COLUMNS)
#define ORB2_LIST 13

struct Level99Orb2Effect {
    struct Effect base;
    Vec3 orbs[ORBS];
    struct Vec motes;
};

struct String;
void level99_orb2_effect_init(struct Effect *self);
void level99_orb2_effect_process(struct Effect *self);
void level99_orb2_effect_render(struct Effect *self);
void level99_orb2_effect_release(struct Effect *self);
static void level99_orb2_effect_destroy(struct Effect *effect);

static const struct EffectOps level99_orb2_effect_ops = {
    .set_submit = effect_set_submit,
    .init = level99_orb2_effect_init,
    .update_range = effect_update_range,
    .process = level99_orb2_effect_process,
    .render = level99_orb2_effect_render,
    .release = level99_orb2_effect_release,
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
    .destroy = level99_orb2_effect_destroy,
    .fade_out = effect_fade_out,
};
#define SIN_DEG(d) (g_sinTable[d])
#define MATRIX_BYTES (12 * sizeof(float))

#define PART_EFFECT_TYPE(p) ((p)->effect_type)
struct Level99Orb2Particle {
    float angle[2];
    float target_angle[2];
    float mote_speed;
};
PARTICLE_EFFECT_DATA_FITS(Level99Orb2Particle);
static __inline struct Level99Orb2Particle *level99_orb2_particle(const struct Particle *p)
{
    return (struct Level99Orb2Particle *)p->effect_data;
}
#define PART_COLUMN(p)      ((p)->index)

static void set_texture(struct Particle *p, const char *name)
{
    struct String texture;
    string_init(&texture);
    string_assign_cstr(&texture, name);
    particle_set_texture(p, texture);
}

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void mote_restart(Vec3 *orb, struct Particle *p)
{
    struct Level99Orb2Particle *mote = level99_orb2_particle(p);
    int k;
    orb->x = 0.0f;
    orb->y = (float)rand_below(100);
    orb->z = 0.0f;
    for (k = 0; k < 2; k++)
        mote->angle[k] = (float)rand_below(360);
    for (k = 0; k < 2; k++)
        mote->target_angle[k] = (float)rand_below(360);
}

static void mote_wander(struct Particle *p)
{
    struct Level99Orb2Particle *mote = level99_orb2_particle(p);
    int k;

    for (k = 0; k < 2; k++) {
        int now = (int)mote->angle[k];
        int diff = (int)mote->target_angle[k] - now;

        mote->angle[k] = (float)(now + (diff > 0 ? 3 : -2));
        if ((diff < 0 ? -diff : diff) < 3) {
            int target;
            if (diff > 0) {
                target = now == 0 ? 0 : rand() % now;
                if (target < 0)
                    target = 0;
            } else {
                int r = 0;
                if (now != 0) {
                    r = rand() % now;
                    if (r < 0)
                        r = 0;
                }
                target = 359 - r;
            }
            mote->target_angle[k] = (float)target;
        }
    }
}

struct Level99Orb2Effect *level99_orb2_effect_construct(struct Level99Orb2Effect *self)
{
    int i;
    effect_construct(&self->base, ORB2_TYPE);
    self->base.ops = &level99_orb2_effect_ops;
    for (i = 0; i < ORBS; i++)
        self->orbs[i].x = self->orbs[i].y = self->orbs[i].z = 0.0f;
    self->motes.first = self->motes.last = self->motes.end = 0;
    return self;
}

struct Effect *level99_orb2_effect_new(void)
{
    struct Level99Orb2Effect *self = mem_try_alloc(sizeof *self);

    return self ? &level99_orb2_effect_construct(self)->base : NULL;
}

void level99_orb2_effect_destruct(struct Level99Orb2Effect *self)
{
    self->base.ops = &level99_orb2_effect_ops;
    vec_free(&self->motes);
    effect_destruct(&self->base);
}

static void level99_orb2_effect_destroy(struct Effect *effect)
{
    struct Level99Orb2Effect *self = (struct Level99Orb2Effect *)effect;
    level99_orb2_effect_destruct(self);
    mem_free(self);
}

void level99_orb2_effect_update_mote(struct Level99Orb2Effect *self, struct Particle *p)
{
    float tilt[12], move[12], size[12], sized[12], m[12];
    Vec3 *orb;
    float speed;

    if (!p)
        return;
    orb = &self->orbs[PART_COLUMN(p) + 4 * p->group];
    speed = level99_orb2_particle(p)->mote_speed;
    if (0.0f > orb->y) {
        if (-30.0f > orb->y) {
            mote_restart(orb, p);
            p->alpha = 0;
        } else {
            mote_wander(p);
            orb->x = SIN_DEG((int)level99_orb2_particle(p)->angle[0]) * speed + orb->x;
            orb->z = SIN_DEG((int)level99_orb2_particle(p)->angle[1]) * speed + orb->z;
            if (-20.0f > orb->y) {
                int alpha = p->alpha - 4;
                p->alpha = alpha < 0 ? 0 : alpha;
            } else if (p->alpha != 255) {
                p->alpha = 255;
            }
        }
    }
    orb->y = orb->y - speed;

    memcpy(tilt, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_rotate_x(tilt, 3.1415920f);
    mat_rotate_x(tilt, DEG_TO_RAD(game_camera_pitch()));
    memcpy(move, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_translation(move, orb->x, orb->y, orb->z);
    memcpy(size, g_identityMatrix.m, MATRIX_BYTES);
    mat_set_scale(size, p->scale, p->scale, 1.0f);
    mat_multiply(sized, size, tilt);
    mat_multiply(m, sized, move);
    memcpy(p->matrix, m, MATRIX_BYTES);
}

void level99_orb2_render_mote(struct Particle *p)
{
    int i;
    if (!p)
        return;
    for (i = 0; i < 2; i++)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void level99_orb2_effect_init(struct Effect *effect)
{
    struct Level99Orb2Effect *self = (struct Level99Orb2Effect *)effect;
    int row, column;

    for (row = 0; row < ORB_ROWS; row++) {
        for (column = 0; column < ORB_COLUMNS; column++) {
            struct Particle *p = particle_new(self->base.pos, ORB2_LIST);
            if (!p)
                continue;
            set_texture(p, "effect\\freezing_circle.bmp");
            PART_EFFECT_TYPE(p) = self->base.type;
            PART_COLUMN(p) = column;
            p->group = row;
            p->owner_tag = 2;
            level99_orb2_particle(p)->mote_speed = 0.15f;
            p->scale = 0.55f;
            p->alpha = 0;
            mote_restart(&self->orbs[column + 4 * row], p);
            vec_push(&self->motes, &p, sizeof p);
        }
    }
}

void level99_orb2_effect_process(struct Effect *effect)
{
    struct Level99Orb2Effect *self = (struct Level99Orb2Effect *)effect;
    struct Particle **it;
    for (it = self->motes.first; it != (struct Particle **)self->motes.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        level99_orb2_effect_update_mote(self, p);
        p->age++;
    }
}

void level99_orb2_effect_render(struct Effect *effect)
{
    struct Level99Orb2Effect *self = (struct Level99Orb2Effect *)effect;
    struct Particle **it;
    for (it = self->motes.first; it != (struct Particle **)self->motes.last; it++)
        level99_orb2_render_mote(*it);
}

void level99_orb2_effect_release(struct Effect *effect)
{
    struct Level99Orb2Effect *self = (struct Level99Orb2Effect *)effect;
    struct Particle **it;
    for (it = self->motes.first; it != (struct Particle **)self->motes.last; it++)
        particle_free(*it);
}
