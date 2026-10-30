#include "json/service_json.h"
#include "web/account_data.h"
#include "web/char_user_data.h"
#include "web/requests.h"
#include "web/service_manager_state.h"
#include "contents/mcstore_backup.h"
#include "contents/user_hotkeys.h"
#include "contents/whisper_block.h"
#include "skills/skill_info.h"
#include "skills/skill_tree.h"
#include "session/chat_macros.h"
#include "session/skill_levels.h"
#include "session/option_pairs.h"
#include "std/std.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
struct AsyncWorkMgr *g_asyncWorkMgr;
struct UserHotkeyMgr *g_userHotkeyMgr;
static int serialize_ok, enqueue_ok, serialized, queued, cleared, kind;
static int collect_ok = 1;
static int allocations;

void *mem_alloc(size_t size)
{
    void *pointer = malloc(size ? size : 1);
    assert(pointer); ++allocations; return pointer;
}
void mem_free(void *pointer)
{
    if (pointer) { assert(allocations > 0); --allocations; free(pointer); }
}
void fatal_error(const char *text) { (void)text; assert(0); abort(); }

int test_json_write_request(const struct JsonTree *root, struct OwnedText *out)
{
    ++serialized;
    assert(!cleared && !out->data && !out->size);
    return serialize_ok ? json_write_request(root, out) : 0;
}

static void expect_text(struct TextView actual, const char *expected)
{
    assert(actual.size == strlen(expected) && !memcmp(actual.data, expected, actual.size));
}

static int submit(struct TextView world, struct TextView url, struct TextView token,
                  struct TextView text)
{
    struct JsonTree root;
    const struct JsonTree *data, *list, *value;
    ++queued;
    assert(!cleared && token.size == 16);
    assert(!memcmp(token.data, "0123456789abcdef", 16));
    expect_text(world, "world");
    expect_text(url, "url");
    assert(text.size && !text.data[text.size]);
    json_tree_init(&root);
    assert(json_parse_text(&root, text.data, text.size));
    data = json_tree_find(&root, "data", 4); assert(data);
    if (kind == 0) {
        list = json_tree_find(data, "WhisperBlockList", 16);
        assert(list && list->count == 2);
        assert(!strcmp(list->u.items[0].u.text, "Alice"));
        assert(!strcmp(list->u.items[1].u.text, "Zoe"));
        value = json_tree_find(data, "UserHotkey_V2", 13); assert(value);
        list = json_tree_find(value, "InterfaceTab", 12); assert(list && list->count == 1);
        value = json_tree_find(&list->u.items[0], "index", 5);
        assert(value && json_value_as_int(value) == 7);
        value = json_tree_find(&list->u.items[0], "key1", 4);
        assert(value && json_value_as_int(value) == 11);
        value = json_tree_find(&list->u.items[0], "key2", 4);
        assert(value && json_value_as_int(value) == 22);
        value = json_tree_find(&list->u.items[0], "desc", 4);
        assert(value && value->count == 7 && !memcmp(value->u.text, "skill\0x", 7));
    } else if (kind == 1) {
        list = json_tree_find(data, "UseSkillInfo", 12); assert(list && list->count == 1);
        assert(json_value_as_int(json_tree_find(&list->u.items[0], "SKID", 4)) == 42);
        assert(json_value_as_int(json_tree_find(&list->u.items[0], "SKLv", 4)) == 7);
    } else {
        value = json_tree_find(data, "Title", 5);
        assert(value && !strcmp(value->u.text, "shop"));
    }
    json_tree_free(&root);
    return enqueue_ok;
}

int async_work_mgr_account_data_save(struct AsyncWorkMgr *manager, int aid,
    struct TextView world, struct TextView url, struct TextView token, struct TextView text)
{
    assert(manager == g_asyncWorkMgr && aid == 101 && kind == 0);
    return submit(world, url, token, text);
}
int async_work_mgr_char_data_save(struct AsyncWorkMgr *manager, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, struct TextView text)
{
    assert(manager == g_asyncWorkMgr && aid == 101 && gid == 202 && kind == 1);
    return submit(world, url, token, text);
}
int async_work_mgr_mcstore_save(struct AsyncWorkMgr *manager, int aid, int gid,
    struct TextView world, struct TextView url, struct TextView token, struct TextView text, int type)
{
    assert(manager == g_asyncWorkMgr && aid == 101 && gid == 202 && kind == 2 && type == 1);
    assert(g_session1.mcStoreBackup.save_type == 9);
    return submit(world, url, token, text);
}

int session_chat_macros_changed(const struct Session *session)
{
    assert(session == &g_session1); return 0;
}
void session_write_chat_macros(const struct Session *session, struct JsonTree *data)
{
    (void)session; (void)data; assert(0);
}
char user_hotkey_mgr_has_keys_to_save(struct UserHotkeyMgr *manager)
{
    assert(manager == g_userHotkeyMgr); return 1;
}
int user_hotkey_mgr_collect(struct UserHotkeyMgr *manager) { assert(manager == g_userHotkeyMgr); return collect_ok; }
void user_hotkey_mgr_clear_saved(struct UserHotkeyMgr *manager)
{
    assert(manager == g_userHotkeyMgr && enqueue_ok && queued == 1 && kind == 0);
    ++cleared;
}
int session_get_instant_var(struct Session *session, int type)
{
    assert(session == &g_session1); return type == 7;
}
int session_get_skill_table_value(struct Session *session, int id)
{
    assert(session == &g_session1 && id == 42); return 7;
}
int session_get_homun_skill_use_level(struct Session *session, int id)
{
    (void)session; (void)id; assert(0); return 0;
}
void skill_tree_visit(const struct Tree *tree, SkillInfoVisitor visit, void *context)
{
    struct SkillInfo skill;
    assert(tree == &g_session1.skills.tree_tabs);
    memset(&skill, 0, sizeof skill); skill.id = 42;
    assert(visit(context, &skill));
}
void mcstore_write_items(struct McStoreBackup *shop, struct JsonTree *out)
{
    struct JsonTree title;
    assert(shop == &g_session1.mcStoreBackup);
    json_value_from_cstr(&title, "shop");
    json_value_assign(json_value_member(out, "Title"), &title);
    json_tree_free(&title);
}

int main(void)
{
    struct AccountUserDataMgr account = {0};
    struct CharUserDataMgr character = {0};
    struct UserHotkeyMgr hotkeys = {0};
    struct WhisperBlockMgr blocks = {0};
    struct UserHotkey key = {0};
    int scenario, baseline;
    static int manager_token;
    g_asyncWorkMgr = (struct AsyncWorkMgr *)&manager_token;
    key.group = 1; key.id = 7; key.key1 = 11; key.key2 = 22;
    assert(owned_text_assign(&key.key_name, text_view("skill\0x", 7)));
    hotkeys.keys.items = &key; hotkeys.keys.size = hotkeys.keys.capacity = 1;
    g_userHotkeyMgr = &hotkeys; g_whisperBlockMgr = &blocks;
    assert(whisper_block_add(&blocks, "Zoe"));
    assert(whisper_block_add(&blocks, "Alice"));
    blocks.loaded = 1; blocks.pending = 7;
    account.aid = character.aid = g_session1.ownAid = 101;
    character.gid = g_session1.ownGid = 202;
    memcpy(&g_session1.webAuthToken, "0123456789abcdef", 16);
    assert(owned_text_assign(&account.world, text_view_cstr("world")));
    assert(owned_text_assign(&account.save_url, text_view_cstr("url")));
    assert(owned_text_assign(&character.world, text_view_cstr("world")));
    assert(owned_text_assign(&character.save_url, text_view_cstr("url")));
    string_init(&g_session1.worldName);
    string_assign_cstr(&g_session1.worldName, "world");
    owned_text_init(&g_session1.mcStoreBackup.save_url);
    assert(owned_text_assign(&g_session1.mcStoreBackup.save_url, text_view("url", 3)));
    baseline = allocations;
    account_user_data_save(&account); char_user_data_save(&character);
    assert(!serialized && !queued && !cleared);
    account.loaded = character.loaded = 1;
    collect_ok = 0; account.pending_saves = 11;
    account_user_data_save(&account);
    assert(!serialized && !queued && !cleared && account.pending_saves == 11);
    assert(hotkeys.keys.items == &key && hotkeys.keys.size == 1);
    assert(key.key_name.size == 7 && !memcmp(key.key_name.data, "skill\0x", 7));
    collect_ok = 1;
    for (kind = 0; kind < 3; ++kind) {
        for (scenario = 0; scenario < 3; ++scenario) {
            serialize_ok = scenario != 0; enqueue_ok = scenario == 2;
            serialized = queued = cleared = 0;
            account.pending_saves = character.pending_saves = g_session1.mcStoreBackup.saves_pending = 11;
            g_session1.mcStoreBackup.save_type = 9;
            if (kind == 0) account_user_data_save(&account);
            else if (kind == 1) char_user_data_save(&character);
            else mcstore_request_save(&g_session1.mcStoreBackup, 1);
            assert(serialized == 1 && queued == serialize_ok);
            assert(cleared == (kind == 0 && enqueue_ok));
            assert(account.pending_saves == 11 + (kind == 0 && enqueue_ok));
            assert(character.pending_saves == 11 + (kind == 1 && enqueue_ok));
            assert(g_session1.mcStoreBackup.saves_pending == 11 + (kind == 2 && enqueue_ok));
            assert(g_session1.mcStoreBackup.save_type == (kind == 2 && enqueue_ok ? 1 : 9));
            assert(allocations == baseline);
            assert(blocks.pending == 7 && whisper_block_count(&blocks) == 2);
        }
    }
    whisper_block_reset(&blocks);
    owned_text_clear(&account.world); owned_text_clear(&account.save_url);
    owned_text_clear(&character.world); owned_text_clear(&character.save_url);
    owned_text_clear(&key.key_name);
    string_free(&g_session1.worldName);
    owned_text_clear(&g_session1.mcStoreBackup.save_url);
    assert(!allocations);
    puts("Service saves: real account, character and merchant save paths preserve counters, saved hotkeys and save type on collection/serialization/queue rejection; accepted payloads and cleanup passed");
    return 0;
}
