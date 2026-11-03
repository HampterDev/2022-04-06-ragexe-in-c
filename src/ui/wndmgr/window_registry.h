#ifndef UI_WNDMGR_WINDOW_REGISTRY_H
#define UI_WNDMGR_WINDOW_REGISTRY_H

#include "ui/wndmgr/window_table.h"

/* The composition point of the window table (window_table.h): every table of window rows,
   installed by client_install_hooks(). */
extern const struct WmWindowTable g_windowTable;

/* The cluster tables it lists, each defined at the end of its make_window_<cluster>.c. */
extern const struct WmWindowRows wm_login_windows;
extern const struct WmWindowRows wm_character_windows;
extern const struct WmWindowRows wm_hud_windows;
extern const struct WmWindowRows wm_trade_windows;
extern const struct WmWindowRows wm_social_windows;

#endif
