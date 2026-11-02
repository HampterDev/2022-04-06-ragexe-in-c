#ifndef UI_SHOPS_MERCHANT_SHOP_MAKE_INTERNAL_H
#define UI_SHOPS_MERCHANT_SHOP_MAKE_INTERNAL_H

#include "ui/shops/shops.h"
#include "ui/shops/merchant_shop_make.h"
#include "ui/controls/controls.h"
#include <stdint.h>

#define ROW_HEIGHT 42
#define ROW_FIRST_Y 0x36
#define ROW_TOP 0x2c
#define ROW_HIT_TOP 0x1b
#define ROW_ICON_X 0x23
#define VENDING_SLOTS_MAX 13
#define BUYING_SLOTS_MAX 5

#define PRICE_MAX 1000000000
#define BUYING_PRICE_MAX 99990000
#define BUYING_COUNT_MAX 9999
#define VENDING_STACK_MAX 30000

#define DRAG_VENDING_ITEM 0xb
#define DRAG_BUYING_ITEM 0x14
#define DRAG_FROM_VENDING_SETUP 0xa
#define DRAG_FROM_BUYING_SETUP 0x13

#define ID_PRICE_CHECK UI_ID_CHECKBOX
#define ID_SHOP_INFO 0x14f
#define ID_LOAD_BACKUP 0x228
#define WND_ID_SHOP_INFO 0x14d
#define WND_ID_VENDING_SETUP 0x29
#define WND_ID_VENDING_ITEMS 0x2a
#define WND_ID_BUYING_SETUP 0xae
#define WND_ID_BUYING_ITEMS 0xaf

#define MSM_MSG_REFRESH SHOP_MSG_REFRESH
#define UI_MSG_PAGE_DOWN 0xa

#define MODE_REQ_OPEN_VENDING 0x52
#define MODE_REQ_OPEN_BUYING 0x10f
#define MODE_REQ_VENDING_SETUP_CANCELLED 0x129
#define MODE_REQ_BUYING_SETUP_CANCELLED 0x12b

#define MSG_STACK_LIMIT 0x6a9
#define MSG_VENDING_TITLE 0xe0
#define MSG_BUYING_TITLE 0x6b8
#define MSG_BUYING_PRICE 0x6b9
#define MSG_TOTAL 0x6bb
#define MSG_ZENY_LINE 0x6ba
#define MSG_BUYING_NO_ITEMS 0x6bc
#define MSG_BUYING_NO_PRICE 0x6bd
#define MSG_BUYING_PRICE_HIGH 0x6be
#define MSG_BUYING_NO_COUNT 0x6bf
#define MSG_BUYING_COUNT_HIGH 0x6c0
#define MSG_BUYING_LIST_FULL 0x6c1
#define MSG_SHOP_TITLE_LABEL 0x11a
#define MSG_VENDING_PRICE 0x171
#define MSG_COUNT_WORD 0x24c
#define MSG_NOT_A_NUMBER 0x25a
#define MSG_NOT_IDENTIFIED 0x25b
#define MSG_ZERO_PRICE 0x25c
#define MSG_SETUP_REFUSED 0x24b
#define MSG_PRICE_TOO_HIGH 0x9a3
#define MSG_PRICE_CONFIRM 0x9a9
#define MSG_PRICE_UNIT_BIG 0x9aa
#define MSG_PRICE_UNIT_BIG_TEXT 0x9ab
#define MSG_PRICE_UNIT_SMALL 0x9ac
#define MSG_PRICE_UNIT_SMALL_TEXT 0x9ad
#define MSG_PRICE_CONFIRM_TAIL 0x9ae
#define MSG_PRICE_CHECK 0x9af
#define MSG_EMPTY_LIST 0x9be
#define MSG_NO_TITLE 0xe1
#define MSG_WITH_COMMISSION 0xc96
#define MSG_VENDING_FOOTER 0xc97
#define MSG_BUYING_FOOTER 0xc98
#define MSG_TOTAL_OVER_ZENY 0xe63
#define MSG_TOTAL_TOO_HIGH 0x74e
#define MSG_WAITING 0xb72
#define MSG_LOAD_BACKUP 0xe35
#define MSG_LOAD_BACKUP_TIP 0xe36

#define CHAT_RED 0x6464ff
#define CHAT_SALE_FRAME 0x96ffff
#define CHAT_SALE_LINE 0x5affff
#define CHAT_SALE_ZENY 0x78ffff

#define EDIT_GREY 0xb6
#define EDIT_LIGHT 0xe8

struct UIMerchantShopMakeWnd {
    struct UIFrameWnd frame;
    struct String title;
    struct UIWindow *shop_name;
    int focused;
    struct UIWindow *price_check;
    struct UIWindow *prices[VENDING_SLOTS_MAX];
    int slots;
    struct UIWindow *counts[BUYING_SLOTS_MAX];
    struct UIWindow *total;
    int buying;
    struct UIScrollBar *scroll;
    int first_row;
    int columns;
    int rows;
    int scrollable;
    struct List items;
    struct WindowInfo *info;
    int base_height;
    int fit_rows;
    int shown_rows;
    struct UIBitmapButton *buttons[2];
    struct UIBasicButton *load_button;
    int last_size;
    struct String last_text;
    BYTE assistant;
    BYTE waiting;
    short cell_x, cell_y;
};

struct UIMerchantShopMakeWnd *ui_merchant_shop_make_wnd_construct(struct UIMerchantShopMakeWnd *self, int buying);
struct UIWindow *ui_merchant_shop_make_wnd_new(int arg);
void ui_merchant_shop_make_wnd_destruct(struct UIMerchantShopMakeWnd *self);
int ui_merchant_shop_make_wnd_commit_price(struct UIMerchantShopMakeWnd *self, int next_row);
void ui_merchant_shop_make_wnd_update_scroll(struct UIMerchantShopMakeWnd *self);
struct ItemInfo *ui_merchant_shop_make_wnd_item_at(struct UIMerchantShopMakeWnd *self, struct ItemInfo *out, int x,
                                                         int y);
int merchant_shop_make_info_is_valid(struct WindowInfo *self);
void merchant_shop_make_info_set_default(struct WindowInfo *self);

void msm_edit_amount_colour(struct UIWindow *wnd, struct UIWindow *edit, int64_t amount);
void msm_edit_reset(struct UIWindow *edit, const char *text);
int msm_question_box(const char *text);
void msm_item_message_box(int msg, const struct ItemInfo *info);

#endif
