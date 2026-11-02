#include "core/app.h"
#include "contents/sound.h"
#include "platform/settings.h"
#include "render/render_api.h"
#include "ui/render/graphic_setting.h"
#include "world/world_imports.h"
#include <string.h>

char g_deviceName[40] = { 0 };
uint32_t g_soundMode;

DWORD g_isVoodoo;
GUID g_deviceGuid;

static void read_startup_value(struct PlatformSettingsKey *key, const char *name, enum PlatformSettingType kind,
                               void *value, uint32_t capacity)
{
    BYTE bytes[sizeof g_deviceName];
    uint32_t type = 0, size = capacity;

    if (capacity > sizeof bytes)
        return;
    if (!platform_settings_read(key, name, &type, bytes, &size)
        || type != kind || size > capacity || (kind != PLATFORM_SETTING_STRING && size != capacity))
        return;
    memcpy(value, bytes, size);
    if (kind == PLATFORM_SETTING_STRING)
        ((char *)value)[capacity - 1] = 0;
}

void read_startup_settings(void)
{
    struct PlatformSettingsKey *key;

    g_isVoodoo = 0;
    g_isVoodooInUse = 0;
    g_trilinearFilter = 0;
    memset(g_deviceName, 0, sizeof g_deviceName);
    memset(&g_deviceGuid, 0, sizeof g_deviceGuid);
    memset(&g_driverGuid, 0, sizeof g_driverGuid);
    g_soundMode = 2;
    g_sound_speaker_type = 0;

    if (!platform_settings_open(g_registryKeyPath, PLATFORM_SETTINGS_QUERY, &key))
        return;

    read_startup_value(key, "ISVOODOO", PLATFORM_SETTING_DWORD, &g_isVoodoo, sizeof g_isVoodoo);
    read_startup_value(key, "TRILINEARFILTER", PLATFORM_SETTING_DWORD, &g_trilinearFilter, sizeof g_trilinearFilter);
    read_startup_value(key, "DEVICENAME", PLATFORM_SETTING_STRING, g_deviceName, sizeof g_deviceName);
    read_startup_value(key, "GUIDDEVICE", PLATFORM_SETTING_BINARY, &g_deviceGuid, sizeof g_deviceGuid);
    read_startup_value(key, "GUIDDRIVER", PLATFORM_SETTING_BINARY, &g_driverGuid, sizeof g_driverGuid);
    read_startup_value(key, "SOUNDMODE", PLATFORM_SETTING_DWORD, &g_soundMode, sizeof g_soundMode);
    read_startup_value(key, "SPEAKERTYPE", PLATFORM_SETTING_DWORD, &g_sound_speaker_type, sizeof g_sound_speaker_type);
    platform_settings_close(key);

    g_isVoodooInUse = stale_guid();
}
