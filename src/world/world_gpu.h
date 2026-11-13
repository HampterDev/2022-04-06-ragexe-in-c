#ifndef WORLD_WORLD_GPU_H
#define WORLD_WORLD_GPU_H

/* custom: the map's static world on the device (render/gfx.h, gfx_world_*; plans/gpu-renderer.md,
   "The static world, GPU-driven"): the ground's blocks, the models' nodes and the water, built
   into the device's shared buffers, texture arrays, draw records and transform slots as the map
   loads and its models stream in, and what each frame gives the device of them. The device draws
   the ground and the opaque models from the records, culled on the GPU, and their shadows, which
   the GPU fits and whose casters it culls (render/gfx.h, GfxShadow); the CPU culls and keys the
   alpha-blended models' records and queues them among the sprites as the original queued their
   triangles, and queues the water's cells (world/ground_draw.c). A frame runs world_gpu_begin,
   then the view's walk (world_gpu_seen), then world_gpu_frame. */

#include <stdint.h>

struct World;
struct C3dActor;
struct GfxWorldDraw;

/* Builds what the device lacks of the world: everything, on a map's first frame or after the
   device made its world again (gfx_world_generation: a new device, a failed upload, textures
   reloaded); then each model the scene graph has taken since (world/models.c,
   world_models_process), appended behind the rest. A model outside every leaf of the scene graph,
   which the frame never draws, gets nothing; a part that does not fit is left out. The
   space that models given up leave (world_gpu_forget) is taken back by building the world again
   once it passes WORLD_GPU_GARBAGE vertices (world/world_gpu.c). Then begins the frame: its view,
   and what the models and the ground are drawn with (render/gfx.h, GfxWorldDraw). */
void world_gpu_begin(struct World *world, const float *view, const struct GfxWorldDraw *models,
                     const struct GfxWorldDraw *ground);
/* A model the frame's walk draws, where the original drew its triangles: built the first time, if
   it is not yet (a skill's trap, effects/skills/skill.c); its pose taken for the frame, if the
   frame has not taken it yet, as its first tick of the frame left it; and its alpha-blended
   records the view keeps queued among the sprites (render/rp_pool.h, renderer_add_world_draw). */
void world_gpu_seen(struct World *world, struct C3dActor *actor);
/* A model leaves the scene graph before it is destroyed: its records are drawn no more, and the
   world reads nothing of it again. */
void world_gpu_forget(struct World *world, struct C3dActor *actor);
/* The frame's end, after its walk: each model the walk did not draw posed as it stands, and one
   that animates and holds a record the view keeps, or one that the box whatever can shade the
   view lies in keeps (render/renderer.h, renderer_shadow_reach), as the device's culls decide,
   first given to `tick`, which ticks it as the walk ticks the models it reaches, so that the device
   never draws a moving part, nor its shadow, in a pose the frame did not tick; their alpha-blended
   records the view keeps queued; then the device given the frame: the poses, the waves, the view
   and the shadows, which it draws the ground and the opaque models with (render/gfx.h,
   gfx_world_frame), bounded along the light by every static record and every animated one in its
   pose (render/renderer.h, renderer_shadow_bounds). */
void world_gpu_frame(struct World *world, void (*tick)(struct C3dActor *actor));
/* What the world holds of the device's world, at a map's end. */
void world_gpu_free(struct World *world);

#endif
