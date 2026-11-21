#ifndef PERF_H
#define PERF_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "diag/perf_counters.h"
#include <stdint.h>

enum perf_phase {
    PERF_FRAME,
    PERF_SCENE,
    PERF_DESTROY,
    PERF_ENDSCENE,
    PERF_OVERLAY,
    PERF_PRESENT,
    PERF_TEXT,
    PERF_TEXLOCK,
    PERF_L_GROUND,
    PERF_L_WATER,
    PERF_L_MULTITEXTURE,
    PERF_L_BLENDED_NO_DEPTH,
    PERF_L_ALPHA_NO_DEPTH,
    PERF_L_ALPHA_DEPTH,
    PERF_L_ADDITIVE_FOG,
    PERF_L_ALPHATEST,
    PERF_L_ALPHA_WRITE_DEPTH,
    PERF_L_ALPHA_CUTOUT,
    PERF_L_ADDITIVE,
    PERF_L_BLENDED_WRAP,
    PERF_L_ADDITIVE_OVERLAY,
    PERF_L_OVERLAY_ORDERED,
    PERF_L_BLENDED_OVERLAY,
    PERF_N
};

struct Renderer;

void perf_begin(int phase);
void perf_end(int phase);
void perf_frame_end(struct Renderer *r);
void perf_overlay_draw(struct Renderer *r);

/* the triangles of a list (4) or a strip (5) of n vertices */
static __inline unsigned perf_prim_count(int prim, int n)
{
    return prim == 4 ? (unsigned)n / 3 : n > 2 ? (unsigned)n - 2 : 0;
}

struct UIWindow;
void ui_window_invalidate_base(struct UIWindow *self);
void ui_window_invalidate_children(struct UIWindow *self);

#endif
