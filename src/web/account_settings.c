#include "web/account_data.h"
#include "web/service_manager_state.h"
#include "contents/whisper_block.h"
#include "contents/user_hotkeys.h"
#include "session/chat_macros.h"
#include "json/service_json.h"
#include <string.h>

enum { TAB_SKILLBAR_1 = 0, TAB_INTERFACE = 1, TAB_EMOTION = 2, TAB_SKILLBAR_2 = 3 };

int account_user_data_read_block_list(const struct JsonTree *data)
{
    const struct JsonTree *list = data ? json_tree_find(data, "WhisperBlockList", 16) : NULL;
    struct WhisperBlockMgr next = {0};
    size_t i, count = json_value_size(list);
    if (!count)
        return 1;
    for (i = 0; i < whisper_block_count(g_whisperBlockMgr); ++i) {
        if (!whisper_block_add(&next, whisper_block_name(g_whisperBlockMgr, i)))
            goto failed;
    }
    next.loaded = g_whisperBlockMgr->loaded;
    next.pending = g_whisperBlockMgr->pending;
    for (i = 0; i < count; ++i) {
        const struct JsonTree *entry = list->type == JSON_TREE_ARRAY ? &list->u.items[i] : NULL;
        if (!whisper_block_add(&next, json_value_text(entry).data))
            goto failed;
    }
    whisper_block_reset(g_whisperBlockMgr);
    *g_whisperBlockMgr = next;
    return 1;
failed:
    whisper_block_reset(&next);
    return 0;
}

int account_user_data_read_chat_macros(struct AccountUserDataMgr *self, const struct JsonTree *data)
{
    (void)self;
    return session_read_chat_macros(&g_session1, data ? json_tree_find(data, "EmotionHotkey", 13) : NULL);
}

void account_user_data_read_hotkeys(struct AccountUserDataMgr *self, struct JsonTree *data)
{
    struct JsonTree hotkey_v2;

    json_value_copy(&hotkey_v2, json_value_member(data, "UserHotkey_V2"));

    if (json_value_empty(&hotkey_v2)) {
        g_userHotkeyMgr->lacks_v2_save = 0;
    } else {
        g_userHotkeyMgr->lacks_v2_save = 1;
        user_hotkey_mgr_clear_saved(g_userHotkeyMgr);
        account_user_data_read_hotkey_tab(self, json_value_member(&hotkey_v2, "SkillBar_1Tab"), TAB_SKILLBAR_1);
        account_user_data_read_hotkey_tab(self, json_value_member(&hotkey_v2, "SkillBar_2Tab"), TAB_SKILLBAR_2);
        account_user_data_read_hotkey_tab(self, json_value_member(&hotkey_v2, "InterfaceTab"), TAB_INTERFACE);
        account_user_data_read_hotkey_tab(self, json_value_member(&hotkey_v2, "EmotionTab"), TAB_EMOTION);
        user_hotkey_mgr_save(g_userHotkeyMgr);
    }
    json_tree_free(&hotkey_v2);
}

void account_user_data_on_loaded(struct AccountUserDataMgr *self, int ok, int aid,
                                            const char *world, const char *response)
{
    struct JsonTree root;

    char parsed;
    int type;

    if (ok && self->aid == 0) {
        json_tree_init(&root);

        parsed = json_parse_text(&root, response, strlen(response)) && json_value_is_object(&root);
        if (parsed) {
            type = json_value_as_int(json_value_member(&root, "Type"));
            if ((type == 1 || type == 2)
                && !owned_text_assign(&self->world, text_view_cstr(world))) {
                json_tree_free(&root);
                return;
            }
            if (type == 1) {
                if (!account_user_data_read_chat_macros(self, json_value_member(&root, "data"))) {
                    json_tree_free(&root);
                    return;
                }
                if (!account_user_data_read_block_list(json_value_member(&root, "data"))) {
                    json_tree_free(&root);
                    return;
                }
                account_user_data_read_hotkeys(self, json_value_member(&root, "data"));
            }
            if (type == 1 || type == 2) {
                g_whisperBlockMgr->loaded = 1;
                g_userHotkeyMgr->settings_read = 1;
                self->aid = aid;
                self->loaded = 1;
            }
        }

        json_tree_free(&root);
    }
}
