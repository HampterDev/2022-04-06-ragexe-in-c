#include "platform/desktop.h"

#ifdef _WIN32
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
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __APPLE__
#define DESKTOP_OPENER "open"
#else
#define DESKTOP_OPENER "xdg-open"
#endif

int platform_desktop_start(void)
{
    return 1;
}

void platform_desktop_stop(void)
{
}

void platform_open_with_desktop(const char *target)
{
    char *const arguments[] = {DESKTOP_OPENER, (char *)target, NULL};
    pid_t child;

    if (target[0] == '-')
        return;
    child = fork();
    if (child == 0) {
        if (fork() == 0) {
            execvp(arguments[0], arguments);
            _exit(127);
        }
        _exit(0);
    }
    if (child > 0)
        waitpid(child, NULL, 0);
}
#endif
