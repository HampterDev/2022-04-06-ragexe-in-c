#ifndef UI_QUESTS_SAY_DIALOG_H
#define UI_QUESTS_SAY_DIALOG_H
#include "ui/window.h"

struct UITagTextViewer;

struct UISayDialogWnd {
    struct UIFrameWnd frame;
    struct UITagTextViewer *text;
    struct List buttons;
    struct List button_ids;
    int cmd_id;
    int npc_id;
    struct WindowInfo *info;
    int clear_on_text;
};
OFFSET_ASSERT(UISayDialogWnd, info, 0xb0);
SIZE_ASSERT(UISayDialogWnd, 0xb8);

void ui_say_dialog_wnd_set_text_align(struct UISayDialogWnd *self, unsigned char align);
int ui_say_dialog_wnd_save_state(struct UIWindow *window, struct ReplayBuffer *out);

struct UISayDialogWnd *ui_say_dialog_wnd_construct(struct UISayDialogWnd *self);
struct UIWindow *ui_say_dialog_wnd_new(void);

#endif
