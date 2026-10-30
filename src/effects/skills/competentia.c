#include "effects/effect_allocator.h"
#include "resources/attr_queries.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_COMPETENTIA 0x73b
#define SPARKLES 10

struct Competentia {
    struct Effect base;
    struct Vec sparkles;
};

void competentia_init(struct Effect *self);
void competentia_process(struct Effect *self);
void competentia_render(struct Effect *self);
void competentia_release(struct Effect *self);
unsigned char competentia_is_alive(struct Effect *self);
void competentia_set_stop_frame(struct Effect *self, float frame);
static void competentia_destroy(struct Effect *effect);

static const struct EffectOps competentia_ops = {
    .set_submit = effect_set_submit,
    .init = competentia_init,
    .update_range = effect_update_range,
    .process = competentia_process,
    .render = competentia_render,
    .release = competentia_release,
    .is_alive = competentia_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = competentia_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = competentia_destroy,
    .fade_out = effect_fade_out,
};

static int still_playing(const struct Ez2StrParticle *p)
{
    return p->base.life_seconds == 0.0f || p->base.life_seconds * 60.0f > (float)p->base.age;
}

struct Competentia *competentia_construct(struct Competentia *self)
{
    effect_construct(&self->base, EFFECT_COMPETENTIA);
    self->base.ops = &competentia_ops;
    self->sparkles.first = self->sparkles.last = self->sparkles.end = 0;
    return self;
}

struct Effect *competentia_new(void)
{
    struct Competentia *self = mem_try_alloc(sizeof *self);

    return self ? &competentia_construct(self)->base : NULL;
}

void competentia_destruct(struct Competentia *self)
{
    self->base.ops = &competentia_ops;
    vec_free(&self->sparkles);
    effect_destruct(&self->base);
}

static void competentia_destroy(struct Effect *effect)
{
    struct Competentia *self = (struct Competentia *)effect;
    competentia_destruct(self);
    mem_free(self);
}

void competentia_release(struct Effect *effect)
{
    struct Competentia *self = (struct Competentia *)effect;
    struct Ez2StrParticle **it;
    for (it = self->sparkles.first; it != (struct Ez2StrParticle **)self->sparkles.last; it++)
        particle_free(&(*it)->base);
    self->sparkles.last = self->sparkles.first;
}

void competentia_init(struct Effect *effect)
{
    struct Competentia *self = (struct Competentia *)effect;
    int i;

    for (i = 0; i < SPARKLES; i++) {
        struct Ez2StrParticle *p;
        const char *path = "competentia\\competentia_cast_particle\\competentia_cast_particle.str";
        float spot[2];
        void *ground;
        Vec3 at = self->base.pos;

        effect_random_point_in_disc(spot, at.x, at.z, 50);
        at.x = spot[0];
        at.z = spot[1];
        ground = game_current_map_attr();
        if (!ground)
            break;
        at.y = c3d_attr_get_height(ground, at.x, at.z);
        p = ez2str_particle_new(at, 5);
        if (!p)
            continue;
        ez2_str_particle_load(p, path);
        p->loop_forever = 1;
        vec_push(&self->sparkles, &p, sizeof p);
    }
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\EF_BeginSpell.wav", 0, 0.0f);
}

unsigned char competentia_is_alive(struct Effect *effect)
{
    struct Competentia *self = (struct Competentia *)effect;
    return self->sparkles.first != self->sparkles.last;
}

void competentia_process(struct Effect *effect)
{
    struct Competentia *self = (struct Competentia *)effect;
    struct Ez2StrParticle **it = self->sparkles.first;
    Vec3 raised = { 0, -80.0f, 0 };

    while (it != (struct Ez2StrParticle **)self->sparkles.last) {
        struct Ez2StrParticle *p = *it;
        if (!still_playing(p)) {
            particle_free(&p->base);
            memmove(it, it + 1, (char *)self->sparkles.last - (char *)(it + 1));
            self->sparkles.last = (struct Ez2StrParticle **)self->sparkles.last - 1;
            continue;
        }
        if (p)
            ez2_str_particle_process(p, raised);
        it++;
    }
}

void competentia_render(struct Effect *effect)
{
    struct Competentia *self = (struct Competentia *)effect;
    struct Ez2StrParticle **it;

    for (it = self->sparkles.first; it != (struct Ez2StrParticle **)self->sparkles.last; it++)
        if (*it) {
            ez2_str_particle_render_current(*it);
        }
}

void competentia_set_stop_frame(struct Effect *effect, float frame)
{
    struct Competentia *self = (struct Competentia *)effect;
    struct Ez2StrParticle **it;
    for (it = self->sparkles.first; it != (struct Ez2StrParticle **)self->sparkles.last; it++)
        if (*it)
            (*it)->stop_frame = (int)frame;
}
