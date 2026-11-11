#ifdef _WIN32
#include "platform/desktop.h"
#include "core/window_state.h"
#include "platform/win_utf8.h"
#include <objbase.h>
#include <shellapi.h>
#include <SDL3/SDL.h>

int platform_desktop_start(void)
{
    return SUCCEEDED(CoInitialize(0));
}

void platform_desktop_stop(void)
{
    CoUninitialize();
}

/* target is the client's UTF-8 text; ShellExecuteA would read it in the Windows code page. The
   shell's own messages belong to the client's window, the HWND of its SDL window. */
void platform_open_with_desktop(const char *target)
{
    HWND owner = g_hMainWnd ? (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(g_hMainWnd),
                                                           SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL)
                            : NULL;
    win_utf8_shell_execute(owner, "open", target, NULL, NULL, SW_SHOWNORMAL);
}
#endif
