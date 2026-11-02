#include "core/client_path.h"
#include "core/app.h"
#include "platform/filesystem.h"
#include <stdarg.h>
#include <stdio.h>

#define CLIENT_PATH_MAX 0x800

int client_path(char *out, size_t size, const char *format, ...)
{
    char relative[CLIENT_PATH_MAX];
    va_list arguments;
    int count;
    va_start(arguments, format);
    count = vsnprintf(relative, sizeof relative, format, arguments);
    va_end(arguments);
    if (count < 0 || (size_t)count >= sizeof relative) {
        if (size) out[0] = 0;
        return 0;
    }
    return platform_path_join(out, size, g_clientDir, relative);
}
