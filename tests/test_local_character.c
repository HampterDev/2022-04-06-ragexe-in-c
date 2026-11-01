#include "contents/character_settings_internal.h"
#include "contents/settings_file.h"
#include "session/state.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
char g_clientDir[0x88];
const char *g_serverAddress = "one.example", *g_serverPort = "6900";
const char *g_registryKeyPath = "TestOnly";
static struct { int before; int values[0x157c]; int after; } player;
static struct { int before; int values[0x3d]; int after; } homun;
static int merc[0x102], registry_missing, registry_invalid, fail_alloc;

static void *staging_calloc(size_t count, size_t size)
{ return fail_alloc ? NULL : calloc(count, size); }
#define calloc staging_calloc
#include "../src/contents/character_settings_json.c"
#undef calloc

static LSTATUS WINAPI registry_open(HKEY key, LPCSTR path, DWORD options, REGSAM access, PHKEY out)
{
    assert(key == HKEY_CURRENT_USER && !options && access == KEY_QUERY_VALUE);
    assert(!strcmp(path, "TestOnly\\SkillUseLevelInfo"));
    if (registry_missing) return ERROR_FILE_NOT_FOUND;
    *out = (HKEY)1;
    return ERROR_SUCCESS;
}
static LSTATUS WINAPI registry_close(HKEY key)
{ assert(key == (HKEY)1); return ERROR_SUCCESS; }
static LSTATUS WINAPI registry_query(HKEY key, LPCSTR name, LPDWORD reserved,
                                     LPDWORD type, LPBYTE out, LPDWORD size)
{
    assert(key == (HKEY)1 && !reserved && *size == 4);
    *type = REG_DWORD;
    if (!strcmp(name, "player0")) *(int *)out = 2;
    else if (!strcmp(name, "homun0")) *(int *)out = 4;
    else if (!strcmp(name, "player1")) *(int *)out = 3;
    else return ERROR_FILE_NOT_FOUND;
    if (registry_invalid == 1) *type = REG_SZ;
    if (registry_invalid == 2) *size = 2;
    if (registry_invalid == 3) *(int *)out = -1;
    if (registry_invalid == 4) *(int *)out = 65536;
    return ERROR_SUCCESS;
}
#define RegOpenKeyExA registry_open
#define RegCloseKey registry_close
#define RegQueryValueExA registry_query
#include "../src/contents/character_settings_seed.c"
#undef RegOpenKeyExA
#undef RegCloseKey
#undef RegQueryValueExA

const char *skill_get_id_name(int id)
{ return id == 42 ? "player" : id == 8000 ? "homun" : "unused"; }
unsigned char skill_info_is_level_used(int id)
{ return id == 42 || id == 8000; }
void player_skill_bundle_set_chosen_level(struct PlayerSkillBundle *self, int id, int value)
{ assert(self == &g_session1.skills && id > 0 && id < 0x157c); player.values[id] = value; }
void session_set_homun_skill_use_level(struct Session *self, int id, int value)
{ assert(self == &g_session1 && id >= 8000 && id < 8061); homun.values[id - 8000] = value; }
void session_set_merc_skill_use_level(struct Session *self, int id, int value)
{ assert(self == &g_session1 && id >= 0x2008 && id < 0x210a); merc[id - 0x2008] = value; }
void *mem_alloc(size_t n) { void *p = malloc(n ? n : 1); assert(p); return p; }
void mem_free(void *p) { free(p); }
void fatal_error(const char *message) { (void)message; abort(); }

static void expect(int p, int h)
{
    assert(player.values[42] == p && homun.values[0] == h);
    assert(player.before == 123 && player.after == 456 && homun.before == 789 && homun.after == 321);
}
static char files[16][MAX_PATH];
static int file_count;
static void remember(void)
{
    int i;
    for (i = 0; i < file_count; ++i)
        if (!strcmp(files[i], g_charUserDataMgr->path)) return;
    assert(file_count < 16);
    strcpy(files[file_count++], g_charUserDataMgr->path);
}
static void load(int aid, int gid, const char *world)
{
    char_user_data_load(g_charUserDataMgr, aid, gid, world);
    assert(g_charUserDataMgr->loaded);
    remember();
}
static void cleanup(void)
{
    char path[MAX_PATH], *p;
    int i;
    size_t root_length = strlen(g_clientDir);
    for (i = 0; i < file_count; ++i) {
        assert(!strncmp(files[i], g_clientDir, root_length) && files[i][root_length] == '\\');
        DeleteFileA(files[i]);
    }
    for (i = 0; i < file_count; ++i) {
        strcpy(path, files[i]);
        while ((p = strrchr(path, '\\')) != NULL && (size_t)(p - path) > root_length) {
            *p = 0; RemoveDirectoryA(path);
        }
    }
    assert(RemoveDirectoryA(g_clientDir));
}
int main(void)
{
    struct JsonTree data;
    struct OwnedText before = {0}, after = {0};
    char parent[MAX_PATH], temporary[MAX_PATH], first[MAX_PATH], account[MAX_PATH];
    const char *world = "World/..\\이름";
    const char *invalid[] = {
        "{truncated", "[]", "{\"version\":2,\"player\":[],\"homunculus\":[]}",
        "{\"version\":1,\"player\":[],\"homunculus\":null}",
        "{\"version\":1,\"player\":[{\"id\":0,\"level\":1}],\"homunculus\":[]}",
        "{\"version\":1,\"player\":[{\"id\":5500,\"level\":1}],\"homunculus\":[]}",
        "{\"version\":1,\"player\":[{\"id\":42,\"level\":-1}],\"homunculus\":[]}",
        "{\"version\":1,\"player\":[{\"id\":42,\"level\":65536}],\"homunculus\":[]}",
        "{\"version\":1,\"player\":[{\"id\":42,\"level\":1.5}],\"homunculus\":[]}",
        "{\"version\":1,\"player\":[{\"id\":42,\"level\":1},{\"id\":42,\"level\":2}],\"homunculus\":[]}",
        "{\"version\":1,\"player\":[{\"id\":42,\"level\":9}],\"homunculus\":[{\"id\":8061,\"level\":1}]}",
        "{\"version\":1,\"player\":[],\"homunculus\":[{\"id\":7999,\"level\":1}]}"
    };
    size_t i;
    assert(GetTempPathA(sizeof parent, parent));
    assert(GetTempFileNameA(parent, "chr", 0, temporary));
    assert(DeleteFileA(temporary) && CreateDirectoryA(temporary, NULL));
    assert(strlen(temporary) < sizeof g_clientDir); strcpy(g_clientDir, temporary);
    player.before = 123; player.after = 456; homun.before = 789; homun.after = 321;
    g_session1.skills.chosen_levels.first = player.values;
    g_session1.skills.chosen_levels.last = g_session1.skills.chosen_levels.end = player.values + 0x157c;
    g_session1.homunSkillUseLevels.first = homun.values;
    g_session1.homunSkillUseLevels.last = g_session1.homunSkillUseLevels.end = homun.values + 0x3d;
    g_session1.merSkillUseLevels.first = merc;
    g_session1.merSkillUseLevels.last = g_session1.merSkillUseLevels.end = merc + 0x102;
    assert(char_user_data_create() && !char_user_data_create());
    assert(!char_user_data_save(NULL) && !char_user_data_save(g_charUserDataMgr));
    load(101, 201, world); expect(2, 4); strcpy(first, g_charUserDataMgr->path);
    assert(settings_file_path(account, g_clientDir, g_serverAddress, g_serverPort, world, 101, 0));
    assert(strcmp(account, first) && settings_file_write(account, text_view_cstr("account sentinel")));
    strcpy(files[file_count++], account);
    player.values[42] = 7; homun.values[0] = 8;
    player.values[0x157b] = 9; homun.values[0x3c] = 10;
    assert(char_user_data_save(g_charUserDataMgr));
    assert(settings_file_read(first, &before) == 1 && !strstr(before.data, "AuthToken"));
    player.values[42] = 6; load(101, 201, world); expect(6, 8);
    g_session1.shortcutCharSlot = 1;
    load(101, 202, world); expect(3, 0);
    assert(!player.values[0x157b] && !homun.values[0x3c]);
    player.values[42] = 5; assert(char_user_data_save(g_charUserDataMgr));
    g_session1.shortcutCharSlot = 0;
    load(101, 201, world); expect(6, 8);
    assert(player.values[0x157b] == 9 && homun.values[0x3c] == 10);
    g_session1.ownAid = 999; g_session1.ownGid = 999;
    player.values[42] = 7; assert(char_user_data_save(g_charUserDataMgr));
    assert(settings_file_read(first, &after) == 1 && before.size == after.size && !memcmp(before.data, after.data, before.size));
    player.values[42] = -1; assert(!char_user_data_save(g_charUserDataMgr));
    assert(settings_file_read(first, &after) == 1 && before.size == after.size && !memcmp(before.data, after.data, before.size));
    player.values[42] = 7;
    json_tree_init(&data); assert(character_settings_capture(&data));
    fail_alloc = 1; assert(!character_settings_apply(&data)); expect(7, 8); fail_alloc = 0;
    json_tree_free(&data);
    registry_missing = 1; load(102, 201, world); expect(0, 0); registry_missing = 0;
    load(101, 201, world); expect(7, 8);
    g_serverAddress = "two.example"; load(101, 201, world); expect(2, 4);
    g_serverAddress = "one.example"; g_serverPort = "6901"; load(101, 201, world); expect(2, 4);
    g_serverPort = "6900"; load(101, 201, "different world"); expect(2, 4);
    load(101, 201, world); expect(7, 8);
    assert(character_settings_clear() && char_user_data_save(g_charUserDataMgr));
    load(101, 202, world); expect(5, 0);
    load(101, 201, world); expect(0, 0);
    assert(settings_file_read(first, &after) == 1);
    json_tree_init(&data); assert(json_parse_text(&data, after.data, after.size));
    assert(field(&data, "player")->type == JSON_TREE_ARRAY && !field(&data, "player")->count);
    assert(field(&data, "homunculus")->type == JSON_TREE_ARRAY && !field(&data, "homunculus")->count);
    json_tree_free(&data);
    char_user_data_destroy(); assert(char_user_data_create());
    load(101, 202, world); expect(5, 0);
    for (i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        assert(settings_file_write(first, text_view_cstr(invalid[i])));
        char_user_data_load(g_charUserDataMgr, 101, 201, world);
        assert(!g_charUserDataMgr->loaded && !char_user_data_save(g_charUserDataMgr)); expect(2, 4);
        assert(settings_file_read(first, &after) == 1 && !strcmp(after.data, invalid[i]));
        load(101, 202, world); expect(5, 0);
    }
    char_user_data_load(g_charUserDataMgr, 101, 0, world);
    assert(!g_charUserDataMgr->loaded && !*g_charUserDataMgr->path); expect(0, 0);
    assert(!char_user_data_save(g_charUserDataMgr));
    load(101, 202, world); expect(5, 0);
    char_user_data_load(g_charUserDataMgr, 0, 202, world);
    assert(!g_charUserDataMgr->loaded && !*g_charUserDataMgr->path); expect(0, 0);
    assert(settings_file_read(account, &after) == 1 && !strcmp(after.data, "account sentinel"));
    for (registry_invalid = 1; registry_invalid <= 4; ++registry_invalid) {
        assert(character_settings_clear()); character_settings_seed(); expect(0, 0);
    }
    registry_invalid = 0;
    owned_text_clear(&before); owned_text_clear(&after);
    char_user_data_destroy(); char_user_data_destroy(); cleanup();
    puts("Local character settings: identity isolation, validated registry seeds, complete snapshots, unavailable-skill retention, fresh-owner reload, empty values, malformed-file preservation, staged failure and frozen save target passed");
    return 0;
}
