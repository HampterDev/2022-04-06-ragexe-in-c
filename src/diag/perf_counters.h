#ifndef DIAG_PERF_COUNTERS_H
#define DIAG_PERF_COUNTERS_H

/* The frame's counters (g_perf), which the graphics device, the renderer and the game bump and the
   frame-time overlay (diag/perf.h) shows. */
#include "platform/thread.h"
#include <stdint.h>

/* Counted whether the overlay is on or not; with RAGEXE_OVERLAY on the overlay starts them again
   every half second, the scene harness with every frame it draws and shows
   (tests/scenes/README.md). They count, for the states the client sets:
   - draws: the draws the device makes of the client's quads and triangles and of Granny's actors
     (after batching) and of the static world, the depth pass's and the shadow maps' included,
     which the device draws for the frame's shadows itself; indirectDraws: those of them whose
     primitives the GPU's cull chooses (a run of the static world's records), which the CPU never
     knows; prims: the primitives of the others; drawCalls: the gfx_draw_quad, gfx_draw_vertices,
     gfx_draw_skin, gfx_world_draw, gfx_world_draw_record and gfx_world_draw_water calls. Blits
     drawn as quads count as blitQuads alone.
   - modesSent and modesSkipped: gfx_set_mode, gfx_set_effect and gfx_set_fog calls that change what
     the device draws with (render/gfx.h), which ends a batch, and that leave it as it was; texSets:
     gfx_set_texture calls that change a stage's texture.
   - vertexBytes: the vertices and indices of a static world mesh (with each vertex's record) when
     gfx_world_mesh gives them, the cells a draw of the water names, and a Granny model's vertices
     and indices when a draw first takes them up; instanceBytes: the records of the quads and
     triangles drawn, their offsets, the matrices they are projected with and the corner sets and
     client's vertices they name, and Granny's actors' poses with the headers and offsets of their
     draws' instances, that the frame's upload gives the device.
   - textureBytes: the texels written by texture_upload_pixels, surface_fill_pixels and
     texture_draw_sprite_frame (the sprite atlas), at the texture's or surface's own depth.
   - drawSceneNs, viewRenderNs and presentNs: the time spent in renderer_draw_scene, in
     view_on_render and in presenting the frame (c3d_device_show_frame), where the device waits
     for its render thread to finish the frame before, which waits while the GPU is two frames
     behind that one, and hands the frame's recorded draws to it.
   - The game's own work in a frame, timed the same way: packetsNs, handling the map server's
     packets (game_mode_poll_network_msg: actors appearing and moving, skills launching their
     effects); effectsNs, stepping the effect manager's effects (effect_mgr_process); actorsNs,
     stepping the world's objects (world_process_actors: actors walking and animating, the effects
     and damage numbers attached to them); objectsRenderNs and effectsRenderNs, the parts of
     view_on_render that build the world's objects' quads (game_object_render: sprites, their
     shadows, names, damage numbers, Granny models' bones) and the effect manager's
     (effect_mgr_render: STR and RagEffect primitives, particles); uiNs, drawing the windows
     (ui_window_mgr_render and ui_window_mgr_render_windows: shop signs, chat rooms and chat
     balloons among them); atlasNs, decoding sprite frames into the sprite atlas
     (texture_draw_sprite_frame); texelsNs, widening texels to the GPU's B8G8R8A8 for an upload
     (render/gpu_device.c, render/gpu_world.c). All of them run on the thread that draws.
   - Which times lie within which: objectsRenderNs and effectsRenderNs within viewRenderNs; atlasNs
     within the time of whatever draws the sprite: objectsRenderNs for the world's objects,
     effectsRenderNs for sprite particles, uiNs for windows, none of the others for the mouse
     cursor and what it drags; texelsNs within the time of whatever sends a texture's new texels to
     the device: drawSceneNs for the renderer's lists, uiNs for a window drawn into a texture of its
     own, presentNs for the static world's textures, which go up with the frame. packetsNs,
     effectsNs, actorsNs, viewRenderNs, uiNs, drawSceneNs and presentNs do not overlap.
   The model loader's thread (world/models.c) creates textures and builds them as well, so
   texCreated, locks, builds, buildsSkipped and textureBytes go up through perf_count_shared, and a
   texture counts in whichever frame the loader built it during; resGets and resHits go up under
   the resource manager's lock. The rest only go up on the thread that draws. */
struct perf_counters {
    unsigned draws;
    unsigned indirectDraws;
    unsigned prims;
    unsigned modesSent;
    unsigned modesSkipped;
    unsigned texSets;
    unsigned vertexBytes;
    unsigned instanceBytes;
    unsigned textureBytes;
    int64_t drawSceneNs;
    int64_t viewRenderNs;
    int64_t presentNs;
    int64_t packetsNs;
    int64_t effectsNs;
    int64_t actorsNs;
    int64_t objectsRenderNs;
    int64_t effectsRenderNs;
    int64_t uiNs;
    int64_t atlasNs;
    int64_t texelsNs;
    unsigned locks;
    unsigned texCreated;
    unsigned surfCreated;
    unsigned blits;
    unsigned blitQuads;
    unsigned cpuBlits;
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
};
extern struct perf_counters g_perf;

/* a counter that threads other than the one that draws bump too */
static __inline void perf_count_shared(unsigned *counter, unsigned amount)
{
    platform_atomic_add((volatile int32_t *)counter, (int32_t)amount);
}

/* a gfx_set_mode, gfx_set_effect or gfx_set_fog call, which changed what the device draws with or
   left it as it was */
static __inline void perf_count_mode(int sent)
{
    if (sent)
        g_perf.modesSent++;
    else
        g_perf.modesSkipped++;
}

#endif
