/* Helpers that make_window.c and the extracted window_slots.c functions
   reference but the ESC window-ID list test never reaches. Each stops the
   test if called. */
#include <stdlib.h>

#define UNREACHED(name) void name(void) { abort(); }

UNREACHED(ui_center_x640)
UNREACHED(ui_center_y480)
UNREACHED(ui_window_create)
UNREACHED(ui_window_find)
UNREACHED(ui_window_mgr_add_window)
UNREACHED(ui_window_mgr_close_drag_plate)
