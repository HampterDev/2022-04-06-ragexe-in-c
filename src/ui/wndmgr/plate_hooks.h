#ifndef UI_WNDMGR_PLATE_HOOKS_H
#define UI_WNDMGR_PLATE_HOOKS_H

struct UIWindowMgr;

/* Installed as g_windowMgrHandlers.drop_dragged_plate (ui/wndmgr/wndmgr_handlers.h). */
void ui_window_mgr_drop_plate_for_hot_key(struct UIWindowMgr *self);

#endif
