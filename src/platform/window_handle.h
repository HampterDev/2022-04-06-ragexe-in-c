#ifndef PLATFORM_WINDOW_HANDLE_H
#define PLATFORM_WINDOW_HANDLE_H

/* The client's window and the displays, as the platform knows them: on Windows a window handle and
   a monitor handle, elsewhere SDL's window and display id. A display of 0 is none, which the
   display queries take as the primary display. */
#ifdef _WIN32
#include "platform/win_types.h"
typedef HWND PlatformWindow;
typedef HMONITOR PlatformDisplay;
#else
#include <stdint.h>
struct SDL_Window;
typedef struct SDL_Window *PlatformWindow;
typedef uint32_t PlatformDisplay;
#endif

#endif
