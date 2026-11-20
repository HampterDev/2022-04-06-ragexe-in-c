#ifndef TEST_HEADLESS_H
#define TEST_HEADLESS_H
/* Forced into platform/window_sdl.c where the tests' client libraries compile it on Windows
   (tests/CMakeLists.txt): the tests run without windows, so a message box that the code under test
   raises (platform_message_box) is written to stderr instead of shown, whatever video driver SDL
   would use. CTest's SDL_VIDEO_DRIVER=dummy already leaves SDL without one; this holds when a test
   program is run by hand as well. */
#include <SDL3/SDL.h>
#include <stdio.h>

static __inline bool headless_message_box(SDL_MessageBoxFlags flags, const char *title, const char *message,
                                          SDL_Window *window)
{
    (void)flags;
    (void)window;
    fprintf(stderr, "message box (not shown in tests): %s: %s\n", title ? title : "", message ? message : "");
    return true;
}

#define SDL_ShowSimpleMessageBox headless_message_box
#endif
