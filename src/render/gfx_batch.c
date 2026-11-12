#include "render/gfx.h"
#include "render/gfx_batch.h"
#include "platform/thread.h"
#include <string.h>

#define BATCH_VERTS   8192
#define BATCH_INDICES (BATCH_VERTS * 3)
#define MAX_STRIDE    GFX_VTX_TEX2_SIZE

static struct {
    batch_drawfn draw;
    int kind, stride;
    int nverts, nindices;
    BYTE verts[BATCH_VERTS * MAX_STRIDE];
    WORD indices[BATCH_INDICES];
} B;

static uint32_t owner;

static int foreign_thread(void)
{
    return owner && platform_thread_id() != owner;
}

void batch_init(batch_drawfn rawDraw)
{
    owner = platform_thread_id();
    B.draw = rawDraw;
    B.nverts = 0;
    B.nindices = 0;
}

void batch_drop(void)
{
    if (foreign_thread())
        return;
    B.nverts = 0;
    B.nindices = 0;
}

void batch_flush(void)
{
    if (foreign_thread())
        return;
    if (B.nindices && B.draw)
        B.draw(GFX_PT_TRIANGLELIST, B.kind, B.verts, B.nverts, B.indices, B.nindices);
    B.nverts = 0;
    B.nindices = 0;
}

void batch_strip_list(WORD *out, int n, const WORD *indices, int base)
{
    int t;
#define IDX(i) ((WORD)(base + (indices ? indices[i] : (i))))
    for (t = 0; t + 2 < n; t++) {
        out[3 * t] = IDX(t & 1 ? t + 1 : t);
        out[3 * t + 1] = IDX(t & 1 ? t : t + 1);
        out[3 * t + 2] = IDX(t + 2);
    }
#undef IDX
}

static int triangles_of(int prim, int n)
{
    switch (prim) {
    case GFX_PT_TRIANGLELIST:  return n / 3;
    case GFX_PT_TRIANGLESTRIP: return n > 2 ? n - 2 : 0;
    }
    return -1;
}

int batch_add(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    int n = nindices ? nindices : nverts;
    int tris = triangles_of(prim, n), stride, base, t;
    int kind = vertexKind & 0xff;
    WORD *out;
    if (!B.draw || tris < 0 || nverts > BATCH_VERTS
        || (kind != GFX_VTX_TEX1 && kind != GFX_VTX_TEX2 && kind != GFX_VTX_WORLD))
        return 0;
    if (tris == 0)
        return 1;
    if (B.nindices && B.kind != vertexKind)
        batch_flush();
    if (B.nverts + nverts > BATCH_VERTS || B.nindices + tris * 3 > BATCH_INDICES)
        batch_flush();
    stride = kind == GFX_VTX_TEX2 ? GFX_VTX_TEX2_SIZE : GFX_VTX_TEX1_SIZE;
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
    default:
        batch_strip_list(out, n, indices, base);
        break;
    }
#undef IDX
    B.nverts += nverts;
    B.nindices += tris * 3;
    return 1;
}
