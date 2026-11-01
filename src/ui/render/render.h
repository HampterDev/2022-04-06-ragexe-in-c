#include "render/rp_pool.h"
#include "render/texture_globals.h"
#include "render/texture_cache.h"
#include "render/texture_types.h"
#ifndef UI_RENDER_RENDER_H
#define UI_RENDER_RENDER_H

#include "ui/window.h"
#include "ui/draw.h"

#define UI_QUAD_Z_BITS 0x358637bdu
#define UI_QUAD_RHW_BITS 0x3f7fffefu

void ui_window_mgr_draw_login_screen(struct UIWindowMgr *self);

#endif
