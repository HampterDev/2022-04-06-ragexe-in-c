#include "core/window_state.h"
#include "render/display_options.h"

PlatformWindow g_hMainWnd;
int g_fullscreenMode = 0;
int g_windowMaximized;
int g_windowX = DISPLAY_NO_POSITION, g_windowY = DISPLAY_NO_POSITION;
int g_savedWidth, g_savedHeight;
int g_windowActive = 1;
int g_activatedNow = 0;
int g_quitRequested;
