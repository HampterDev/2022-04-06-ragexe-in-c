#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session/state.h"
#include "session/session_talk.h"
#include "session/talk_options_lua.h"
#include "contents/sound.h"
#include "game/talk_type.h"
#include <lauxlib.h>
#include "owned_storage_test_allocator.h"

struct Session g_session1;
struct SoundSettings g_sound_settings;
int g_frameskip, g_lightmap;
static unsigned parses, freed_arguments;

void string_free(struct String *s)
{
    assert(s->size == 0 && !s->data);
    ++freed_arguments;
}

int session_get_talk_type(struct Session *session, const char *text, int *type, struct String *args)
{
    assert(session == &g_session1 && freed_arguments == 3 * parses && !args[0].data);
    ++parses;
    if (!strncmp(text, "/integer", 8)) *type = 0x67;
    else if (!strcmp(text, "/byte")) *type = 0x6a;
    else if (!strcmp(text, "/icon")) *type = 0xc3;
    else if (!strcmp(text, "/cashshop")) *type = 0xd8;
    else if (!strcmp(text, "/goldpc")) *type = 0xd9;
    else if (!strcmp(text, "/mirror")) *type = TALK_SHOW_NAME;
    else if (!strcmp(text, "/music")) *type = 7;
    else if (!strcmp(text, "/sound")) *type = 11;
    else if (!strcmp(text, "/global")) *type = 0x75;
    else if (!strcmp(text, "/known-unbound")) *type = 4;
    else *type = TALK_UNKNOWN;
    return -1;
}

#define malloc checked_malloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/session/talk_option_storage.c"
#undef free
#undef malloc
#include "../src/game/talk_options.c"
#include "../src/session/talk_options_lua.c"

struct UIWindowMgr g_windowMgr;
struct UIWindow *ui_window_find(int id) { (void)id; assert(0); return NULL; }
char *msg_str(int id) { (void)id; assert(0); return ""; }
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *manager, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{ (void)manager; (void)msg; (void)a; (void)b; (void)c; (void)d; assert(0); return 0; }
intptr_t ui_window_send(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{ (void)window; (void)sender; (void)msg; (void)a; (void)b; (void)c; (void)d; assert(0); return 0; }

static int query(lua_State *L, const char *name)
{
    int result;
    lua_pushcfunction(L, lua_get_cmd_on_off_value); lua_pushstring(L, name);
    assert(lua_pcall(L, 1, 1, 0) == 0);
    result = (int)lua_tonumber(L, -1); lua_pop(L, 1);
    return result;
}
static void set(lua_State *L, const char *name, int value)
{
    lua_pushcfunction(L, lua_set_cmd_on_off_list); lua_pushstring(L, name); lua_pushnumber(L, value);
    assert(lua_pcall(L, 2, 0, 0) == 0 && freed_arguments == 3 * parses);
}

int main(void)
{
    lua_State *L = luaL_newstate();
    int values[] = {0, 1, -1, 256, 257, INT_MIN, INT_MAX};
    unsigned i, calls, size, live;
    struct TalkOption option;
    char long_command[2048];
    assert(L);
    g_session1.loadingScreenRandom = 0xa5; g_session1.connectionIndex = 0x12345678;
    for (i = 0; i < sizeof values / sizeof values[0]; ++i) {
        unsigned before = parses;
        set(L, "/byte", values[i]);
        assert(parses == before + 1 && g_session1.showName == (unsigned char)values[i]);
        assert(query(L, "/byte") == (unsigned char)values[i]);
        set(L, "/integer", values[i]);
        assert(g_session1.noShift == values[i] && query(L, "/integer") == (unsigned char)values[i]);
        assert(g_session1.loadingScreenRandom == 0xa5 && g_session1.connectionIndex == 0x12345678);
    }
    set(L, "/mirror", 345);
    assert(g_session1.map_rule_flags[0] == 345 && g_session1.map_rule_flags[1] == 345 && query(L, "/mirror") == 89);
    set(L, "/music", 0); set(L, "/sound", -3); set(L, "/global", 258);
    assert(!g_sound_settings.bgm_on && g_sound_settings.effects_on == -3 && g_lightmap == 258);
    assert(query(L, "/sound") == 253 && query(L, "/global") == 2);
    size = g_session1.talkOptionSettings.size; calls = allocation_calls;
    assert(!talk_option_set(NULL, 2) && !talk_option_set("", 2));
    set(L, "/unknown", 2); set(L, "/known-unbound", 2); set(L, "/icon", 2);
    set(L, "/cashshop", 2); set(L, "/goldpc", 2);
    assert(query(L, "/icon") == -1 && query(L, "/cashshop") == -1 && query(L, "/goldpc") == -1);
    assert(query(L, "/unknown") == -1 && query(L, "/known-unbound") == -1);
    assert(g_session1.talkOptionSettings.size == size && allocation_calls == calls);
    option = talk_option_find("/known-unbound");
    assert(option.type == 4 && !option.integer && !option.byte);
    live = outstanding; fail_at = allocation_calls + 1;
    assert(!talk_option_set("/mirror", 777)); fail_at = 0;
    assert(g_session1.map_rule_flags[0] == 345 && g_session1.map_rule_flags[1] == 345);
    assert(g_session1.talkOptionSettings.size == size && outstanding == live);
    for (i = 0; i < 2; ++i) {
        unsigned before = parses;
        calls = allocation_calls;
        lua_pushcfunction(L, lua_set_cmd_on_off_list);
        if (i == 0) lua_newtable(L); else lua_pushstring(L, "/integer");
        if (i == 1) lua_newtable(L); else lua_pushnumber(L, 1);
        assert(lua_pcall(L, 2, 0, 0) != 0); lua_pop(L, 1);
        assert(allocation_calls == calls && parses == before && freed_arguments == 3 * parses && outstanding == live);
    }
    memset(long_command, 'x', sizeof long_command); memcpy(long_command, "/integer ", 9); long_command[sizeof long_command - 1] = 0;
    set(L, long_command, 456);
    lua_gc(L, LUA_GCCOLLECT, 0);
    assert(!strcmp(owned_text_cstr(&g_session1.talkOptionSettings.items[size].text), long_command));
    lua_close(L);
    assert(!strcmp(owned_text_cstr(&g_session1.talkOptionSettings.items[size].text), long_command));
    memset(long_command, 0, sizeof long_command);
    assert(g_session1.talkOptionSettings.items[size].text.size == sizeof long_command - 1);
    talk_option_settings_clear(&g_session1.talkOptionSettings);
    assert(!outstanding && freed_arguments == 3 * parses);
    puts("Talk options and real Lua: byte-neighbor isolation, integer low-byte queries, borrowed string lifetime, duplicate order, special settings and failure rollback passed");
    return 0;
}
