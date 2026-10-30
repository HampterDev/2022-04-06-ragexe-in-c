#include "ui/controls/listbox.h"
#include "ui/social/entry_queue_windows.h"
#include "ui/social/entry_queue_standby_internal.h"
#include "ui/controls/text_viewer.h"
#include "core/msg_str.h"
#include <stdio.h>
#include <string.h>

#define MSG_ENTRY_STANDBY_QUEUE 0x858
#define MSG_ENTRY_STANDBY_ARG1 0x859
#define MSG_ENTRY_STANDBY_ARG2 0x85a

void entry_queue_standby_status(const struct UIEntryQueueStandByWnd *self,
                                struct EntryQueueStatusView *out)
{
    out->text = string_cstr(&self->queue);
    out->size = self->queue.size;
    out->arg1 = self->arg1;
    out->arg2 = self->arg2;
}

void entry_queue_standby_restore(struct UIEntryQueueStandByWnd *self,
                                 const char *text, size_t size, int arg1, int arg2)
{
    const char *zero = size ? memchr(text, 0, size) : NULL;
    int i;
    if (zero) size = (size_t)(zero - text);
    string_assign(&self->queue, size ? text : "", size);
    self->arg1 = arg1;
    self->arg2 = arg2;
    _snprintf_s(self->lines[0], sizeof self->lines[0], _TRUNCATE,
                msg_str(MSG_ENTRY_STANDBY_QUEUE), string_cstr(&self->queue));
    _snprintf_s(self->lines[1], sizeof self->lines[1], _TRUNCATE, msg_str(MSG_ENTRY_STANDBY_ARG1), arg1);
    _snprintf_s(self->lines[2], sizeof self->lines[2], _TRUNCATE, msg_str(MSG_ENTRY_STANDBY_ARG2), arg2);
    if (!self->text) return;
    ui_list_box_clear((struct UIListBox *)self->text);
    for (i = 0; i < 3; ++i) ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)self->lines[i], 0x26, 0, 0);
}

void ui_entry_queue_stand_by_wnd_set_status(struct UIEntryQueueStandByWnd *self,
                                     const char *queue, int arg1, int arg2)
{
    entry_queue_standby_restore(self, queue, strlen(queue), arg1, arg2);
}
