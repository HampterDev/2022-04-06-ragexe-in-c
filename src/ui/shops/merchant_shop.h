#ifndef UI_SHOPS_MERCHANT_SHOP_H
#define UI_SHOPS_MERCHANT_SHOP_H

struct UIWindow;

unsigned ui_merchant_shop_seller(const struct UIWindow *wnd);

struct UIMerchantItemMyShopWnd;
void ui_merchant_item_my_shop_wnd_add_assistant_bar(struct UIMerchantItemMyShopWnd *self);
struct UIMerchantItemMyShopWnd *ui_merchant_item_my_shop_wnd_construct(struct UIMerchantItemMyShopWnd *self,
                                                                             int variant);
struct UIWindow *ui_merchant_item_my_shop_wnd_new(int arg);
void ui_merchant_item_my_shop_wnd_set_timer(struct UIMerchantItemMyShopWnd *self, DWORD length);
struct UIMerchantItemShopWnd *ui_merchant_item_shop_wnd_construct(struct UIMerchantItemShopWnd *self, int variant);
struct UIWindow *ui_merchant_item_shop_wnd_new(int arg);
void ui_merchant_item_shop_wnd_set_timer(struct UIMerchantItemShopWnd *self, DWORD length);

#endif
