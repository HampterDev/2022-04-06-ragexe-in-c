#include "contents/user_hotkeys.h"
#include "contents/client_lua_calls.h"
#include "session/hotkey_bindings.h"
#include "session/state.h"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct { void *pointer; size_t size; } blocks[256];
static int attempts, fail_at, live;

static int find_block(void *pointer)
{
    int i;
    for (i = 0; i < 256; ++i) if (blocks[i].pointer == pointer) return i;
    assert(0); return 0;
}
static void *checked_malloc(size_t size)
{
    int slot;
    unsigned char *pointer;
    if (++attempts == fail_at) return NULL;
    assert(size <= SIZE_MAX - 32);
    slot = find_block(NULL); pointer = malloc(size + 32); assert(pointer);
    memset(pointer + size, 0xa7, 32);
    blocks[slot].pointer = pointer; blocks[slot].size = size; ++live;
    return pointer;
}
static void checked_free(void *pointer)
{
    size_t i;
    int slot;
    if (!pointer) return;
    slot = find_block(pointer);
    for (i = 0; i < 32; ++i) assert(((unsigned char *)pointer)[blocks[slot].size + i] == 0xa7);
    blocks[slot].pointer = NULL; --live; free(pointer);
}
static void *checked_realloc(void *pointer, size_t size)
{
    void *next = checked_malloc(size);
    if (next && pointer) {
        size_t old = blocks[find_block(pointer)].size;
        memcpy(next, pointer, old < size ? old : size);
        checked_free(pointer);
    }
    return next;
}

#define malloc checked_malloc
#define realloc checked_realloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/contents/user_hotkeys.c"
#undef malloc
#undef realloc
#undef free

struct Session g_session1;
static int generation = 1, empty_lists, nil_names;
static int groups[4], group_calls, lua_calls, actions;
static char borrowed_name[80];

static void reset_calls(void)
{
    group_calls = lua_calls = 0;
}

int session_get_hot_key_list_size(struct Session *session, int group)
{
    assert(session == &g_session1 && group_calls < 4);
    groups[group_calls++] = group;
    return empty_lists ? 0 : 10;
}
int session_get_original_hot_key_index(struct Session *session, int group, int index)
{
    assert(session == &g_session1 && group >= 0 && group < 4 && index >= 0 && index < 10);
    return index == 1 ? -1 : group * 100 + index;
}

static void expected_name(char *out, int id, int version)
{
    if (!id) strcpy(out, "이름");
    else sprintf(out, "binding-%d-generation-%d", id, version);
}

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list args;
    int group, id, *key1, *key2;
    const char **unused, **name;
    assert(lua == g_session1.luaState);
    va_start(args, format);
    group = va_arg(args, int); id = va_arg(args, int);
    assert(group >= 1 && group <= 4);
    if (!strcmp(format, "ddsdd")) {
        const char *text = va_arg(args, const char *);
        int first = va_arg(args, int), second = va_arg(args, int);
        assert(!strcmp(function, "ChangeUserHotKey"));
        assert(group == 4 && id == 17 && first == 3 && second == 4 && !strcmp(text, "binding"));
        ++actions; va_end(args); return 1;
    }
    assert(!strcmp(format, "dd>ddss") && group == id / 100 + 1);
    key1 = va_arg(args, int *); key2 = va_arg(args, int *);
    unused = va_arg(args, const char **); name = va_arg(args, const char **);
    va_end(args); ++lua_calls;
    assert(!strcmp(function, "GetUserHotKeyInfo"));
    assert(!*key1 && !*key2 && !*unused && !*name);
    if (id % 100 == 2) return 0;
    *key1 = id * 2; *key2 = id * 2 + 1; *unused = "unused";
    if (nil_names || id % 100 == 4) { *name = NULL; return 1; }
    expected_name(borrowed_name, id, generation);
    if (id % 100 == 3) borrowed_name[0] = '\0';
    *name = borrowed_name;
    return 1;
}

static void expect_snapshot(const struct UserHotkeyMgr *manager, int version)
{
    static const int order[] = {0, 3, 1, 2};
    size_t at = 0;
    int tab, i;
    char name[80];
    assert(manager->keys.size == 24);
    for (tab = 0; tab < 4; ++tab) for (i = 0; i < 10; ++i) {
        const struct UserHotkey *key;
        int id;
        if (i >= 1 && i <= 4) continue;
        key = &manager->keys.items[at++]; id = order[tab] * 100 + i;
        assert(key->group == order[tab] && key->id == id && key->key1 == id * 2 && key->key2 == id * 2 + 1);
        expected_name(name, id, version);
        assert(!strcmp(owned_text_cstr(&key->key_name), name));
    }
}

static void test_collect(void)
{
    struct UserHotkeyMgr manager = {0};
    struct UserHotkey *previous;
    int failure, baseline, collected = 0;
    reset_calls(); assert(user_hotkey_mgr_collect(&manager));
    assert(group_calls == 4 && groups[0] == 0 && groups[1] == 3 && groups[2] == 1 && groups[3] == 2);
    assert(lua_calls == 36);
    memset(borrowed_name, 'x', sizeof borrowed_name);
    expect_snapshot(&manager, 1);
    previous = manager.keys.items; baseline = live;
    generation = 2;
    for (failure = 1; failure < 40; ++failure) {
        reset_calls(); attempts = 0; fail_at = failure;
        collected = user_hotkey_mgr_collect(&manager);
        if (collected) break;
        assert(manager.keys.items == previous && live == baseline);
        expect_snapshot(&manager, 1);
    }
    fail_at = 0; assert(collected && failure == 27);
    expect_snapshot(&manager, 2);
    assert(live == baseline);
    empty_lists = 1; reset_calls(); assert(user_hotkey_mgr_collect(&manager));
    assert(!manager.keys.size && !manager.keys.items && !live);
    empty_lists = 0; nil_names = 1; reset_calls(); assert(user_hotkey_mgr_collect(&manager));
    assert(!manager.keys.size && !live);
    nil_names = 0;
    user_hotkey_mgr_reset(&manager); user_hotkey_mgr_reset(&manager);
}

static void test_lifetime(void)
{
    attempts = 0; fail_at = 1;
    assert(!user_hotkey_mgr_create() && !g_userHotkeyMgr && !live);
    fail_at = 0;
    assert(user_hotkey_mgr_create() && !user_hotkey_mgr_create());
    reset_calls(); assert(user_hotkey_mgr_collect(g_userHotkeyMgr));
    user_hotkey_mgr_release(); user_hotkey_mgr_release();
    assert(!g_userHotkeyMgr && !live);
}

int main(void)
{
    test_collect(); test_lifetime();
    user_hotkey_mgr_change(NULL, 3, 17, 3, 4, "binding");
    assert(actions == 1);
    puts("User hotkeys: Lua order, owned Korean names, all 26 allocation failures, nil outputs, lifetime passed");
    return 0;
}
