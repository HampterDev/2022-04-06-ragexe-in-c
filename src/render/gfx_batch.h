#ifndef GFX_BATCH_H
#define GFX_BATCH_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"

typedef void (*batch_drawfn)(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices);

void batch_init(batch_drawfn rawDraw);
int  batch_add(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices);
void batch_flush(void);
void batch_drop(void);

#endif
