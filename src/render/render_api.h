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

struct RPMesh {
    float m[12];
    struct VertexBuffer *vb;
    DWORD start, count;
    void *indices;
    DWORD nindices;
    struct Texture *texture;
};

#include "render/cdc.h"

#include "render/textdc.h"

void rp_quad_draw(struct RPQuad *self);
extern int   g_wobbleFramesLeft;
extern BYTE  g_wobbleMode;
extern short g_wobbleAmplitude;
extern GUID g_driverGuid;

struct VertexBuffer {
    DWORD count;
    DWORD vertexSize;
    DWORD used;
    gfx_vbuffer *vb;
};

extern GUID init_adapter;
int set_display(int w, int h, int fullscreen, const GUID *adapter);
int borderless_rect(const GUID *adapter, RECT *rc);
extern int borderless_setting;
extern float g_texelOffsetU, g_texelOffsetV;
extern DWORD g_fogVertexMode;
extern DWORD g_fogColor;
extern DWORD g_fogUseRange;
extern float g_fogDensity;
extern float g_fogNear, g_fogFar;

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
