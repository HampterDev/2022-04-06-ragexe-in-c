#ifndef UI_CONTROLS_POPUPS_H
#define UI_CONTROLS_POPUPS_H

#include "ui/chat/balloon.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/buttons.h"

struct UIBackdropBox;
struct UICntWnd;
struct UIImgMenu;
struct UIMiniBtnWnd;
struct UIToolTipWnd;
struct UITransMenuWnd;
struct UIWindow;

struct UIWindow *ui_cnt_wnd_new(void);
void ui_cnt_wnd_destruct(struct UICntWnd *self);
void ui_cnt_wnd_destroy(struct UIWindow *window);
void ui_cnt_wnd_on_create(struct UIWindow *self, int cx, int cy);
void ui_cnt_wnd_on_draw(struct UIWindow *self);
int ui_cnt_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender,
                                int msg, int val1, int val2, int val3, int val4);

int ui_img_menu_hit_test(struct UIImgMenu *self, int x, int y);
void ui_img_menu_destroy(struct UIWindow *window);
void ui_img_menu_move(struct UIWindow *self, int x, int y);
void ui_img_menu_on_draw(struct UIWindow *self);
void ui_img_menu_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_img_menu_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_img_menu_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_img_menu_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_img_menu_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1,
                                 int val2, int val3, int val4);

void ui_mini_btn_wnd_destruct(struct UIMiniBtnWnd *self);
void ui_mini_btn_wnd_destroy(struct UIWindow *window);
void ui_mini_btn_wnd_on_draw(struct UIWindow *self);
void ui_mini_btn_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);

struct UIToolTipWnd *ui_tool_tip_wnd_construct(struct UIToolTipWnd *self);
struct UIWindow *ui_tool_tip_wnd_new(void);
void ui_tool_tip_wnd_destroy(struct UIWindow *window);
void ui_tool_tip_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_tool_tip_wnd_on_draw(struct UIWindow *self);
int ui_tool_tip_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    int val1, int val2, int val3, int val4);

struct UITransMenuWnd *ui_trans_menu_wnd_construct(struct UITransMenuWnd *self);
struct UIWindow *ui_trans_menu_wnd_new(void);
void ui_trans_menu_wnd_destroy(struct UIWindow *window);
void ui_trans_menu_wnd_resize(struct UIWindow *self, int width, int height);
void ui_trans_menu_wnd_move(struct UIWindow *self, int x, int y);
BYTE ui_trans_menu_wnd_get_backdrop_box(struct UIWindow *window, struct UIBackdropBox *out);
void ui_trans_menu_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_trans_menu_wnd_on_draw(struct UIWindow *self);
void ui_trans_menu_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_trans_menu_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
int ui_trans_menu_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                      int val1, int val2, int val3, int val4);

#include "ui/controls/agit.h"

struct UINoticeMessageBox_LockSellItem;
struct UINoticeMessageBox_LockSellItem *ui_notice_message_box_lock_sell_item_new(void);
void ui_notice_message_box_lock_sell_item_on_process(struct UIWindow *window);
#define UI_MSGBOX_ANSWER_NO 0xb4

#endif
