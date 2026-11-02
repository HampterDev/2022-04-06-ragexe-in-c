#ifndef UI_CONTROLS_ITEM_CTRLS_H
#define UI_CONTROLS_ITEM_CTRLS_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/character/selection_buttons.h"

struct ItemInfo;
struct DragInfo;
struct UISlotBmpButton;
struct UIWindow;

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
