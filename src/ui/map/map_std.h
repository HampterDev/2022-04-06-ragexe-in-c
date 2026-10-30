#ifndef UI_MAP_MAP_STD_H
#define UI_MAP_MAP_STD_H

#include "ui/ui_std.h"
#include <time.h>

struct UIEditctrlWnd *ui_editctrl_wnd_construct(struct UIEditctrlWnd *self);
struct UIWindow *ui_editctrl_wnd_new(void);
struct UIMiniMapWnd *ui_mini_map_wnd_construct(struct UIMiniMapWnd *self);
struct UIWindow *ui_mini_map_wnd_new(void);

#endif
