#include "core/app.h"
#include "platform/client_options.h"
#include "platform/settings.h"
#include "render/render_options.h"
#include <string.h>

uint32_t g_soundMode;

/* custom: the display device's name and GUIDs (DEVICENAME, GUIDDEVICE, GUIDDRIVER) are no longer
   read: SDL picks the GPU (render/gfx_gpu.c). */
static void read_startup_dword(struct PlatformSettingsKey *key, const char *name, void *value)
{
    BYTE bytes[4];
    uint32_t type = 0, size = sizeof bytes;

    if (!platform_settings_read(key, name, &type, bytes, &size) || type != PLATFORM_SETTING_DWORD
        || size != sizeof bytes)
        return;
    memcpy(value, bytes, sizeof bytes);
}

void read_startup_settings(void)
{
    struct PlatformSettingsKey *key;

    g_trilinearFilter = 0;
    g_soundMode = 2;

    if (!platform_settings_open(g_registryKeyPath, PLATFORM_SETTINGS_QUERY, &key))
        return;

    read_startup_dword(key, "TRILINEARFILTER", &g_trilinearFilter);
    read_startup_dword(key, "SOUNDMODE", &g_soundMode);
    platform_settings_close(key);
}
