#ifdef _WIN32
#include "platform/display_depth.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

int platform_display_has_16bit_modes(void)
{
    OSVERSIONINFOA version;

    memset(&version, 0, sizeof version);
    version.dwOSVersionInfoSize = sizeof version;
    GetVersionExA(&version);
    return !(version.dwMajorVersion >= 6 && version.dwMinorVersion >= 2);
}
#endif
