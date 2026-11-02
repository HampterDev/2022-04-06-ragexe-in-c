#ifndef UI_CHAT_EDIT_HISTORY_H
#define UI_CHAT_EDIT_HISTORY_H

#include "ui/controls/edit_types.h"
#include "core/text_array.h"

struct UIItemTagCtrl;

struct UIChatEditCtrl {
    struct UIEditCtrl edit;
    struct TextArray history;
    struct String typed;
    unsigned index;
    struct UIItemTagCtrl *popup;
};

struct UIChatEditCtrl *ui_chat_edit_ctrl_construct(struct UIChatEditCtrl *self);
struct UIWhisperEditCtrl {
    struct UIEditCtrl edit;
    struct TextArray history;
    unsigned index;
};

struct UIWhisperEditCtrl *ui_whisper_edit_ctrl_construct(struct UIWhisperEditCtrl *self);

void ui_chat_edit_ctrl_destruct(struct UIChatEditCtrl *self);
void ui_chat_edit_ctrl_destroy(struct UIWindow *window);
intptr_t ui_chat_edit_ctrl_send_msg(struct UIWindow *self, struct UIWindow *sender,
                          int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
void ui_chat_edit_ctrl_commit(struct UIChatEditCtrl *self);
void ui_whisper_edit_ctrl_destruct(struct UIWhisperEditCtrl *self);
void ui_whisper_edit_ctrl_destroy(struct UIWindow *window);
void ui_whisper_edit_ctrl_on_focus(struct UIWindow *self);
void ui_whisper_edit_ctrl_on_focus_lost(struct UIWindow *self);
intptr_t ui_whisper_edit_ctrl_send_msg(struct UIWindow *self, struct UIWindow *sender,
                             int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
void ui_whisper_edit_ctrl_set_text(struct UIEditCtrl *base_edit, const char *text);

#endif
