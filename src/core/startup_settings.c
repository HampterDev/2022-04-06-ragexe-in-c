#include "core/app.h"
#include "contents/sound.h"
#include "render/render_api.h"
#include "ui/render/graphic_setting.h"
#include "world/world_imports.h"
#include <string.h>

char g_deviceName[40] = { 0 };
uint32_t g_soundMode;

DWORD g_isVoodoo;
GUID g_deviceGuid;

static void read_startup_value(HKEY key, const char *name, DWORD kind, void *value, DWORD capacity)
{
    BYTE bytes[sizeof g_deviceName];
    DWORD type = 0, size = capacity;

    if (capacity > sizeof bytes)
        return;
    if (RegQueryValueExA(key, name, NULL, &type, bytes, &size) != ERROR_SUCCESS
        || type != kind || size > capacity || (kind != REG_SZ && size != capacity))
        return;
    memcpy(value, bytes, size);
    if (kind == REG_SZ)
        ((char *)value)[capacity - 1] = 0;
}

void read_startup_settings(void)
{
    HKEY key;

    g_isVoodoo = 0;
    g_isVoodooInUse = 0;
    g_trilinearFilter = 0;
    memset(g_deviceName, 0, sizeof g_deviceName);
    memset(&g_deviceGuid, 0, sizeof g_deviceGuid);
    memset(&g_driverGuid, 0, sizeof g_driverGuid);
    g_soundMode = 2;
    g_sound_speaker_type = 0;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, g_registryKeyPath, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return;

    read_startup_value(key, "ISVOODOO", REG_DWORD, &g_isVoodoo, sizeof g_isVoodoo);
    read_startup_value(key, "TRILINEARFILTER", REG_DWORD, &g_trilinearFilter, sizeof g_trilinearFilter);
    read_startup_value(key, "DEVICENAME", REG_SZ, g_deviceName, sizeof g_deviceName);
    read_startup_value(key, "GUIDDEVICE", REG_BINARY, &g_deviceGuid, sizeof g_deviceGuid);
    read_startup_value(key, "GUIDDRIVER", REG_BINARY, &g_driverGuid, sizeof g_driverGuid);
    read_startup_value(key, "SOUNDMODE", REG_DWORD, &g_soundMode, sizeof g_soundMode);
    read_startup_value(key, "SPEAKERTYPE", REG_DWORD, &g_sound_speaker_type, sizeof g_sound_speaker_type);
    RegCloseKey(key);

    g_isVoodooInUse = stale_guid();
}
