/* Settings for scene runs: a store that starts empty and lives in memory. Nothing is read from
   the registry or client_settings.ini and nothing is written back, so every scene starts from
   the client's defaults on every machine and leaves the player's settings alone. Keys and values
   behave as the registry's: names compare without case, a key must exist before it opens. */
#include "platform/settings.h"
#include <stdlib.h>
#include <string.h>

struct SettingValue {
    char *key, *name;
    uint32_t type, size;
    unsigned char *data;
    struct SettingValue *next;
};

struct SettingKey {
    char *path;
    struct SettingKey *next;
};

struct PlatformSettingsKey {
    enum PlatformSettingsAccess access;
    char path[];
};

static struct SettingKey *g_keys;
static struct SettingValue *g_values;

static int same(const char *a, const char *b)
{
    for (;; a++, b++) {
        int x = *a >= 'A' && *a <= 'Z' ? *a - 'A' + 'a' : *a;
        int y = *b >= 'A' && *b <= 'Z' ? *b - 'A' + 'a' : *b;
        if (x != y)
            return 0;
        if (!x)
            return 1;
    }
}

static char *copy_path(const char *path)
{
    size_t length = strlen(path);
    char *copy;
    while (length && path[length - 1] == '\\')
        --length;
    copy = malloc(length + 1);
    if (copy) {
        memcpy(copy, path, length);
        copy[length] = 0;
    }
    return copy;
}

static int key_exists(const char *path)
{
    struct SettingKey *k;
    for (k = g_keys; k; k = k->next)
        if (same(k->path, path))
            return 1;
    return 0;
}

static struct PlatformSettingsKey *new_key(const char *path, enum PlatformSettingsAccess access)
{
    struct PlatformSettingsKey *key = malloc(sizeof *key + strlen(path) + 1);
    if (key) {
        key->access = access;
        strcpy(key->path, path);
    }
    return key;
}

int platform_settings_open(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key)
{
    char *trimmed = copy_path(path);
    struct PlatformSettingsKey *opened = trimmed && key_exists(trimmed) ? new_key(trimmed, access) : NULL;
    free(trimmed);
    if (!opened)
        return 0;
    *key = opened;
    return 1;
}

int platform_settings_create(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key)
{
    char *trimmed = copy_path(path);
    struct PlatformSettingsKey *created;
    if (!trimmed)
        return 0;
    if (!key_exists(trimmed)) {
        struct SettingKey *k = malloc(sizeof *k);
        if (!k) {
            free(trimmed);
            return 0;
        }
        k->path = trimmed;
        k->next = g_keys;
        g_keys = k;
        created = new_key(k->path, access);
    } else {
        created = new_key(trimmed, access);
        free(trimmed);
    }
    if (!created)
        return 0;
    *key = created;
    return 1;
}

static struct SettingValue *find_value(const char *key, const char *name)
{
    struct SettingValue *v;
    for (v = g_values; v; v = v->next)
        if (same(v->key, key) && same(v->name, name))
            return v;
    return NULL;
}

int platform_settings_read(struct PlatformSettingsKey *key, const char *name, uint32_t *type, void *data,
                           uint32_t *size)
{
    struct SettingValue *value;
    if (key->access == PLATFORM_SETTINGS_SET || !(value = find_value(key->path, name)))
        return 0;
    *type = value->type;
    if (value->size > *size) {
        *size = value->size;
        return 0;
    }
    memcpy(data, value->data, value->size);
    *size = value->size;
    return 1;
}

int platform_settings_write(struct PlatformSettingsKey *key, const char *name, enum PlatformSettingType type,
                            const void *data, uint32_t size)
{
    struct SettingValue *value;
    unsigned char *bytes;
    if (key->access == PLATFORM_SETTINGS_QUERY)
        return 0;
    if (type == PLATFORM_SETTING_DWORD ? size != 4 : type != PLATFORM_SETTING_STRING && type != PLATFORM_SETTING_BINARY)
        return 0;
    bytes = malloc(size ? size : 1);
    if (!bytes)
        return 0;
    memcpy(bytes, data, size);
    value = find_value(key->path, name);
    if (!value) {
        value = calloc(1, sizeof *value);
        if (!value || !(value->key = copy_path(key->path)) || !(value->name = copy_path(name))) {
            if (value)
                free(value->key);
            free(value);
            free(bytes);
            return 0;
        }
        value->next = g_values;
        g_values = value;
    }
    free(value->data);
    value->data = bytes;
    value->type = (uint32_t)type;
    value->size = size;
    return 1;
}

void platform_settings_close(struct PlatformSettingsKey *key)
{
    free(key);
}
