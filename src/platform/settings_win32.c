#ifdef _WIN32
#include "platform/settings.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "platform/win_utf8.h"

/* The registry under HKEY_CURRENT_USER. Key paths, value names and string values are the client's
   UTF-8 text; platform/win_utf8.c stores them through the W calls, so they do not depend on the
   Windows code page. */

typedef char settings_types_are_registry_types[PLATFORM_SETTING_STRING == REG_SZ
    && PLATFORM_SETTING_BINARY == REG_BINARY && PLATFORM_SETTING_DWORD == REG_DWORD
    && sizeof(DWORD) == sizeof(uint32_t) ? 1 : -1];

static REGSAM key_access(enum PlatformSettingsAccess access)
{
    switch (access) {
    case PLATFORM_SETTINGS_QUERY:
        return KEY_QUERY_VALUE;
    case PLATFORM_SETTINGS_SET:
        return KEY_SET_VALUE;
    default:
        return KEY_ALL_ACCESS;
    }
}

int platform_settings_open(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key)
{
    HKEY handle;

    if (win_utf8_reg_open_key(HKEY_CURRENT_USER, path, 0, key_access(access), &handle) != ERROR_SUCCESS)
        return 0;
    *key = (struct PlatformSettingsKey *)handle;
    return 1;
}

int platform_settings_create(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key)
{
    HKEY handle;

    if (win_utf8_reg_create_key(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE, key_access(access), NULL,
                                &handle, NULL) != ERROR_SUCCESS)
        return 0;
    *key = (struct PlatformSettingsKey *)handle;
    return 1;
}

int platform_settings_read(struct PlatformSettingsKey *key, const char *name, uint32_t *type, void *data,
                           uint32_t *size)
{
    return win_utf8_reg_query_value((HKEY)key, name, NULL, (DWORD *)type, data, (DWORD *)size) == ERROR_SUCCESS;
}

int platform_settings_write(struct PlatformSettingsKey *key, const char *name, enum PlatformSettingType type,
                            const void *data, uint32_t size)
{
    return win_utf8_reg_set_value((HKEY)key, name, 0, type, data, size) == ERROR_SUCCESS;
}

void platform_settings_close(struct PlatformSettingsKey *key)
{
    RegCloseKey((HKEY)key);
}
#endif
