#include "iteminfo/item_types.h"
#ifndef CONTENTS_ITEM_REFORM_WND_H
#define CONTENTS_ITEM_REFORM_WND_H

#include "base.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"

#define REFORM_WND_ID           0x15c
#define REFORM_BUTTON_ID        0xb0
#define REFORM_CLOSE_BUTTON_ID  0xc1
#define REFORM_LIST_PRESSED     0x148

#define UI_MSG_SCROLL_UP        9
#define UI_MSG_SCROLL_DOWN      10
#define UI_MSG_REFORM_ITEM      0x17
#define UI_MSG_DETAIL_ENTER     0xa7
#define UI_MSG_DETAIL_LEAVE     0xa8

#define PACKET_REFORM_CLOSE     0xb90
#define PACKET_REFORM_ITEM      0xb91

struct UIItemReformWnd_Item {
    struct UIWindow wnd;
    struct ItemInfo *item;
};
OFFSET_ASSERT(UIItemReformWnd_Item, item, 0x7c);
SIZE_ASSERT(UIItemReformWnd_Item, 0x80);

struct UIItemReformWnd_List {
    struct UIFrameWnd frame;
    unsigned row_height;
    unsigned visible_rows;
    unsigned pressed;
    unsigned hover;
    int top;
    struct UIScrollBar *scroll_bar;
    struct Vec items;
};

struct UIItemReformWnd {
    struct UIFrameWnd frame;
    struct UIWindow *reform_button;
    struct UIItemReformWnd_List *list;
    struct UIItemReformWnd_Item *base_slot;
    struct UIItemReformWnd_Item *result_slot;
    struct Vec material_slots;
    struct UIWindow *detail_button;
    struct UIWindow *detail_popup;
    struct ItemInfo reform_item;
    struct ItemInfo *base_item;
    struct ItemInfo result;
    struct Vec materials;
    struct String information;
    struct String notice;
};

struct UIItemReformWnd *item_reform_wnd_construct(struct UIItemReformWnd *self);
struct UIWindow *item_reform_wnd_new(void);
void item_reform_wnd_delete(struct UIWindow *window);
void item_reform_wnd_destruct(struct UIItemReformWnd *self);
void item_reform_wnd_on_create(struct UIWindow *self, int w, int h);
void item_reform_wnd_place_popup(struct UIWindow *self);
void item_reform_wnd_on_draw(struct UIWindow *self);
int item_reform_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                        int a, int b, int c, int d);

void item_reform_item_delete(struct UIWindow *window);
void item_reform_item_on_draw(struct UIWindow *self);
void item_reform_item_show_name(struct UIWindow *self, int x, int y);
void item_reform_item_show_info(struct UIWindow *self, int x, int y);

void item_reform_detail_btn_delete(struct UIWindow *window);
void item_reform_detail_btn_on_draw(struct UIWindow *self);
void item_reform_detail_btn_enter(struct UIWindow *self);
void item_reform_detail_btn_leave(struct UIWindow *self);
void item_reform_detail_msg_delete(struct UIWindow *window);
void item_reform_detail_msg_on_draw(struct UIWindow *self);

struct UIItemReformWnd_List *item_reform_list_new(void);
struct UIItemReformWnd_List *item_reform_list_construct(struct UIItemReformWnd_List *self);
void item_reform_list_destruct(struct UIItemReformWnd_List *self);
void item_reform_list_delete(struct UIWindow *window);
void item_reform_list_move(struct UIWindow *self, int x, int y);
void item_reform_list_on_create(struct UIWindow *self, int w, int h);
void item_reform_list_on_draw(struct UIWindow *self);
void item_reform_list_on_l_btn_down(struct UIWindow *self, int x, int y);
void item_reform_list_on_mouse_move(struct UIWindow *self, int x, int y);
void item_reform_list_hover(struct UIWindow *self, int x, int y);
void item_reform_list_press(struct UIWindow *self, int x, int y);
void item_reform_list_on_r_btn_down(struct UIWindow *self, int x, int y);
void item_reform_list_show_info(struct UIWindow *self, int x, int y);
void item_reform_list_wheel(struct UIWindow *window, int delta);
int item_reform_list_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                         int a, int b, int c, int d);

#endif
