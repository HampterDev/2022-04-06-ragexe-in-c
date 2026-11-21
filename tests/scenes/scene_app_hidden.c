/* Scene runs: the client's window is never shown, on every platform. The client keeps it for what
   it reads of it (its size in pixels, which the resize scene changes); the GPU device claims a
   window, and makes a swapchain for it, only once it is shown (render/gpu_device.c), so a scene run
   has no swapchain at all: frames are drawn and the captures download them (gfx_read_frame). SDL's
   GPU drivers need no window, only a video driver that can make what a window would be presented
   through: Vulkan surfaces (Windows', X11's, Wayland's, the offscreen driver's) or Metal views
   (Cocoa's, so macOS keeps Cocoa, whose window stays hidden). A run never takes the desktop's
   focus, never covers the screen and never has the mouse: SDL does not raise a hidden window, and
   captures and clips the mouse only for the window that has the focus. Everything else is
   core/app_sdl.c. */
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
