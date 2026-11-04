#include "core/window_state.h"

PlatformWindow g_hMainWnd;
int g_fullscreenMode = 0;
int g_savedWidth, g_savedHeight;
int g_screenWidth = 1024;
int g_screenHeight = 768;
int g_windowActive = 1;
int g_activatedNow = 0;
int g_quitRequested;
