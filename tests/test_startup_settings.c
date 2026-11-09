#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "core/app.h"
#include "contents/sound.h"

struct Settings {
    int trilinear;
    char device[40];
    GUID device_guid, driver_guid;
    uint32_t sound;
    int speakers;
};
struct Value {
    const char *name;
    DWORD type;
    size_t offset;
    DWORD size;
};
#define VALUE(name, type, member) { name, type, offsetof(struct Settings, member), sizeof(((struct Settings *)0)->member) }
static const struct Value values[] = {
    VALUE("TRILINEARFILTER", REG_DWORD, trilinear),
    VALUE("DEVICENAME", REG_SZ, device),
    VALUE("GUIDDEVICE", REG_BINARY, device_guid),
    VALUE("GUIDDRIVER", REG_BINARY, driver_guid),
    VALUE("SOUNDMODE", REG_DWORD, sound),
    VALUE("SPEAKERTYPE", REG_DWORD, speakers)
};
#undef VALUE
static const struct Settings defaults = { .sound = 2 };
static struct Settings saved;
static LONG open_error;
static int opens, closes, queries, live_key, normalized;
static int empty_key, bad_value = -1, fault, key_token;
const char *g_registryKeyPath = "Software\\TestClient";
int g_trilinearFilter;
GUID g_driverGuid;
int g_sound_speaker_type;

static LONG WINAPI read_open(HKEY root, const char *path, DWORD options, REGSAM access, HKEY *key)
{
    assert(root == HKEY_CURRENT_USER && !strcmp(path, g_registryKeyPath));
    assert(options == 0 && access == KEY_QUERY_VALUE && ++opens == 1);
    if (open_error)
        return open_error;
    live_key = 1;
    *key = (HKEY)&key_token;
    return ERROR_SUCCESS;
}
static LONG WINAPI read_value(HKEY key, const char *name, DWORD *reserved, DWORD *type, BYTE *data, DWORD *size)
{
    size_t i;
    const struct Value *value;
    DWORD capacity = *size;
    assert(live_key && key == (HKEY)&key_token && reserved == NULL);
    ++queries;
    for (i = 0; i < sizeof values / sizeof *values; ++i)
        if (!strcmp(name, values[i].name))
            break;
    assert(i < sizeof values / sizeof *values);
    value = &values[i];
    assert(capacity == value->size);
    if (empty_key)
        return ERROR_FILE_NOT_FOUND;
    *type = value->type;
    *size = value->size;
    memcpy(data, (const BYTE *)&saved + value->offset, capacity);
    if ((int)i == bad_value) {
        switch (fault) {
        case 0: *size = 2048; return ERROR_MORE_DATA;
        case 1: *type = REG_MULTI_SZ; break;
        case 2: *size = 1; break;
        case 3: *size = 0; return ERROR_ACCESS_DENIED;
        }
    }
    return ERROR_SUCCESS;
}
static LONG WINAPI read_close(HKEY key)
{
    assert(live_key && key == (HKEY)&key_token);
    live_key = 0;
    ++closes;
    return ERROR_SUCCESS;
}
#include "platform/win_utf8.h"
#define win_utf8_reg_open_key read_open
#define win_utf8_reg_query_value read_value
#define RegCloseKey read_close
#include "../src/platform/settings_win32.c"
#include "../src/core/startup_settings.c"
#undef win_utf8_reg_open_key
#undef win_utf8_reg_query_value
#undef RegCloseKey

void stale_guid(void)
{
    assert(!live_key);
    ++normalized;
}
static void expect(const struct Settings *expected)
{
    assert(g_trilinearFilter == expected->trilinear);
    assert(!memcmp(g_deviceName, expected->device, sizeof g_deviceName));
    assert(!memcmp(&g_deviceGuid, &expected->device_guid, sizeof g_deviceGuid));
    assert(!memcmp(&g_driverGuid, &expected->driver_guid, sizeof g_driverGuid));
    assert(g_soundMode == expected->sound);
    assert(g_sound_speaker_type == expected->speakers);
    assert(opens == 1 && !live_key);
    assert(closes == !open_error && normalized == !open_error);
    assert(queries == (open_error ? 0 : sizeof values / sizeof *values));
}
static void load(void)
{
    opens = closes = queries = live_key = normalized = 0;
    read_startup_settings();
}
int main(void)
{
    struct Settings expected;
    size_t i;
    saved = (struct Settings){ .trilinear = 1,
        .device = "Saved display", .device_guid = {0x1234, 2, 3, {4}},
        .driver_guid = {0x5678, 6, 7, {8}}, .sound = 2, .speakers = 3 };
    for (i = 0; i < 3; ++i) {
        saved.sound = (uint32_t)i;
        load();
        expect(&saved);
    }
    saved.sound = 1;
    load();
    expect(&saved);
    open_error = ERROR_FILE_NOT_FOUND;
    load();
    expect(&defaults);
    open_error = ERROR_ACCESS_DENIED;
    load();
    expect(&defaults);
    open_error = 0;
    empty_key = 1;
    load();
    expect(&defaults);
    empty_key = 0;
    for (i = 0; i < sizeof values / sizeof *values; ++i) {
        bad_value = (int)i;
        expected = saved;
        memcpy((BYTE *)&expected + values[i].offset,
               (const BYTE *)&defaults + values[i].offset, values[i].size);
        for (fault = 0; fault < 4; ++fault) {
            if (fault == 2 && values[i].type == REG_SZ)
                continue;
            load();
            expect(&expected);
        }
    }
    bad_value = -1;
    memset(saved.device, 'd', sizeof saved.device);
    expected = saved;
    expected.device[sizeof expected.device - 1] = 0;
    load();
    expect(&expected);
    puts("Startup settings: missing/inaccessible keys, saved values, per-value fallback, malformed types/sizes, partial failures, bounded strings and handle ownership passed.");
    return 0;
}
