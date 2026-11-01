#include "game/drag.h"
#include "session/shop_inventory.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_SHOPS_MERCHANT_H
#define UI_SHOPS_MERCHANT_H

#include "ui/shops/shops.h"

#define MSG_CART_TITLE 0xde
#define MSG_MIRROR_TITLE 0xe8
#define MSG_MIRROR_TITLE_1 0x6c5
#define MSG_MIRROR_TITLE_2 0x6ca
#define MSG_TIP_CART_OPTIONS 0x9f7
#define MSG_TIP_MIRROR_OPTIONS 0xac4
#define MSG_TIP_COUNT_WORD 0x24c

#define MERCHANT_RESIZE_EDGE 7
#define MERCHANT_TITLE_WIDTH 0x5a
#define MIRROR_TITLE_WIDTH 0x96
#define MERCHANT_BOTTOM 0x26
#define MERCHANT_FULL_COLOUR 0xff
#define MERCHANT_COUNT_COLOUR 0x232323

#define DRAG_STORAGE 6
#define DRAG_CART 9
#define DRAG_INVENTORY_A 0xa
#define DRAG_FROM_BUYING_SETUP 0x13
#define DRAG_BUYING_STORE_SELL 0x15
#define DRAG_MIRROR 0xb
#define DRAG_MIRROR_1 0x14
#define DRAG_MIRROR_2 0x16
#define COUNT_FOR_CART 8
#define COUNT_FOR_MIRROR 0xa
#define COUNT_FOR_MIRROR_2 0x12

#define MSG_SHOP_TITLE 0xe3
#define MSG_SHOP_TITLE_VARIANT 0x6c9
#define MSG_SHOP_NOTICE 0x6c7
#define MSG_SHOP_COUNT 0xb6
#define MSG_SHOP_TIME_LEFT 0xb6f
#define MSG_TIP_SHOP_OPTIONS 0xa28
#define SHOP_WND_BOTTOM 0x15
#define SHOP_WND_BOTTOM_BAR 0x39
#define DRAG_SHOP_ITEM 0xc

#define EDIT_ID_WHISPER 0x1e2
#define EDIT_ID_RODEX 0x1e3
#define EDIT_ID_MEGAPHONE 0x1e4
#define EDIT_ID_CHAT 0x1e5
#define EDIT_ID_CHAT_ROOM 0x1e6

void merchant_store_info(struct UIWindow *wnd, struct WindowInfo *info);

struct UIMerchantItemWnd {
    struct UIItemBaseWnd base;
    struct WindowInfo *info;
    struct UIWindow *close;
    struct UIWindow *status;
};
OFFSET_ASSERT(UIMerchantItemWnd, status, 0xd8);
SIZE_ASSERT(UIMerchantItemWnd, 0xdc);

struct UIMerchantMirrorItemWnd {
    struct UIItemBaseWnd base;
    struct WindowInfo *info;
    int variant;
};
SIZE_ASSERT(UIMerchantMirrorItemWnd, 0xd8);

struct UIMerchantItemShopWnd {
    struct UIItemBaseWnd base;
    int fixed_height;
    struct UIWindow *title;
    struct WindowInfo *info;
    int hover;
    int actor_id;
    struct UIWindow *notice;
    int notice_amount;
    int variant;
    DWORD timer_start;
    DWORD timer_length;
    int bottom;
};
OFFSET_ASSERT(UIMerchantItemShopWnd, info, 0xd8);
OFFSET_ASSERT(UIMerchantItemShopWnd, variant, 0xec);
SIZE_ASSERT(UIMerchantItemShopWnd, 0xfc);

#define MSG_PURCHASE_TITLE 0xe4
#define MSG_PURCHASE_TITLE_VARIANT 0xa8
#define MSG_ALREADY_LISTED 0x77
#define MSG_ONLY_IN_STOCK 0x6aa
#define MSG_ONLY_IN_STOCK_VARIANT 0x6cb
#define MSG_AT_MOST 0x6a8
#define MSG_AT_MOST_VARIANT 0x6a9
#define MSG_NOT_ENOUGH_ZENY 0x37
#define MSG_ZENY_OVERFLOW 0x2b6
#define MSG_OVER_LIMIT 0x6cc
#define PURCHASE_MAX_COUNT 30000
#define PURCHASE_RESIZE_EDGE 3
#define PURCHASE_MIN_ROWS 4
#define PURCHASE_MSG_SET_REQUEST 0x1c
#define DRAG_VENDING_PURCHASE 0xd
#define COUNT_FOR_PURCHASE 0xc
#define COUNT_FOR_PURCHASE_MIRROR 0x11
#define MODE_REQ_SHOP_CLOSE 0x28
#define MODE_REQ_PURCHASE_VARIANT 0x111
#define WND_ID_PURCHASE_CONFIRM 0xe6
#define WND_ID_VENDING_SHOP 0x2b
#define WND_ID_VENDING_PURCHASE 0x2c
#define WND_ID_BUYING_STORE_SHOP 0xb1
#define WND_ID_BUYING_STORE_SELL 0xb2
#define WND_ID_BUYING_STORE_MIRROR 0xb3
#define UIWM_MSG_SHOW_CHAT 3
#define REFUSAL_COLOUR 0xff

void ui_merchant_item_purchase_msg_wnd_set_purchase(struct UIMerchantItemPurchaseMSGWnd *self, int arg1,
                                                         int arg2);

struct UIMerchantItemPurchaseWnd {
    struct UIItemBaseWnd base;
    int fixed_height;
    struct UIWindow *total;
    int request_arg1;
    struct WindowInfo *info;
    int hover;
    int request_arg2;
    int variant;
};
OFFSET_ASSERT(UIMerchantItemPurchaseWnd, info, 0xdc);
SIZE_ASSERT(UIMerchantItemPurchaseWnd, 0xec);

#define MSG_MY_SHOP_TITLE 0xca5
#define MSG_MY_SHOP_TITLE_VARIANT 0x6c6
#define MSG_MY_SHOP_CHECK 0xa52
#define MSG_MY_SHOP_CHECK_VARIANT 0xa8a
#define MSG_TIP_MY_SHOP_VARIANT 0x6c8
#define MSG_TIP_MY_SHOP_OPTIONS 0xac5
#define MY_SHOP_RESIZE_EDGE 7
#define UI_ID_MY_SHOP_CLOSE 0xc1

struct UIMerchantItemMyShopWnd {
    struct UIItemBaseWnd base;
    int notice_amount;
    struct UIWindow *title;
    struct WindowInfo *info;
    struct UIWindow *notice;
    int variant;
    struct UIWindow *check;
    BYTE assistant;
    int owner_aid;
    DWORD timer_start;
    DWORD timer_length;
    int bottom;
};
OFFSET_ASSERT(UIMerchantItemMyShopWnd, check, 0xe4);
OFFSET_ASSERT(UIMerchantItemMyShopWnd, timer_start, 0xf0);
SIZE_ASSERT(UIMerchantItemMyShopWnd, 0xfc);

void ui_merchant_item_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_merchant_item_shop_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);

void ui_merchant_item_wnd_on_r_btn_down(struct UIWindow *window, int x, int y);
int ui_merchant_item_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4);
void ui_merchant_mirror_item_wnd_on_r_btn_down(struct UIWindow *window, int x, int y);
void ui_merchant_item_purchase_wnd_on_r_btn_down(struct UIWindow *window, int x, int y);
void ui_merchant_item_my_shop_wnd_on_r_btn_down(struct UIWindow *window, int x, int y);
int ui_merchant_item_my_shop_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                               int val1, int val2, int val3, int val4);
void ui_merchant_item_shop_wnd_on_r_btn_down(struct UIWindow *window, int x, int y);
int ui_merchant_item_shop_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4);

struct UIMerchantItemWnd *ui_merchant_item_wnd_construct(struct UIMerchantItemWnd *self);
struct UIWindow *ui_merchant_item_wnd_new(void);
struct UIMerchantMirrorItemWnd *ui_merchant_mirror_item_wnd_construct(struct UIMerchantMirrorItemWnd *self,
                                                                             int variant);
struct UIWindow *ui_merchant_mirror_item_wnd_new(int arg);

#endif
