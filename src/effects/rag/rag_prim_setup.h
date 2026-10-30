#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "resources/attr_queries.h"
#ifndef EFFECTS_RAG_RAG_PRIM_SETUP_H
#define EFFECTS_RAG_RAG_PRIM_SETUP_H

#include "render/projection.h"

#include "render/math3d.h"

#include <stdlib.h>
#include <string.h>
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"

#define RAG_SPRITE_DIR "이팩트\\"

static __inline void prim_ring_fill_columns(struct PrimRing *ring, float height, unsigned char flag)
{
    int i;

    for (i = 0; i < PRIM_RING_COLUMNS; i++) {
        ring->column[i] = height;
        ring->column_flag[i] = flag;
    }
}

static __inline int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static __inline struct Texture *rag_texture(const char *name)
{
    return texture_cache_get(&g_textureCache, name, 0, 0);
}

static __inline void prim_new_textures(struct EffectPrim *p)
{
    p->textures = rag_prim_new_textures(p);
}

static __inline void prim_set_texture(struct EffectPrim *p, const char *name)
{
    prim_new_textures(p);
    p->textures[0] = rag_texture(name);
}

static __inline struct EffectPrim *rag_launch_textured(struct RagEffect *self, int kind, const char *texture)
{
    struct EffectPrim *p = rag_launch_prim(self, kind, 0.0f, 0.0f, 0.0f);
    if (!p)
        return NULL;

    self->last_prim = p;
    p->list_flags = 5;
    p->duration = self->duration;
    prim_set_texture(p, texture);
    return p;
}

static __inline void prim_matrix_pitch(struct EffectPrim *p, float degrees)
{
    mat_set_rotate_x(p->matrix, deg_to_rad((int)degrees));
}
static __inline void prim_matrix_yaw(struct EffectPrim *p, float degrees)
{
    mat_set_rotate_y(p->matrix, deg_to_rad((int)degrees));
}
static __inline void prim_matrix_turn_yaw(struct EffectPrim *p, float degrees)
{
    mat_rotate_y(p->matrix, deg_to_rad((int)degrees));
}
static __inline void prim_matrix_turn_pitch(struct EffectPrim *p, float degrees)
{
    mat_rotate_x(p->matrix, deg_to_rad((int)degrees));
}

static __inline void prim_keep_matrix(struct EffectPrim *p)
{
    memcpy(p->matrix2, p->matrix, sizeof(p->matrix2));
}

static __inline void mat_apply(const float *m, float x, float y, float z, Vec3 *out)
{
    out->x = x * m[0] + y * m[3] + z * m[6] + m[9];
    out->y = x * m[1] + y * m[4] + z * m[7] + m[10];
    out->z = x * m[2] + y * m[5] + z * m[8] + m[11];
}

#define RAG_FOREVER 99999999
static __inline int rag_restart_forever(struct RagEffect *self)
{
    if (self->frame == RAG_FOREVER - 1) {
        self->frame = 0;
        self->duration = RAG_FOREVER;
        return 1;
    }
    return self->frame == 0;
}

#define GAME_MSG_QUAKE 0x7a

#endif
