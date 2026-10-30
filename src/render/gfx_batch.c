#include "render/gfx.h"
#include "render/gfx_batch.h"
#include <string.h>

#define BATCH_VERTS   8192
#define BATCH_INDICES (BATCH_VERTS * 3)
#define MAX_STRIDE    40

static struct {
    batch_drawfn draw;
    int kind, stride;
    int nverts, nindices;
    BYTE verts[BATCH_VERTS * MAX_STRIDE];
    WORD indices[BATCH_INDICES];
} B;

void batch_init(batch_drawfn rawDraw)
{
    B.draw = rawDraw;
    B.nverts = 0;
    B.nindices = 0;
}

void batch_drop(void)
{
    B.nverts = 0;
    B.nindices = 0;
}

void batch_flush(void)
{
    if (B.nindices && B.draw)
        B.draw(GFX_PT_TRIANGLELIST, B.kind, B.verts, B.nverts, B.indices, B.nindices);
    B.nverts = 0;
    B.nindices = 0;
}

static int triangles_of(int prim, int n)
{
    switch (prim) {
    case GFX_PT_TRIANGLELIST:  return n / 3;
    case GFX_PT_TRIANGLESTRIP:
    case GFX_PT_TRIANGLEFAN:   return n > 2 ? n - 2 : 0;
    }
    return -1;
}

int batch_add(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    int n = nindices ? nindices : nverts;
    int tris = triangles_of(prim, n), stride, base, t;
    int kind = vertexKind & 0xff;
    WORD *out;
    if (!B.draw || tris < 0 || nverts > BATCH_VERTS || (kind != GFX_VTX_TEX1 && kind != GFX_VTX_TEX2))
        return 0;
    if (tris == 0)
        return 1;
    if (B.nindices && B.kind != vertexKind)
        batch_flush();
    if (B.nverts + nverts > BATCH_VERTS || B.nindices + tris * 3 > BATCH_INDICES)
        batch_flush();
    stride = kind == GFX_VTX_TEX2 ? 40 : 32;
    B.kind = vertexKind;
    B.stride = stride;
    memcpy(B.verts + B.nverts * stride, verts, (size_t)nverts * stride);
    base = B.nverts;
    out = B.indices + B.nindices;
#define IDX(i) ((WORD)(base + (indices ? indices[i] : (i))))
    switch (prim) {
    case GFX_PT_TRIANGLELIST:
        for (t = 0; t < tris; t++) {
            out[3 * t] = IDX(3 * t);
            out[3 * t + 1] = IDX(3 * t + 1);
            out[3 * t + 2] = IDX(3 * t + 2);
        }
        break;
    case GFX_PT_TRIANGLESTRIP:
        for (t = 0; t < tris; t++) {
            if (t & 1) {
                out[3 * t] = IDX(t + 1);
                out[3 * t + 1] = IDX(t);
            } else {
                out[3 * t] = IDX(t);
                out[3 * t + 1] = IDX(t + 1);
            }
            out[3 * t + 2] = IDX(t + 2);
        }
        break;
    default:
        for (t = 0; t < tris; t++) {
            out[3 * t] = IDX(0);
            out[3 * t + 1] = IDX(t + 1);
            out[3 * t + 2] = IDX(t + 2);
        }
        break;
    }
#undef IDX
    B.nverts += nverts;
    B.nindices += tris * 3;
    return 1;
}
