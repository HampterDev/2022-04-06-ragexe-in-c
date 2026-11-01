#include "session/chat_macros.h"
#include "session/state.h"
#include "json/service_json.h"

void session_write_chat_macros(const struct Session *session, struct JsonTree *data)
{
    struct JsonTree list, item;
    int i;
    json_tree_init(&list);
    for (i = 0; i < CHAT_MACRO_COUNT; ++i) {
        json_value_from_text(&item, session_chat_macro(session, i));
        json_value_append(&list, &item);
        json_tree_free(&item);
    }
    json_value_assign(json_value_member(data, "EmotionHotkey"), &list);
    json_tree_free(&list);
}
