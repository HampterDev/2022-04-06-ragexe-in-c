#ifndef GFX_BATCH_H
#define GFX_BATCH_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"

typedef void (*batch_drawfn)(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices);

void batch_init(batch_drawfn rawDraw);
int  batch_add(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices);
void batch_flush(void);
void batch_drop(void);
/* A triangle strip of n vertices as a triangle list: (n - 2) * 3 indices into out, each base plus
   indices[i], or plus i without indices. The odd triangles are (t + 1, t, t + 2), the order a GPU
   draws a strip in, with Direct3D 9's windings; batched and unbatched strips are drawn alike. */
void batch_strip_list(WORD *out, int n, const WORD *indices, int base);

#endif
