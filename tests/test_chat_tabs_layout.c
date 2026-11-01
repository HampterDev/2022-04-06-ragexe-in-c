#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session/state.h"
#include <lauxlib.h>
#include "session/chat_tabs_layout.h"

struct Session g_session1;
static char output[16384];
static size_t output_size;
static char test_directory[MAX_PATH], test_path[MAX_PATH];

static void save_and_read(const struct ChatTabs *main_tabs, const struct ChatTabs *separate_tabs,
                          int editable, const char *path)
{
    HANDLE file;
    DWORD size, read;
    assert(!strcmp(path, "SaveData\\ChatWndInfo_U.lua"));
    assert(chat_tabs_save_layout(main_tabs, separate_tabs, editable, test_path));
    file = CreateFileA(test_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    size = GetFileSize(file, NULL);
    assert(size < sizeof output);
    assert(ReadFile(file, output, size, &read, NULL) && read == size);
    assert(CloseHandle(file));
    output_size = size;
    output[size] = 0;
}

static int field(lua_State *L, const char *name)
{
    int value;
    lua_getfield(L, -1, name);
    assert(lua_isnumber(L, -1));
    value = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    return value;
}

static void check_record(lua_State *L, int number, const char *name, int main_window, int n)
{
    char key[32];
    int i;
    snprintf(key, sizeof key, "ChatSubWnd_%d", number);
    lua_getglobal(L, key);
    assert(lua_istable(L, -1));
    lua_getfield(L, -1, "TabName");
    assert(!strcmp(lua_tostring(L, -1), name));
    lua_pop(L, 1);
    lua_getfield(L, -1, "TabState");
    assert(!strcmp(lua_tostring(L, -1), main_window ? "On" : "Off"));
    lua_pop(L, 1);
    assert(field(L, "XPos") == (main_window ? 0 : -7));
    assert(field(L, "YPos") == (main_window ? 0 : 25));
    assert(field(L, "Width") == (main_window ? 0 : 480));
    assert(field(L, "Height") == (main_window ? 0 : 120));
    for (i = 1; i <= CHAT_TAB_FLAG_COUNT; ++i) {
        snprintf(key, sizeof key, "option%d", i);
        assert(field(L, key) == (i + n) % 3);
    }
    lua_pop(L, 1);
}

static int check_literal_name(const char *label, const char *name, size_t length)
{
    struct ChatTabSettings tab = {0};
    lua_State *L = luaL_newstate();
    int status, number, failed = 0;
    assert(L);
    assert(owned_text_assign(&tab.name, text_view(name, length)));
    assert(chat_tabs_set(&g_session1.chatWndTabs, 0, &tab));
    assert(chat_tabs_set(&g_session1.subChatWnds, 0, &tab));
    chat_tab_settings_clear(&tab);
    output_size = 0;
    save_and_read(&g_session1.chatWndTabs, &g_session1.subChatWnds, 1,
                         "SaveData\\ChatWndInfo_U.lua");
    status = luaL_loadbuffer(L, output, output_size, "literal-layout");
    if (!status) status = lua_pcall(L, 0, 0, 0);
    if (status) {
        fprintf(stderr, "FAIL %s: saved layout is not valid Lua\n", label);
        failed = 1;
    } else {
        for (number = 1; number <= 2; ++number) {
            char key[32];
            size_t actual_length;
            const char *actual;
            snprintf(key, sizeof key, "ChatSubWnd_%d", number);
            lua_getglobal(L, key);
            assert(lua_istable(L, -1));
            lua_getfield(L, -1, "TabName");
            actual = lua_tolstring(L, -1, &actual_length);
            if (!actual || actual_length != length || memcmp(actual, name, length)) {
                fprintf(stderr, "FAIL %s: %s name changed\n", label,
                        number == 1 ? "docked" : "detached");
                failed = 1;
            }
            lua_pop(L, 1);
            lua_getfield(L, -1, "injected");
            assert(lua_isnil(L, -1));
            lua_pop(L, 2);
        }
        lua_getglobal(L, "injected");
        assert(lua_isnil(L, -1));
    }
    lua_close(L);
    chat_tabs_clear(&g_session1.chatWndTabs);
    chat_tabs_clear(&g_session1.subChatWnds);
    return failed;
}

static int check_literal_names(void)
{
    static const char *const names[] = {
        "", "Party", "A\"B", "A'B", "A\\B", "A\\nB", "end\\",
        "A]]B", "[=[brackets]=]", "\nleading", "A\r\nB\n\rC", "A\tB",
        "한글", "100% %s %n", "A]], injected=true, spare=[[B",
        "A\", injected=true, spare=\"B"
    };
    char bytes[256], long_name[4097], label[32];
    unsigned i;
    int failures = 0;
    for (i = 0; i < sizeof names / sizeof *names; ++i) {
        snprintf(label, sizeof label, "literal case %u", i);
        failures += check_literal_name(label, names[i], strlen(names[i]));
    }
    for (i = 0; i < sizeof bytes; ++i) bytes[i] = (char)i;
    failures += check_literal_name("all 256 byte values", bytes, sizeof bytes);
    memset(long_name, 'x', sizeof long_name);
    /* Exercise escaping at output-buffer boundaries and before decimal digits. */
    for (i = 0; i < sizeof long_name; i += 127) long_name[i] = '\\';
    long_name[255] = '\1'; long_name[256] = '2'; long_name[257] = '3';
    failures += check_literal_name("long mixed name", long_name, sizeof long_name);
    if (failures) return 1;
    puts("Chat names: 18 cases in both docked/detached layouts preserve literal bytes through real Lua.");
    return 0;
}

int main(void)
{
    lua_State *L = luaL_newstate();
    struct ChatTabSettings tab = {0};
    const char *names[] = {"한글", "Second", "Window"};
    int keys[] = {20, -3, 8};
    int n, i;
    char temporary[MAX_PATH];
    assert(L);
    assert(GetTempPathA(MAX_PATH, temporary) > 0);
    assert(GetTempFileNameA(temporary, "clt", 0, test_directory));
    assert(DeleteFileA(test_directory));
    assert(CreateDirectoryA(test_directory, NULL));
    assert(snprintf(test_path, sizeof test_path, "%s\\ChatWndInfo_U.lua", test_directory) < MAX_PATH);
    g_session1.canDragWnd = 1;
    tab.x = -7; tab.y = 25; tab.width = 480; tab.height = 120;
    for (n = 0; n < 3; ++n) {
        assert(owned_text_assign(&tab.name, text_view_cstr(names[n])));
        for (i = 0; i < CHAT_TAB_FLAG_COUNT; ++i) tab.shown[i] = (unsigned char)((i + 1 + n) % 3);
        assert(chat_tabs_set(n < 2 ? &g_session1.chatWndTabs : &g_session1.subChatWnds, keys[n], &tab));
    }
    chat_tab_settings_clear(&tab);
    save_and_read(&g_session1.chatWndTabs, &g_session1.subChatWnds,
                          g_session1.canDragWnd == 1, "SaveData\\ChatWndInfo_U.lua");
    assert(strstr(output, "CanDragWnd = 1\r\n\r\n") == output);
    assert(strstr(output, "TabName = \"Window\",\r\n"));
    assert(luaL_loadbuffer(L, output, output_size, "saved-layout") == 0);
    assert(lua_pcall(L, 0, 0, 0) == 0);
    lua_getglobal(L, "CanDragWnd");
    assert(lua_tonumber(L, -1) == 1);
    lua_pop(L, 1);
    check_record(L, 1, "Second", 1, 1);
    check_record(L, 2, "한글", 1, 0);
    check_record(L, 3, "Window", 0, 2);
    lua_getglobal(L, "ChatSubWnd_4");
    assert(lua_isnil(L, -1));
    lua_pop(L, 1);
    assert(lua_gettop(L) == 0);
    lua_close(L);
    chat_tabs_clear(&g_session1.chatWndTabs);
    chat_tabs_clear(&g_session1.subChatWnds);
    {
        char long_name[4097];
        memset(long_name, 'x', sizeof long_name - 1);
        long_name[sizeof long_name - 1] = 0;
        assert(owned_text_assign(&tab.name, text_view_cstr(long_name)));
        assert(chat_tabs_set(&g_session1.chatWndTabs, 0, &tab));
        chat_tab_settings_clear(&tab);
        output_size = 0;
        save_and_read(&g_session1.chatWndTabs, &g_session1.subChatWnds, 0, "SaveData\\ChatWndInfo_U.lua");
        L = luaL_newstate();
        assert(L);
        assert(luaL_loadbuffer(L, output, output_size, "long-layout") == 0);
        assert(lua_pcall(L, 0, 0, 0) == 0);
        lua_getglobal(L, "ChatSubWnd_1");
        lua_getfield(L, -1, "TabName");
        assert(lua_objlen(L, -1) == sizeof long_name - 1);
        assert(!memcmp(lua_tostring(L, -1), long_name, sizeof long_name));
        lua_close(L);
        chat_tabs_clear(&g_session1.chatWndTabs);
        output_size = 0;
        save_and_read(&g_session1.chatWndTabs, &g_session1.subChatWnds, 0, "SaveData\\ChatWndInfo_U.lua");
        assert(!strcmp(output, "CanDragWnd = 0\r\n\r\n"));
    }
    if (check_literal_names()) return 1;
    assert(DeleteFileA(test_path));
    assert(RemoveDirectoryA(test_directory));
    puts("Chat-tab layout: production writer, ordered main/separate tabs, Korean and 4096-byte names, geometry and all options round-trip through Lua");
    return 0;
}
