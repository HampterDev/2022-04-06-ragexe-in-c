#ifdef _WIN32
#include "platform/desktop.h"
#include "core/app.h"
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

void platform_open_with_desktop(const char *target)
{
    ShellExecuteA(g_hMainWnd, "open", target, NULL, NULL, SW_SHOWNORMAL);
}
#endif
