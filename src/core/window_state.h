#ifndef CORE_WINDOW_STATE_H
#define CORE_WINDOW_STATE_H

#include "platform/window_handle.h"

/* The client's main window, whether it is fullscreen or maximised, where its frame last was when it
   was neither (g_windowX, g_windowY: DISPLAY_NO_POSITION when not known) and the windowed size that
   position was saved with (render/display_options.h; the windowed size itself is g_displayWidth
   and g_displayHeight, render/render_options.h), whether it is active, and the quit request that
   ends the main and modal loops. Defined in core/window_state.c. */
extern PlatformWindow g_hMainWnd;
extern int g_fullscreenMode;
extern int g_windowMaximized;
extern int g_windowX, g_windowY;
extern int g_savedWidth, g_savedHeight;
extern int g_windowActive;
extern int g_activatedNow;
extern int g_quitRequested;

#endif
