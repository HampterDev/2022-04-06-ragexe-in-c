#include "web/account_data.h"
#include "contents/user_hotkeys.h"
#include "json/service_json.h"

static int hotkey_integer(const struct JsonTree *entry, const char *key)
{
    const struct JsonTree *value = json_tree_find(entry, key, strlen(key));
    return value ? json_value_as_int(value) : -1;
}

void account_user_data_read_hotkey_tab(struct AccountUserDataMgr *self, const struct JsonTree *tab,
                                      int group)
{
    size_t i;
    (void)self;
    for (i = 0; i < json_value_size(tab); ++i) {
        const struct JsonTree *entry = tab->type == JSON_TREE_ARRAY ? &tab->u.items[i] : &tab->u.members[i].value;
        if (json_value_is_object(entry)) {
            int index = hotkey_integer(entry, "index");
            int key1 = hotkey_integer(entry, "key1");
            int key2 = hotkey_integer(entry, "key2");
            struct TextView description = json_value_text(json_tree_find(entry, "desc", 4));
            if (index != -1 || description.size == 0)
                user_hotkey_mgr_change(g_userHotkeyMgr, group, index, key1, key2, description.data);
        }
    }
}
