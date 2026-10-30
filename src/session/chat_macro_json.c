#include "session/chat_macros.h"
#include "session/state.h"
#include "json/service_json.h"

int session_read_chat_macros(struct Session *session, const struct JsonTree *list)
{
    struct TextArray current = {0}, saved = {0};
    size_t i, count = 0;
    if (!session) return 0;
    if (list && list->type != JSON_TREE_NULL) {
        if (list->type != JSON_TREE_ARRAY) return 0;
        count = list->count < CHAT_MACRO_COUNT ? list->count : CHAT_MACRO_COUNT;
    }
    if (!text_array_resize(&current, CHAT_MACRO_COUNT)
        || !text_array_resize(&saved, CHAT_MACRO_COUNT)) goto failed;
    for (i = 0; i < CHAT_MACRO_COUNT; ++i) {
        struct TextView value = i < count ? json_value_text(&list->u.items[i])
            : session_chat_macro(session, (int)i);
        if (!owned_text_assign(&current.items[i], value)) goto failed;
        if (i < count && !owned_text_assign(&saved.items[i], value)) goto failed;
    }
    text_array_clear(&session->chatMacros);
    text_array_clear(&session->chatMacrosLoaded);
    session->chatMacros = current;
    session->chatMacrosLoaded = saved;
    return 1;
failed:
    text_array_clear(&current);
    text_array_clear(&saved);
    return 0;
}

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
