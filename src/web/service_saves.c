#include "web/account_data.h"
#include "web/char_user_data.h"
#include "web/service_manager_state.h"
#include "web/requests.h"
#include "contents/mcstore_backup.h"
#include "contents/whisper_block.h"
#include "contents/user_hotkeys.h"
#include "skills/skill_info.h"
#include "skills/skill_tree.h"
#include "session/chat_macros.h"
#include "session/skill_levels.h"
#include "session/option_pairs.h"
#include "json/service_json.h"
#include "std/string.h"

enum {
    TAB_SKILLBAR_1 = 0, TAB_INTERFACE = 1, TAB_EMOTION = 2, TAB_SKILLBAR_2 = 3,
    SKILL_KIND_PLAYER = 7, SKILL_KIND_HOMUN = 8
};

static void write_block_list(struct JsonTree *data)
{
    struct JsonTree list, item;
    size_t i;
    json_tree_init(&list);
    for (i = 0; i < whisper_block_count(g_whisperBlockMgr); ++i) {
        json_value_from_cstr(&item, whisper_block_name(g_whisperBlockMgr, i));
        json_value_append(&list, &item);
        json_tree_free(&item);
    }
    json_value_assign(json_value_member(data, "WhisperBlockList"), &list);
    json_tree_free(&list);
}

int account_user_data_write_hotkeys(struct AccountUserDataMgr *self, struct JsonTree *data)
{
    struct JsonTree hotkey_v2, tabs[4], entry, field, old;
    size_t at;
    int i;

    (void)self;

    if (!user_hotkey_mgr_collect(g_userHotkeyMgr))
        return 0;
    json_tree_init(&hotkey_v2);
    for (i = 0; i < 4; i++)
        json_tree_init(&tabs[i]);
    for (at = 0; at < g_userHotkeyMgr->keys.size; ++at) {
        const struct UserHotkey *key = &g_userHotkeyMgr->keys.items[at];

        json_tree_init(&entry);
        json_value_from_int(&field, key->id);
        json_value_assign(json_value_member(&entry, "index"), &field);
        json_tree_free(&field);
        json_value_from_int(&field, key->key1);
        json_value_assign(json_value_member(&entry, "key1"), &field);
        json_tree_free(&field);
        json_value_from_int(&field, key->key2);
        json_value_assign(json_value_member(&entry, "key2"), &field);
        json_tree_free(&field);
        json_value_from_text(&field, owned_text_view(&key->key_name));
        json_value_assign(json_value_member(&entry, "desc"), &field);
        json_tree_free(&field);
        if ((unsigned)key->group <= TAB_SKILLBAR_2)
            json_value_append(&tabs[key->group], &entry);
        json_tree_free(&entry);
    }
    json_value_assign(json_value_member(&hotkey_v2, "SkillBar_1Tab"), &tabs[TAB_SKILLBAR_1]);
    json_value_assign(json_value_member(&hotkey_v2, "SkillBar_2Tab"), &tabs[TAB_SKILLBAR_2]);
    json_value_assign(json_value_member(&hotkey_v2, "InterfaceTab"), &tabs[TAB_INTERFACE]);
    json_value_assign(json_value_member(&hotkey_v2, "EmotionTab"), &tabs[TAB_EMOTION]);
    json_value_assign(json_value_member(data, "UserHotkey_V2"), &hotkey_v2);
    json_tree_init(&old);
    json_value_assign(json_value_member(data, "UserHotkey"), &old);
    json_tree_free(&old);
    json_tree_free(&tabs[TAB_EMOTION]);
    json_tree_free(&tabs[TAB_INTERFACE]);
    json_tree_free(&tabs[TAB_SKILLBAR_2]);
    json_tree_free(&tabs[TAB_SKILLBAR_1]);
    json_tree_free(&hotkey_v2);
    return 1;
}

void account_user_data_save(struct AccountUserDataMgr *self)
{
    struct JsonTree root, data;
    struct OwnedText text = {0};
    int save_hotkeys;
    if (!self->loaded) return;
    json_tree_init(&root);
    json_tree_init(&data);
    if ((__int64)g_whisperBlockMgr->pending > 0)
        write_block_list(&data);
    if (session_chat_macros_changed(&g_session1))
        session_write_chat_macros(&g_session1, &data);
    save_hotkeys = user_hotkey_mgr_has_keys_to_save(g_userHotkeyMgr) == 1;
    if (save_hotkeys && !account_user_data_write_hotkeys(self, &data))
        goto done;
    if (!json_value_is_null(&data)) {
        json_value_assign(json_value_member(&root, "data"), &data);
        if (json_write_request(&root, &text)
            && async_work_mgr_account_data_save(g_asyncWorkMgr, self->aid,
                owned_text_view(&self->world), owned_text_view(&self->save_url),
                text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), owned_text_view(&text))) {
            self->pending_saves++;
            if (save_hotkeys) user_hotkey_mgr_clear_saved(g_userHotkeyMgr);
        }
    }
done:
    owned_text_clear(&text);
    json_tree_free(&data);
    json_tree_free(&root);
}

static void append_level(struct JsonTree *list, const char *id_key, int id, const char *level_key, int level)
{
    struct JsonTree entry, field;

    json_tree_init(&entry);
    json_value_from_int(&field, id);
    json_value_assign(json_value_member(&entry, id_key), &field);
    json_tree_free(&field);
    json_value_from_int(&field, level);
    json_value_assign(json_value_member(&entry, level_key), &field);
    json_tree_free(&field);
    json_value_append(list, &entry);
    json_tree_free(&entry);
}

static unsigned char append_chosen_skill(void *context, const struct SkillInfo *info)
{
    struct JsonTree *list = context;
    int level = session_get_skill_table_value(&g_session1, info->id);
    if (level != 0)
        append_level(list, "SKID", info->id, "SKLv", level);
    return 1;
}

void char_user_data_write_skills(struct CharUserDataMgr *self, struct JsonTree *data)
{
    struct JsonTree list;

    (void)self;

    json_tree_init(&list);
    skill_tree_visit(&g_session1.skills.tree_tabs, append_chosen_skill, &list);
    json_value_assign(json_value_member(data, "UseSkillInfo"), &list);
    json_tree_free(&list);
}

void char_user_data_write_homun_skills(struct CharUserDataMgr *self, struct JsonTree *data)
{
    struct JsonTree list;
    struct ListNode *node;
    int id, level;

    (void)self;

    json_tree_init(&list);
    for (node = g_session1.homunSkills.head->next; node != g_session1.homunSkills.head; node = node->next) {
        id = LIST_VALUE(node, struct SkillInfo)->id;
        level = session_get_homun_skill_use_level(&g_session1, id);
        if (level != 0)
            append_level(&list, "HomunSKID", id, "HomunSKLv", level);
    }
    json_value_assign(json_value_member(data, "HomunSkillInfo"), &list);
    json_tree_free(&list);
}

void char_user_data_save(struct CharUserDataMgr *self)
{
    struct JsonTree root, data;

    struct OwnedText text = {0};

    if (!self->loaded)
        return;
    json_tree_init(&root);
    json_tree_init(&data);
    if (session_get_instant_var(&g_session1, SKILL_KIND_PLAYER))
        char_user_data_write_skills(self, &data);
    if (session_get_instant_var(&g_session1, SKILL_KIND_HOMUN))
        char_user_data_write_homun_skills(self, &data);
    if (json_value_is_null(&data) != 1) {
        json_value_assign(json_value_member(&root, "data"), &data);

        if (json_write_request(&root, &text)
            && async_work_mgr_char_data_save(g_asyncWorkMgr, self->aid, self->gid,
                owned_text_view(&self->world), owned_text_view(&self->save_url),
                text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), owned_text_view(&text)))
            self->pending_saves++;
        owned_text_clear(&text);
    }
    json_tree_free(&data);
    json_tree_free(&root);
}

void mcstore_request_save(struct McStoreBackup *self, int type)
{
    struct JsonTree root, items;
    struct OwnedText text = {0};
    json_tree_init(&root);
    json_tree_init(&items);
    mcstore_write_items(self, &items);
    if (!json_value_is_null(&items)) {
        json_value_assign(json_value_member(&root, "data"), &items);
        if (json_write_request(&root, &text)
            && async_work_mgr_mcstore_save(g_asyncWorkMgr, g_session1.ownAid, g_session1.ownGid,
                text_view_cstr(string_cstr(&g_session1.worldName)), owned_text_view(&self->save_url),
                text_view(g_session1.webAuthToken, sizeof g_session1.webAuthToken), owned_text_view(&text), type)) {
            self->save_type = type;
            self->saves_pending++;
        }
    }
    owned_text_clear(&text);
    json_tree_free(&items);
    json_tree_free(&root);
}
