#include "effects/effect_allocator.h"
#include "render/projection.h"
#include "render/math3d.h"
#include <math.h>
#include <string.h>
#include "platform/win_types.h"
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"
#include "platform/clock.h"

#define CANE_LIST 5
#define CANE_COUNT 8
#define LIGHT_PERIOD_MS 1000

enum CanePhase {
    PHASE_START = 1,
    PHASE_LOOP = 2,
    PHASE_END = 3,
};

struct CaneOfEvilEyeEffect {
    struct Effect base;
    DWORD last_lights_ms;
    struct Vec canes;
    struct Vec lights;
};

struct CaneOfEvilEyeParticle {
    unsigned char str_phase;
};
PARTICLE_EFFECT_DATA_FITS(CaneOfEvilEyeParticle);
static __inline struct CaneOfEvilEyeParticle *cane_of_evil_eye_particle(const void *p)
{
    return (struct CaneOfEvilEyeParticle *)((const struct Particle *)p)->effect_data;
}
#define CANE_OFFSET(p) ((p)->base.offset)
#define LIGHT_OFFSET(p) ((p)->base.second_offset)
#define PARTICLE_LIFE(p) ((p)->base.life_seconds)

void cane_of_evil_eye_effect_init(struct Effect *self);
void cane_of_evil_eye_effect_process(struct Effect *self);
void cane_of_evil_eye_effect_render(struct Effect *self);
void cane_of_evil_eye_effect_release(struct Effect *self);
unsigned char cane_of_evil_eye_effect_is_alive(struct Effect *self);
static void cane_of_evil_eye_effect_destroy(struct Effect *effect);

static const struct EffectOps cane_of_evil_eye_effect_ops = {
    .set_submit = effect_set_submit,
    .init = cane_of_evil_eye_effect_init,
    .update_range = effect_update_range,
    .process = cane_of_evil_eye_effect_process,
    .render = cane_of_evil_eye_effect_render,
    .release = cane_of_evil_eye_effect_release,
    .is_alive = cane_of_evil_eye_effect_is_alive,
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
    .destroy = cane_of_evil_eye_effect_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 cane_offsets[CANE_COUNT] = {
    { -20.0f, -12.0f, -20.0f }, { 0.0f, -12.0f, -20.0f }, { 20.0f, -12.0f, -20.0f },
    { -20.0f, -12.0f, 0.0f },                             { 20.0f, -12.0f, 0.0f },
    { -20.0f, -12.0f, 20.0f },  { 0.0f, -12.0f, 20.0f },  { 20.0f, -12.0f, 20.0f },
};

#define CANE_START_STR "NPC_CANE_OF_EVIL_EYE\\npc_cane_of_evil_eye\\npc_cane_of_evil_eye_start.str"
#define CANE_LOOP_STR "NPC_CANE_OF_EVIL_EYE\\npc_cane_of_evil_eye\\npc_cane_of_evil_eye.str"
#define CANE_END_STR "NPC_CANE_OF_EVIL_EYE\\npc_cane_of_evil_eye\\npc_cane_of_evil_eye_end.str"
#define LIGHT_START_STR "NPC_CANE_OF_EVIL_EYE\\npc_cane_of_evil_eye_light\\npc_cane_of_evil_eye_light_03c_start.str"
#define LIGHT_END_STR "NPC_CANE_OF_EVIL_EYE\\npc_cane_of_evil_eye_light\\npc_cane_of_evil_eye_light_03c_end.str"

static void load_str(struct Ez2StrParticle *p, const char *str)
{
    ez2_str_particle_load(p, str);
}

static void particles_sweep(struct Vec *v)
{
    struct Ez2StrParticle **it = v->first;
    while (it != (struct Ez2StrParticle **)v->last) {
        if (!*it) {
            memmove(it, it + 1, (char *)v->last - (char *)(it + 1));
            v->last = (struct Ez2StrParticle **)v->last - 1;
        } else {
            it++;
        }
    }
}

static void particles_free_all(struct Vec *v)
{
    struct Ez2StrParticle **it;
    for (it = v->first; it != (struct Ez2StrParticle **)v->last; it++)
        if (*it)
            particle_free(&(*it)->base);
    v->last = v->first;
}

static void particles_render(struct Vec *v)
{
    struct Ez2StrParticle **it;
    for (it = v->first; it != (struct Ez2StrParticle **)v->last; it++) {
        struct Ez2StrParticle *p = *it;
        if (p) {
            ez2_str_particle_render_current(p);
        }
    }
}

static int life_over(const struct Ez2StrParticle *p)
{
    if (PARTICLE_LIFE(p) == 0.0f)
        return 0;
    return !(PARTICLE_LIFE(p) * 60.0f > (float)p->base.age);
}

struct CaneOfEvilEyeEffect *cane_of_evil_eye_effect_construct(struct CaneOfEvilEyeEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &cane_of_evil_eye_effect_ops;
    self->canes.first = self->canes.last = self->canes.end = 0;
    self->lights.first = self->lights.last = self->lights.end = 0;
    self->last_lights_ms = platform_ticks_ms();
    return self;
}

struct Effect *cane_of_evil_eye_effect_new(void)
{
    struct CaneOfEvilEyeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &cane_of_evil_eye_effect_construct(self)->base : NULL;
}

/* Effect types created as CaneOfEvilEyeEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow cane_of_evil_eye_effect_type_rows[] = {
    EFFECT_TYPE(0x903, cane_of_evil_eye_effect_new),
};
const struct EffectTypeTable cane_of_evil_eye_effect_types = EFFECT_TYPE_TABLE(cane_of_evil_eye_effect_type_rows);

void cane_of_evil_eye_effect_destruct(struct CaneOfEvilEyeEffect *self)
{
    self->base.ops = &cane_of_evil_eye_effect_ops;
    vec_free(&self->lights);
    vec_free(&self->canes);
    effect_destruct(&self->base);
}

static void cane_of_evil_eye_effect_destroy(struct Effect *effect)
{
    struct CaneOfEvilEyeEffect *self = (struct CaneOfEvilEyeEffect *)effect;
    cane_of_evil_eye_effect_destruct(self);
    mem_free(self);
}

void cane_of_evil_eye_effect_add_lights(struct CaneOfEvilEyeEffect *self)
{
    struct Ez2StrParticle **it;

    for (it = self->canes.first; it != (struct Ez2StrParticle **)self->canes.last; it++) {
        struct Ez2StrParticle *cane = *it, *light;
        Vec3 offset;

        if (!cane)
            continue;
        offset.x = CANE_OFFSET(cane).x + 0.0f;
        offset.y = CANE_OFFSET(cane).y + 3.5f;
        offset.z = CANE_OFFSET(cane).z + 0.0f;
        light = ez2str_particle_new(self->base.pos, CANE_LIST);
        if (light) {
            load_str(light, LIGHT_START_STR);
            LIGHT_OFFSET(light) = offset;
            cane_of_evil_eye_particle(light)->str_phase = PHASE_START;
        }
        vec_push(&self->lights, &light, sizeof light);
    }
}

void cane_of_evil_eye_effect_sweep(struct CaneOfEvilEyeEffect *self)
{
    particles_sweep(&self->canes);
    particles_sweep(&self->lights);
}

void cane_of_evil_eye_effect_end(struct CaneOfEvilEyeEffect *self)
{
    struct Ez2StrParticle **it;

    for (it = self->canes.first; it != (struct Ez2StrParticle **)self->canes.last; it++) {
        struct Ez2StrParticle *cane = *it, *end;
        Vec3 offset;

        if (!cane)
            continue;
        offset = CANE_OFFSET(cane);
        end = ez2str_particle_new(self->base.pos, CANE_LIST);
        if (end) {
            load_str(end, CANE_END_STR);
            end->loop_forever = 0;
            CANE_OFFSET(end) = offset;
            cane_of_evil_eye_particle(end)->str_phase = PHASE_END;
        }
        *it = end;
        particle_free(&cane->base);
    }
    for (it = self->lights.first; it != (struct Ez2StrParticle **)self->lights.last; it++) {
        struct Ez2StrParticle *light = *it, *end;
        Vec3 offset;

        if (!light || cane_of_evil_eye_particle(light)->str_phase == PHASE_END)
            continue;
        offset = LIGHT_OFFSET(light);
        end = ez2str_particle_new(self->base.pos, CANE_LIST);
        if (end) {
            load_str(end, LIGHT_END_STR);
            LIGHT_OFFSET(end) = offset;
            cane_of_evil_eye_particle(end)->str_phase = PHASE_END;
        }
        *it = end;
        particle_free(&light->base);
    }
}

void cane_of_evil_eye_effect_step_str(struct CaneOfEvilEyeEffect *self, struct Ez2StrParticle *p)
{
    static const Vec3 no_offset = { 0.0f, 0.0f, 0.0f };
    float m[12], inv[12], wx, wy, wz, vx, vy, vz;

    if (!p)
        return;
    wx = CANE_OFFSET(p).x + self->base.pos.x;
    wy = CANE_OFFSET(p).y + self->base.pos.y;
    wz = CANE_OFFSET(p).z + self->base.pos.z;
    if (!game_copy_view_matrix(m))
        return;
    vx = m[0] * wx + m[3] * wy + m[6] * wz + m[9];
    vy = m[1] * wx + m[4] * wy + m[7] * wz + m[10];
    vz = m[2] * wx + m[5] * wy + m[8] * wz + m[11];
    mat_invert(inv, m);
    p->base.pos.x = vx * inv[0] + vy * inv[3] + vz * inv[6] + inv[9];
    p->base.pos.y = vx * inv[1] + vy * inv[4] + vz * inv[7] + inv[10];
    p->base.pos.z = vx * inv[2] + vy * inv[5] + vz * inv[8] + inv[11];
    ez2_str_particle_process(p, no_offset);
}

void cane_of_evil_eye_effect_step_canes(struct CaneOfEvilEyeEffect *self)
{
    struct Ez2StrParticle **it;

    for (it = self->canes.first; it != (struct Ez2StrParticle **)self->canes.last; it++) {
        struct Ez2StrParticle *cane = *it;

        if (!life_over(cane))
            continue;
        if (cane_of_evil_eye_particle(cane)->str_phase == PHASE_START) {
            Vec3 offset = CANE_OFFSET(cane);
            struct Ez2StrParticle *loop = ez2str_particle_new(self->base.pos, CANE_LIST);
            if (loop) {
                load_str(loop, CANE_LOOP_STR);
                loop->loop_forever = 1;
                CANE_OFFSET(loop) = offset;
                cane_of_evil_eye_particle(loop)->str_phase = PHASE_LOOP;
            }
            *it = loop;
        } else {
            *it = 0;
        }
        particle_free(&cane->base);
    }
    for (it = self->canes.first; it != (struct Ez2StrParticle **)self->canes.last; it++)
        cane_of_evil_eye_effect_step_str(self, *it);
}

void cane_of_evil_eye_effect_step_lights(struct CaneOfEvilEyeEffect *self)
{
    struct Ez2StrParticle **it;

    for (it = self->lights.first; it != (struct Ez2StrParticle **)self->lights.last; it++) {
        struct Ez2StrParticle *light = *it;
        unsigned char phase;

        if (!life_over(light))
            continue;
        phase = cane_of_evil_eye_particle(light)->str_phase;
        if (phase == PHASE_START || phase == PHASE_LOOP) {
            Vec3 offset = LIGHT_OFFSET(light);
            struct Ez2StrParticle *end = ez2str_particle_new(self->base.pos, CANE_LIST);
            if (end) {
                load_str(end, LIGHT_END_STR);
                LIGHT_OFFSET(end) = offset;
                cane_of_evil_eye_particle(end)->str_phase = PHASE_END;
            }
            *it = end;
        } else {
            *it = 0;
        }
        particle_free(&light->base);
    }

    for (it = self->lights.first; it != (struct Ez2StrParticle **)self->lights.last; it++) {
        struct Ez2StrParticle *light = *it;
        Vec3 at, middle;
        float view[12], light_x, light_y, light_rhw = 0.0f, middle_x, middle_y, middle_rhw = 0.0f;
        float dx, dy, dist2, degrees;
        double angle;

        if (!light)
            continue;
        at.x = self->base.pos.x + LIGHT_OFFSET(light).x;
        at.y = LIGHT_OFFSET(light).y + self->base.pos.y;
        at.z = LIGHT_OFFSET(light).z + self->base.pos.z;
        middle = self->base.pos;
        if (!game_copy_view_matrix(view))
            return;
        renderer_project_point(g_renderer, &at, view, &light_x, &light_y, &light_rhw);

        renderer_project_point(g_renderer, &middle, view, &middle_x, &middle_y, &middle_rhw);

        dx = middle_x - light_x;
        dy = middle_y - light_y;
        dist2 = dy * dy + dx * dx;
        degrees = (float)acos((double)dx / (double)float_sqrt(dist2));
        degrees = degrees / 3.141592f * 180.0f;
        angle = degrees;
        if (light_y > middle_y)
            angle = 360.0 - angle;
        light->spin_degrees = (float)angle;
        light->zoom = float_sqrt(dist2) * 0.0044f;
        cane_of_evil_eye_effect_step_str(self, light);
    }
}

void cane_of_evil_eye_effect_init(struct Effect *effect)
{
    struct CaneOfEvilEyeEffect *self = (struct CaneOfEvilEyeEffect *)effect;
    int i;

    self->base.has_instance_key = 1;
    for (i = 0; i < CANE_COUNT; i++) {
        Vec3 offset = cane_offsets[i];
        struct Ez2StrParticle *cane = ez2str_particle_new(self->base.pos, CANE_LIST);
        if (cane) {
            load_str(cane, CANE_START_STR);
            cane->loop_forever = 0;
            CANE_OFFSET(cane) = offset;
            cane_of_evil_eye_particle(cane)->str_phase = PHASE_START;
        }
        vec_push(&self->canes, &cane, sizeof cane);
    }
    cane_of_evil_eye_effect_add_lights(self);
}

void cane_of_evil_eye_effect_process(struct Effect *effect)
{
    struct CaneOfEvilEyeEffect *self = (struct CaneOfEvilEyeEffect *)effect;
    cane_of_evil_eye_effect_sweep(self);
    cane_of_evil_eye_effect_step_canes(self);
    if (self->last_lights_ms + LIGHT_PERIOD_MS <= platform_ticks_ms()) {
        cane_of_evil_eye_effect_add_lights(self);
        self->last_lights_ms += LIGHT_PERIOD_MS;
    }
    cane_of_evil_eye_effect_step_lights(self);
}

void cane_of_evil_eye_effect_render(struct Effect *effect)
{
    struct CaneOfEvilEyeEffect *self = (struct CaneOfEvilEyeEffect *)effect;
    particles_render(&self->canes);
    particles_render(&self->lights);
}

void cane_of_evil_eye_effect_release(struct Effect *effect)
{
    struct CaneOfEvilEyeEffect *self = (struct CaneOfEvilEyeEffect *)effect;
    particles_free_all(&self->canes);
    particles_free_all(&self->lights);
}

unsigned char cane_of_evil_eye_effect_is_alive(struct Effect *effect)
{
    struct CaneOfEvilEyeEffect *self = (struct CaneOfEvilEyeEffect *)effect;
    if (!effect_is_owner_alive(&self->base))
        return 0;
    return self->canes.first != self->canes.last || self->lights.first != self->lights.last;
}
