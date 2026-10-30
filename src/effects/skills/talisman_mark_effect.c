#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "core/fatal.h"
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define TALISMANS 4
#define LIST_GROUND 0xd
#define ORBIT_RADIUS 8.5f
#define ORBIT_HEIGHT 20.0f

struct TalismanMarkEffect {
    struct Effect base;
    struct Vec talismans;
};

enum TalismanPhase {
    TALISMAN_START = 0,
    TALISMAN_LOOP = 1,
    TALISMAN_END = 2,
};
struct TalismanMarkParticle {
    int talisman_angle;
    signed char talisman_phase;
};
PARTICLE_EFFECT_DATA_FITS(TalismanMarkParticle);
static __inline struct TalismanMarkParticle *talisman_mark_particle(const void *p)
{
    return (struct TalismanMarkParticle *)((const struct Particle *)p)->effect_data;
}

void talisman_mark_effect_init(struct Effect *self);
void talisman_mark_effect_process(struct Effect *self);
void talisman_mark_effect_render(struct Effect *self);
void talisman_mark_effect_release(struct Effect *self);
unsigned char talisman_mark_effect_is_alive(struct Effect *self);
static void talisman_mark_effect_destroy(struct Effect *effect);

static const struct EffectOps talisman_mark_effect_ops = {
    .set_submit = effect_set_submit,
    .init = talisman_mark_effect_init,
    .update_range = effect_update_range,
    .process = talisman_mark_effect_process,
    .render = talisman_mark_effect_render,
    .release = talisman_mark_effect_release,
    .is_alive = talisman_mark_effect_is_alive,
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
    .destroy = talisman_mark_effect_destroy,
    .fade_out = effect_fade_out,
};
static const char *const talisman_start_paths[TALISMANS] = {
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\blue_dragon_start.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\white_tiger_start.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\red_phoenix_start.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\black_tortoise_start.str",
    };
static const char *const talisman_loop_paths[TALISMANS] = {
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\blue_dragon_loop.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\white_tiger_loop.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\red_phoenix_loop.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\black_tortoise_loop.str",
    };
static const char *const talisman_end_paths[TALISMANS] = {
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\blue_dragon_end.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\white_tiger_end.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\red_phoenix_end.str",
    "SOUL_ASCETIC\\EFST_T_XXXX_GOD\\black_tortoise_end.str",
    };

static const Vec3 no_offset = { 0, 0, 0 };

static void transform(Vec3 *out, const float *m, float x, float y, float z)
{
    out->x = y * m[3] + x * m[0] + z * m[6] + m[9];
    out->y = y * m[4] + x * m[1] + z * m[7] + m[10];
    out->z = y * m[5] + x * m[2] + z * m[8] + m[11];
}

static struct Ez2StrParticle *new_talisman(struct TalismanMarkEffect *self, const char *const *names, int god)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, LIST_GROUND);

    if (!p)
        return 0;
    p->base.effect_type = self->base.type;
    p->base.index = god;
    p->base.group = 0;
    p->base.owner_tag = 1;
    particle_quad_unit_corners(p->base.corners);
    p->actor_aid = self->base.owner_aid;
    if ((unsigned)god >= TALISMANS)
        fatal_error("invalid talisman index");
    ez2_str_particle_open(p, names[god]);
    return p;
}

static struct Ez2StrParticle *replace_talisman(struct TalismanMarkEffect *self, struct Ez2StrParticle **it,
                                                const char *const *names, int phase)
{
    struct Ez2StrParticle *old = *it, *p;
    int god = old->base.index;
    int angle = talisman_mark_particle(old)->talisman_angle;

    particle_free(&old->base);
    p = new_talisman(self, names, god);
    if (p) {
        talisman_mark_particle(p)->talisman_angle = angle;
        talisman_mark_particle(p)->talisman_phase = (signed char)phase;
    }
    *it = p;
    return p;
}

static int still_playing(const struct Ez2StrParticle *p)
{
    float life = p->base.life_seconds;
    return life == 0.0f || life * 60.0f > (float)p->base.age;
}

struct TalismanMarkEffect *talisman_mark_effect_construct(struct TalismanMarkEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &talisman_mark_effect_ops;
    self->talismans.first = self->talismans.last = self->talismans.end = 0;
    return self;
}

struct Effect *talisman_mark_effect_new(void)
{
    struct TalismanMarkEffect *self = mem_try_alloc(sizeof *self);

    return self ? &talisman_mark_effect_construct(self)->base : NULL;
}

void talisman_mark_effect_destruct(struct TalismanMarkEffect *self)
{
    self->base.ops = &talisman_mark_effect_ops;
    vec_free(&self->talismans);
    effect_destruct(&self->base);
}

static void talisman_mark_effect_destroy(struct Effect *effect)
{
    struct TalismanMarkEffect *self = (struct TalismanMarkEffect *)effect;
    talisman_mark_effect_destruct(self);
    mem_free(self);
}

void talisman_mark_effect_release(struct Effect *effect)
{
    struct TalismanMarkEffect *self = (struct TalismanMarkEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->talismans.first; it != (struct Ez2StrParticle **)self->talismans.last; it++)
        if (*it)
            particle_free(&(*it)->base);
    self->talismans.last = self->talismans.first;
}

void talisman_mark_effect_init(struct Effect *effect)
{
    struct TalismanMarkEffect *self = (struct TalismanMarkEffect *)effect;
    unsigned god;

    self->base.has_instance_key = 1;
    for (god = 0; god < ((unsigned)self->base.level < TALISMANS ? (unsigned)self->base.level : TALISMANS); god++) {
        struct Ez2StrParticle *p = new_talisman(self, talisman_start_paths, god);

        if (!p)
            continue;
        talisman_mark_particle(p)->talisman_angle = ((int)god / 2 + (int)god % 2 * 2) * 90;
        talisman_mark_particle(p)->talisman_phase = TALISMAN_START;
        vec_push(&self->talismans, &p, sizeof(struct Ez2StrParticle *));
    }
}

void talisman_mark_effect_finish(struct TalismanMarkEffect *self)
{
    struct Ez2StrParticle **it;
    for (it = self->talismans.first; it != (struct Ez2StrParticle **)self->talismans.last; it++)
        if (*it)
            replace_talisman(self, it, talisman_end_paths, TALISMAN_END);
}

unsigned char talisman_mark_effect_is_alive(struct Effect *effect)
{
    struct TalismanMarkEffect *self = (struct TalismanMarkEffect *)effect;
    return effect_is_owner_alive(&self->base) == 1 && self->talismans.first != self->talismans.last;
}

void talisman_mark_effect_place_talisman(struct TalismanMarkEffect *self,
                                                   struct Ez2StrParticle *p)
{
    float view[12], to_world[12];
    Vec3 here, seen;
    float rad, x, y, z;
    int angle;

    if (!p)
        return;
    here = self->base.pos;
    if (!game_copy_view_matrix(view))
        return;
    transform(&seen, view, here.x, here.y, here.z);
    mat_invert(to_world, view);

    angle = talisman_mark_particle(p)->talisman_angle;
    if (talisman_mark_particle(p)->talisman_phase == TALISMAN_LOOP) {
        angle = (angle + 1) % 360;
        talisman_mark_particle(p)->talisman_angle = angle;
    }
    rad = (float)angle / 180.0f * 3.141592f;
    x = seen.x + float_cos(rad) * ORBIT_RADIUS;
    y = float_sin(rad) * ORBIT_RADIUS - ORBIT_HEIGHT + seen.y;
    z = seen.z + 0.0f;
    transform(&p->base.pos, to_world, x, y, z);
    ez2_str_particle_process(p, no_offset);
}

void talisman_mark_effect_process(struct Effect *effect)
{
    struct TalismanMarkEffect *self = (struct TalismanMarkEffect *)effect;
    struct Ez2StrParticle **it = self->talismans.first;

    while (it != (struct Ez2StrParticle **)self->talismans.last) {
        struct Ez2StrParticle *p = *it;

        if (!p)
            continue;
        switch (talisman_mark_particle(p)->talisman_phase) {
        case TALISMAN_START:
            if (!still_playing(p)) {
                p = replace_talisman(self, it, talisman_loop_paths, TALISMAN_LOOP);
                if (!p)
                    continue;
                p->loop_forever = 1;
            }
            break;
        case TALISMAN_LOOP:
            break;
        case TALISMAN_END:
            if (!still_playing(p)) {
                particle_free(&p->base);
                memmove(it, it + 1, (char *)self->talismans.last - (char *)(it + 1));
                self->talismans.last = (struct Ez2StrParticle **)self->talismans.last - 1;
                continue;
            }
            break;
        default:
            continue;
        }
        talisman_mark_effect_place_talisman(self, p);
        it++;
    }
}

void talisman_mark_effect_render(struct Effect *effect)
{
    struct TalismanMarkEffect *self = (struct TalismanMarkEffect *)effect;
    struct Ez2StrParticle **it;

    for (it = self->talismans.first; it != (struct Ez2StrParticle **)self->talismans.last; it++) {
        if (*it) {
            ez2_str_particle_render_current(*it);
        }
    }
}
