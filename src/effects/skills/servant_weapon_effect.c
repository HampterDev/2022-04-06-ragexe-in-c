#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_controls.h"
#include "std/list.h"

struct Effect;
struct ServantWeaponEffect;
void servant_weapon_effect_init(struct Effect *self);
void servant_weapon_effect_process(struct Effect *self);
void servant_weapon_effect_render(struct Effect *self);
void servant_weapon_effect_release(struct Effect *self);
static void servant_weapon_effect_destroy(struct Effect *effect);

static const struct EffectOps servant_weapon_effect_ops = {
    .set_submit = effect_set_submit,
    .init = servant_weapon_effect_init,
    .update_range = effect_update_range,
    .process = servant_weapon_effect_process,
    .render = servant_weapon_effect_render,
    .release = servant_weapon_effect_release,
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
    .destroy = servant_weapon_effect_destroy,
    .fade_out = effect_fade_out,
};

#define EFFECT_SERVANT_WEAPON_LIST 0xd
#define SLOTS 5

struct ServantWeaponEffect {
    struct Effect base;
    Vec3 slot_offset[SLOTS];
    struct Ez2StrParticle *weapons[SLOTS];
    struct Ez2StrParticle *casts[SLOTS];
    struct Ez2StrParticle *shots[SLOTS];
};

static struct Ez2StrParticle *open_animation(struct ServantWeaponEffect *self, const char *path)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, EFFECT_SERVANT_WEAPON_LIST);

    ez2_str_particle_open(p, path);
    return p;
}

#define WEAPON_STR "servantweapon\\servantweapon\\servantweapon.str"
#define CAST_STR "servantweapon\\servantweapon_cast\\servantweapon_cast.str"

static int animation_alive(const struct Ez2StrParticle *p)
{
    return p->base.life_seconds == 0.0f || p->base.life_seconds * 60.0f > (float)p->base.age;
}

struct ServantWeaponEffect *servant_weapon_effect_construct(struct ServantWeaponEffect *self)
{
    int i;

    effect_construct(&self->base, -1);
    self->base.ops = &servant_weapon_effect_ops;
    for (i = 0; i < SLOTS; i++)
        self->slot_offset[i].x = self->slot_offset[i].y = self->slot_offset[i].z = 0;
    return self;
}

struct Effect *servant_weapon_effect_new(void)
{
    struct ServantWeaponEffect *self = mem_try_alloc(sizeof *self);

    return self ? &servant_weapon_effect_construct(self)->base : NULL;
}

static void servant_weapon_effect_destroy(struct Effect *effect)
{
    struct ServantWeaponEffect *self = (struct ServantWeaponEffect *)effect;
    self->base.ops = &servant_weapon_effect_ops;
    effect_destruct(&self->base);
    mem_free(self);
}

void servant_weapon_effect_add_weapon(struct ServantWeaponEffect *self, int i)
{
    struct Ez2StrParticle *p;

    if (self->weapons[i])
        return;
    p = open_animation(self, WEAPON_STR);
    p->loop_forever = 1;
    self->weapons[i] = p;
}

void servant_weapon_effect_add_cast(struct Effect *effect)
{
    struct ServantWeaponEffect *self = (struct ServantWeaponEffect *)effect;
    int i;

    for (i = 0; i < SLOTS; i++)
        if (!self->weapons[i] && !self->casts[i])
            break;
    if (i == SLOTS)
        return;
    self->casts[i] = open_animation(self, CAST_STR);
}

void servant_weapon_effect_release(struct Effect *effect)
{
    struct ServantWeaponEffect *self = (struct ServantWeaponEffect *)effect;
    int i;

    for (i = 0; i < SLOTS; i++) {
        if (self->weapons[i]) {
            particle_free(&self->weapons[i]->base);
            self->weapons[i] = 0;
        }
        if (self->casts[i]) {
            particle_free(&self->casts[i]->base);
            self->casts[i] = 0;
        }
        if (self->shots[i]) {
            particle_free(&self->shots[i]->base);
            self->shots[i] = 0;
        }
    }
}

void servant_weapon_effect_init(struct Effect *effect)
{
    struct ServantWeaponEffect *self = (struct ServantWeaponEffect *)effect;
    static const float offsets[SLOTS][2] = {
        { -7.0f, -2.0f }, { -10.0f, -12.0f }, { 0.0f, -19.0f }, { 10.0f, -12.0f }, { 7.0f, -2.0f },
    };
    int i;

    self->base.has_instance_key = 1;
    for (i = 0; i < SLOTS; i++) {
        self->slot_offset[i].x = offsets[i][0];
        self->slot_offset[i].y = offsets[i][1];
        self->slot_offset[i].z = 0.0f;
        self->weapons[i] = 0;
        self->casts[i] = 0;
        self->shots[i] = 0;
    }
    for (i = 0; i < SLOTS; i++)
        servant_weapon_effect_add_cast(&self->base);
}

void servant_weapon_effect_step(struct ServantWeaponEffect *self,
                                           struct Ez2StrParticle *p, unsigned i)
{
    float view[12], inv[12];
    Vec3 pos, v, no_offset;
    float a, b, c;

    if (!p || i >= SLOTS)
        return;
    pos = self->base.pos;
    if (!game_copy_view_matrix(view))
        return;
    v.x = pos.x * view[0] + pos.y * view[3] + view[6] * pos.z + view[9];
    v.y = view[4] * pos.y + view[1] * pos.x + view[7] * pos.z + view[10];
    v.z = view[2] * pos.x + view[5] * pos.y + view[8] * pos.z + view[11];
    mat_invert(inv, view);

    a = self->slot_offset[i].x + v.x;
    b = self->slot_offset[i].y + v.y;
    c = self->slot_offset[i].z + v.z;
    p->base.pos.x = a * inv[0] + b * inv[3] + c * inv[6] + inv[9];
    p->base.pos.y = inv[1] * a + inv[4] * b + inv[7] * c + inv[10];
    p->base.pos.z = inv[2] * a + inv[5] * b + inv[8] * c + inv[11];
    no_offset.x = no_offset.y = no_offset.z = 0;
    ez2_str_particle_process(p, no_offset);
    p->base.age++;
}

void servant_weapon_effect_process(struct Effect *effect)
{
    struct ServantWeaponEffect *self = (struct ServantWeaponEffect *)effect;
    int i;

    for (i = 0; i < SLOTS; i++) {
        struct Ez2StrParticle *p;

        if (self->weapons[i])
            servant_weapon_effect_step(self, self->weapons[i], i);

        p = self->casts[i];
        if (p) {
            if (animation_alive(p)) {
                servant_weapon_effect_step(self, p, i);
            } else {
                particle_free(&p->base);
                self->casts[i] = 0;
                servant_weapon_effect_add_weapon(self, i);
            }
        }

        p = self->shots[i];
        if (p) {
            if (animation_alive(p)) {
                servant_weapon_effect_step(self, p, i);
            } else {
                particle_free(&p->base);
                self->shots[i] = 0;
            }
        }
    }
}

void servant_weapon_effect_render_particle(struct Ez2StrParticle *p)
{
    if (!p)
        return;
    ez2_str_particle_render_current(p);
}

void servant_weapon_effect_render(struct Effect *effect)
{
    struct ServantWeaponEffect *self = (struct ServantWeaponEffect *)effect;
    int i;

    for (i = 0; i < SLOTS; i++) {
        servant_weapon_effect_render_particle(self->weapons[i]);
        servant_weapon_effect_render_particle(self->casts[i]);
        servant_weapon_effect_render_particle(self->shots[i]);
    }
}
