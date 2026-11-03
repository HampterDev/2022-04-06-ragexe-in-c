#include <stdlib.h>
#include "game/drag_info.h"
#include "ui/window.h"
#include "ui/controls/ui_hooks.h"
#ifndef UI_DROP_OWNER
#define UI_DROP_OWNER 2
#endif

struct UIHooks g_uiHooks;

void test_drop_invoke(struct UIWindow *window, size_t size,
                     void (*callback)(struct UIWindow *, int, int, const struct DragInfo *),
                     int x, int y, const struct DragInfo *drag, int parent);

#if UI_DROP_OWNER == 2
#include "../src/ui/items/refining_mouse.c"
#define DROP_OWNER UIRefiningWnd
#define DROP_CALLBACK ui_refining_wnd_drag_and_drop
#define DROP_TEST test_refine_drop
#elif UI_DROP_OWNER == 4
#include "../src/ui/controls/tab_control.c"
#define DROP_OWNER UITabControl
#define DROP_CALLBACK ui_tab_control_drag_and_drop
#define DROP_TEST test_tab_drop
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
