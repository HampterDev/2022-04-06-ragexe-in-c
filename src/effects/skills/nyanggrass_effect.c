#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct NyanggrassEffect {
    struct Effect base;
    struct Vec grass;
    int side;
};

#define GRASS_SPACING 5

void nyanggrass_effect_init(struct Effect *self);
void nyanggrass_effect_process(struct Effect *self);
void nyanggrass_effect_render(struct Effect *self);
void nyanggrass_effect_release(struct Effect *self);
static void nyanggrass_effect_destroy(struct Effect *effect);

static const struct EffectOps nyanggrass_effect_ops = {
    .set_submit = effect_set_submit,
    .init = nyanggrass_effect_init,
    .update_range = effect_update_range,
    .process = nyanggrass_effect_process,
    .render = nyanggrass_effect_render,
    .release = nyanggrass_effect_release,
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
    .destroy = nyanggrass_effect_destroy,
    .fade_out = effect_fade_out,
};

static void particle_tag(void *p, int effect_type, int index)
{
    struct Particle *head = p;
    head->effect_type = effect_type;
    head->index = index;
    head->group = 0;
    head->owner_tag = 1;
}

struct NyanggrassEffect *nyanggrass_effect_construct(struct NyanggrassEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &nyanggrass_effect_ops;
    self->grass.first = self->grass.last = self->grass.end = 0;
    self->side = 0;
    return self;
}

struct Effect *nyanggrass_effect_new(void)
{
    struct NyanggrassEffect *self = mem_try_alloc(sizeof *self);

    return self ? &nyanggrass_effect_construct(self)->base : NULL;
}

void nyanggrass_effect_destruct(struct NyanggrassEffect *self)
{
    self->base.ops = &nyanggrass_effect_ops;
    vec_free(&self->grass);
    effect_destruct(&self->base);
}

static void nyanggrass_effect_destroy(struct Effect *effect)
{
    struct NyanggrassEffect *self = (struct NyanggrassEffect *)effect;
    nyanggrass_effect_destruct(self);
    mem_free(self);
}

void nyanggrass_effect_plant_field(struct NyanggrassEffect *self)
{
    int row, col;

    for (row = 0; row < self->side; row++) {
        for (col = 0; col < self->side; col++) {
            struct Ez2StrParticle *grass = ez2str_particle_new(self->base.pos, 1);
            Vec3 here;
            int half;

            if (!grass)
                continue;
            particle_tag(grass, self->base.type, 0);
            grass->loop_forever = 1;
            here = self->base.pos;
            half = self->side / 2;
            grass->base.pos.x = (float)((col - half) * GRASS_SPACING) + here.x;
            grass->base.pos.y = here.y;
            grass->base.pos.z = (float)((row - half) * GRASS_SPACING) + here.z;
            particle_quad_unit_corners(grass->base.corners);
            ez2_str_particle_load(grass, "su_nyanggrass\\su_nyanggrass.str");
            vec_push(&self->grass, &grass, sizeof(struct Ez2StrParticle *));
        }
    }
}

void nyanggrass_effect_init(struct Effect *effect)
{
    struct NyanggrassEffect *self = (struct NyanggrassEffect *)effect;
    switch (self->base.level) {
    case 1: case 2: self->side = 5; break;
    case 3: case 4: self->side = 7; break;
    case 5: self->side = 9; break;
    }
    nyanggrass_effect_plant_field(self);
}

void nyanggrass_effect_release(struct Effect *effect)
{
    struct NyanggrassEffect *self = (struct NyanggrassEffect *)effect;
    struct Particle **it;
    for (it = self->grass.first; it != (struct Particle **)self->grass.last; it++)
        particle_free(*it);
    self->grass.last = self->grass.first;
}

void nyanggrass_effect_process(struct Effect *effect)
{
    struct NyanggrassEffect *self = (struct NyanggrassEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->grass.first; it != (struct Ez2StrParticle **)self->grass.last; it++) {
        struct Ez2StrParticle *grass = *it;
        if (grass) {
            Vec3 no_offset = { 0, 0, 0 };
            ez2_str_particle_process(grass, no_offset);
        }
        grass->base.age++;
    }
}

void nyanggrass_effect_render(struct Effect *effect)
{
    struct NyanggrassEffect *self = (struct NyanggrassEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->grass.first; it != (struct Ez2StrParticle **)self->grass.last; it++) {
        struct Ez2StrParticle *grass = *it;
        if (grass) {
            ez2_str_particle_render_current(grass);
        }
    }
}
