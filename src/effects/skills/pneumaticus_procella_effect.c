#include "effects/effect_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_PNEUMATICUS_PROCELLA 0x7a5
#define LIST_WORLD 5
#define LIST_GROUND 0xd
#define BLADES 40
#define BLADE_HEIGHT (-25.0f)

struct PneumaticusProcellaEffect {
    struct Effect base;
    struct Ez2StrParticle *mark;
    struct Vec blades;
};

#define BLADE_OFFSET(p) (&(p)->base.offset)

void pneumaticus_procella_effect_init(struct Effect *self);
void pneumaticus_procella_effect_process(struct Effect *self);
void pneumaticus_procella_effect_render(struct Effect *self);
void pneumaticus_procella_effect_release(struct Effect *self);
static void pneumaticus_procella_effect_destroy(struct Effect *effect);

static const struct EffectOps pneumaticus_procella_effect_ops = {
    .set_submit = effect_set_submit,
    .init = pneumaticus_procella_effect_init,
    .update_range = effect_update_range,
    .process = pneumaticus_procella_effect_process,
    .render = pneumaticus_procella_effect_render,
    .release = pneumaticus_procella_effect_release,
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
    .destroy = pneumaticus_procella_effect_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 drawn_up = { 0.0f, -80.0f, 0.0f };

static const double grid_x[6] = { -12.0, -6.0, 0.0, 0.0, 6.0, 12.0 };
static const double grid_z[9] = { -12.0, -10.0, -6.0, -4.0, 0.0, 4.0, 6.0, 10.0, 12.0 };

static void render_with_view(struct Ez2StrParticle *p)
{
    ez2_str_particle_render_current(p);
}

static int random_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

struct PneumaticusProcellaEffect *pneumaticus_procella_effect_construct(
    struct PneumaticusProcellaEffect *self)
{
    effect_construct(&self->base, EFFECT_PNEUMATICUS_PROCELLA);
    self->base.ops = &pneumaticus_procella_effect_ops;
    self->blades.first = self->blades.last = self->blades.end = 0;
    return self;
}

struct Effect *pneumaticus_procella_effect_new(void)
{
    struct PneumaticusProcellaEffect *self = mem_try_alloc(sizeof *self);

    return self ? &pneumaticus_procella_effect_construct(self)->base : NULL;
}

void pneumaticus_procella_effect_destruct(struct PneumaticusProcellaEffect *self)
{
    self->base.ops = &pneumaticus_procella_effect_ops;
    vec_free(&self->blades);
    effect_destruct(&self->base);
}

static void pneumaticus_procella_effect_destroy(struct Effect *effect)
{
    struct PneumaticusProcellaEffect *self = (struct PneumaticusProcellaEffect *)effect;
    pneumaticus_procella_effect_destruct(self);
    mem_free(self);
}

void pneumaticus_procella_effect_release(struct Effect *effect)
{
    struct PneumaticusProcellaEffect *self = (struct PneumaticusProcellaEffect *)effect;
    struct Ez2StrParticle **it;

    if (self->mark) {
        particle_free(&self->mark->base);
        self->mark = 0;
    }
    for (it = self->blades.first; it < (struct Ez2StrParticle **)self->blades.last; it++) {
        if (*it) {
            particle_free(&(*it)->base);
            *it = 0;
        }
    }
    self->blades.last = self->blades.first;
}

double pneumaticus_procella_effect_level_scale(struct PneumaticusProcellaEffect *self)
{
    switch (self->base.level) {
    case 1: case 2: case 3: return (double)0.6f;
    case 4: case 5: case 6: return (double)0.8f;
    case 10: return (double)1.2f;
    default: return 1.0;
    }
}

void pneumaticus_procella_effect_init(struct Effect *effect)
{
    struct PneumaticusProcellaEffect *self = (struct PneumaticusProcellaEffect *)effect;
    double scale = pneumaticus_procella_effect_level_scale(self);
    int i;

    self->mark = ez2str_particle_new(self->base.pos, LIST_GROUND);
    if (self->mark) {
        const char *name = "pneumaticusprocella\\pneumaticusprocella_target\\pneumaticusprocella_target.str";
        ez2_str_particle_open(self->mark, name);
        self->mark->loop_forever = 1;
        self->mark->zoom = (float)scale;
    }

    scale += (double)0.6f;
    for (i = 0; i < BLADES; i++) {
        double x = grid_x[random_below(6)] * scale;
        double z = grid_z[random_below(9)] * scale;
        int delay = random_below(200) + (i + i / 10 * 20) * 100;
        struct Ez2StrParticle *blade = ez2str_particle_new(self->base.pos, LIST_WORLD);
        char name[0x104];
        int look;

        if (!blade)
            continue;
        memset(name, 0, sizeof name);
        look = rand() % 4;
        if (look < 1)
            look = 1;
        snprintf(name, sizeof name,
                 "pneumaticusprocella\\pneumaticusprocella\\pneumaticusprocella_%d.str", look);
        ez2_str_particle_open(blade, name);

        blade->start_delay = delay;
        BLADE_OFFSET(blade)->y = BLADE_HEIGHT;
        BLADE_OFFSET(blade)->x = (float)x;
        BLADE_OFFSET(blade)->z = (float)z;
        vec_push(&self->blades, &blade, sizeof(struct Ez2StrParticle *));
    }
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\CD_PNEUMATICUS_PROCELLA.wav", 0, 0.0f);
}

void pneumaticus_procella_effect_process(struct Effect *effect)
{
    struct PneumaticusProcellaEffect *self = (struct PneumaticusProcellaEffect *)effect;
    struct Ez2StrParticle **it;

    if (self->mark)
        ez2_str_particle_process(self->mark, drawn_up);
    for (it = self->blades.first; it != (struct Ez2StrParticle **)self->blades.last; it++) {
        struct Ez2StrParticle *blade = *it;
        float life;

        if (!blade)
            continue;
        life = blade->base.life_seconds;
        if (!(life == 0.0f) && !(life * 60.0f > (float)blade->base.age)) {
            particle_free(&blade->base);
            *it = 0;
            continue;
        }
        {
            Vec3 here = self->base.pos;
            blade->base.pos.x = BLADE_OFFSET(blade)->x + here.x;
            blade->base.pos.y = BLADE_OFFSET(blade)->y + here.y;
            blade->base.pos.z = BLADE_OFFSET(blade)->z + here.z;
        }
        ez2_str_particle_process(*it, drawn_up);
    }
}

void pneumaticus_procella_effect_render(struct Effect *effect)
{
    struct PneumaticusProcellaEffect *self = (struct PneumaticusProcellaEffect *)effect;
    struct Ez2StrParticle **it;

    if (self->mark)
        render_with_view(self->mark);
    for (it = self->blades.first; it < (struct Ez2StrParticle **)self->blades.last; it++)
        if (*it)
            render_with_view(*it);
}
