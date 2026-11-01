#include "iteminfo/item_api.h"
#include "session/state.h"
#ifndef UI_SHOPS_CASH_SHOP_H
#define UI_SHOPS_CASH_SHOP_H

#include "ui/shops/shops.h"
#include "ui/ui_std.h"

#define CASH_TAB_COUNT 11
#define CASH_TAB_SEARCH 10
struct Vec *cash_shop_cart_lines(struct UIWindow *cart);

struct UICashShopWnd *ui_cash_shop_wnd_construct(struct UICashShopWnd *self);
struct UIWindow *ui_cash_shop_wnd_new(void);

struct UICashShopWnd_Cart *ui_cash_shop_wnd_cart_new(void);
int ui_cash_shop_wnd_cart_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4);
struct UIWindow *u_in_cash_call_wnd_new(void);

#endif
