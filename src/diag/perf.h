#ifndef PERF_H
#define PERF_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"

enum perf_phase {
    PERF_FRAME,
    PERF_SCENE,
    PERF_DESTROY,
    PERF_ENDSCENE,
    PERF_OVERLAY,
    PERF_TARGETLOCK,
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

struct perf_counters {
    unsigned draws;
    unsigned prims;
    unsigned statesSent;
    unsigned statesSkipped;
    unsigned texSets;
    unsigned locks;
    unsigned targetLocks;
    unsigned texCreated;
    unsigned surfCreated;
    unsigned blits;
    unsigned blitQuads;
    unsigned cpuBlits;
    unsigned dcGets;
    unsigned textMeasured;
    unsigned textHits;
    unsigned drawCalls;
    unsigned builds;
    unsigned buildsSkipped;
    unsigned invalidates;
    unsigned ondraws;
    unsigned resGets;
    unsigned resHits;
    unsigned actorLookups;
    unsigned actorIndexHits;
};
extern struct perf_counters g_perf;

struct Renderer;

void perf_begin(int phase);
void perf_end(int phase);
void perf_frame_end(struct Renderer *r);
void perf_overlay_draw(struct Renderer *r);

static __inline unsigned perf_prim_count(int prim, int n)
{
    switch (prim) {
    case 1:  return (unsigned)n;
    case 2:  return (unsigned)n / 2;
    case 3:  return n > 1 ? n - 1 : 0;
    case 4:  return (unsigned)n / 3;
    case 5:
    case 6:  return n > 2 ? n - 2 : 0;
    }
    return (unsigned)n;
}

struct UIWindow;
void ui_window_invalidate_base(struct UIWindow *self);
void ui_window_invalidate_children(struct UIWindow *self);

#endif
