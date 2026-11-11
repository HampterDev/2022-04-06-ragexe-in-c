#ifndef PERF_H
#define PERF_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "platform/thread.h"
#include <stdint.h>

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

/* Counted whether the overlay is on or not; with RAGEXE_OVERLAY on the overlay starts them again
   every half second, the scene harness with every frame it draws and shows
   (tests/scenes/README.md). Both devices count the same way for the states the client sets:
   - draws and prims: the draws the device makes of gfx_draw's vertices (after batching) and of
     meshes, the shadow maps' included; drawCalls: the gfx_draw and gfx_draw_mesh calls. Blits
     drawn as quads count as blitQuads alone.
   - statesSent and statesSkipped: gfx_set_render_state and gfx_set_stage_state calls that change a
     state the device keeps, and that leave it as it was; texSets: gfx_set_texture calls that
     change a stage's texture.
   - vertexBytes: the vertices and indices a draw of gfx_draw's vertices gives the device, and those
     of a mesh when the device fills its buffers.
   - textureBytes: the texels written by texture_upload_pixels, surface_fill_pixels and
     texture_draw_sprite_frame (the sprite atlas), at the texture's or surface's own depth.
   - drawSceneNs and viewRenderNs: the time spent in renderer_draw_scene and view_on_render.
   The model loader's thread (world/models.c) creates textures and builds them as well, so
   texCreated, locks, builds, buildsSkipped and textureBytes go up through perf_count_shared, and a
   texture counts in whichever frame the loader built it during; resGets and resHits go up under
   the resource manager's lock. The rest only go up on the thread that draws. */
struct perf_counters {
    unsigned draws;
    unsigned prims;
    unsigned statesSent;
    unsigned statesSkipped;
    unsigned texSets;
    unsigned vertexBytes;
    unsigned textureBytes;
    int64_t drawSceneNs;
    int64_t viewRenderNs;
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

/* a counter that threads other than the one that draws bump too */
static __inline void perf_count_shared(unsigned *counter, unsigned amount)
{
    platform_atomic_add((volatile int32_t *)counter, (int32_t)amount);
}

/* a gfx_set_render_state or gfx_set_stage_state call, which sent the state or left it as it was */
static __inline void perf_count_state(int sent)
{
    if (sent)
        g_perf.statesSent++;
    else
        g_perf.statesSkipped++;
}

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
