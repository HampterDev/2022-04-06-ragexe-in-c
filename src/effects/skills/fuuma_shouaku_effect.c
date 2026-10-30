#include "effects/effect_allocator.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/skills/ez2str_effect_ex.h"

#define EFFECT_FUUMA_SHOUAKU 0x930

struct FuumaShouakuEffect {
    struct Effect base;
    struct Ez2StrParticle *shuriken;
    struct Ez2StrParticle *hit;
    Vec3 step;
    int arrive_frame;
};

void fuuma_shouaku_effect_init(struct Effect *self);
void fuuma_shouaku_effect_process(struct Effect *self);
void fuuma_shouaku_effect_render(struct Effect *self);
void fuuma_shouaku_effect_release(struct Effect *self);
static void fuuma_shouaku_effect_destroy(struct Effect *effect);

static const struct EffectOps fuuma_shouaku_effect_ops = {
    .set_submit = effect_set_submit,
    .init = fuuma_shouaku_effect_init,
    .update_range = effect_update_range,
    .process = fuuma_shouaku_effect_process,
    .render = fuuma_shouaku_effect_render,
    .release = fuuma_shouaku_effect_release,
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
    .destroy = fuuma_shouaku_effect_destroy,
    .fade_out = effect_fade_out,
};

static void open_str(struct Ez2StrParticle *p, const char *path)
{
    ez2_str_particle_open(p, path);
}

struct FuumaShouakuEffect *fuuma_shouaku_effect_construct(struct FuumaShouakuEffect *self)
{
    effect_construct(&self->base, EFFECT_FUUMA_SHOUAKU);
    self->base.ops = &fuuma_shouaku_effect_ops;
    self->shuriken = 0;
    self->hit = 0;
    self->step.x = 0;
    self->step.y = 0;
    self->step.z = 0;
    self->arrive_frame = 0;
    return self;
}

struct Effect *fuuma_shouaku_effect_new(void)
{
    struct FuumaShouakuEffect *self = mem_try_alloc(sizeof *self);

    return self ? &fuuma_shouaku_effect_construct(self)->base : NULL;
}

void fuuma_shouaku_effect_destruct(struct FuumaShouakuEffect *self)
{
    self->base.ops = &fuuma_shouaku_effect_ops;
    effect_destruct(&self->base);
}

static void fuuma_shouaku_effect_destroy(struct Effect *effect)
{
    struct FuumaShouakuEffect *self = (struct FuumaShouakuEffect *)effect;
    fuuma_shouaku_effect_destruct(self);
    mem_free(self);
}

void fuuma_shouaku_effect_release(struct Effect *effect)
{
    struct FuumaShouakuEffect *self = (struct FuumaShouakuEffect *)effect;
    if (self->shuriken) {
        particle_free(&self->shuriken->base);
        self->shuriken = 0;
    }
    if (self->hit) {
        particle_free(&self->hit->base);
        self->hit = 0;
    }
}

void fuuma_shouaku_effect_init(struct Effect *effect)
{
    struct FuumaShouakuEffect *self = (struct FuumaShouakuEffect *)effect;
    struct Ez2StrParticle *p;
    Vec3 from, to;
    float dx, dy, dz, inv, view[12];

    self->base.has_instance_key = 1;
    p = self->shuriken = ez2str_particle_new(self->base.pos, 5);
    if (!p)
        return;
    open_str(p, "SHINKIRO_SHIRANUI\\SS_FUUMASHOUAKU\\fuumashouaku\\fuumashouaku.str");

    from = self->base.pos;
    to = self->base.target_pos;
    self->arrive_frame = (int)(float_sqrt((to.y - from.y) * (to.y - from.y) + (to.x - from.x) * (to.x - from.x) +
                                     (to.z - from.z) * (to.z - from.z)) * 0.5f);

    dx = to.x - from.x;
    dy = to.y - from.y;
    dz = to.z - from.z;
    self->step.x = dx;
    self->step.y = dy;
    self->step.z = dz;
    inv = 1.0f / float_sqrt(dx * dx + dy * dy + dz * dz);
    dx = self->step.x * inv;
    dy = self->step.y * inv;
    dz = self->step.z * inv;
    self->step.x = dx + dx;
    self->step.y = dy + dy;
    self->step.z = dz + dz;

    if (game_copy_view_matrix(view))
        self->shuriken->spin_degrees = effect_screen_angle(self->base.pos, self->base.target_pos, view);
}

void fuuma_shouaku_effect_process(struct Effect *effect)
{
    struct FuumaShouakuEffect *self = (struct FuumaShouakuEffect *)effect;
    Vec3 zero = { 0, 0, 0 };

    if (self->base.frame >= self->arrive_frame) {
        if (!self->hit) {
            if (self->shuriken) {
                particle_free(&self->shuriken->base);
                self->shuriken = 0;
            }
            self->hit = ez2str_particle_new(self->base.target_pos, 5);
            if (self->hit) {
                open_str(self->hit, "SHINKIRO_SHIRANUI\\SS_FUUMASHOUAKU\\fuumashouaku_00\\fuumashouaku_00.str");
                self->hit->loop_forever = 1;
                effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\SHINKIRO_SHIRANUI\\SS_FUUMASHOUAKU.wav",
                                     self->arrive_frame + 1, 0.0f);
            }
        } else if (self->hit->frame == 1) {
            self->base.frame = self->arrive_frame;
        }
    } else if (self->shuriken) {
        struct Ez2StrParticle *p = self->shuriken;
        Vec3 at = p->base.pos;
        p->base.pos.x = self->step.x + at.x;
        p->base.pos.y = self->step.y + at.y;
        p->base.pos.z = self->step.z + at.z;
    }
    if (self->shuriken)
        ez2_str_particle_process(self->shuriken, zero);
    if (self->hit)
        ez2_str_particle_process(self->hit, zero);
}

void fuuma_shouaku_effect_render(struct Effect *effect)
{
    struct FuumaShouakuEffect *self = (struct FuumaShouakuEffect *)effect;

    if (self->shuriken) {
        ez2_str_particle_render_current(self->shuriken);
    }
    if (self->hit) {
        ez2_str_particle_render_current(self->hit);
    }
}
