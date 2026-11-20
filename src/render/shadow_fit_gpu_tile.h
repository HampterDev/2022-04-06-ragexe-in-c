#ifndef RENDER_SHADOW_FIT_GPU_TILE_H
#define RENDER_SHADOW_FIT_GPU_TILE_H

/* custom: the tiles the shadow fit's range and extents passes take the screen in, a group of
   SHADOW_FIT_GPU_TILE_THREADS by SHADOW_FIT_GPU_TILE_THREADS threads to each tile of
   SHADOW_FIT_GPU_TILE by SHADOW_FIT_GPU_TILE pixels. Defines alone, so that the C that dispatches
   the passes (render/shadow_fit_gpu.h) and the shaders that run them
   (src/render/shaders/shadow_fit.hlsli) include the same numbers. */

#define SHADOW_FIT_GPU_TILE 32
#define SHADOW_FIT_GPU_TILE_THREADS 8

#endif
