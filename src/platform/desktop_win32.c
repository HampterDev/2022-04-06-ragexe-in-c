#ifdef _WIN32
#include "platform/desktop.h"
#include "core/app.h"
#include "platform/win_utf8.h"
#include <objbase.h>
#include <shellapi.h>

int platform_desktop_start(void)
{
    return SUCCEEDED(CoInitialize(0));
}

void platform_desktop_stop(void)
{
    CoUninitialize();
}

/* target is the client's UTF-8 text; ShellExecuteA would read it in the Windows code page. */
void platform_open_with_desktop(const char *target)
{
    win_utf8_shell_execute(g_hMainWnd, "open", target, NULL, NULL, SW_SHOWNORMAL);
}
#endif
