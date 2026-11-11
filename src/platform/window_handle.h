#ifndef PLATFORM_WINDOW_HANDLE_H
#define PLATFORM_WINDOW_HANDLE_H

/* The client's window and the displays, as the platform knows them: SDL's window and display id on
   every platform. A display of 0 is none, which the display queries take as the primary display. */
#include <stdint.h>
struct SDL_Window;
typedef struct SDL_Window *PlatformWindow;
typedef uint32_t PlatformDisplay;

#endif
