#include "effects/effect_allocator.h"
#include <math.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

#define MONSTER_DIR "몬스터\\"
#define EFFECT_SVG_SPIRIT 0x468

struct SvgSpiritEffect {
    struct Effect base;
    struct SpriteParticle *spirit;
};

struct SvgSpiritParticle {
    int heading;
};
PARTICLE_EFFECT_DATA_FITS(SvgSpiritParticle);
static __inline struct SvgSpiritParticle *svg_spirit_particle(const void *p)
{
    return (struct SvgSpiritParticle *)((const struct Particle *)p)->effect_data;
}
#define SPRITE_OFFSET(sp) (&(sp)->sprite_offset)

void svg_spirit_effect_init(struct Effect *self);
void svg_spirit_effect_process(struct Effect *self);
void svg_spirit_effect_render(struct Effect *self);
void svg_spirit_effect_release(struct Effect *self);
unsigned char svg_spirit_effect_is_alive(struct Effect *self);
static void svg_spirit_effect_destroy(struct Effect *effect);

static const struct EffectOps svg_spirit_effect_ops = {
    .set_submit = effect_set_submit,
    .init = svg_spirit_effect_init,
    .update_range = effect_update_range,
    .process = svg_spirit_effect_process,
    .render = svg_spirit_effect_render,
    .release = svg_spirit_effect_release,
    .is_alive = svg_spirit_effect_is_alive,
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
    .destroy = svg_spirit_effect_destroy,
    .fade_out = effect_fade_out,
};

struct SvgSpiritEffect *svg_spirit_effect_construct(struct SvgSpiritEffect *self)
{
    effect_construct(&self->base, EFFECT_SVG_SPIRIT);
    self->base.ops = &svg_spirit_effect_ops;
    self->spirit = 0;
    return self;
}

struct Effect *svg_spirit_effect_new(void)
{
    struct SvgSpiritEffect *self = mem_try_alloc(sizeof *self);

    return self ? &svg_spirit_effect_construct(self)->base : NULL;
}

void svg_spirit_effect_destruct(struct SvgSpiritEffect *self)
{
    self->base.ops = &svg_spirit_effect_ops;
    effect_destruct(&self->base);
}

static void svg_spirit_effect_destroy(struct Effect *effect)
{
    struct SvgSpiritEffect *self = (struct SvgSpiritEffect *)effect;
    svg_spirit_effect_destruct(self);
    mem_free(self);
}

void svg_spirit_effect_release(struct Effect *effect)
{
    struct SvgSpiritEffect *self = (struct SvgSpiritEffect *)effect;
    if (self->spirit) {
        particle_free(&self->spirit->base);
        self->spirit = 0;
    }
}

void svg_spirit_effect_init(struct Effect *effect)
{
    struct SvgSpiritEffect *self = (struct SvgSpiritEffect *)effect;
    struct SpriteParticle *sp;
    struct Particle *head;
    Vec3 here, target;
    float dx, dz, deg, heading;

    self->spirit = sprite_particle_new(self->base.pos, 0x101);
    if (!self->spirit)
        return;
    head = (struct Particle *)self->spirit;
    head->effect_type = self->base.type;
    head->index = 0;
    head->group = 0;
    head->owner_tag = 1;
    sprite_particle_set_spr(self->spirit, MONSTER_DIR "svg_spirit.spr");
    sprite_particle_set_act(self->spirit, MONSTER_DIR "svg_spirit.act");
    sp = self->spirit;
    SPRITE_OFFSET(sp)->x = 0;
    SPRITE_OFFSET(sp)->y = 0;
    SPRITE_OFFSET(sp)->z = 0;
    here = self->base.pos;
    target = self->base.target_pos;
    dx = target.x - here.x;
    dz = target.z - here.z;
    self->base.duration = 16;
    deg = (float)atan2((double)dx, (double)-dz) / g_pi * g_halfTurnDegrees;
    if (0.0f > deg)
        deg += 360.0f;
    heading = g_halfTurnDegrees - deg;
    if (0.0f > heading)
        heading += 360.0f;
    svg_spirit_particle(self->spirit)->heading = (int)heading;
    self->spirit->frame_delay = float_sqrt(dz * dz + dx * dx) / (float)self->base.duration;
    self->spirit->base.age = 0;
}

unsigned char svg_spirit_effect_is_alive(struct Effect *effect)
{
    struct SvgSpiritEffect *self = (struct SvgSpiritEffect *)effect;
    return self->spirit->base.age < self->base.duration;
}

void svg_spirit_effect_process(struct Effect *effect)
{
    struct SvgSpiritEffect *self = (struct SvgSpiritEffect *)effect;
    if (self->spirit->base.age >= 0) {
        struct SpriteParticle *sp;
        Vec3 here, target, step;
        float dx, dy, dz, inv_len, speed;
        int heading;

        effect_set_target(&self->base, self->base.target_aid);
        heading = svg_spirit_particle(self->spirit)->heading;
        self->spirit->base.roll =
            (float)heading - game_camera_heading();
        here = self->base.pos;
        target = self->base.target_pos;
        dy = target.y - here.y;
        dx = target.x - here.x;
        dz = target.z - here.z;
        inv_len = (float)(1.0 / (double)float_sqrt(dy * dy + dx * dx + dz * dz));
        sp = self->spirit;
        speed = sp->frame_delay;
        step.y = inv_len * dy * speed + SPRITE_OFFSET(sp)->y;
        step.x = inv_len * dx * speed + SPRITE_OFFSET(sp)->x;
        step.z = inv_len * dz * speed + SPRITE_OFFSET(sp)->z;
        SPRITE_OFFSET(sp)->y = step.y;
        SPRITE_OFFSET(sp)->x = step.x;
        SPRITE_OFFSET(sp)->z = step.z;
        here = self->base.pos;
        sp = self->spirit;
        sp->base.pos.x = step.x + here.x;
        sp->base.pos.y = here.y - 10.0f;
        sp->base.pos.z = step.z + here.z;
        if (self->base.frame >= 10)
            self->spirit->base.alpha -= 40;
    }
    self->spirit->base.age++;
}

void svg_spirit_effect_render(struct Effect *effect)
{
    struct SvgSpiritEffect *self = (struct SvgSpiritEffect *)effect;
    int age = self->spirit->base.age;
    if (age > 0 && age < self->base.duration)
        sprite_particle_render(self->spirit);
}
