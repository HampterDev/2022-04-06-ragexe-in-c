#include "session/chat_macros.h"
#include "session/state.h"
#include <string.h>

struct TextView session_chat_macro(const struct Session *session, int index)
{
    if (!session || index < 0 || index >= CHAT_MACRO_COUNT) return text_view("", 0);
    return text_array_at(&session->chatMacros, (size_t)index);
}

int session_set_chat_macro(struct Session *session, int index, struct TextView text)
{
    struct OwnedText copy = {0};
    if (!session || index < 0 || index >= CHAT_MACRO_COUNT) return 0;
    if (!owned_text_assign(&copy, text)) return 0;
    if (!text_array_resize(&session->chatMacros, CHAT_MACRO_COUNT)) {
        owned_text_clear(&copy);
        return 0;
    }
    owned_text_clear(&session->chatMacros.items[index]);
    session->chatMacros.items[index] = copy;
    return 1;
}

int session_chat_macros_changed(const struct Session *session)
{
    size_t i;
    if (!session || session->chatMacros.size != CHAT_MACRO_COUNT
        || session->chatMacrosLoaded.size != CHAT_MACRO_COUNT) return 1;
    for (i = 0; i < CHAT_MACRO_COUNT; ++i) {
        struct TextView current = text_array_at(&session->chatMacros, i);
        struct TextView saved = text_array_at(&session->chatMacrosLoaded, i);
        if (current.size != saved.size || memcmp(current.data, saved.data, current.size)) return 1;
    }
    return 0;
}
