#include "ui/wndmgr/plate_hooks.h"
#include "ui/wndmgr/wndmgr.h"

/* The step ui_window_mgr_make_window took itself before: a hotkey that opens a window drops
   the party plate being dragged. In a file of its own so the window tests can compile it. */
void ui_window_mgr_drop_plate_for_hot_key(struct UIWindowMgr *self)
{
    if (ui_window_mgr_is_dragging_member_plate(self) && ui_window_mgr_is_hot_key_of_window(self))
        ui_window_mgr_close_drag_plate(self);
}
