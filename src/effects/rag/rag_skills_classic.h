#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "resources/attr_queries.h"
#include "session/state.h"
#ifndef EFFECTS_RAG_RAG_SKILLS_CLASSIC_H
#define EFFECTS_RAG_RAG_SKILLS_CLASSIC_H

#include "render/projection.h"

#include "render/math3d.h"

#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "std/vec.h"
#include "render/render_api.h"

static __inline int rnd(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static __inline float abs_f(float x)
{
    union { float f; unsigned u; } v;
    v.f = x;
    v.u &= 0x7fffffffu;
    return v.f;
}

static __inline int master_facing(const struct RagEffect *self)
{
    int a = (int)(180.0f - self->master->roty);
    return a < 0 ? a + 360 : a;
}

static __inline struct Texture *rag_texture(const char *name)
{
    return texture_cache_get(&g_textureCache, name, 0, 0);
}

static __inline void prim_alloc_textures(struct EffectPrim *p)
{
    p->textures = rag_prim_new_textures(p);
}

static __inline void prim_set_texture(struct EffectPrim *p, const char *name)
{
    prim_alloc_textures(p);
    p->textures[0] = rag_texture(name);
}

static __inline void prim_set_sprite(struct EffectPrim *p, const char *spr, const char *act)
{
    render_object_load_spr_res(&p->base, spr);
    render_object_load_act_res(&p->base, act);
}

static __inline void prim_tilt_then_turn(struct EffectPrim *p)
{
    mat_set_rotate_x(p->matrix, deg_to_rad((int)PRIM_PITCH(p)));
    mat_rotate_y(p->matrix, deg_to_rad((int)PRIM_YAW(p)));
}

static __inline void prim_turn_then_tilt(struct EffectPrim *p)
{
    mat_set_rotate_y(p->matrix, deg_to_rad((int)PRIM_YAW(p)));
    mat_rotate_x(p->matrix, deg_to_rad((int)PRIM_PITCH(p)));
}

static __inline Vec3 mat_point(const float *m, float x, float y, float z)
{
    Vec3 r;
    r.x = y * m[3] + x * m[0] + z * m[6] + m[9];
    r.y = y * m[4] + x * m[1] + z * m[7] + m[10];
    r.z = y * m[5] + x * m[2] + z * m[8] + m[11];
    return r;
}

static __inline void prim_fly_along_z(struct EffectPrim *p, float speed)
{
    p->drift = mat_point(p->matrix, 0.0f, 0.0f, speed);
    p->delta2.x = p->drift.x + 0.0f;
    p->delta2.y = p->drift.y - 10.0f;
    p->delta2.z = p->drift.z + 0.0f;
}

static __inline void prim_burst_out(struct EffectPrim *p)
{
    prim_turn_then_tilt(p);
    prim_fly_along_z(p, (float)(rnd(4) + 4));
}

static __inline void alpha_key_push(struct Vec *v, float value)
{
    vec_push(v, &value, sizeof value);
}

static __inline void prim_add_alpha_key(struct EffectPrim *p, float frame, float value, float ceiling)
{
    alpha_key_push(&p->alpha_key_frames, frame);
    alpha_key_push(&p->alpha_key_values, value);
    alpha_key_push(&p->alpha_key_ceilings, ceiling);
}

static __inline void rag_play_two_waves(struct RagEffect *self, const char *first, const char *second)
{
    rag_play_wave_here(self, first);
    rag_play_wave_here(self, second);
}

#define SPR_DIR "이팩트\\"

static __inline void prim_aim_at_offset(struct RagEffect *self, struct EffectPrim *p)
{
    render_object_turn_toward(&p->base, self->offset.x, self->offset.z);
    PRIM_YAW(p) = -p->base.roty;
    p->base.roty = 180.0f - p->base.roty;
    if (0.0f > p->base.roty)
        p->base.roty += 360.0f;
    p->speed = -(render_object_calc_ground_dist(&p->base, self->offset.x, self->offset.z) / (float)p->duration);
    mat_set_rotate_y(p->matrix, deg_to_rad((int)PRIM_YAW(p)));
}

static __inline struct EffectPrim *agility_flake(struct RagEffect *self, int tilt, float dy)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 16, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    p->duration = 50;
    mat_set_rotate_y(p->matrix, deg_to_rad(rnd(360)));
    p->delta2 = mat_point(p->matrix, 0.0f, 0.0f, (float)(rnd(7) + 2));
    if (dy != 0.0f)
        p->delta2.y = p->delta2.y - dy;
    mat_set_rotate_x(p->matrix, deg_to_rad(tilt));
    return p;
}

static __inline void agility_flake_look(struct EffectPrim *p, unsigned char style)
{
    p->size[1].value = (float)(rnd(60) + 30) / 10.0f;
    p->size[2].value = 0.18f;
    p->alpha = 0.0f;
    p->alpha_max = 200.0f;
    p->alpha_speed = p->alpha_max / 20.0f;
    p->fade_frame = p->duration - 20;
    prim_set_texture(p, "effect\\ac_center2.tga");
    p->variant = style;
}

static __inline void agility_picture(struct RagEffect *self, const char *texture, unsigned char style)
{
    struct EffectPrim *p = self->last_prim = rag_launch_prim(self, 0, 0.0f, 0.0f, 0.0f);
    if (!p)
        return;

    p->duration = self->duration;
    PRIM_YAW(p) = 0.0f;
    p->speed = 1.5f;
    p->accel = p->speed / (float)p->duration * -1.2f;
    p->size[1].value = 40.0f;
    p->size[2].value = 20.0f;
    p->alpha = 0.0f;
    p->alpha_max = 200.0f;
    p->alpha_speed = p->alpha_max / 15.0f;
    p->fade_frame = p->duration - 15;
    prim_set_texture(p, texture);
    p->variant = style;
}

#endif
