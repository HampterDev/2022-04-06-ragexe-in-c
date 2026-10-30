#ifndef UI_SOCIAL_ENTRY_QUEUE_STANDBY_INTERNAL_H
#define UI_SOCIAL_ENTRY_QUEUE_STANDBY_INTERNAL_H
#include "ui/window.h"
struct UIEntryQueueStandByWnd {
    struct UIFrameWnd frame;
    struct String queue;
    int arg1, arg2;
    struct UIWindow *text;
    struct UIBitmapButton *leave;
    struct UIBitmapButton *close;
    char lines[3][0x104];
};

struct UIEntryQueueHelpWnd *ui_entry_queue_help_wnd_construct(struct UIEntryQueueHelpWnd *self);
struct UIWindow *ui_entry_queue_help_wnd_new(void);
struct UIEntryQueueRequestWnd *ui_entry_queue_request_wnd_construct(struct UIEntryQueueRequestWnd *self);
struct UIWindow *ui_entry_queue_request_wnd_new(void);
BYTE ui_entry_queue_request_wnd_init(struct UIEntryQueueRequestWnd *self);
struct UIEntryQueueStandByWnd *ui_entry_queue_stand_by_wnd_construct(struct UIEntryQueueStandByWnd *self);
struct UIWindow *ui_entry_queue_stand_by_wnd_new(void);
BYTE ui_entry_queue_stand_by_wnd_init(struct UIEntryQueueStandByWnd *self);

#endif
