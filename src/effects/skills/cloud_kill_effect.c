#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include "effects/particle.h"
#include "std/list.h"

#define EFFECT_CLOUD_KILL 1080

struct CloudKillEffect {
    struct Effect base;
    struct List risers;
    struct List grid;
    int riser_fade;
    int grid_fade;
    unsigned char spawning;
};

#define ORIENT(p) ((p)->base.orientation)
#define CLIMB(p) (&(p)->sprite_offset)
#define SPIN(p) ((p)->roll_step)
#define RISE_ACCEL(p) ((p)->frame_delay_step)
#define PUSH_OUT(p) (&(p)->base.second_offset)
struct CloudKillParticle {
    float alpha_step;
    float fade_out_at;
};
PARTICLE_EFFECT_DATA_FITS(CloudKillParticle);
static __inline struct CloudKillParticle *cloud_kill_particle(const struct SpriteParticle *p)
{
    return (struct CloudKillParticle *)p->base.effect_data;
}

#define PARTICLE3_SPR "이팩트\\particle3.spr"
#define PARTICLE3_ACT "이팩트\\particle3.act"

void cloud_kill_effect_process(struct Effect *self);
void cloud_kill_effect_render(struct Effect *self);
void cloud_kill_effect_release(struct Effect *self);
static void cloud_kill_effect_destroy(struct Effect *effect);

static const struct EffectOps cloud_kill_effect_ops = {
    .set_submit = effect_set_submit,
    .init = effect_init,
    .update_range = effect_update_range,
    .process = cloud_kill_effect_process,
    .render = cloud_kill_effect_render,
    .release = cloud_kill_effect_release,
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
    .destroy = cloud_kill_effect_destroy,
    .fade_out = effect_fade_out,
};

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void free_particles(struct List *l)
{
    struct ListNode *node;
    for (node = l->head->next; node != l->head; node = node->next)
        particle_free(&(*LIST_VALUE(node, struct SpriteParticle *))->base);
    list_clear(l, NULL);
}

static int spent(const struct SpriteParticle *p)
{
    float life = p->base.life_seconds;
    return life != 0.0f && !(life * 60.0f > (float)p->base.age);
}

static struct SpriteParticle *new_particle3(struct CloudKillEffect *self)
{
    struct SpriteParticle *p = sprite_particle_new(self->base.pos, 1);
    mat_is_identity(ORIENT(p));
    mat_set_rotate_y(ORIENT(p), deg_to_rad(rand_below(360)));
    return p;
}

struct CloudKillEffect *cloud_kill_effect_construct(struct CloudKillEffect *self)
{
    effect_construct(&self->base, EFFECT_CLOUD_KILL);
    self->base.ops = &cloud_kill_effect_ops;
    list_init(&self->risers);
    list_init(&self->grid);
    self->base.duration = 9999;
    self->spawning = 1;
    self->riser_fade = 10;
    self->grid_fade = 40;
    return self;
}

struct Effect *cloud_kill_effect_new(void)
{
    struct CloudKillEffect *self = mem_try_alloc(sizeof *self);

    return self ? &cloud_kill_effect_construct(self)->base : NULL;
}

/* Effect types created as CloudKillEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow cloud_kill_effect_type_rows[] = {
    EFFECT_TYPE(0x438, cloud_kill_effect_new),
};
const struct EffectTypeTable cloud_kill_effect_types = EFFECT_TYPE_TABLE(cloud_kill_effect_type_rows);

void cloud_kill_effect_destruct(struct CloudKillEffect *self)
{
    self->base.ops = &cloud_kill_effect_ops;
    list_free(&self->grid, NULL);
    list_free(&self->risers, NULL);
    effect_destruct(&self->base);
}

static void cloud_kill_effect_destroy(struct Effect *effect)
{
    struct CloudKillEffect *self = (struct CloudKillEffect *)effect;
    cloud_kill_effect_destruct(self);
    mem_free(self);
}

void cloud_kill_effect_release(struct Effect *effect)
{
    struct CloudKillEffect *self = (struct CloudKillEffect *)effect;
    free_particles(&self->risers);
    free_particles(&self->grid);
}

void cloud_kill_effect_add_riser(struct CloudKillEffect *self)
{
    struct SpriteParticle *p = new_particle3(self);
    float speed;

    mat_set_rotate_x(ORIENT(p), deg_to_rad(90));
    speed = (float)(rand_below(20) + 20) / 80.0f;
    p->frame_delay = speed;
    RISE_ACCEL(p) = speed / 10.0f * -0.5f;
    p->base.scale = 0.8f;
    SPIN(p) = 3.0f;
    p->base.alpha = 70;
    cloud_kill_particle(p)->alpha_step = 25.4f;
    cloud_kill_particle(p)->fade_out_at = 8.0f;
    sprite_particle_set_spr(p, PARTICLE3_SPR);
    sprite_particle_set_act(p, PARTICLE3_ACT);
    p->base.life_seconds = (float)self->riser_fade / 60.0f;
    list_push_back(&self->risers, &p, sizeof p);
}

void cloud_kill_effect_add_grid(struct CloudKillEffect *self)
{
    int row, col;

    for (row = -10; row <= 10; row += 4)
        for (col = -10; col <= 10; col += 4) {
            struct SpriteParticle *p = new_particle3(self);
            const float *m = ORIENT(p);
            float out = (float)(rand_below(6) + 2), speed;

            PUSH_OUT(p)->x = m[3] * 0.0f + m[0] * 0.0f + m[6] * out + m[9] + (float)col;
            PUSH_OUT(p)->y = m[4] * 0.0f + m[1] * 0.0f + m[7] * out + m[10] + 0.0f;
            PUSH_OUT(p)->z = m[5] * 0.0f + m[2] * 0.0f + m[8] * out + m[11] + (float)row;
            mat_set_rotate_x(ORIENT(p), deg_to_rad(90));
            speed = (float)(rand_below(30) + 30) / 100.0f;
            p->frame_delay = speed;
            RISE_ACCEL(p) = speed / 40.0f * -0.5f;
            p->base.scale = (float)(rand_below(50) + 40) / 170.0f;
            SPIN(p) = 3.0f;
            p->base.alpha = 0;
            cloud_kill_particle(p)->alpha_step = 25.4f;
            cloud_kill_particle(p)->fade_out_at = 32.0f;
            sprite_particle_set_spr(p, PARTICLE3_SPR);
            sprite_particle_set_act(p, PARTICLE3_ACT);
            p->base.life_seconds = (float)self->grid_fade / 60.0f;
            list_push_back(&self->grid, &p, sizeof p);
        }
}

static void climb(struct SpriteParticle *p)
{
    const float *m = ORIENT(p);
    Vec3 *c = CLIMB(p);
    float v = RISE_ACCEL(p) + p->frame_delay;

    p->frame_delay = v;
    c->x = m[3] * 0.0f + m[0] * 0.0f + m[6] * v + m[9] + c->x;
    c->y = m[4] * 0.0f + m[1] * 0.0f + m[7] * v + m[10] + c->y;
    c->z = m[5] * 0.0f + m[2] * 0.0f + m[8] * v + m[11] + c->z;
}

static void spin_and_fade(struct SpriteParticle *p, int fade_frames)
{
    float alpha, step, a;

    p->base.roll = SPIN(p) + p->base.roll;
    alpha = (float)p->base.alpha;
    step = cloud_kill_particle(p)->alpha_step;
    if (p->base.age == (int)cloud_kill_particle(p)->fade_out_at)
        step = -(alpha / (float)(fade_frames - p->base.age));
    a = alpha + step;
    a = 0.0f > a ? 0.0f : a;
    a = 254.0f < a ? 254.0f : a;
    p->base.alpha = (int)a;
    cloud_kill_particle(p)->alpha_step = step;
}

void cloud_kill_effect_step_riser(struct CloudKillEffect *self,
                                            struct SpriteParticle *p)
{
    Vec3 here;
    if (!p)
        return;
    climb(p);
    here = self->base.pos;
    p->base.pos.x = here.x + CLIMB(p)->x;
    p->base.pos.y = here.y + CLIMB(p)->y;
    p->base.pos.z = here.z + CLIMB(p)->z;
    spin_and_fade(p, self->riser_fade);
}

void cloud_kill_effect_step_grid(struct CloudKillEffect *self,
                                           struct SpriteParticle *p)
{
    Vec3 here;
    if (!p)
        return;
    climb(p);
    here = self->base.pos;
    p->base.pos.x = here.x + PUSH_OUT(p)->x + CLIMB(p)->x;
    p->base.pos.y = here.y + PUSH_OUT(p)->y + CLIMB(p)->y;
    p->base.pos.z = here.z + PUSH_OUT(p)->z + CLIMB(p)->z;
    spin_and_fade(p, self->grid_fade);
}

void cloud_kill_effect_process(struct Effect *effect)
{
    struct CloudKillEffect *self = (struct CloudKillEffect *)effect;
    struct ListNode *node;

    if (self->spawning && self->base.frame % 5 == 0)
        cloud_kill_effect_add_riser(self);
    for (node = self->risers.head->next; node != self->risers.head;) {
        struct SpriteParticle *p = *LIST_VALUE(node, struct SpriteParticle *);
        if (spent(p)) {
            particle_free(&p->base);
            node = list_erase(&self->risers, node, NULL);
            continue;
        }
        cloud_kill_effect_step_riser(self, p);
        p->base.age++;
        node = node->next;
    }

    if (self->spawning && self->base.frame % 9 == 0)
        cloud_kill_effect_add_grid(self);
    for (node = self->grid.head->next; node != self->grid.head;) {
        struct SpriteParticle *p = *LIST_VALUE(node, struct SpriteParticle *);
        if (spent(p)) {
            particle_free(&p->base);
            node = list_erase(&self->grid, node, NULL);
            continue;
        }
        cloud_kill_effect_step_grid(self, p);
        p->base.age++;
        node = node->next;
    }

    if (!effect_is_owner_alive(&self->base)) {
        self->spawning = 0;
        self->base.duration = self->base.frame + 50;
    }
}

void cloud_kill_effect_render(struct Effect *effect)
{
    struct CloudKillEffect *self = (struct CloudKillEffect *)effect;
    struct ListNode *node;
    for (node = self->risers.head->next; node != self->risers.head; node = node->next) {
        struct SpriteParticle *p = *LIST_VALUE(node, struct SpriteParticle *);
        if (p)
            sprite_particle_render(p);
    }
    for (node = self->grid.head->next; node != self->grid.head; node = node->next) {
        struct SpriteParticle *p = *LIST_VALUE(node, struct SpriteParticle *);
        if (p)
            sprite_particle_render(p);
    }
}
