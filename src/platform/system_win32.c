#ifdef _WIN32
#include "platform/system.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void platform_debug_output(const char *text)
{
    OutputDebugStringA(text);
}

unsigned platform_user_language(void)
{
    return GetUserDefaultLangID();
}

void platform_set_background_priority(int background)
{
    SetPriorityClass(GetCurrentProcess(), background ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
}
#endif
