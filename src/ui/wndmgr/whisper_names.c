#include "ui/chat/chat_window.h"
#include <stdlib.h>
#include <string.h>

static unsigned whisper_name_index(const struct UIWhisperEditCtrl *box, const char *name)
{
    size_t size = strlen(name);
    unsigned i;

    for (i = 0; i < box->history.size; ++i) {
        struct TextView line = text_array_at(&box->history, i);
        if (line.size == size && !memcmp(line.data, name, size))
            break;
    }
    return i;
}

static void whisper_name_append(struct UIWhisperEditCtrl *box, const char *name)
{
    if (!text_array_append(&box->history, text_view_cstr(name)))
        abort();
}

void ui_window_mgr_set_whisper_name(struct UIWindowMgr *self, const char *name)
{
    struct UIWhisperEditCtrl *box;
    unsigned i;

    if (!name || !self->new_chat_wnd)
        return;
    box = ((struct UINewChatWnd *)self->new_chat_wnd)->whisper;
    i = whisper_name_index(box, name);
    if (i == box->history.size)
        whisper_name_append(box, name);
    ((struct UINewChatWnd *)self->new_chat_wnd)->whisper->index = i;
    ui_window_send_msg(self->new_chat_wnd, &((struct UINewChatWnd *)self->new_chat_wnd)->whisper->edit.base, COMBO_MSG_CHOSEN, i, 0,
             0, 0);
}

void ui_window_mgr_add_whisper_name(struct UIWindowMgr *self, const char *name)
{
    struct UIWhisperEditCtrl *box;
    unsigned count;

    if (!self->new_chat_wnd)
        return;
    box = ((struct UINewChatWnd *)self->new_chat_wnd)->whisper;
    if (whisper_name_index(box, name) != box->history.size)
        return;
    whisper_name_append(box, name);
    count = box->history.size;
    box = ((struct UINewChatWnd *)self->new_chat_wnd)->whisper;
    if (box->index >= count - 1)
        box->index = count;
}
