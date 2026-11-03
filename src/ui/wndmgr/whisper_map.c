#include "ui/window_access.h"
#include <string.h>
#include "ui/wndmgr/wndmgr.h"
#include "ui/chat/chat.h"

int wm_whisper_name_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

void wm_whisper_wnd_destroy(void *value)
{
    string_free(&((struct WmWhisperWnd *)value)->name);
}

void ui_window_mgr_forget_whisper_wnd(struct UIWindowMgr *self, struct String name)
{
    tree_erase_key(&self->whisper_wnds, &name, wm_whisper_name_less, wm_whisper_wnd_destroy);
    string_free(&name);
}

void ui_window_mgr_notify_whisper_wnd(struct UIWindowMgr *self, const char *name)
{
    struct String key;
    struct WmWhisperWnd *entry;

    string_init(&key);
    string_assign_cstr(&key, name);
    entry = tree_find(&self->whisper_wnds, &key, wm_whisper_name_less);
    string_free(&key);
    if (entry)
        ui_window_send_msg(entry->wnd, NULL, 0x30, 0, 0, 0, 0);
}

void ui_window_mgr_set_chat_line(struct UIWindowMgr *self, const char *text)
{
    string_assign_cstr(&self->chat_line, text);
}
