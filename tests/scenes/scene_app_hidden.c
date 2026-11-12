/* Scene runs on Windows and macOS: the client's window is never shown. The GPU device draws for it
   hidden (SDL's Metal driver needs a window's view, so macOS keeps the window where Linux has SDL's
   offscreen driver), so a run never takes the desktop's focus, never covers the screen and never
   has the mouse: SDL does not raise a hidden window, and captures and clips the mouse only for the
   window that has the focus. Everything else is core/app_sdl.c. */
#include <SDL3/SDL.h>

static bool scene_show_window(SDL_Window *window);

#define SDL_ShowWindow scene_show_window
#include "core/app_sdl.c"
#undef SDL_ShowWindow

static bool scene_show_window(SDL_Window *window)
{
    (void)window;
    return true;
}
