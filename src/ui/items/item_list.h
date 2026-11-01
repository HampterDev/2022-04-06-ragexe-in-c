#include "game/item_list_packets.h"
#ifndef UI_ITEM_LIST_H
#define UI_ITEM_LIST_H

#include "ui/items/items.h"

#define ITEM_IDENTIFY_WND_ID 0x49
#define ITEM_COMPOSITION_WND_ID 0x4a
#define PET_EGG_LIST_WND_ID 0x5a
#define MAKING_ARROW_LIST_WND_ID 0x5e
#define SPELL_LIST_WND_ID 0x63

#define MSG_ITEM_IDENTIFY_TITLE 0x209
#define MSG_ITEM_COMPOSITION_TITLE 0x20a
#define MSG_PET_EGG_LIST_TITLE 0x257
#define MSG_SPELL_LIST_TITLE 0x2b9

#define GAME_REQ_IDENTIFY 0x79
#define GAME_REQ_COMPOSE_CARD 0x7c
#define GAME_REQ_HATCH_EGG 0x92
#define GAME_REQ_MAKE_ARROW 0x99
#define GAME_REQ_SPELL 0xa3
#define GAME_REQ_MAKE_ITEM 0xcf
#define GAME_REQ_SPELL_KIND 0x100

#define UI_MSG_LIST_ADD 0x1f
#define UI_MSG_LIST_SET_KIND 0x22
#define UI_MSG_SPELL_SET_KIND 0x1b
#define UI_MSG_LIST_CHANGED 0x3c
#define UI_MSG_LIST_CLEAR 0x4b
#define UI_MSG_COMPOSE_CARD 0x4d

#define MAKING_KIND_ARROW 2

#define PACKET_ITEM_IDENTIFY_LIST 0x177
#define PACKET_ITEM_COMPOSITION_LIST 0x17b

#define LIST_TOP 0x22
#define LIST_ROW_HEIGHT 32

struct UIItemIdentifyWnd {
    struct UIFrameWnd frame;
    struct UIScrollBar *scroll;
    int first_row;
    int selected;
    int columns;
    int rows;
    int scrollable;
    struct List items;
    struct String title;
    struct WindowInfo *info;
};
OFFSET_ASSERT(UIItemIdentifyWnd, scroll, 0x94);
OFFSET_ASSERT(UIItemIdentifyWnd, items, 0xac);
OFFSET_ASSERT(UIItemIdentifyWnd, title, 0xb4);

struct UIItemIdentifyWnd *ui_item_identify_wnd_construct(struct UIItemIdentifyWnd *self);
struct UIWindow *ui_item_identify_wnd_new(void);
void ui_item_identify_wnd_destruct(struct UIItemIdentifyWnd *self);
void ui_item_identify_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_item_identify_wnd_on_draw(struct UIWindow *self);
struct ItemInfo *ui_item_identify_wnd_get_item(struct UIItemIdentifyWnd *self, struct ItemInfo *out, int n,
                                                      int *found);
int ui_item_identify_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4);

void item_list_update_scroll(struct UIItemIdentifyWnd *self);
int item_list_scroll(struct UIItemIdentifyWnd *self, int msg, int rows);
void item_list_set_info(struct UIItemIdentifyWnd *self, struct WindowInfo **slot, struct WindowInfo *info);
void item_list_add_inventory_item(struct UIItemIdentifyWnd *self, int index);

void ui_item_identify_wnd_store_info(struct UIWindow *window);
void ui_item_identify_wnd_on_l_btn_down(struct UIWindow *window, int x, int y);
void ui_item_identify_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y);
void ui_item_identify_wnd_on_r_btn_up(struct UIWindow *self, int x_unused, int y_unused);
void ui_item_identify_wnd_on_wheel(struct UIWindow *window, int delta);

struct ItemInfo *ui_item_identify_wnd_get_selected_item(struct UIItemIdentifyWnd *self, struct ItemInfo *out, int *found);

struct UIItemCompositionWnd *ui_item_composition_wnd_construct(struct UIItemCompositionWnd *self);
struct UIWindow *ui_item_composition_wnd_new(void);
struct UIMakingArrowListWnd *ui_making_arrow_list_wnd_construct(struct UIMakingArrowListWnd *self);
struct UIWindow *ui_making_arrow_list_wnd_new(void);
struct UIPetEggListWnd *ui_pet_egg_list_wnd_construct(struct UIPetEggListWnd *self);
struct UIWindow *ui_pet_egg_list_wnd_new(void);
struct UISpellListWnd *ui_spell_list_wnd_construct(struct UISpellListWnd *self);
struct UIWindow *ui_spell_list_wnd_new(void);

#endif
