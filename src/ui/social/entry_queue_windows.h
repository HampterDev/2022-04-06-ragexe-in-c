#ifndef UI_SOCIAL_ENTRY_QUEUE_WINDOWS_H
#define UI_SOCIAL_ENTRY_QUEUE_WINDOWS_H
#include <stddef.h>
struct UIEntryQueueWnd;
struct UIEntryQueueStandByWnd;
struct EntryQueueStatusView {
    const char *text;
    size_t size;
    int arg1, arg2;
};
int ui_entry_queue_wnd_get_selected(struct UIEntryQueueWnd *self);
void entry_queue_standby_status(const struct UIEntryQueueStandByWnd *self, struct EntryQueueStatusView *out);
void entry_queue_standby_restore(struct UIEntryQueueStandByWnd *self, const char *text, size_t size, int arg1, int arg2);
void ui_entry_queue_stand_by_wnd_set_status(struct UIEntryQueueStandByWnd *self, const char *queue, int arg1, int arg2);

struct UIEntryQueueWnd *ui_entry_queue_wnd_construct(struct UIEntryQueueWnd *self);
struct UIWindow *ui_entry_queue_wnd_new(void);

#endif
