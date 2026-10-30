#include "iteminfo/item_types.h"
#ifndef UI_ITEMS_ITEM_COLLECTION_INTERNAL_H
#define UI_ITEMS_ITEM_COLLECTION_INTERNAL_H

#include "ui/items/items.h"
#include <stdlib.h>
#include <time.h>

#define WND_ITEM_COLLECTION 0xc
#define WND_COMBINED_CARD_COLLECTION 0x52
#define WND_ITEM_COLLECTION_CMP 0xea
#define WND_COMBINED_CARD_COLLECTION_CMP 0xfa
#define WND_BOOK 0x6a
#define WND_ITEM_MOVE_INFO 0x114
#define WND_EQUIP_PREVIEW 0x126

#define UI_MSG_COLLECTION_ITEM 0x18
#define UI_MSG_COLLECTION_REFRESH 0x1d
#define UI_MSG_COLLECTION_IDENTIFIED 0x4c
#define UI_MSG_BOOK_OPEN 0x5e
#define UI_MSG_PREVIEW_ITEM 0x17
#define UI_MSG_MOVE_INFO_ITEM 0x93
#define UI_MSG_PARAM_ITEM 0x40
#define UIWM_MSG_SHOW_CHAT 3

#define ID_VIEW_BUTTON 0xf2
#define ID_BOOK_BUTTON_1 0x117
#define ID_BOOK_BUTTON_2 0x118
#define ID_PREVIEW_BUTTON 0x15a
#define ID_MOVE_INFO 0x1df

#define MSG_BOOK_TOOLTIP_1 0x50e
#define MSG_BOOK_TOOLTIP_2 0x50f
#define MSG_MOVE_INFO 0xaec
#define MSG_OPTION_TITLE 0xf2e
#define MSG_EXPIRE_TIME_FORMAT 0x4ec
#define MSG_EXPIRE_LINE 0x4e7
#define MSG_EXPIRE_DAYS_HOURS 0xf86
#define MSG_EGG_LINE 0xb31
#define MSG_EGG_STATE_OTHER 0xb32
#define MSG_PREVIEW_BUTTON 0xb6a

#define TEXT_LAYOUT 0x1a
#define TEXT_LAYOUT_SERVICE_6 0x18
#define TEXT_LAYOUT_EXPIRE 0x20

#define BOOK_NAME "책읽기"
#define NO_BOOK_TEXT "책이 없습니다"

#define ITEM_EGG_STATE(info) ((info)->slot[2])

#define ITEM_OPTION(info, i) (&(info)->options[i])

struct OptionLines {
    int index;
    struct List lines;
};
void option_lines_destroy(void *value);

struct UIItemMoveInfo {
    struct UITextButton button;
    int item_id;
    struct ItemInfo item;
};

struct UIItemCollectionWnd {
    struct UIFrameWnd frame;
    struct ItemInfo item;
    struct UIWindow *text;
    struct UIBitmapButton *view_btn;
    struct UIWindow *book_btn[2];
    DWORD book_anim_time;
    struct String picture;
    struct String card_picture;
    int wnd_id;
    int option_count;
    struct WindowInfo *info;
    struct UIResizer *resizer;
    struct UIWindow *close_btn;
    BYTE show_cards;
    struct UIItemMoveInfo *move_info;
    short pet_level;
    short pet_hunger;
    short pet_intimacy;
    char pet_name[0x18];
    short pet_accessory;
    int pet_renamed;
    struct UIWindow *preview_btn;
    struct Tree options;
    int options_height;
};
OFFSET_ASSERT(UIItemCollectionWnd, item, 0x94);

struct UICombinedCardItemCollectionWnd {
    struct UIItemCollectionWnd base;
    struct WindowInfo *info;
};

#define COLLECTION_WND(self) (&(self)->frame.base)
#define COLLECTION_ITEM_ID(self) atoi(string_cstr(&(self)->item.id_text))

void item_collection_place_left_of(struct UIWindow *wnd, int keep_left_of);
int item_collection_card_at(struct UIItemCollectionWnd *self, int x, int y, int *slot_x, int *slot_y);
void item_collection_card_item(struct ItemInfo *card, int card_id);
void item_collection_show_card(int wnd_id, struct ItemInfo *card, int past_slots);
void item_collection_take_info(struct UIWindow *wnd, struct WindowInfo *info, int default_y);
void item_collection_set_height(struct UIItemCollectionWnd *self, int height);
int item_collection_set_item(struct UIItemCollectionWnd *self, struct ItemInfo *info, int toggle, int no_compare);
void item_collection_refresh(struct UIItemCollectionWnd *self);

struct UIItemCollectionWnd *ui_item_collection_wnd_construct(struct UIItemCollectionWnd *self);
struct UIWindow *ui_item_collection_wnd_new(void);
void ui_item_collection_wnd_destruct(struct UIItemCollectionWnd *self);
void ui_item_collection_wnd_on_create(struct UIWindow *self, int width, int height);
int ui_item_collection_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
struct UIItemMoveInfo *ui_item_move_info_construct(struct UIItemMoveInfo *self);

void ui_item_collection_wnd_destroy(struct UIWindow *window);

void ui_item_collection_wnd_move(struct UIWindow *window, int x, int y);
void ui_item_collection_wnd_store_info(struct UIWindow *window);
void ui_item_collection_wnd_on_process(struct UIWindow *window);
void ui_item_collection_wnd_on_draw(struct UIWindow *window);
void ui_item_collection_wnd_on_l_btn_down(struct UIWindow *window, int x, int y);
void ui_item_collection_wnd_on_mouse_hover(struct UIWindow *window, int x, int y);
void ui_item_collection_wnd_on_l_btn_up(struct UIWindow *window, int x, int y);
void ui_item_collection_wnd_on_r_btn_down(struct UIWindow *window, int x, int y);

struct UICombinedCardItemCollectionWnd *
ui_combined_card_item_collection_wnd_construct(struct UICombinedCardItemCollectionWnd *self);
struct UIWindow *ui_combined_card_item_collection_wnd_new(void);
struct UIItemCollectionWnd *
ui_combined_card_item_collection_comparison_wnd_construct(struct UIItemCollectionWnd *self);
struct UIWindow *ui_combined_card_item_collection_comparison_wnd_new(void);
struct UIItemCollectionWnd *ui_item_collection_comparison_wnd_construct(struct UIItemCollectionWnd *self);
struct UIWindow *ui_item_collection_comparison_wnd_new(void);

#endif
