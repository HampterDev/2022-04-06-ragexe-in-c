#ifndef UI_CONTROLS_ITEM_CTRLS_H
#define UI_CONTROLS_ITEM_CTRLS_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/character/selection_buttons.h"

struct ItemInfo;
struct UIBasicItemCtrl;
struct UIBasicSkillCtrl;
struct DragInfo;
struct UISlotBmpButton;
struct UIStaticItem;
struct UIWindow;

void ui_basic_item_ctrl_destroy(struct UIWindow *window);
void ui_basic_item_ctrl_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_basic_item_ctrl_on_draw(struct UIWindow *self);
void ui_basic_item_ctrl_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_basic_item_ctrl_on_mouse_hover(struct UIWindow *self, int x_unused, int y_unused);
void ui_basic_item_ctrl_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_basic_skill_ctrl_destroy(struct UIWindow *window);
void ui_basic_skill_ctrl_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_basic_skill_ctrl_on_draw(struct UIWindow *self);
void ui_basic_skill_ctrl_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_basic_skill_ctrl_on_mouse_hover(struct UIWindow *self, int x_unused, int y_unused);
void ui_basic_skill_ctrl_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_static_item_destruct(struct UIStaticItem *self);
void ui_static_item_illust_destruct(struct UIStaticItem *self);
void ui_static_item_destroy(struct UIWindow *window);
void ui_static_item_illust_destroy(struct UIWindow *window);
void ui_static_item_on_draw(struct UIWindow *self);
void ui_static_item_illust_on_draw(struct UIWindow *self);
void ui_static_item_on_mouse_hover(struct UIWindow *self, int x_unused, int y_unused);
void ui_static_item_illust_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_static_item_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_slot_bmp_button_destruct(struct UISlotBmpButton *self);
void ui_slot_bmp_button_destroy(struct UIWindow *window);
void ui_slot_bmp_button_on_create(struct UIWindow *self, int width, int height);
void ui_slot_bmp_button_set_slot(struct UISlotBmpButton *self, int slot);
void ui_slot_bmp_button_on_process(struct UIWindow *self);
void ui_slot_bmp_button_on_draw(struct UIWindow *self);
void ui_slot_bmp_button_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_slot_bmp_button_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_slot_bmp_button_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_slot_bmp_button_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                       int val1, int val2, int val3, int val4);
struct UISlotBmpButton *ui_slot_bmp_button_construct(struct UISlotBmpButton *self,
                                                             int occupied);

#define SLOT_MSG_SET_STYLE 0x1a7
#define SLOT_VAL_R_BTN_DOWN 0xda

#endif
