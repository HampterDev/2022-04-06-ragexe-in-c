#include "contents/user_hotkeys.h"
#include "contents/client_lua_calls.h"
#include "session/hotkey_bindings.h"
#include "session/state.h"
#include <stdint.h>
#include <stdlib.h>

struct UserHotkeyMgr *g_userHotkeyMgr;

enum { USER_HOTKEY_TABS = 4 };

static void hotkey_list_clear(struct UserHotkeyList *list)
{
    size_t i;
    for (i = 0; i < list->size; ++i)
        owned_text_clear(&list->items[i].key_name);
    free(list->items);
    *list = (struct UserHotkeyList){0};
}

static int hotkey_list_append(struct UserHotkeyList *list, int group, int id,
                              int key1, int key2, const char *name)
{
    struct UserHotkey key = {group, id, key1, key2, {0}};
    if (!owned_text_assign(&key.key_name, text_view_cstr(name)))
        return 0;
    if (list->size == list->capacity) {
        size_t maximum = SIZE_MAX / sizeof *list->items;
        size_t capacity = list->capacity ? list->capacity : 8;
        struct UserHotkey *items;
        capacity = capacity > maximum / 2 ? maximum : capacity * 2;
        if (capacity <= list->size) {
            owned_text_clear(&key.key_name);
            return 0;
        }
        items = realloc(list->items, capacity * sizeof *items);
        if (!items) {
            owned_text_clear(&key.key_name);
            return 0;
        }
        list->items = items;
        list->capacity = capacity;
    }
    list->items[list->size++] = key;
    return 1;
}

void user_hotkey_mgr_init(struct UserHotkeyMgr *self)
{
    *self = (struct UserHotkeyMgr){0};
}

void user_hotkey_mgr_reset(struct UserHotkeyMgr *self)
{
    hotkey_list_clear(&self->keys);
    user_hotkey_mgr_init(self);
}

int user_hotkey_mgr_create(void)
{
    struct UserHotkeyMgr *manager;
    if (g_userHotkeyMgr)
        return 0;
    manager = malloc(sizeof *manager);
    if (!manager)
        return 0;
    user_hotkey_mgr_init(manager);
    g_userHotkeyMgr = manager;
    return 1;
}

void user_hotkey_mgr_release(void)
{
    if (!g_userHotkeyMgr)
        return;
    user_hotkey_mgr_reset(g_userHotkeyMgr);
    free(g_userHotkeyMgr);
    g_userHotkeyMgr = NULL;
}

static int tab_group(int tab)
{
    switch (tab) {
    case 1: return 3;
    case 2: return 1;
    case 3: return 2;
    default: return 0;
    }
}

int user_hotkey_tab_group(struct UserHotkeyMgr *self, int tab)
{
    (void)self;
    return tab_group(tab);
}

static int get_hotkey_info(const char *function, int group, int id, int *key1, int *key2,
                           const char **key_name)
{
    const char *text = NULL;

    return client_lua_call(g_session1.luaState, function, "dd>ddss",
                           group + 1, id, key1, key2, &text, key_name);
}

int user_hotkey_mgr_collect(struct UserHotkeyMgr *self)
{
    struct UserHotkeyList next = {0};
    int tab, group, count, i, id;
    for (tab = 0; tab < USER_HOTKEY_TABS; ++tab) {
        group = tab_group(tab);
        count = session_get_hot_key_list_size(&g_session1, group);
        for (i = 0; i < count; ++i) {
            int key1 = 0, key2 = 0;
            const char *key_name = NULL;
            id = session_get_original_hot_key_index(&g_session1, group, i);
            if (id == -1)
                continue;
            if (get_hotkey_info("GetUserHotKeyInfo", group, id, &key1, &key2, &key_name)
                && key_name && *key_name) {
                if (!hotkey_list_append(&next, group, id, key1, key2, key_name)) {
                    hotkey_list_clear(&next);
                    return 0;
                }
            }
        }
    }
    hotkey_list_clear(&self->keys);
    self->keys = next;
    return 1;
}

void user_hotkey_mgr_change(struct UserHotkeyMgr *self, int group, int id,
                           int key1, int key2, const char *key_name)
{
    (void)self;
    client_lua_call(g_session1.luaState, "ChangeUserHotKey", "ddsdd",
                    group + 1, id, key_name, key1, key2);
}
