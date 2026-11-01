#include "contents/account_settings_internal.h"
#include "contents/user_hotkeys.h"
#include "contents/whisper_block.h"
#include "session/chat_macros.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>

static const char *const tabs[] = {"SkillBar_1Tab", "InterfaceTab", "EmotionTab", "SkillBar_2Tab"};

static void array_init(struct JsonTree *out)
{
    json_tree_init(out);
    out->type = JSON_TREE_ARRAY;
}

static void put_int(struct JsonTree *out, const char *name, int value)
{
    struct JsonTree number;
    json_value_from_int(&number, value);
    json_value_assign(json_value_member(out, name), &number);
}

int account_settings_capture(struct JsonTree *out)
{
    struct JsonTree next, list, entry, text, hotkeys;
    size_t i;
    int group;
    if (!g_userHotkeyMgr || !g_whisperBlockMgr || !user_hotkey_mgr_collect(g_userHotkeyMgr))
        return 0;
    json_tree_init(&next);
    put_int(&next, "version", 1);
    array_init(&list);
    for (i = 0; i < whisper_block_count(g_whisperBlockMgr); ++i) {
        json_value_from_cstr(&text, whisper_block_name(g_whisperBlockMgr, i));
        json_value_append(&list, &text);
        json_tree_free(&text);
    }
    json_value_assign(json_value_member(&next, "WhisperBlockList"), &list);
    json_tree_free(&list);
    session_write_chat_macros(&g_session1, &next);
    json_tree_init(&hotkeys);
    for (group = 0; group < 4; ++group) {
        array_init(&list);
        for (i = 0; i < g_userHotkeyMgr->keys.size; ++i) {
            const struct UserHotkey *key = &g_userHotkeyMgr->keys.items[i];
            if (key->group != group) continue;
            json_tree_init(&entry);
            put_int(&entry, "index", key->id);
            put_int(&entry, "key1", key->key1);
            put_int(&entry, "key2", key->key2);
            json_value_from_text(&text, owned_text_view(&key->key_name));
            json_value_assign(json_value_member(&entry, "desc"), &text);
            json_value_append(&list, &entry);
            json_tree_free(&text);
            json_tree_free(&entry);
        }
        json_value_assign(json_value_member(&hotkeys, tabs[group]), &list);
        json_tree_free(&list);
    }
    json_value_assign(json_value_member(&next, "UserHotkey_V2"), &hotkeys);
    json_tree_free(&hotkeys);
    json_tree_free(out);
    *out = next;
    return 1;
}

static const struct JsonTree *field(const struct JsonTree *data, const char *name)
{
    return json_tree_find(data, name, strlen(name));
}

static int valid_string(const struct JsonTree *value, size_t max)
{
    return value && value->type == JSON_TREE_STRING && value->count <= max &&
        !memchr(value->u.text, 0, value->count);
}

static int integer(const struct JsonTree *data, const char *name, int max, int *out)
{
    const struct JsonTree *v = field(data, name);
    if (!v || (v->type != JSON_TREE_INT64 && v->type != JSON_TREE_UINT64) ||
        v->u.i64 < 0 || v->u.u64 > (unsigned)max)
        return 0;
    *out = (int)v->u.i64;
    return 1;
}

static int is_list(const struct JsonTree *list, size_t max)
{
    return list && list->type == JSON_TREE_ARRAY && list->count <= max;
}

int account_settings_apply(const struct JsonTree *data)
{
    const struct JsonTree *macros = field(data, "EmotionHotkey");
    const struct JsonTree *blocks = field(data, "WhisperBlockList");
    const struct JsonTree *hotkeys = field(data, "UserHotkey_V2"), *list, *entry, *name;
    struct TextArray next_macros = {0};
    struct WhisperBlockMgr next_blocks = {0};
    struct UserHotkeyList keys = {0};
    size_t i, count = 0;
    int group, version, ok = 0;
    if (!data || data->type != JSON_TREE_OBJECT || !integer(data, "version", 1, &version) || version != 1 ||
        !is_list(macros, CHAT_MACRO_COUNT) || macros->count != CHAT_MACRO_COUNT ||
        !is_list(blocks, 10000) || !hotkeys || hotkeys->type != JSON_TREE_OBJECT)
        return 0;
    for (group = 0; group < 4; ++group) {
        list = field(hotkeys, tabs[group]);
        if (!is_list(list, 1000)) return 0;
        count += list->count;
    }
    keys.items = count ? calloc(count, sizeof *keys.items) : NULL;
    if (count && !keys.items) goto done;
    if (!text_array_resize(&next_macros, CHAT_MACRO_COUNT)) goto done;
    for (i = 0; i < CHAT_MACRO_COUNT; ++i) {
        if (!valid_string(&macros->u.items[i], 4096) ||
            !owned_text_assign(&next_macros.items[i], json_value_text(&macros->u.items[i])))
            goto done;
    }
    for (i = 0; i < blocks->count; ++i) {
        if (!valid_string(&blocks->u.items[i], 255) || !blocks->u.items[i].count ||
            !whisper_block_add(&next_blocks, blocks->u.items[i].u.text))
            goto done;
    }
    for (group = 0; group < 4; ++group) {
        list = field(hotkeys, tabs[group]);
        for (i = 0; i < list->count; ++i) {
            struct UserHotkey *key = &keys.items[keys.size++];
            entry = &list->u.items[i];
            name = field(entry, "desc");
            key->group = group;
            if (!integer(entry, "index", 10000, &key->id) ||
                !integer(entry, "key1", 65535, &key->key1) ||
                !integer(entry, "key2", 65535, &key->key2) || !valid_string(name, 4096) ||
                !owned_text_assign(&key->key_name, json_value_text(name)))
                goto done;
        }
    }
    if (!user_hotkey_mgr_apply(&keys)) goto done;
    text_array_clear(&g_session1.chatMacros);
    g_session1.chatMacros = next_macros;
    next_macros = (struct TextArray){0};
    whisper_block_reset(g_whisperBlockMgr);
    *g_whisperBlockMgr = next_blocks;
    next_blocks = (struct WhisperBlockMgr){0};
    ok = 1;
done:
    for (i = 0; i < keys.size; ++i) owned_text_clear(&keys.items[i].key_name);
    free(keys.items);
    text_array_clear(&next_macros);
    whisper_block_reset(&next_blocks);
    return ok;
}
