#include "effects/effect_allocator.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

enum TetraElement {
    TETRA_FIRE = 0x4ee,
    TETRA_WATER = 0x4ef,
    TETRA_WIND = 0x4f0,
    TETRA_EARTH = 0x4f1,
};

#define TETRA_SLOTS 5

struct TetraBallEffect {
    struct Effect base;
    struct Ez2StrParticle *balls[TETRA_SLOTS];
};

void tetra_ball_effect_init(struct Effect *self);
void tetra_ball_effect_process(struct Effect *self);
void tetra_ball_effect_render(struct Effect *self);
void tetra_ball_effect_release(struct Effect *self);
static void tetra_ball_effect_destroy(struct Effect *effect);

const struct EffectOps tetra_ball_effect_ops = {
    .set_submit = effect_set_submit,
    .init = tetra_ball_effect_init,
    .update_range = effect_update_range,
    .process = tetra_ball_effect_process,
    .render = tetra_ball_effect_render,
    .release = tetra_ball_effect_release,
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
    .destroy = tetra_ball_effect_destroy,
    .fade_out = effect_fade_out,
};

static struct Ez2StrParticle *tetra_ball_new(struct TetraBallEffect *self, int element)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, 5);
    const char *str = 0;

    p->loop_forever = 1;
    switch (element) {
    case TETRA_FIRE:  str = "tetravortex\\tetra_ball_01_fire\\tetra_01_fire.str";   break;
    case TETRA_WATER: str = "tetravortex\\tetra_ball_04_water\\tetra_04_water.str"; break;
    case TETRA_WIND:  str = "tetravortex\\tetra_ball_02_wind\\tetra_02_wind.str";   break;
    case TETRA_EARTH: str = "tetravortex\\tetra_ball_03_earth\\tetra_03_earth.str"; break;
    }
    if (str) {
        ez2_str_particle_load(p, str);
    }
    return p;
}

struct TetraBallEffect *tetra_ball_effect_construct(struct TetraBallEffect *self)
{
    int i;
    effect_construct(&self->base, -1);
    self->base.ops = &tetra_ball_effect_ops;
    self->base.params.rise_angle = 0;
    for (i = 0; i < TETRA_SLOTS; i++)
        self->balls[i] = 0;
    return self;
}

struct Effect *tetra_ball_effect_new(void)
{
    struct TetraBallEffect *self = mem_try_alloc(sizeof *self);

    return self ? &tetra_ball_effect_construct(self)->base : NULL;
}

/* Effect types created as TetraBallEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow tetra_ball_effect_type_rows[] = {
    EFFECT_TYPES(0x4ee, 0x4f1, tetra_ball_effect_new),
};
const struct EffectTypeTable tetra_ball_effect_types = EFFECT_TYPE_TABLE(tetra_ball_effect_type_rows);

void tetra_ball_effect_destruct(struct TetraBallEffect *self)
{
    self->base.ops = &tetra_ball_effect_ops;
    effect_destruct(&self->base);
}

static void tetra_ball_effect_destroy(struct Effect *effect)
{
    struct TetraBallEffect *self = (struct TetraBallEffect *)effect;
    tetra_ball_effect_destruct(self);
    mem_free(self);
}

int tetra_ball_effect_count(struct TetraBallEffect *self)
{
    int i, n = 0;
    for (i = 0; i < TETRA_SLOTS; i++)
        if (self->balls[i])
            n++;
    return n;
}

void tetra_ball_effect_spread(struct TetraBallEffect *self)
{
    unsigned count = (unsigned)tetra_ball_effect_count(self);
    int i;

    if (count <= 1)
        return;
    for (i = 0; i < TETRA_SLOTS; i++) {
        if (!self->balls[i])
            continue;
        ez2_str_particle_rewind(self->balls[i]);
        ez2_str_particle_skip(self->balls[i], (float)i / (float)(int)count);
    }
}

void tetra_ball_effect_add_ball(struct TetraBallEffect *self, int element, int slot)
{
    self->balls[slot] = tetra_ball_new(self, element);
    tetra_ball_effect_spread(self);
}

void tetra_ball_effect_remove_ball(struct TetraBallEffect *self, int slot)
{
    if (!self->balls[slot])
        return;
    particle_free(&self->balls[slot]->base);
    self->balls[slot] = 0;
    tetra_ball_effect_spread(self);
}

void tetra_ball_effect_move_first_to(struct TetraBallEffect *self, int slot)
{
    if (!slot)
        return;
    self->balls[slot] = self->balls[0];
    self->balls[0] = 0;
}

void tetra_ball_effect_update_ball(struct TetraBallEffect *self,
                                             struct Ez2StrParticle *p)
{
    Vec3 none = { 0.0f, 0.0f, 0.0f };
    if (!p)
        return;
    p->base.pos = self->base.pos;
    ez2_str_particle_process(p, none);
    p->base.age++;
}

void tetra_ball_render(struct Ez2StrParticle *p)
{
    if (!p)
        return;
    ez2_str_particle_render_current(p);
}

void tetra_ball_effect_init(struct Effect *effect)
{
    struct TetraBallEffect *self = (struct TetraBallEffect *)effect;
    self->base.has_instance_key = 1;
    self->balls[0] = tetra_ball_new(self, self->base.type);
    tetra_ball_effect_spread(self);
}

void tetra_ball_effect_process(struct Effect *effect)
{
    struct TetraBallEffect *self = (struct TetraBallEffect *)effect;
    int i;
    for (i = 0; i < TETRA_SLOTS; i++)
        tetra_ball_effect_update_ball(self, self->balls[i]);
}

void tetra_ball_effect_render(struct Effect *effect)
{
    struct TetraBallEffect *self = (struct TetraBallEffect *)effect;
    int i;
    for (i = 0; i < TETRA_SLOTS; i++)
        tetra_ball_render(self->balls[i]);
}

void tetra_ball_effect_release(struct Effect *effect)
{
    struct TetraBallEffect *self = (struct TetraBallEffect *)effect;
    int i;
    for (i = 0; i < TETRA_SLOTS; i++) {
        if (!self->balls[i])
            continue;
        particle_free(&self->balls[i]->base);
        self->balls[i] = 0;
    }
}
