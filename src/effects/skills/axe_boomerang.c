#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include "session/state.h"
#include <math.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

struct AxeBoomerang {
    struct Effect base;
    struct Ez2StrParticle *axe;
    unsigned char alive;
    unsigned char returning;
    int frames;
    float t;
    Vec3 axe_pos;
    Vec3 target_pos;
};

void axe_boomerang_init(struct Effect *self);
void axe_boomerang_process(struct Effect *self);
void axe_boomerang_render(struct Effect *self);
void axe_boomerang_release(struct Effect *self);
unsigned char axe_boomerang_is_alive(struct Effect *self);
static void axe_boomerang_destroy(struct Effect *effect);

static const struct EffectOps axe_boomerang_ops = {
    .set_submit = effect_set_submit,
    .init = axe_boomerang_init,
    .update_range = effect_update_range,
    .process = axe_boomerang_process,
    .render = axe_boomerang_render,
    .release = axe_boomerang_release,
    .is_alive = axe_boomerang_is_alive,
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
    .destroy = axe_boomerang_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 no_offset = { 0, 0, 0 };

static void bezier(Vec3 *out, const Vec3 *p0, const Vec3 *p1, const Vec3 *p2, float t)
{
    float s = 1.0f - t;
    float a = (float)pow((double)s, 2.0);
    float b = (float)pow((double)t, 2.0);
    float c = (t + t) * s;
    out->x = c * p1->x + b * p2->x + a * p0->x;
    out->y = c * p1->y + b * p2->y + a * p0->y;
    out->z = c * p1->z + b * p2->z + a * p0->z;
}

struct AxeBoomerang *axe_boomerang_construct(struct AxeBoomerang *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &axe_boomerang_ops;
    self->axe_pos.x = self->axe_pos.y = self->axe_pos.z = 0;
    self->target_pos.x = 0;
    self->base.params.rise_angle = 0;
    self->target_pos.x = self->target_pos.y = self->target_pos.z = 0;
    return self;
}

struct Effect *axe_boomerang_new(void)
{
    struct AxeBoomerang *self = mem_try_alloc(sizeof *self);

    return self ? &axe_boomerang_construct(self)->base : NULL;
}

void axe_boomerang_destruct(struct AxeBoomerang *self)
{
    self->base.ops = &axe_boomerang_ops;
    effect_destruct(&self->base);
}

static void axe_boomerang_destroy(struct Effect *effect)
{
    struct AxeBoomerang *self = (struct AxeBoomerang *)effect;
    axe_boomerang_destruct(self);
    mem_free(self);
}

void axe_boomerang_release(struct Effect *effect)
{
    struct AxeBoomerang *self = (struct AxeBoomerang *)effect;
    particle_free(&self->axe->base);
}

void axe_boomerang_init(struct Effect *effect)
{
    struct AxeBoomerang *self = (struct AxeBoomerang *)effect;
    struct Ez2StrParticle *axe = ez2str_particle_new(self->axe_pos, 5);
    const char *name;

    if (g_session1.map_rule_flags[0] == 1)
        name = "mineffect\\new_axeboomerang\\new_axeboomerang\\new_axeboomerang.str";
    else
        name = "new_axeboomerang\\new_axeboomerang\\new_axeboomerang.str";

    ez2_str_particle_load(axe, name);
    axe->loop_forever = 1;
    self->axe = axe;
    self->alive = 1;
    self->returning = 0;
    self->frames = 0;
    self->t = 1.0f;
}

unsigned char axe_boomerang_is_alive(struct Effect *effect)
{
    struct AxeBoomerang *self = (struct AxeBoomerang *)effect;
    return self->alive != 0;
}

void axe_boomerang_fly(struct AxeBoomerang *self, struct Ez2StrParticle *axe)
{
    void *caster, *target;
    float scale, angle, dx, dy, dz, inv_len;
    float turn[12];
    Vec3 caster_pos, way, ctrl, target_pos;

    if (!axe)
        return;
    caster = effect_find_actor(self->base.owner_aid);
    if (!caster) {
        self->alive = 0;
        return;
    }
    target = effect_find_actor(self->base.target_aid);
    if (target)
        self->target_pos = *ACTOR_POS(target);

    if (!self->returning) {
        self->t -= 0.05f;
        if (self->t <= 0.0f)
            self->returning = 1;
        scale = 20.0f;
        angle = 1.745329f;
    } else {
        self->t += 0.02f;
        scale = 40.0f;
        angle = 4.537855f;
    }
    mat_set_rotate_y(turn, angle);

    dy = self->target_pos.y - ACTOR_POS(caster)->y;
    dx = self->target_pos.x - ACTOR_POS(caster)->x;
    dz = self->target_pos.z - ACTOR_POS(caster)->z;
    inv_len = 1.0f / float_sqrt(dy * dy + dx * dx + dz * dz);
    way.x = inv_len * dx * scale;
    way.y = inv_len * dy * scale;
    way.z = inv_len * dz * scale;
    ctrl.x = way.y * turn[3] + way.x * turn[0] + way.z * turn[6] + turn[9] + self->target_pos.x;
    ctrl.y = way.y * turn[4] + way.x * turn[1] + way.z * turn[7] + turn[10] + self->target_pos.y;
    ctrl.z = way.y * turn[5] + way.x * turn[2] + way.z * turn[8] + turn[11] + self->target_pos.z;

    caster_pos = *ACTOR_POS(caster);
    target_pos = self->target_pos;
    bezier(&self->axe_pos, &target_pos, &ctrl, &caster_pos, self->t);
    if (self->t >= 1.0f)
        self->alive = 0;

    axe->base.pos = self->axe_pos;
    ez2_str_particle_process(axe, no_offset);
}

void axe_boomerang_process(struct Effect *effect)
{
    struct AxeBoomerang *self = (struct AxeBoomerang *)effect;
    axe_boomerang_fly(self, self->axe);
    self->frames++;
}

void axe_boomerang_render(struct Effect *effect)
{
    struct AxeBoomerang *self = (struct AxeBoomerang *)effect;
    if (self->axe) {
        ez2_str_particle_render_current(self->axe);
    }
}
