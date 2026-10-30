#ifndef UI_CONTROLS_AGIT_H
#define UI_CONTROLS_AGIT_H

#include "ui/controls/listbox.h"

struct UIAgitMessageBox {
    struct UIFrameWnd frame;
    WORD agit_id;
};

struct UIListBoxAgitMenu {
    struct UIListBox list;
    int agit_index;
};

void ui_agit_message_box_create(struct UIAgitMessageBox *self, const char *text, int width, int height);
void ui_agit_message_box_destruct(struct UIAgitMessageBox *self);
void ui_agit_message_box_destroy(struct UIWindow *window);
void ui_agit_message_box_on_draw(struct UIWindow *self);
int ui_agit_message_box_send_msg(struct UIWindow *self, struct UIWindow *sender,
                            int msg, int val1, int val2, int val3, int val4);
void agit_message_box_open(WORD agit_id, const char *text);

#endif
