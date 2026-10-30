#include "ui/window_access.h"
#include "ui/chat/edit_history.h"
#include "ui/controls/edit.h"
#include "ui/window_mgr.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include "core/object_type_tables.h"

#define EDIT_MSG_UP 0x12
#define EDIT_MSG_DOWN 0x13

#define EDIT_HISTORY_MAX 30

static const char *history_line(const struct TextArray *history, unsigned i)
{
    return text_array_at(history, i).data;
}

static unsigned edit_text_len(struct UIWindow *edit)
{
    return ui_window_mgr_get_focus_edit(&g_windowMgr) == edit ? g_language->input_length
                                                        : ((struct UIEditCtrl *)edit)->text.size;
}

static const char *edit_synced_text(struct UIWindow *edit)
{
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == edit)
        ui_edit_ctrl_sync_from_ime((struct UIEditCtrl *)edit);
    return string_cstr(&((struct UIEditCtrl *)edit)->text);
}

static void history_append_text(struct TextArray *history, struct UIWindow *edit)
{
    if (!text_array_append(history, text_view_cstr(edit_synced_text(edit))))
        abort();
}

struct UIChatEditCtrl *ui_chat_edit_ctrl_construct(struct UIChatEditCtrl *self)
{
    ui_edit_ctrl_construct(&self->edit.base);
    self->edit.base.ops = &ui_chat_edit_ctrl_ops.base;
    text_array_init(&self->history);
    string_init(&self->typed);
    self->index = 0;
    if (!text_array_reserve(&self->history, EDIT_HISTORY_MAX))
        abort();
    self->popup = NULL;
    return self;
}

void ui_chat_edit_ctrl_destruct(struct UIChatEditCtrl *self)
{
    self->edit.base.ops = &ui_chat_edit_ctrl_ops.base;
    if (self->popup)
        ui_window_destroy(UI_WINDOW(self->popup));
    string_free(&self->typed);
    text_array_clear(&self->history);
    ui_edit_ctrl_destruct(&self->edit);
}

void ui_chat_edit_ctrl_destroy(struct UIWindow *window)
{
    struct UIChatEditCtrl *self = (struct UIChatEditCtrl *)window;
    ui_chat_edit_ctrl_destruct(self);
    mem_free(self);
}

static void chat_show_line(struct UIChatEditCtrl *self)
{
    const char *text = self->index == self->history.size
                           ? string_cstr(&self->typed)
                           : history_line(&self->history, self->index);

    ui_edit_ctrl_set_text_default(&self->edit, text);
    ui_edit_ctrl_on_focus(&self->edit.base);
}

int ui_chat_edit_ctrl_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UIChatEditCtrl *self = (struct UIChatEditCtrl *)window;
    unsigned count;

    switch (msg) {
    case EDIT_MSG_UP:
        count = self->history.size;
        if (!count)
            break;
        if (self->index == count) {
            const char *typed = ui_edit_ctrl_get_text(&self->edit.base);

            string_assign_cstr(&self->typed, typed);
        }
        if (self->index)
            self->index--;
        ui_edit_ctrl_set_text_default(&self->edit, history_line(&self->history, self->index));
        ui_edit_ctrl_on_focus(&self->edit.base);
        break;
    case EDIT_MSG_DOWN:
        count = self->history.size;
        if (!count || self->index >= count)
            break;
        self->index++;
        chat_show_line(self);
        break;
    default:
        return ui_edit_ctrl_send_msg((struct UIWindow *)(&self->edit), sender, msg, val1, val2, val3, val4);
    }
    return 0;
}

void ui_chat_edit_ctrl_commit(struct UIChatEditCtrl *self)
{
    struct UIWindow *wnd = &self->edit.base;
    unsigned count;

    if (!edit_text_len(wnd))
        return;
    count = self->history.size;
    if (self->index == count) {
        if (count >= EDIT_HISTORY_MAX) {
            self->index = count - 1;
            text_array_erase(&self->history, 0);
        }
        history_append_text(&self->history, wnd);
    } else {
        const char *text = edit_synced_text(wnd);

        if (!owned_text_assign(&self->history.items[self->index], text_view_cstr(text)))
            abort();
    }
    self->index++;
    ui_edit_ctrl_set_text_default(&self->edit, self->index == self->history.size
                                ? ""
                                : history_line(&self->history, self->index));
}

struct UIWhisperEditCtrl *ui_whisper_edit_ctrl_construct(struct UIWhisperEditCtrl *self)
{
    ui_edit_ctrl_construct(&self->edit.base);
    self->edit.base.ops = &ui_whisper_edit_ctrl_ops.base;
    text_array_init(&self->history);
    self->index = 0;
    return self;
}

void ui_whisper_edit_ctrl_destruct(struct UIWhisperEditCtrl *self)
{
    text_array_clear(&self->history);
    ui_edit_ctrl_destruct(&self->edit);
}

void ui_whisper_edit_ctrl_destroy(struct UIWindow *window)
{
    struct UIWhisperEditCtrl *self = (struct UIWhisperEditCtrl *)window;
    ui_whisper_edit_ctrl_destruct(self);
    mem_free(self);
}

void ui_whisper_edit_ctrl_on_focus(struct UIWindow *window)
{
    struct UIWhisperEditCtrl *self = (struct UIWhisperEditCtrl *)window;
    ui_edit_ctrl_on_focus((struct UIWindow *)(&self->edit));
    ui_edit_ctrl_select_all(&self->edit.base);
}

static int whisper_text_is(struct UIWhisperEditCtrl *self, unsigned i)
{
    const char *text = edit_synced_text(&self->edit.base);
    struct TextView line = text_array_at(&self->history, i);

    return line.size == strlen(text) && !memcmp(line.data, text, line.size);
}

static void whisper_remember(struct UIWhisperEditCtrl *self)
{
    struct UIWindow *wnd = &self->edit.base;
    unsigned len, count, i;

    len = edit_text_len(wnd);
    if (!len) {
        self->index = self->history.size;
        return;
    }
    for (i = 0; i < self->history.size; i++)
        if (whisper_text_is(self, i))
            return;

    count = self->history.size;
    if (self->index == count) {
        if (count >= EDIT_HISTORY_MAX) {
            self->index = count - 1;
            text_array_erase(&self->history, 0);
        }
        history_append_text(&self->history, wnd);
    } else {
        for (i = 0; i < count; i++)
            if (whisper_text_is(self, i))
                break;
        if (i == count) {
            history_append_text(&self->history, wnd);
            i = self->history.size - 1;
        }
        self->index = i;
    }
    ui_whisper_edit_ctrl_set_text(&self->edit, self->index == self->history.size
                                ? ""
                                : history_line(&self->history, self->index));
}

void ui_whisper_edit_ctrl_on_focus_lost(struct UIWindow *window)
{
    struct UIWhisperEditCtrl *self = (struct UIWhisperEditCtrl *)window;
    whisper_remember(self);
    ui_edit_ctrl_on_focus_lost((struct UIWindow *)(&self->edit));
}

static void whisper_show_line(struct UIWhisperEditCtrl *self)
{
    const char *text = self->index == self->history.size
                           ? ""
                           : history_line(&self->history, self->index);

    ui_whisper_edit_ctrl_set_text(&self->edit, text);
    ui_whisper_edit_ctrl_on_focus(&self->edit.base);
}

int ui_whisper_edit_ctrl_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIWhisperEditCtrl *self = (struct UIWhisperEditCtrl *)window;
    unsigned count;

    switch (msg) {
    case EDIT_MSG_UP:
        if (!self->history.size)
            break;
        if (self->index)
            self->index--;
        ui_whisper_edit_ctrl_set_text(&self->edit, history_line(&self->history, self->index));
        ui_whisper_edit_ctrl_on_focus(&self->edit.base);
        break;
    case EDIT_MSG_DOWN:
        count = self->history.size;
        if (!count || self->index >= count)
            break;
        self->index++;
        whisper_show_line(self);
        break;
    default:
        return ui_edit_ctrl_send_msg((struct UIWindow *)(&self->edit), sender, msg, val1, val2, val3, val4);
    }
    return 0;
}

void ui_whisper_edit_ctrl_set_text(struct UIEditCtrl *base_edit, const char *text)
{
    struct UIWhisperEditCtrl *self = (struct UIWhisperEditCtrl *)base_edit;
    ui_edit_ctrl_set_text_default(&self->edit, text);
    if (!strlen(text))
        self->index = self->history.size;
}
