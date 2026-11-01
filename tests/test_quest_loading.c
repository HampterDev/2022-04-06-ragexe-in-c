#include <assert.h>
#include <stdio.h>
#include "../src/contents/renew_quest_info.c"

struct Session g_session1;
int g_serverType;
static unsigned registered, loaded;
static lua_CFunction callback;

int renew_quest_lua_add_ongoing_description(lua_State *lua) { (void)lua; return 0; }
int renew_quest_lua_add_ongoing_reward_info(lua_State *lua) { (void)lua; return 0; }
void lua_pushcclosure(lua_State *lua, lua_CFunction fn, int count)
{
    assert(lua == g_session1.luaState->L && !count);
    callback = fn;
}
void lua_setfield(lua_State *lua, int index, const char *name)
{
    assert(lua == g_session1.luaState->L && index == LUA_GLOBALSINDEX);
    if (registered == 0) {
        assert(!strcmp(name, "AddOngoingDescription"));
        assert(callback == renew_quest_lua_add_ongoing_description);
    } else {
        assert(registered == 1 && !strcmp(name, "AddOngoingRewardInfo"));
        assert(callback == renew_quest_lua_add_ongoing_reward_info);
    }
    ++registered;
}

char client_lua_load_file(struct ClientLua *lua, const char *path, char packed, char disk)
{
    assert(lua == g_session1.luaState);
    if (loaded == 0) {
        assert(!strcmp(path, "Lua Files\\DataInfo\\QuestInfo_f") && packed == 1 && !disk);
    } else if (loaded == 1) {
        assert(!strcmp(path, "savedata\\OngoingSimpleViewList") && !packed && disk == 1);
    } else {
        assert(loaded == 2 && !packed && disk == 1);
        assert(!strcmp(path, "system\\OngoingQuestInfoList_True"));
    }
    ++loaded;
    return 1;
}

int main(void)
{
    struct ClientLua lua = {0};
    int service;
    lua.L = (lua_State *)1;
    g_session1.luaState = &lua;
    for (service = 0; service < 24; ++service) {
        g_serverType = service;
        registered = loaded = 0;
        renew_quest_load();
        assert(registered == 2 && loaded == 3);
    }
    puts("Quest loading: normal quest table, ongoing callbacks and saved tracking settings across 24 server-type values");
    return 0;
}

struct UIWindowMgr g_windowMgr;
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...) { abort(); }
void string_free(struct String *s) { abort(); }
void string_copy(struct String *dst, const struct String *src) { abort(); }
void string_assign(struct String *s, const char *text, size_t len) { abort(); }
void string_assign_cstr(struct String *s, const char *text) { abort(); }
size_t string_find(const struct String *s, const char *text, size_t pos) { abort(); }
int once_begin(INIT_ONCE *once) { abort(); }
void once_complete(INIT_ONCE *once) { abort(); }
void *tree_find(const struct Tree *tree, const void *key, TreeLessFn less) { abort(); }
void *tree_add(struct Tree *tree, const void *value, size_t size, TreeLessFn less, int *added) { abort(); }
size_t tree_erase_key(struct Tree *tree, const void *key, TreeLessFn less, TreeDestroyFn destroy) { abort(); }
int tree_less_uint(const void *a, const void *b) { abort(); }
void vec_copy(struct Vec *dst, const struct Vec *src, size_t size) { abort(); }
void vec_free(struct Vec *v) { abort(); }
void string_vec_free(struct Vec *v) { abort(); }
void string_vec_copy(struct Vec *dst, const struct Vec *src) { abort(); }
struct RenewQuestB *quest_hunt_construct(struct RenewQuestB *quest) { abort(); }
void quest_hunt_destroy(struct RenewQuestB *quest) { abort(); }
void renew_quest_base_destruct(struct RenewQuestBase *self) { abort(); }
struct RenewQuestBase *renew_quest_base_copy(struct RenewQuestBase *self, const struct RenewQuestBase *src) { abort(); }
void ongoing_quest_entry_destroy(void *entry) { abort(); }
struct String *ui_window_fit_text(struct UIWindow *self, struct String *out, int max_width, const char *text,
                                 int face, int height, int bold) { abort(); }
