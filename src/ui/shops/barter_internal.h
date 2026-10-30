#include "session/state.h"
#ifndef UI_SHOPS_BARTER_INTERNAL_H
#define UI_SHOPS_BARTER_INTERNAL_H
#include "ui/shops/shops.h"
#include "ui/ui_std.h"

#define WND_ID_BARTER_MARKET 0x14e
#define WND_ID_BARTER_PURCHASE 0x14f

#define MSG_BARTER_TITLE 0xba
#define MSG_BARTER_PURCHASE_TITLE 0xe69
#define MSG_BARTER_PAY 0xde1
#define MSG_BARTER_NOT_LISTED 0xde2
#define MSG_BARTER_PAY_ID_REFUSED 0xe51
#define MSG_BARTER_TYPE_REFUSED 0x77
#define MSG_BARTER_CASH_TYPE_REFUSED 0xdf1
#define MSG_BARTER_OVER_MAX 0x6a8
#define MSG_BARTER_TOO_HEAVY 0x38
#define BARTER_MAX_COUNT 30000
#define BARTER_REFUSAL_COLOUR 0xff

#define DRAG_BARTER_CART 4
#define DRAG_BARTER_GOODS 0x1f
#define COUNT_FOR_BARTER_MARKET 0x17
#define COUNT_FOR_BARTER_PURCHASE 0x18
#define BARTER_PAY_ITEM_TYPE 0x14
#define BARTER_TIP_MAX_LINES 30

#define PURCHASE_RESIZE_EDGE 3
#define MARKET_RESIZE_EDGE 7
#define UIWM_MSG_SHOW_CHAT 3
#define MODE_MSG_BARTER_BUY 0x134
#define MODE_MSG_BARTER_CLOSE 0x28

#define BARTER_INVENTORY_BASE 100
#define MSG_BARTER_NO_ROOM 0xdf0
#define MSG_BARTER_PAY_SHORT 0xde4
#define MSG_BARTER_HELD_OVER_MAX 0xdef

void barter_refusal(int msg);
struct BarterFound *barter_goods_at(struct UIItemBaseWnd *base, const struct List *list,
                                    struct BarterFound *out, int x, int y);
void barter_show_item_at(struct UIItemBaseWnd *base, const struct List *list, int x, int y);
void barter_show_pay_tooltip(struct UIWindow *wnd, int x, int y, const struct BarterGoods *goods, int count);
void barter_refresh_items(struct UIItemBaseWnd *base, const struct List *list);
void barter_draw_pay(struct UIWindow *wnd, int row_y, const struct BarterGoods *goods, int count, int clear);
void barter_draw_highlight(struct UIWindow *wnd, int col_x, int row_y, int width);

void barter_can_buy_refusal(struct UIWindow *wnd, int msg);
void barter_count_add(struct Tree *counts, const struct String *id, int count);
void barter_count_destroy(void *value);
int ui_barter_item_purchase_wnd_held_count(int item_id);

#endif
