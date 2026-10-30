#ifndef UI_CONTROLS_WINDOW_FIT_H
#define UI_CONTROLS_WINDOW_FIT_H

#include <windows.h>

struct UIWindow;

void ui_window_move_centred_for(struct UIWindow *self, int width, int height);

#endif
