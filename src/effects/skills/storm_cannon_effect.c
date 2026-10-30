#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define EFFECT_STORM_CANNON 0x6db
#define LIST_WORLD 5
#define LIST_GROUND 0xd
#define FADE_STEP 10

struct StormCannonEffect {
    struct Effect base;
    struct Vec parts;
    Vec3 heading;
    Vec3 start;
    double speed;
    double range;
};

#define EFFECT_ALPHA(e) ((e)->base.params.alpha)

void storm_cannon_effect_init(struct Effect *self);
void storm_cannon_effect_process(struct Effect *self);
void storm_cannon_effect_render(struct Effect *self);
void storm_cannon_effect_release(struct Effect *self);
unsigned char storm_cannon_effect_is_alive(struct Effect *self);
static void storm_cannon_effect_destroy(struct Effect *effect);

static const struct EffectOps storm_cannon_effect_ops = {
    .set_submit = effect_set_submit,
    .init = storm_cannon_effect_init,
    .update_range = effect_update_range,
    .process = storm_cannon_effect_process,
    .render = storm_cannon_effect_render,
    .release = storm_cannon_effect_release,
    .is_alive = storm_cannon_effect_is_alive,
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
    .destroy = storm_cannon_effect_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 no_offset = { 0, 0, 0 };

static void render_with_view(struct Ez2StrParticle *p)
{
    ez2_str_particle_render_current(p);
}

static void add_part(struct StormCannonEffect *self, int list_flags, const char *name)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, list_flags);

    if (!p)
        return;
    ez2_str_particle_open(p, name);
    p->loop_forever = 1;
    vec_push(&self->parts, &p, sizeof(struct Ez2StrParticle *));
}

struct StormCannonEffect *storm_cannon_effect_construct(struct StormCannonEffect *self)
{
    effect_construct(&self->base, EFFECT_STORM_CANNON);
    self->base.ops = &storm_cannon_effect_ops;
    self->parts.first = self->parts.last = self->parts.end = 0;
    self->heading.x = self->heading.y = self->heading.z = 0;
    self->start.x = self->start.y = self->start.z = 0;
    return self;
}

struct Effect *storm_cannon_effect_new(void)
{
    struct StormCannonEffect *self = mem_try_alloc(sizeof *self);

    return self ? &storm_cannon_effect_construct(self)->base : NULL;
}

void storm_cannon_effect_destruct(struct StormCannonEffect *self)
{
    self->base.ops = &storm_cannon_effect_ops;
    vec_free(&self->parts);
    effect_destruct(&self->base);
}

static void storm_cannon_effect_destroy(struct Effect *effect)
{
    struct StormCannonEffect *self = (struct StormCannonEffect *)effect;
    storm_cannon_effect_destruct(self);
    mem_free(self);
}

void storm_cannon_effect_release(struct Effect *effect)
{
    struct StormCannonEffect *self = (struct StormCannonEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        particle_free(&(*it)->base);
    self->parts.last = self->parts.first;
}

void storm_cannon_effect_init(struct Effect *effect)
{
    struct StormCannonEffect *self = (struct StormCannonEffect *)effect;
    float dx, dy, dz, inv_len;
    Vec3 from, to;

    add_part(self, LIST_GROUND, "stormcannon\\stormcannon_bottom\\stormcannon_bottom.str");
    add_part(self, LIST_WORLD, "stormcannon\\stormcannon\\stormcannon.str");

    from = self->base.pos;
    to = self->base.target_pos;
    dx = to.x - from.x;
    dy = to.y - from.y;
    dz = to.z - from.z;
    self->heading.x = dx;
    self->heading.y = dy;
    self->heading.z = dz;
    inv_len = (float)(1.0 / (double)float_sqrt(dx * dx + dy * dy + dz * dz));
    self->heading.x = self->heading.x * inv_len;
    self->heading.y = self->heading.y * inv_len;
    self->heading.z = self->heading.z * inv_len;

    self->start = self->base.pos;
    self->speed = 1.7;

    from = self->base.pos;
    to = self->base.target_pos;
    dx = to.x - from.x;
    dy = to.y - from.y;
    dz = to.z - from.z;
    self->range = float_sqrt(dy * dy + dx * dx + dz * dz);
    EFFECT_ALPHA(self) = 255;
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\AG_STORM_CANNON.wav", 0, 0.0f);
}

unsigned char storm_cannon_effect_is_alive(struct Effect *effect)
{
    struct StormCannonEffect *self = (struct StormCannonEffect *)effect;
    return EFFECT_ALPHA(self) > 0;
}

void storm_cannon_effect_process_part(struct StormCannonEffect *self,
                                                struct Ez2StrParticle *p)
{
    Vec3 here;

    if (!p)
        return;
    p->alpha_scale = (float)((double)EFFECT_ALPHA(self) / 255.0);
    here = self->base.pos;
    p->base.pos.x = here.x - 1.5f;
    p->base.pos.y = here.y - 17.0f;
    p->base.pos.z = here.z + 0.0f;
    ez2_str_particle_process(p, no_offset);
}

void storm_cannon_effect_process(struct Effect *effect)
{
    struct StormCannonEffect *self = (struct StormCannonEffect *)effect;
    float step = (float)self->speed;
    Vec3 here = self->base.pos;
    float x, y, z, gone;
    struct Ez2StrParticle **it;

    y = self->heading.y * step + here.y;
    x = self->heading.x * step + here.x;
    z = self->heading.z * step + here.z;
    self->base.pos.y = y;
    self->base.pos.x = x;
    self->base.pos.z = z;
    y -= self->start.y;
    x -= self->start.x;
    z -= self->start.z;
    gone = float_sqrt(y * y + x * x + z * z);
    if ((double)gone > self->range) {
        int alpha = EFFECT_ALPHA(self) - FADE_STEP;
        EFFECT_ALPHA(self) = alpha <= 0 ? 0 : alpha;
    }
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        storm_cannon_effect_process_part(self, *it);
}

void storm_cannon_effect_render(struct Effect *effect)
{
    struct StormCannonEffect *self = (struct StormCannonEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        if (*it)
            render_with_view(*it);
}
