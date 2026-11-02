#include "platform/settings.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, key_access(access), &handle) != ERROR_SUCCESS)
        return 0;
    *key = (struct PlatformSettingsKey *)handle;
    return 1;
}

int platform_settings_create(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key)
{
    HKEY handle;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE, key_access(access), NULL, &handle,
                        NULL) != ERROR_SUCCESS)
        return 0;
    *key = (struct PlatformSettingsKey *)handle;
    return 1;
}

int platform_settings_read(struct PlatformSettingsKey *key, const char *name, uint32_t *type, void *data,
                           uint32_t *size)
{
    return RegQueryValueExA((HKEY)key, name, NULL, (DWORD *)type, data, (DWORD *)size) == ERROR_SUCCESS;
}

int platform_settings_write(struct PlatformSettingsKey *key, const char *name, enum PlatformSettingType type,
                            const void *data, uint32_t size)
{
    return RegSetValueExA((HKEY)key, name, 0, type, data, size) == ERROR_SUCCESS;
}

void platform_settings_close(struct PlatformSettingsKey *key)
{
    RegCloseKey((HKEY)key);
}
#else
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SETTINGS_FILE "client_settings.ini"
#define SETTINGS_VALUE_LIMIT (1024 * 1024)

struct SettingValue {
    char *name;
    uint32_t type;
    uint32_t size;
    unsigned char *data;
};

struct SettingSection {
    char *path;
    struct SettingValue *values;
    size_t count;
    size_t capacity;
};

struct PlatformSettingsKey {
    enum PlatformSettingsAccess access;
    char path[];
};

static struct SettingSection *g_sections;
static size_t g_sectionCount, g_sectionCapacity;
static int g_dirty;

static int fold(int c)
{
    return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

static int same_text(const char *a, const char *b, size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i)
        if (fold((unsigned char)a[i]) != fold((unsigned char)b[i]))
            return 0;
    return 1;
}

static size_t path_length(const char *path)
{
    size_t length = strlen(path);

    while (length && path[length - 1] == '\\')
        --length;
    return length;
}

static char *copy_text(const char *text, size_t length)
{
    char *copy = malloc(length + 1);

    if (copy) {
        memcpy(copy, text, length);
        copy[length] = 0;
    }
    return copy;
}

static struct SettingSection *find_section(const char *path, size_t length)
{
    size_t i;

    for (i = 0; i < g_sectionCount; ++i)
        if (strlen(g_sections[i].path) == length && same_text(g_sections[i].path, path, length))
            return &g_sections[i];
    return NULL;
}

static int key_exists(const char *path, size_t length)
{
    size_t i;

    for (i = 0; i < g_sectionCount; ++i) {
        const char *stored = g_sections[i].path;
        if (strlen(stored) >= length && same_text(stored, path, length) && (!stored[length] || stored[length] == '\\'))
            return 1;
    }
    return 0;
}

static struct SettingSection *add_section(const char *path, size_t length)
{
    struct SettingSection *section;

    if (g_sectionCount == g_sectionCapacity) {
        size_t capacity = g_sectionCapacity ? g_sectionCapacity * 2 : 8;
        struct SettingSection *grown = realloc(g_sections, capacity * sizeof *grown);
        if (!grown)
            return NULL;
        g_sections = grown;
        g_sectionCapacity = capacity;
    }
    section = &g_sections[g_sectionCount];
    memset(section, 0, sizeof *section);
    section->path = copy_text(path, length);
    if (!section->path)
        return NULL;
    ++g_sectionCount;
    return section;
}

static struct SettingValue *find_value(struct SettingSection *section, const char *name)
{
    size_t i, length = strlen(name);

    for (i = 0; i < section->count; ++i)
        if (strlen(section->values[i].name) == length && same_text(section->values[i].name, name, length))
            return &section->values[i];
    return NULL;
}

static int set_value(struct SettingSection *section, const char *name, uint32_t type, const void *data, uint32_t size)
{
    struct SettingValue *value = find_value(section, name);
    unsigned char *bytes = malloc(size ? size : 1);

    if (!bytes)
        return 0;
    memcpy(bytes, data, size);
    if (!value) {
        char *copy;
        if (section->count == section->capacity) {
            size_t capacity = section->capacity ? section->capacity * 2 : 16;
            struct SettingValue *grown = realloc(section->values, capacity * sizeof *grown);
            if (!grown) {
                free(bytes);
                return 0;
            }
            section->values = grown;
            section->capacity = capacity;
        }
        copy = copy_text(name, strlen(name));
        if (!copy) {
            free(bytes);
            return 0;
        }
        value = &section->values[section->count++];
        value->name = copy;
        value->data = NULL;
    }
    free(value->data);
    value->type = type;
    value->size = size;
    value->data = bytes;
    return 1;
}

static void clear_store(void)
{
    size_t i, j;

    for (i = 0; i < g_sectionCount; ++i) {
        for (j = 0; j < g_sections[i].count; ++j) {
            free(g_sections[i].values[j].name);
            free(g_sections[i].values[j].data);
        }
        free(g_sections[i].values);
        free(g_sections[i].path);
    }
    free(g_sections);
    g_sections = NULL;
    g_sectionCount = g_sectionCapacity = 0;
}

static int hex_digit(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

static int unescape(const char *text, size_t length, unsigned char *out, size_t *size)
{
    size_t i, n = 0;

    for (i = 0; i < length; ++i) {
        if (text[i] == '%') {
            int high, low;
            if (length - i < 3 || (high = hex_digit(text[i + 1])) < 0 || (low = hex_digit(text[i + 2])) < 0)
                return 0;
            out[n++] = (unsigned char)(high << 4 | low);
            i += 2;
        } else {
            out[n++] = (unsigned char)text[i];
        }
    }
    *size = n;
    return 1;
}

static int unescape_text(const char *text, size_t length, char **out)
{
    char *result = malloc(length + 1);
    size_t size;

    if (!result)
        return -1;
    if (!unescape(text, length, (unsigned char *)result, &size) || memchr(result, 0, size)) {
        free(result);
        return 0;
    }
    result[size] = 0;
    *out = result;
    return 1;
}

static int parse_number(const char *text, size_t length, uint32_t *number)
{
    uint32_t value = 0;
    size_t i;

    if (!length)
        return 0;
    for (i = 0; i < length; ++i) {
        if (text[i] < '0' || text[i] > '9' || value > (UINT32_MAX - (uint32_t)(text[i] - '0')) / 10)
            return 0;
        value = value * 10 + (uint32_t)(text[i] - '0');
    }
    *number = value;
    return 1;
}

static int decode_value(const char *data, size_t length, uint32_t *type, uint32_t *size, unsigned char **bytes)
{
    const char *text;
    size_t text_length, count, i;
    uint32_t number = 0;

    if (length >= 6 && !memcmp(data, "dword:", 6)) {
        if (!parse_number(data + 6, length - 6, &number))
            return 0;
        *type = PLATFORM_SETTING_DWORD;
        *size = 4;
        text = NULL;
        text_length = 4;
    } else if (length >= 3 && !memcmp(data, "sz:", 3) && (text = memchr(data + 3, ':', length - 3)) != NULL) {
        if (!parse_number(data + 3, (size_t)(text - data - 3), size) || *size > SETTINGS_VALUE_LIMIT)
            return 0;
        *type = PLATFORM_SETTING_STRING;
        ++text;
        text_length = length - (size_t)(text - data);
        if (text_length > SETTINGS_VALUE_LIMIT * 3)
            return 0;
    } else if (length >= 4 && !memcmp(data, "hex:", 4) && length % 2 == 0 && length - 4 <= SETTINGS_VALUE_LIMIT * 2) {
        *type = PLATFORM_SETTING_BINARY;
        *size = (uint32_t)((length - 4) / 2);
        text = data + 4;
        text_length = length - 4;
    } else {
        return 0;
    }
    *bytes = calloc(1, (text_length > *size ? text_length : *size) + 1);
    if (!*bytes)
        return -1;
    if (*type == PLATFORM_SETTING_DWORD) {
        memcpy(*bytes, &number, 4);
        return 1;
    }
    if (*type == PLATFORM_SETTING_STRING) {
        if (unescape(text, text_length, *bytes, &count) && count <= *size)
            return 1;
    } else {
        for (i = 0; i < *size; ++i) {
            int high = hex_digit(text[i * 2]), low = hex_digit(text[i * 2 + 1]);
            if (high < 0 || low < 0)
                break;
            (*bytes)[i] = (unsigned char)(high << 4 | low);
        }
        if (i == *size)
            return 1;
    }
    free(*bytes);
    return 0;
}

static int load_value(struct SettingSection *section, const char *line, size_t length)
{
    const char *equals = memchr(line, '=', length);
    unsigned char *bytes;
    uint32_t type, size;
    char *name;
    int result;

    if (!equals)
        return 0;
    result = decode_value(equals + 1, length - (size_t)(equals - line) - 1, &type, &size, &bytes);
    if (result <= 0)
        return result;
    result = unescape_text(line, (size_t)(equals - line), &name);
    if (result > 0) {
        result = set_value(section, name, type, bytes, size) ? 1 : -1;
        free(name);
    }
    free(bytes);
    return result;
}

static int load_section(const char *line, size_t length, struct SettingSection **section)
{
    char *path;
    size_t trimmed;
    int result = unescape_text(line, length, &path);

    if (result <= 0) {
        *section = NULL;
        return result;
    }
    trimmed = path_length(path);
    *section = find_section(path, trimmed);
    if (!*section)
        *section = add_section(path, trimmed);
    free(path);
    return *section ? 1 : -1;
}

static int parse_store(const char *text, size_t length)
{
    struct SettingSection *section = NULL;
    const char *line = text, *end = text + length;

    while (line < end) {
        const char *next = memchr(line, '\n', (size_t)(end - line));
        size_t count = next ? (size_t)(next - line) : (size_t)(end - line);
        if (count && line[count - 1] == '\r')
            --count;
        if (count >= 2 && line[0] == '[' && line[count - 1] == ']') {
            if (load_section(line + 1, count - 2, &section) < 0)
                return 0;
        } else if (count && section && load_value(section, line, count) < 0) {
            return 0;
        }
        line = next ? next + 1 : end;
    }
    return 1;
}

static int load_store(void)
{
    FILE *file;
    char *text = NULL;
    size_t length = 0, capacity = 0;
    int ok;

    clear_store();
    file = fopen(SETTINGS_FILE, "rb");
    if (!file)
        return errno == ENOENT;
    for (;;) {
        size_t count;
        if (length == capacity) {
            char *grown = capacity <= SIZE_MAX / 2 ? realloc(text, capacity ? capacity * 2 : 4096) : NULL;
            if (!grown) {
                free(text);
                fclose(file);
                return 0;
            }
            text = grown;
            capacity = capacity ? capacity * 2 : 4096;
        }
        count = fread(text + length, 1, capacity - length, file);
        length += count;
        if (!count)
            break;
    }
    ok = !ferror(file);
    fclose(file);
    if (ok)
        ok = parse_store(text, length);
    free(text);
    return ok;
}

static void write_escaped(FILE *file, const unsigned char *bytes, size_t size, const char *special)
{
    size_t i;

    for (i = 0; i < size; ++i) {
        if (bytes[i] < 0x20 || bytes[i] >= 0x7f || bytes[i] == '%' || strchr(special, bytes[i]))
            fprintf(file, "%%%02X", bytes[i]);
        else
            fputc(bytes[i], file);
    }
}

static void write_value(FILE *file, const struct SettingValue *value)
{
    size_t i, used = value->size;

    write_escaped(file, (const unsigned char *)value->name, strlen(value->name), "=[");
    if (value->type == PLATFORM_SETTING_DWORD) {
        uint32_t number;
        memcpy(&number, value->data, 4);
        fprintf(file, "=dword:%lu\n", (unsigned long)number);
    } else if (value->type == PLATFORM_SETTING_STRING) {
        while (used && !value->data[used - 1])
            --used;
        fprintf(file, "=sz:%lu:", (unsigned long)value->size);
        write_escaped(file, value->data, used, "");
        fputc('\n', file);
    } else {
        fputs("=hex:", file);
        for (i = 0; i < value->size; ++i)
            fprintf(file, "%02x", value->data[i]);
        fputc('\n', file);
    }
}

static int save_store(void)
{
    char temporary[sizeof SETTINGS_FILE + 32];
    int descriptor, ok;
    FILE *file;
    size_t i, j;

    snprintf(temporary, sizeof temporary, "%s.%ld.tmp", SETTINGS_FILE, (long)getpid());
    descriptor = open(temporary, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (descriptor < 0)
        return 0;
    file = fdopen(descriptor, "wb");
    if (!file) {
        close(descriptor);
        remove(temporary);
        return 0;
    }
    for (i = 0; i < g_sectionCount; ++i) {
        if (i)
            fputc('\n', file);
        fputc('[', file);
        write_escaped(file, (const unsigned char *)g_sections[i].path, strlen(g_sections[i].path), "");
        fputs("]\n", file);
        for (j = 0; j < g_sections[i].count; ++j)
            write_value(file, &g_sections[i].values[j]);
    }
    ok = fflush(file) == 0 && !ferror(file) && fsync(fileno(file)) == 0;
    if (fclose(file))
        ok = 0;
    if (ok)
        ok = rename(temporary, SETTINGS_FILE) == 0;
    if (!ok)
        remove(temporary);
    return ok;
}

static struct PlatformSettingsKey *new_key(const char *path, size_t length, enum PlatformSettingsAccess access)
{
    struct PlatformSettingsKey *key = malloc(sizeof *key + length + 1);

    if (key) {
        key->access = access;
        memcpy(key->path, path, length);
        key->path[length] = 0;
    }
    return key;
}

int platform_settings_open(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key)
{
    size_t length = path_length(path);
    struct PlatformSettingsKey *opened;

    if (!g_dirty && !load_store())
        return 0;
    if (!key_exists(path, length) || !(opened = new_key(path, length, access)))
        return 0;
    *key = opened;
    return 1;
}

int platform_settings_create(const char *path, enum PlatformSettingsAccess access, struct PlatformSettingsKey **key)
{
    size_t length = path_length(path);
    struct PlatformSettingsKey *created;

    if (!g_dirty && !load_store())
        return 0;
    if (!key_exists(path, length)) {
        if (!add_section(path, length))
            return 0;
        g_dirty = 1;
    }
    if (!(created = new_key(path, length, access)))
        return 0;
    *key = created;
    return 1;
}

int platform_settings_read(struct PlatformSettingsKey *key, const char *name, uint32_t *type, void *data,
                           uint32_t *size)
{
    struct SettingSection *section;
    struct SettingValue *value;

    if (key->access == PLATFORM_SETTINGS_SET || !(section = find_section(key->path, strlen(key->path)))
        || !(value = find_value(section, name)))
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
    struct SettingSection *section;

    if (key->access == PLATFORM_SETTINGS_QUERY || size > SETTINGS_VALUE_LIMIT)
        return 0;
    if (type == PLATFORM_SETTING_DWORD ? size != 4 : type != PLATFORM_SETTING_STRING && type != PLATFORM_SETTING_BINARY)
        return 0;
    section = find_section(key->path, strlen(key->path));
    if (!section && !(section = add_section(key->path, strlen(key->path))))
        return 0;
    if (!set_value(section, name, type, data, size))
        return 0;
    g_dirty = 1;
    return 1;
}

void platform_settings_close(struct PlatformSettingsKey *key)
{
    free(key);
    if (g_dirty && save_store())
        g_dirty = 0;
}
#endif
