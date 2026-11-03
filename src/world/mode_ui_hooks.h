#ifndef WORLD_MODE_UI_HOOKS_H
#define WORLD_MODE_UI_HOOKS_H

#include "base.h"

/* The running mode's side of g_uiHooks (ui/controls/ui_hooks.h): the help balloon and
   the mouse cursor of whichever mode, login or game, client_current_mode returns. */
void mode_show_current_tooltip(const char *text, int x, int y, DWORD colour, BYTE flags);
void mode_set_current_cursor_action(int action);

#endif
