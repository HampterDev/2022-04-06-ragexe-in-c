#include "session/chat_macros.h"
#include "session/state.h"
#include "platform/client_config.h"
#include "contents/client_res.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/msg_str.h"

const char *g_shortcutListSubkey = "\\ShortCutList";

static const int s_shortcutMsg[CHAT_MACRO_COUNT] = { 0x220, 0x221, 0x222, 0x223, 0x225, 0x226, 0x227, 0x228, 0x229, 0x22a };
static const char *const s_shortcutEmotes[CHAT_MACRO_COUNT] = {
    "/!", "/?", "/기쁨", "/하트", "/땀", "/아하", "/짜증", "/화", "/돈", "/..."
};

void session_init_short_cut_texts(struct Session *self)
{
    struct TextArray replacement = {0};
    char path[0x200], value[0x100], name[8];
    HKEY key = NULL;
    DWORD type, size;
    const char *message;
    int i;

    if (!text_array_resize(&replacement, CHAT_MACRO_COUNT)) return;
    for (i = 0; i < CHAT_MACRO_COUNT; ++i)
        if (!owned_text_assign(&replacement.items[i], text_view_cstr(msg_str(s_shortcutMsg[i])))) goto failed;

    snprintf(path, sizeof path, "%s%s\\", g_registryKeyPath, g_shortcutListSubkey);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
        for (i = 0; i < CHAT_MACRO_COUNT; ++i) {
            _itoa(i, name, 10);
            size = sizeof value;
            memset(value, 0, sizeof value);
            if (RegQueryValueExA(key, name, NULL, &type, (BYTE *)value, &size) == ERROR_SUCCESS
                && type == REG_SZ) {
                value[sizeof value - 1] = 0;
                if (!owned_text_assign(&replacement.items[i], text_view_cstr(value))) goto failed;
            }
        }
        RegCloseKey(key);
        key = NULL;
    }
    for (i = 0; i < CHAT_MACRO_COUNT; ++i) {
        struct TextView text = text_array_at(&replacement, i);
        if (!strcmp(text.data, s_shortcutEmotes[i])) {
            message = msg_str(s_shortcutMsg[i]);
            if (!owned_text_assign(&replacement.items[i], text_view_cstr(message))) goto failed;
        }
    }
    text_array_clear(&self->chatMacros);
    self->chatMacros = replacement;
    return;
failed:
    if (key) RegCloseKey(key);
    text_array_clear(&replacement);
}
