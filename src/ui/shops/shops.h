#include "game/actor_names.h"
#include "game/drag.h"
#include "session/item_predicates.h"
#include "ui/item_draw.h"
#include "ui/window_paint.h"
#include "iteminfo/item_api.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/connection_api.h"
#ifndef UI_SHOPS_H
#define UI_SHOPS_H

#include "game/barter_drag.h"

#include "web/barter_market.h"

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include <stdlib.h>
#include <time.h>
#include "game/drag_info.h"

struct UIItemBaseWnd {
    struct UIFrameWnd frame;
    int show_counts;
    int button_bar;
    struct UIResizer *resizer;
    struct UIScrollBar *scroll_bar;
    struct UIBitmapButton *close;
    int extra_buttons[4];
    int scroll;
    int rows;
    int columns;
    int scrollable;
    struct List items;
};
OFFSET_ASSERT(UIItemBaseWnd, resizer, 0x9c);
OFFSET_ASSERT(UIItemBaseWnd, scroll, 0xb8);
OFFSET_ASSERT(UIItemBaseWnd, items, 0xc8);
SIZE_ASSERT(UIItemBaseWnd, 0xd0);
int ui_item_base_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4);
void ui_item_base_wnd_update_scroll(struct UIItemBaseWnd *self);
void ui_item_base_wnd_on_draw(struct UIWindow *self);

struct ItemWndInfo;

struct UIItemWnd {
    struct UIItemBaseWnd base;
    struct ItemWndInfo *info;
    struct UIWindow *sys_button;
    struct UIWindow *mini_button;
    struct UIWindow *close_button;
    int usable_filter;
    int equip_filter;
    int etc_filter;
    int tab;
    int full_height;
    struct UIWindow *drop_lock;
    struct UITabControl *tabs;
    struct UIWindow *compare;
    struct UIBitmapButton *item_number;
    struct UIBitmapButton *deal_lock;
    struct UIWindow *deal_lock_notice;
    struct UIBitmapButton *sort;
};

#define DRAG_INVENTORY 1
#define DRAG_CASH_CART 4
#define DRAG_CASH_GOODS 6

#define WND_ID_COUNT_INPUT 0xf
#define COUNT_MSG_OPEN 0xd
#define MODE_REQ_DRAG_END 0x12
#define UI_MSG_ADD_DRAGGED 0x26
#define ITEM_WND_ID 0xc
#define UI_MSG_SHOW_ITEM 0x18

static __inline int item_grid_hidden(const struct ItemInfo *info)
{
    return session_is_pet_egg_id(&g_session1, atoi(string_cstr(&info->id_text))) && info->damaged;
}
#define ITEM_GRID_X 0x28
#define ITEM_GRID_Y 0x11
#define ITEM_GRID_CELL 32
int ui_item_base_wnd_index_at(struct UIItemBaseWnd *self, int x, int y);
struct ItemInfo *ui_item_base_wnd_get_item(struct UIItemBaseWnd *self, struct ItemInfo *out, int index);
struct ItemInfo *ui_item_base_wnd_item_at(struct UIItemBaseWnd *self, struct ItemInfo *out, int x, int y);

#define SHOP_MSG_REFRESH 0x17
#define SHOP_BOTTOM 0x1d
#define MSG_SHOP_UNIT_CASH 0x51b

struct UIItemShopWnd2 {
    struct UIItemBaseWnd base;
    struct UIWindow *title;
    struct UIWindow *check;
    int with_count;
    struct WindowInfo *info;
    int hover;
    int bottom;
    int extra;
};
OFFSET_ASSERT(UIItemShopWnd2, title, 0xd0);
OFFSET_ASSERT(UIItemShopWnd2, extra, 0xe8);
SIZE_ASSERT(UIItemShopWnd2, 0xec);
struct UIItemShopWnd2 *ui_item_shop_wnd2_construct(struct UIItemShopWnd2 *self);
struct UIWindow *ui_item_shop_wnd2_new(void);
void ui_item_shop_wnd2_destruct(struct UIItemShopWnd2 *self);

struct UIItemShopWnd {
    struct UIItemShopWnd2 shop;
    int cash;
};
OFFSET_ASSERT(UIItemShopWnd, cash, 0xec);
SIZE_ASSERT(UIItemShopWnd, 0xf0);
struct UIItemShopWnd *ui_item_shop_wnd_construct(struct UIItemShopWnd *self);
struct UIWindow *ui_item_shop_wnd_new(void);
void ui_item_shop_wnd_destruct(struct UIItemShopWnd *self);

struct UIItemPurchaseWnd {
    struct UIItemBaseWnd base;
    struct UIWindow *total;
    struct WindowInfo *info;
    int hover;
    int cash;
    struct UIWindow *points;
};
OFFSET_ASSERT(UIItemPurchaseWnd, total, 0xd0);
OFFSET_ASSERT(UIItemPurchaseWnd, points, 0xe0);
SIZE_ASSERT(UIItemPurchaseWnd, 0xe4);
struct UIItemPurchaseWnd *ui_item_purchase_wnd_construct(struct UIItemPurchaseWnd *self);
struct UIWindow *ui_item_purchase_wnd_new(void);
void ui_item_purchase_wnd_destruct(struct UIItemPurchaseWnd *self);

void shop_draw_side_strips(struct UIWindow *wnd, int height);
struct UIBitmapButton *shop_add_button(struct UIWindow *wnd, const char *const names[3], int x, int y, int id);
extern const char *const g_shopBuyButton[3];
extern const char *const g_shopCancelButton[3];

struct UIItemSellWnd {
    struct UIItemBaseWnd base;
    struct UIWindow *total;
    struct WindowInfo *info;
    int hover;
};
OFFSET_ASSERT(UIItemSellWnd, hover, 0xd8);
SIZE_ASSERT(UIItemSellWnd, 0xdc);

struct UIItemSellWnd2 {
    struct UIItemSellWnd sell;
    int window_type;
};
SIZE_ASSERT(UIItemSellWnd2, 0xe0);

void shop_draw_frame(struct UIItemBaseWnd *base, int body, int bar_variant);
void shop_draw_cells(struct UIWindow *wnd, int rows);
void shop_fill_names(struct UIWindow *wnd, int rows);
struct UIBitmapButton *shop_place_button(struct UIWindow *wnd, struct UIBitmapButton *button,
                                         const char *const names[3], int x, int y, int id);
void shop_resize(struct UIItemShopWnd2 *self, int width, int height, int with_check);
int shop_info_usable(struct WindowInfo *self);
void shop_info_default(struct WindowInfo *self);

struct UIWindow *shop_add_title(struct UIWindow *wnd, int width, int msg);
void shop_add_resizer(struct UIItemBaseWnd *base, int edge);
void shop_add_scroll_bar(struct UIItemBaseWnd *base, int length);
void shop_tooltip_cell(struct UIWindow *wnd, int x, int y, int *tip_x, int *tip_y);

#define CASH_GOODS_PRICE(info) ((info)->price)
#define CASH_GOODS_START(info) ((info)->sale_start_time)
#define CASH_GOODS_END(info) ((info)->sale_end_time)
#define CASH_GOODS_STOCK(info) ((info)->sale_remaining)

#define CASH_TAB_SALE 9
#define CASH_MSG_SET_GOODS 0x17
#define CASH_MSG_ADD 0x1f
#define CASH_MSG_REMOVE 0x68
#define CASH_MSG_CART_TOTAL 0x9b
#define CASH_ID_ADD 0xd8
#define CASH_ID_REMOVE 0xcb
#define CASH_ID_PLUS 0x10f
#define CASH_ID_MINUS 0x110
#define COUNT_FOR_CASH_CART 0x13
#define COUNT_FOR_CASH_SHOP 0x14

static __inline void send_packet(const void *buf, int len)
{
    rag_connection_send_packet(rag_connection_instance(), len, buf);
}

void ui_item_base_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);

void ui_item_base_wnd_store_info(struct UIWindow *self);
void ui_item_base_wnd_on_wheel(struct UIWindow *window, int delta);
void ui_item_shop_wnd_store_info(struct UIWindow *window);
void ui_item_shop_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y);
void ui_item_purchase_wnd_store_info(struct UIWindow *window);
void ui_item_shop_wnd_on_create(struct UIWindow *window, int width, int height);
void ui_item_shop_wnd_on_l_btn_down(struct UIWindow *window, int x, int y);
void ui_item_shop_wnd_on_mouse_hover(struct UIWindow *window, int x, int y);
void ui_item_shop_wnd_on_r_btn_down(struct UIWindow *window, int x, int y);
void ui_item_purchase_wnd_on_create(struct UIWindow *window, int width, int height);
void ui_item_purchase_wnd_on_draw(struct UIWindow *window);
void ui_item_purchase_wnd_on_l_btn_down(struct UIWindow *window, int x, int y);
void ui_item_purchase_wnd_on_mouse_hover(struct UIWindow *window, int x, int y);
void ui_item_purchase_wnd_on_r_btn_down(struct UIWindow *window, int x, int y);

struct UIBank_NewWnd *ui_bank_new_wnd_construct(struct UIBank_NewWnd *self);
struct UIWindow *ui_bank_new_wnd_new(void);

struct UIItemShopWnd *ui_bartermarket_wnd_construct(struct UIItemShopWnd *self);
struct UIWindow *ui_bartermarket_wnd_new(void);

struct UIItemPurchaseWnd *ui_barter_item_purchase_wnd_construct(struct UIItemPurchaseWnd *self);
struct UIWindow *ui_barter_item_purchase_wnd_new(void);

struct UICashShopWnd_Item *ui_cash_shop_wnd_item_new(void);

struct UIMerchantItemLogWnd *ui_merchant_item_log_wnd_construct(struct UIMerchantItemLogWnd *self, int variant);
struct UIWindow *ui_merchant_item_log_wnd_new(int arg);

struct UIMerchantItemPurchaseWnd *ui_merchant_item_purchase_wnd_construct(struct UIMerchantItemPurchaseWnd *self,
                                                                                 int variant);
struct UIWindow *ui_merchant_item_purchase_wnd_new(int arg);

struct UIMileageWnd *ui_mileage_wnd_construct(struct UIMileageWnd *self);
struct UIWindow *ui_mileage_wnd_new(void);

struct UIItemSellWnd2 *ui_item_sell_wnd2_construct(struct UIItemSellWnd2 *self);
struct UIWindow *ui_item_sell_wnd2_new(void);
struct UIItemSellWnd *ui_item_sell_wnd_construct(struct UIItemSellWnd *self);
struct UIWindow *ui_item_sell_wnd_new(void);

struct UIItemPurchaseWnd *ui_para_item_purchase_wnd_construct(struct UIItemPurchaseWnd *self);
struct UIWindow *ui_para_item_purchase_wnd_new(void);
struct UIItemShopWnd *ui_para_item_shop_wnd_construct(struct UIItemShopWnd *self);
struct UIWindow *ui_para_item_shop_wnd_new(void);
struct UIParaResultWnd;
void ui_para_result_wnd_add_bought_record(struct UIParaResultWnd *self, const struct ItemRecordA *record,
                           int count);
void ui_para_result_wnd_add_item(struct UIParaResultWnd *self, const struct ItemInfo *item);
struct UIParaResultWnd *ui_para_result_wnd_construct(struct UIParaResultWnd *self);
struct UIWindow *ui_para_result_wnd_new(void);

struct UISerachStoreInfoWnd *ui_serach_store_info_wnd_construct(struct UISerachStoreInfoWnd *self);
struct UIWindow *ui_serach_store_info_wnd_new(void);

struct UIExchangeWnd *ui_exchange_wnd_construct(struct UIExchangeWnd *self);
struct UIWindow *ui_exchange_wnd_new(void);

struct UIMerchantItemPurchaseMSGWnd *ui_merchant_item_purchase_msg_wnd_new(void);
struct UIMerchantShopTitle *ui_merchant_shop_title_new(int buying);

#endif
