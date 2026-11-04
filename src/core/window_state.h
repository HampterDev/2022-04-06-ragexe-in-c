#ifndef CORE_WINDOW_STATE_H
#define CORE_WINDOW_STATE_H

#include "platform/window_handle.h"

/* The client's main window, its size and fullscreen mode, whether it is active, and the quit
   request that ends the main and modal loops. Defined in core/window_state.c. */
extern PlatformWindow g_hMainWnd;
extern int g_fullscreenMode;
extern int g_savedWidth, g_savedHeight;
extern int g_screenWidth, g_screenHeight;
extern int g_windowActive;
extern int g_activatedNow;
extern int g_quitRequested;

#endif
