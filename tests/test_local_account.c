#include "contents/account_settings_internal.h"
#include "contents/settings_file.h"
#include "contents/settings_json.h"
#include "contents/user_hotkeys.h"
#include "contents/whisper_block.h"
#include "contents/client_lua.h"
#include "session/state.h"
#include "session/chat_macros.h"
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
char g_clientDir[0x88];
const char *g_serverAddress = "one.example", *g_serverPort = "6900";
UINT g_codePage = 949;
static int fail_write, fail_flush, fail_move;
static BOOL WINAPI write_file(HANDLE h, LPCVOID p, DWORD n, LPDWORD out, LPOVERLAPPED o)
{ if (fail_write) return FALSE; return WriteFile(h, p, n, out, o); }
static BOOL WINAPI flush_file(HANDLE h)
{ if (fail_flush) return FALSE; return FlushFileBuffers(h); }
static BOOL WINAPI move_file(LPCSTR a, LPCSTR b, DWORD flags)
{ if (fail_move) return FALSE; return MoveFileExA(a, b, flags); }
#define WriteFile write_file
#define FlushFileBuffers flush_file
#define MoveFileExA move_file
#include "../src/contents/settings_file.c"
#include "../src/platform/filesystem.c"
#undef WriteFile
#undef FlushFileBuffers
#undef MoveFileExA

void *mem_alloc(size_t n) { void *p = malloc(n ? n : 1); assert(p); return p; }
void mem_free(void *p) { free(p); }
void fatal_error(const char *m) { (void)m; abort(); }
int session_get_hot_key_list_size(struct Session *s, int group)
{ assert(s == &g_session1 && group >= 0 && group < 4); return 3; }
int session_get_original_hot_key_index(struct Session *s, int group, int index)
{ assert(s == &g_session1 && group >= 0 && group < 4); return index; }

static void script(const char *text)
{ assert(luaL_dostring(g_session1.luaState->L, text) == 0); }
static void macro(const char *text)
{ assert(session_set_chat_macro(&g_session1, 0, text_view_cstr(text))); }
static void expect(const char *text, const char *blocked, int hotkey)
{
    assert(!strcmp(session_chat_macro(&g_session1, 0).data, text));
    assert(whisper_block_count(g_whisperBlockMgr) == (blocked != NULL));
    if (blocked) assert(whisper_block_contains(g_whisperBlockMgr, blocked));
    assert(user_hotkey_mgr_collect(g_userHotkeyMgr));
    assert(g_userHotkeyMgr->keys.size == (hotkey != 0));
    if (hotkey) assert(g_userHotkeyMgr->keys.items[0].key1 == hotkey);
}
static void load(unsigned aid)
{ account_user_data_load(g_accountUserDataMgr, aid, "World/..\\이름"); }
static char files[12][MAX_PATH];
static int file_count;
static void remember(void)
{
    int i;
    for (i = 0; i < file_count; ++i)
        if (!strcmp(files[i], g_accountUserDataMgr->path)) return;
    assert(file_count < 12);
    strcpy(files[file_count++], g_accountUserDataMgr->path);
}
static void cleanup(void)
{
    char path[MAX_PATH], *p;
    int i;
    for (i = 0; i < file_count; ++i) {
        assert(strncmp(files[i], g_clientDir, strlen(g_clientDir)) == 0);
        DeleteFileA(files[i]);
        strcpy(path, files[i]); p = strrchr(path, '\\'); assert(p); *p = 0;
        RemoveDirectoryA(path);
    }
    snprintf(path, sizeof path, "%s\\SaveData\\Accounts", g_clientDir); assert(RemoveDirectoryA(path));
    snprintf(path, sizeof path, "%s\\SaveData", g_clientDir); assert(RemoveDirectoryA(path));
    assert(RemoveDirectoryA(g_clientDir));
}
int main(void)
{
    struct ClientLua lua;
    struct OwnedText before = {0}, after = {0};
    struct JsonTree valid, invalid;
    char parent[MAX_PATH], temporary[MAX_PATH], first[MAX_PATH], alternate[MAX_PATH];
    int i;
    assert(GetTempPathA(sizeof parent, parent));
    assert(GetTempFileNameA(parent, "acc", 0, temporary));
    assert(DeleteFileA(temporary) && CreateDirectoryA(temporary, NULL));
    assert(strlen(temporary) < sizeof g_clientDir); strcpy(g_clientDir, temporary);
    lua.L = luaL_newstate(); assert(lua.L); luaL_openlibs(lua.L); g_session1.luaState = &lua;
    script("function GetUserHotKeyInfo(g,i) local t=_G['USERKEY_'..g]; local v=t and t[i]; if v then return v.KEY1,v.KEY2,'',v.EXE end return 0,0,'','' end");
    assert(user_hotkey_mgr_create() && whisper_block_create());
    for (i = 0; i < CHAT_MACRO_COUNT; ++i) assert(session_set_chat_macro(&g_session1, i, text_view_cstr("initial")));
    script("USERKEY_1={[0]={KEY1=10,KEY2=0,EXE='base'}}");
    assert(account_user_data_create() && !account_user_data_create());
    assert(!account_user_data_is_loaded(NULL) && !account_user_data_save(NULL));
    account_user_data_reset(g_accountUserDataMgr);
    load(101); assert(account_user_data_is_loaded(g_accountUserDataMgr)); remember(); strcpy(first, g_accountUserDataMgr->path);
    expect("initial", NULL, 10);
    macro("안녕\\\"line\n"); assert(whisper_block_add(g_whisperBlockMgr, "차단"));
    script("USERKEY_1={[1]={KEY1=99,KEY2=2,EXE='A'}}");
    assert(account_user_data_save(g_accountUserDataMgr));
    assert(settings_file_read(first, &before) == 1);
    assert(strstr(before.data, "\xec\x95\x88\xeb\x85\x95") && !strstr(before.data, "AuthToken"));
    load(202); remember(); expect("initial", NULL, 10);
    macro("account B"); script("USERKEY_1={}"); assert(account_user_data_save(g_accountUserDataMgr));
    load(101); expect("안녕\\\"line\n", "차단", 99);
    macro("not written");
    for (i = 0; i < 3; ++i) {
        fail_write = i == 0; fail_flush = i == 1; fail_move = i == 2;
        assert(!account_user_data_save(g_accountUserDataMgr));
        assert(settings_file_read(first, &after) == 1);
        assert(before.size == after.size && !memcmp(before.data, after.data, before.size));
        owned_text_clear(&after);
    }
    fail_write = fail_flush = fail_move = 0;
    macro("안녕\\\"line\n");
    g_serverAddress = "two.example"; load(101); remember(); expect("initial", NULL, 10);
    assert(strcmp(first, g_accountUserDataMgr->path)); assert(account_user_data_save(g_accountUserDataMgr));
    g_serverAddress = "one.example"; g_serverPort = "6901"; load(101); remember(); expect("initial", NULL, 10);
    assert(strcmp(first, g_accountUserDataMgr->path)); assert(account_user_data_save(g_accountUserDataMgr));
    g_serverPort = "6900"; account_user_data_load(g_accountUserDataMgr, 101, "different world"); remember(); expect("initial", NULL, 10);
    assert(strcmp(first, g_accountUserDataMgr->path)); assert(account_user_data_save(g_accountUserDataMgr));
    load(202); expect("account B", NULL, 0);
    load(101); expect("안녕\\\"line\n", "차단", 99);
    json_tree_init(&valid); json_tree_init(&invalid); assert(account_settings_capture(&valid));
    json_value_copy(&invalid, &valid);
    json_tree_free(json_value_member(&invalid, "UserHotkey_V2"));
    assert(!account_settings_apply(&invalid)); expect("안녕\\\"line\n", "차단", 99);
    json_tree_free(&invalid);
    json_value_copy(&invalid, &valid); json_value_member(&invalid, "version")->u.i64 = 99;
    assert(!account_settings_apply(&invalid)); expect("안녕\\\"line\n", "차단", 99);
    json_tree_free(&invalid);
    json_value_copy(&invalid, &valid);
    json_value_member(&invalid, "EmotionHotkey")->u.items[0].u.text[0] = (char)0xff;
    assert(!settings_json_convert(&invalid, CP_UTF8, g_codePage));
    expect("안녕\\\"line\n", "차단", 99);
    json_tree_free(&invalid); json_tree_free(&valid);
    whisper_block_clear(g_whisperBlockMgr); macro(""); script("USERKEY_1={}");
    assert(account_user_data_save(g_accountUserDataMgr));
    load(202); load(101); expect("", NULL, 0);
    load(202);
    assert(settings_file_write(first, text_view_cstr("{truncated")));
    load(101); assert(!account_user_data_is_loaded(g_accountUserDataMgr)); expect("initial", NULL, 10);
    assert(!account_user_data_save(g_accountUserDataMgr));
    assert(settings_file_read(first, &after) == 1 && !strcmp(after.data, "{truncated"));
    owned_text_clear(&after);
    assert(settings_file_path(alternate, g_clientDir, "one", "23", "4", 1, 0));
    assert(settings_file_path(temporary, g_clientDir, "one2", "3", "4", 1, 0)); assert(strcmp(alternate, temporary));
    assert(!settings_file_path(alternate, g_clientDir, "one", "23", "4", 0, 0));
    assert(!settings_file_write(first, text_view("x", 1024 * 1024 + 1)));
    load(202); expect("account B", NULL, 0);
    account_user_data_load(g_accountUserDataMgr, 0, "world");
    assert(!account_user_data_is_loaded(g_accountUserDataMgr)); expect("initial", NULL, 10);
    assert(!account_user_data_save(g_accountUserDataMgr));
    account_user_data_destroy();
    assert(account_user_data_create()); load(202); expect("account B", NULL, 0);
    account_user_data_destroy(); account_user_data_destroy();
    user_hotkey_mgr_release(); whisper_block_destroy();
    text_array_clear(&g_session1.chatMacros);
    lua_close(lua.L); owned_text_clear(&before);
    cleanup();
    puts("Local account settings: actual Lua/JSON/file round trips, UTF-8 Korean text, account/server/port/world isolation, initial preferences, empty-state persistence, rejected schemas/corruption and write/flush/replace failures passed; no HTTP linked.");
    return 0;
}
