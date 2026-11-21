#ifndef RENDER_H
#define RENDER_H

#include "platform/error_message.h"
#include "render/device.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "render/renderer.h"
#include "render/frame_capture.h"
#include "render/texture_quad.h"
#include "render/sprite_draw.h"
#include "render/math3d.h"
#include "render/render_options.h"

#include "fileio/resource_api.h"

#include "render/gfx.h"
#include "render/texture_cache.h"
#include "render/primitive_types.h"
#include "base.h"
#include "render/device.h"
#include <stddef.h>

#include "render/cdc.h"

#include "render/textdc.h"

void rp_quad_draw(struct RPQuad *self);
extern int   g_wobbleFramesLeft;
extern BYTE  g_wobbleMode;
extern short g_wobbleAmplitude;

/* render/resize.c: the window in its display mode and the device at its client size in pixels
   (render/display_options.h); display_follow_window, once a pump, keeps the device at the window's
   size and the display options at what the window is. */
void set_display(int fullscreen, int maximized, int w, int h);
void display_follow_window(void);
/* custom: the offset from a texel's corner at which a pixel centre reads that texel alone, in
   texels, for both coordinates: half a texel, where Direct3D 9 put pixel centres. The original
   measured it at startup by drawing sub-texel offsets and reading them back; the GPU device puts
   pixel centres where Direct3D 9 did (render/gpu_device.c), so it is always half a texel. */
#define TEXEL_CENTRE 0.5f
extern DWORD g_fogColor;
extern float g_fogNear, g_fogFar;

/* The camera-space depth range of the 3D view. The original draws to 1500; custom draws to 40000
   so that the unlocked zoom (game/camera.h) keeps the world in view. Fog keeps the original's
   1490-unit scale. */
#define RENDER_NEAR_Z 10.0f
#define RENDER_FAR_Z 40000.0f
#define RENDER_DEPTH_SCALE (RENDER_FAR_Z / (RENDER_FAR_Z - RENDER_NEAR_Z))

extern const char g_msgBgr565NotSupported[];
extern const char g_msgModeNotSupported[];

struct List;
char render_quads_to_texture(int w, int h, const struct List *quads, struct Texture **out);

#endif
