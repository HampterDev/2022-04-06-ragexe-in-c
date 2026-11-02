#ifndef UI_CONTROLS_POPUPS_H
#define UI_CONTROLS_POPUPS_H

#include "ui/chat/balloon.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/buttons.h"

struct UIBackdropBox;
struct UICntWnd;
struct UIWindow;

struct UIWindow *ui_cnt_wnd_new(void);
void ui_cnt_wnd_destruct(struct UICntWnd *self);
void ui_cnt_wnd_destroy(struct UIWindow *window);
void ui_cnt_wnd_on_create(struct UIWindow *self, int cx, int cy);
void ui_cnt_wnd_on_draw(struct UIWindow *self);
int ui_cnt_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender,
                                int msg, int val1, int val2, int val3, int val4);

#include "ui/controls/agit.h"

struct UINoticeMessageBox_LockSellItem;
struct UINoticeMessageBox_LockSellItem *ui_notice_message_box_lock_sell_item_new(void);
void ui_notice_message_box_lock_sell_item_on_process(struct UIWindow *window);
#define UI_MSGBOX_ANSWER_NO 0xb4

#endif
