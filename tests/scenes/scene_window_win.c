/* Windows scene runs: SDL's message box dialog is replaced by a line on stdout, so that an
   unattended run never waits for a click. Everything else is platform/window_sdl.c, as in the scene
   runs on the other platforms: the key and cursor state there is what the scene script's input set
   (core/user_input.h), never the desktop's. */
#include <SDL3/SDL.h>
#include <stdio.h>

static bool scene_message_box(SDL_MessageBoxFlags flags, const char *title, const char *message,
                              SDL_Window *window);

#define SDL_ShowSimpleMessageBox scene_message_box
#include "platform/window_sdl.c"
#undef SDL_ShowSimpleMessageBox

/* The text as platform_message_box gives it to SDL: UTF-8, a CP949 resource name converted. */
static bool scene_message_box(SDL_MessageBoxFlags flags, const char *title, const char *message,
                              SDL_Window *window)
{
    (void)flags;
    (void)window;
    printf("scene: message box \"%s\": %s\n", title ? title : "", message ? message : "");
    fflush(stdout);
    return true;
}
