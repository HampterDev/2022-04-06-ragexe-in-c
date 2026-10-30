#include "web/account_data.h"
#include "contents/user_hotkeys.h"
#include "json/service_json.h"
#include <assert.h>
#include <stdio.h>

struct UserHotkeyMgr *g_userHotkeyMgr;
static unsigned changes;

void user_hotkey_mgr_change(struct UserHotkeyMgr *self, int group, int id,
                           int key1, int key2, const char *desc)
{
    static const int ids[] = {7, -1, 8, 0, 9};
    static const char *names[] = {"kept", "", "prefix", "true", ""};
    assert(self == g_userHotkeyMgr && group == 2 && changes < 5);
    assert(id == ids[changes] && !strcmp(desc, names[changes]));
    assert(key1 == (changes == 3 ? 12 : -1));
    assert(key2 == (changes == 3 ? 0 : -1));
    ++changes;
}

int main(void)
{
    const char *input = "[{\"desc\":\"skip\"},{\"index\":7,\"desc\":\"kept\"},{},"
        "{\"index\":8,\"desc\":\"prefix\\u0000tail\"},{\"desc\":\"\\u0000hidden\"},"
        "{\"index\":null,\"key1\":12,\"key2\":null,\"desc\":true},"
        "{\"index\":9,\"desc\":42},17]";
    struct JsonTree root;
    json_tree_init(&root);
    assert(json_parse_text(&root, input, strlen(input)));
    account_user_data_read_hotkey_tab(NULL, &root, 2);
    assert(changes == 5);
    json_tree_free(&root);
    puts("hotkey JSON: missing defaults, null fields, booleans, counted-NUL selection and borrowed Lua text passed");
    return 0;
}
