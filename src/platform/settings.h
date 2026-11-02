#ifndef PLATFORM_SETTINGS_H
#define PLATFORM_SETTINGS_H

#include <stdint.h>

enum PlatformSettingType {
    PLATFORM_SETTING_STRING = 1,
    PLATFORM_SETTING_BINARY = 3,
    PLATFORM_SETTING_DWORD = 4
};

enum PlatformSettingsAccess {
    PLATFORM_SETTINGS_QUERY,
    PLATFORM_SETTINGS_SET,
    PLATFORM_SETTINGS_ALL
};

struct PlatformSettingsKey;

int platform_settings_open(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key);
int platform_settings_create(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key);
int platform_settings_read(struct PlatformSettingsKey *key, const char *name, uint32_t *type, void *data,
                           uint32_t *size);
int platform_settings_write(struct PlatformSettingsKey *key, const char *name, enum PlatformSettingType type,
                            const void *data, uint32_t size);
void platform_settings_close(struct PlatformSettingsKey *key);

#endif
