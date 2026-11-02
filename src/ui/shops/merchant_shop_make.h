#ifndef UI_SHOPS_MERCHANT_SHOP_MAKE_H
#define UI_SHOPS_MERCHANT_SHOP_MAKE_H

#include <stdint.h>

struct UIMerchantShopMakeWnd;
struct UIWindow;

void ui_merchant_shop_make_wnd_set_waiting(struct UIMerchantShopMakeWnd *self, unsigned char waiting);
int ui_merchant_shop_make_wnd_is_waiting(const struct UIMerchantShopMakeWnd *self);
void ui_merchant_shop_make_wnd_set_chosen_cell(struct UIMerchantShopMakeWnd *self, int x, int y);
int ui_merchant_shop_make_wnd_get_chosen_cell(const struct UIMerchantShopMakeWnd *self, int *x, int *y);
void ui_merchant_shop_make_wnd_request_assistant(struct UIMerchantShopMakeWnd *self);

intptr_t ui_merchant_shop_make_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

void ui_merchant_shop_make_wnd_add_load_backup_button(struct UIMerchantShopMakeWnd *self);

void ui_merchant_shop_make_wnd_print_buy_list(struct UIMerchantShopMakeWnd *self);
void ui_merchant_shop_make_wnd_set_assistant_items(struct UIMerchantShopMakeWnd *self, int count,
                                                        const void *items);
void ui_merchant_shop_make_wnd_print_sale_list(struct UIMerchantShopMakeWnd *self);

#endif
