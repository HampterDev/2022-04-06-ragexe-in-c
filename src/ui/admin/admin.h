#ifndef UI_ADMIN_ADMIN_H
#define UI_ADMIN_ADMIN_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"

struct UIAccountLimitedToolWnd *ui_account_limited_tool_wnd_construct(struct UIAccountLimitedToolWnd *self);
struct UIWindow *ui_account_limited_tool_wnd_new(void);

#endif
