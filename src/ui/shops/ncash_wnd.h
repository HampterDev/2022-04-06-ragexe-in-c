#ifndef UI_SHOPS_NCASH_WND_H
#define UI_SHOPS_NCASH_WND_H

#include "platform/client_config.h"

#include "ui/shops/cash_shop.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"

int n_cash_info_is_on_screen(struct WindowInfo *self);
void n_cash_info_set_default(struct WindowInfo *self);

#define NCASH_TAB_COUNT 10
#define NCASH_TAB_SALE 8
#define NCASH_BUTTON_COUNT 19
#define NCASH_PAGE_GOODS 8
#define NCASH_ROWS 4
#define NCASH_CART_MAX 8

struct UInCashWnd {
    struct UIItemBaseWnd base;
    struct UIWindow *count;
    struct UIWindow *search;
    struct List cart;
    struct WindowInfo *info;
    struct UIWindow *buttons[NCASH_BUTTON_COUNT];
    struct UIScrollBar *scroll;
    struct UIWindow *title;
    int cart_pos;
    int hit;
    int page_count;
    int page;
    int tab;
    int cash;
    int cart_total;
    int goods_bind_type;
    int points;
    int row_height;
    int grid_top;
    int top;
    unsigned long long drawn_key;
};

struct UInCashWnd *u_in_cash_wnd_construct(struct UInCashWnd *self);
struct UIWindow *u_in_cash_wnd_new(void);

#endif
