#include "session/chat_macros.h"
#include "session/state.h"
#include "platform/client_config.h"
#include "platform/settings.h"
#include "contents/client_res.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/msg_str.h"

const char *g_shortcutListSubkey = "\\ShortCutList";

static const int s_shortcutMsg[CHAT_MACRO_COUNT] = { 0x220, 0x221, 0x222, 0x223, 0x225, 0x226, 0x227, 0x228, 0x229, 0x22a };
static const char *const s_shortcutEmotes[CHAT_MACRO_COUNT] = {
    "/!", "/?",
    "/\xea\xb8\xb0\xec\x81\xa8" /* "/기쁨" */,
    "/\xed\x95\x98\xed\x8a\xb8" /* "/하트" */,
    "/\xeb\x95\x80" /* "/땀" */,
    "/\xec\x95\x84\xed\x95\x98" /* "/아하" */,
    "/\xec\xa7\x9c\xec\xa6\x9d" /* "/짜증" */,
    "/\xed\x99\x94" /* "/화" */,
    "/\xeb\x8f\x88" /* "/돈" */,
    "/..."
};

void session_init_short_cut_texts(struct Session *self)
{
    struct TextArray replacement = {0};
    char path[0x200], value[0x100], name[8];
    struct PlatformSettingsKey *key = NULL;
    uint32_t type, size;
    const char *message;
    int i;

    if (!text_array_resize(&replacement, CHAT_MACRO_COUNT)) return;
    for (i = 0; i < CHAT_MACRO_COUNT; ++i)
        if (!owned_text_assign(&replacement.items[i], text_view_cstr(msg_str(s_shortcutMsg[i])))) goto failed;

    snprintf(path, sizeof path, "%s%s\\", g_registryKeyPath, g_shortcutListSubkey);
    if (platform_settings_open(path, PLATFORM_SETTINGS_QUERY, &key)) {
        for (i = 0; i < CHAT_MACRO_COUNT; ++i) {
            snprintf(name, sizeof name, "%d", i);
            size = sizeof value;
            memset(value, 0, sizeof value);
            if (platform_settings_read(key, name, &type, value, &size) && type == PLATFORM_SETTING_STRING) {
                value[sizeof value - 1] = 0;
                if (!owned_text_assign(&replacement.items[i], text_view_cstr(value))) goto failed;
            }
        }
        platform_settings_close(key);
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
    if (key) platform_settings_close(key);
    text_array_clear(&replacement);
}
