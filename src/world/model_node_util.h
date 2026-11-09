#ifndef WORLD_MODEL_NODE_UTIL_H
#define WORLD_MODEL_NODE_UTIL_H

#include "world/model.h"

static inline void xform(const float *m, const Vec3 *p, float *out)
{
    out[0] = p->x * m[0] + p->y * m[3] + p->z * m[6] + m[9];
    out[1] = p->x * m[1] + p->y * m[4] + p->z * m[7] + m[10];
    out[2] = p->x * m[2] + p->y * m[5] + p->z * m[8] + m[11];
}

static inline float unsigned_to_float(DWORD v)
{
    return (float)(double)v;
}

static inline DWORD pack(BYTE a, BYTE r, BYTE g, BYTE b)
{
    return (DWORD)b | (DWORD)g << 8 | (DWORD)r << 16 | (DWORD)a << 24;
}
static inline struct Texture *run_texture(struct C3dNode *self, WORD texture)
{
    struct Texture *found = self->textureCount ? self->textures[texture % self->textureCount] : NULL;
    return found ? found : &g_whiteTexture;
}

const struct UvTransform *uv_xform_find(const struct C3dNodeV2 *node, int texture);
/* custom: a texture-coordinate animation's turn as a transform (render/gfx.h, GfxMeshDraw): about
   (0.5, 0.5) by its angle, then scaled and offset, as the original turned each corner's
   coordinates; the identity without x */
void uv_turn_matrix(const struct UvTransform *x, float *uv);

#endif
