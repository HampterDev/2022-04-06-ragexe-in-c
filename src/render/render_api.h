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
extern GUID g_driverGuid;

extern GUID init_adapter;
int set_display(int w, int h, int fullscreen, const GUID *adapter);
/* Fullscreen is drawn as a frameless window covering the monitor when nonzero (the
   ISBORDERLESS option). borderless_rect gives that monitor's rectangle, or 0 when the
   setting is off or the adapter has no monitor. Both in render/c3ddevice.c; a monitor's
   full and work-area rectangles come from platform_monitor_rects (platform/window.h). */
int borderless_rect(const GUID *adapter, RECT *rc);
extern int borderless_setting;
extern float g_texelOffsetU, g_texelOffsetV;
extern DWORD g_fogVertexMode;
extern DWORD g_fogColor;
extern DWORD g_fogUseRange;
extern float g_fogDensity;
extern float g_fogNear, g_fogFar;

/* The camera-space depth range of the 3D view. The original draws to 1500; custom draws to 40000
   so that the unlocked zoom (game/camera.h) keeps the world in view. Fog keeps the original's
   1490-unit scale. */
#define RENDER_NEAR_Z 10.0f
#define RENDER_FAR_Z 40000.0f
#define RENDER_DEPTH_SCALE (RENDER_FAR_Z / (RENDER_FAR_Z - RENDER_NEAR_Z))

extern BYTE  g_waterBumpSupported;
extern RECT  g_monitorRect;
extern const char g_msgBgr565NotSupported[];
extern const char g_msgModeNotSupported[];

struct List;
char render_quads_to_texture(int w, int h, const struct List *quads, struct Texture **out);

struct UIGraphicSettingWnd;
int apply_display_mode(struct UIGraphicSettingWnd *wnd);
char *display_prompt_text(struct UIGraphicSettingWnd *wnd, int restartId);
void settings_add_row(struct UIGraphicSettingWnd *wnd, struct UIWindow *fullscreenBox);
int settings_display_changed(struct UIGraphicSettingWnd *wnd);
void settings_draw_labels(struct UIGraphicSettingWnd *wnd);
void settings_hide_rows(struct UIGraphicSettingWnd *wnd);
int settings_is_display_box(struct UIWindow *sender, struct UIGraphicSettingWnd *wnd);
void settings_place_rows(struct UIGraphicSettingWnd *wnd);

#endif
