#include <stdlib.h>
#include "game/drag_info.h"
#include "ui/window.h"
#ifndef UI_DROP_OWNER
#define UI_DROP_OWNER 1
#endif

void test_drop_invoke(struct UIWindow *window, size_t size,
                     void (*callback)(struct UIWindow *, int, int, const struct DragInfo *),
                     int x, int y, const struct DragInfo *drag, int parent);

#if UI_DROP_OWNER == 1
#include "../src/ui/items/grade_enchant.c"
#define DROP_OWNER UIGradeEnchantWnd
#define DROP_CALLBACK ui_grade_enchant_wnd_drag_and_drop
#define DROP_TEST test_grade_drop
#elif UI_DROP_OWNER == 2
#include "../src/ui/items/refining_mouse.c"
#define DROP_OWNER UIRefiningWnd
#define DROP_CALLBACK ui_refining_wnd_drag_and_drop
#define DROP_TEST test_refine_drop
#elif UI_DROP_OWNER == 3
#include "../src/ui/controls/bitmap_tab.c"
#define DROP_OWNER UIBitmapTab
#define DROP_CALLBACK ui_bitmap_tab_drag_and_drop
#define DROP_TEST test_bitmap_tab_drop
#elif UI_DROP_OWNER == 4
#include "../src/ui/controls/tab_control.c"
#define DROP_OWNER UITabControl
#define DROP_CALLBACK ui_tab_control_drag_and_drop
#define DROP_TEST test_tab_drop
#elif UI_DROP_OWNER == 5
#include "../src/ui/shops/cash_shop_cart.c"
#define DROP_OWNER UICashShopWnd_CartItem
#define DROP_CALLBACK ui_cash_shop_wnd_cart_item_drag_and_drop
#define DROP_TEST test_cart_drop
#elif UI_DROP_OWNER == 6
#include "../src/ui/shops/cash_shop_goods.c"
#define DROP_OWNER UICashShopWnd_Item
#define DROP_CALLBACK ui_cash_shop_wnd_item_drag_and_drop
#define DROP_TEST test_goods_drop
#elif UI_DROP_OWNER == 7
#include "../src/ui/controls/button.c"
#define DROP_OWNER UIBitmapButton
#define DROP_CALLBACK ui_bitmap_button2_drag_and_drop
#define DROP_TEST test_button_drop
#elif UI_DROP_OWNER == 8
#include "../src/ui/controls/window_base.c"
#define DROP_OWNER UIWindow
#define DROP_CALLBACK ui_window_drag_and_drop_base
#define DROP_TEST test_window_drop
#elif UI_DROP_OWNER == 9
#include "../src/ui/items/item_base.c"
#define DROP_OWNER UIItemBaseWnd
#define DROP_CALLBACK ui_item_base_wnd_drag_and_drop
#define DROP_TEST test_item_base_drop
#elif UI_DROP_OWNER == 10
#include "../src/ui/shops/para_shop.c"
#define DROP_OWNER UIParaResultWnd
#define DROP_CALLBACK ui_para_result_wnd_drag_and_drop
#define DROP_TEST test_result_drop
#else
#error Select a UI drop owner
#endif

void DROP_TEST(int x, int y, const struct DragInfo *drag, int parent)
{
    struct DROP_OWNER owner = {0};
    test_drop_invoke((struct UIWindow *)&owner, sizeof owner, DROP_CALLBACK, x, y, drag, parent);
}

#include "ui_drop_unreached.h"
