#include "contents/mcstore_requests.h"
#include "session/shop_inventory.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "game/context.h"
#include <string.h>
#include "ui/shops/merchant.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"

#define MODE_REQ_CART_TO_STORAGE 0x4f
#define MODE_REQ_CART_TO_INVENTORY 0x4d
#define MODE_REQ_INVENTORY_TO_CART 0x4c
#define MODE_REQ_STORAGE_TO_CART 0x4e
#define MODE_REQ_CLOSE_SHOP 0x51
#define MODE_REQ_CLOSE_BUYING 0x10e
#define MODE_REQ_CLOSE_SHOP_KEEP 0x12d
#define COUNT_FOR_SHOP 0xb
#define WND_ID_CART 0x28
#define WND_ID_MY_SHOP 0x2d
#define WND_ID_MY_BUYING 0xb0
#define WND_ID_SHOP_SETUP 0x101
#define WND_ID_BUYING_SETUP 0x102
#define UI_ID_MY_SHOP_END 0x1f2
#define MSG_MY_SHOP_END 0xb6e
#define MSG_MY_SHOP_TITLE_PLAIN 0xe2
#define MSG_SHOP_TITLE_PLAIN 0xba
#define SHOP_MSG_SET_ACTOR 0x1c
#define SHOP_MSG_TITLE 0x24
#define SHOP_MSG_SET_KEEP 0x1c
#define SHOP_MSG_NOTICE 0x77
#define CART_MSG_STATUS 0x37

static void show_item_window(struct ItemInfo *info)
{
    ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, (intptr_t)info, 0, 0, 0);
}

static int hover_at(struct UIItemBaseWnd *base, int *hover, int x, int y)
{
    int old = *hover;

    *hover = ui_item_base_wnd_index_at(base, x, y);
    if (old != *hover)
        ui_window_invalidate_base(&base->frame.base);
    return *hover;
}

static void refill(struct UIItemBaseWnd *base, int count,
                   struct ItemInfo *(*at)(struct Session *, struct ItemInfo *, int))
{
    int i;

    list_clear(&base->items, item_info_destroy);
    for (i = 0; i < count; i++) {
        struct ItemInfo info;

        at(&g_session1, &info, i);
        if (info.index)
            item_info_copy(list_push_back(&base->items, NULL, sizeof(struct ItemInfo)), &info);
        item_info_destruct(&info);
    }
}

static void take_info(struct UIItemBaseWnd *base, struct WindowInfo *info, int edge)
{
    ui_window_send_msg(&base->frame.base, NULL, UI_MSG_RESIZE, edge, info->width, info->height, 0);
    ui_frame_wnd_move(&base->frame.base, info->x, info->y);
}

static void clamp_size(int *width, int *height, int max_width, int max_height)
{
    int w = *width, h = *height;

    if (w > max_width)
        w = max_width;
    if (h > max_height)
        h = max_height;
    if (w < 0xe8)
        w = 0xe8;
    if (h < 0x78)
        h = 0x78;
    *width = (w - 0x118) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x118;
    *height = (h - 0x78) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x78;
}

static void set_notice(struct UIWindow *wnd, struct UIWindow *notice, int amount, int from_bottom)
{
    char text[0x50], money[0x50];

    sprintf(text, msg_str(MSG_SHOP_NOTICE), format_money(amount, money));
    ui_static_text_set_text_fit(notice, text);
    ui_window_move(notice, 0xc, wnd->height - from_bottom);
}

static int info_fits(const struct WindowInfo *info, int min_width, int min_height)
{
    return info->width >= min_width && info->height >= min_height && ui_info_on_screen(info);
}

static int shop_info_fits(const struct WindowInfo *info, BYTE wide)
{
    return info_fits(info, wide ? 0x15e : 0x118, 0x78);
}

static void shop_info_reset(struct WindowInfo *info, BYTE wide)
{
    info->x = 0;
    info->y = 0x78;
    info->width = wide ? 0x15e : 0x118;
    info->height = 0x138;
}

static void own_info_reset(struct WindowInfo *info)
{
    info->x = 200;
    info->y = 0x78;
    info->width = 0x118;
    info->height = 0x78;
}

void ui_merchant_item_my_shop_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    struct ItemInfo info;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if ((unsigned)info.index > 0) {
        game_current_base_mode();
        show_item_window(&info);
    }
    item_info_destruct(&info);
}

void ui_merchant_item_purchase_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    struct ItemInfo info;

    ui_item_base_wnd_get_item(&self->base, &info, hover_at(&self->base, &self->hover, x, y));
    if ((unsigned)info.index > 0) {
        struct GameMode *mode = game_current_mode();

        if ((GetAsyncKeyState(VK_SHIFT) >> 8) & 0xff) {
            if (g_windowMgr.merchant_item_shop_wnd) {
                struct ItemInfo listed;

                session_purchase_item_by_id(&g_session1, &listed, string_cstr(&info.id_text));
                item_info_assign(&info, &listed);
                item_info_destruct(&listed);
                game_mode_drag_item(mode, DRAG_VENDING_PURCHASE, &info, info.count, -1);
                ui_window_send_msg(g_windowMgr.merchant_item_shop_wnd, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
            }
        } else {
            show_item_window(&info);
        }
    }
    item_info_destruct(&info);
}

void ui_merchant_item_shop_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    struct ItemInfo info;

    ui_item_base_wnd_get_item(&self->base, &info, hover_at(&self->base, &self->hover, x, y));
    if ((unsigned)info.index > 0) {
        struct GameMode *mode = game_current_mode();

        if ((GetAsyncKeyState(VK_SHIFT) >> 8) & 0xff) {
            if (g_windowMgr.merchant_item_purchase_wnd) {
                game_mode_drag_item(mode, DRAG_SHOP_ITEM, &info, info.count, -1);
                ui_window_send_msg(g_windowMgr.merchant_item_purchase_wnd, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
            }
        } else {
            show_item_window(&info);
        }
    }
    item_info_destruct(&info);
}

void ui_merchant_item_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    struct ItemInfo info;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if ((unsigned)info.index > 0) {
        struct Mode *mode = game_current_base_mode();

        if ((GetAsyncKeyState(VK_SHIFT) >> 8) & 0xff) {
            if (!g_windowMgr.exchange_wnd && !g_windowMgr.merchant_shop_make_wnd) {
                if (g_windowMgr.item_store_wnd) {
                    if (game_take_item_move())
                        mode_send_message(mode, MODE_REQ_CART_TO_STORAGE, info.index, info.count, 0, 0);
                } else if (g_windowMgr.item_wnd && game_take_item_move()) {
                    mode_send_message(mode, MODE_REQ_CART_TO_INVENTORY, info.index, info.count, 0, 0);
                }
            }
        } else {
            show_item_window(&info);
        }
    }
    item_info_destruct(&info);
}

void ui_merchant_mirror_item_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantMirrorItemWnd *self = (struct UIMerchantMirrorItemWnd *)window;
    struct ItemInfo info;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if ((unsigned)info.index > 0) {
        struct GameMode *mode = game_current_mode();

        if ((GetAsyncKeyState(VK_SHIFT) >> 8) & 0xff) {
            struct UIWindow *target = g_windowMgr.merchant_shop_make_wnd ? g_windowMgr.merchant_shop_make_wnd : g_windowMgr.buying_store_make_wnd;

            if (target) {
                struct ItemInfo held;

                session_mirror_item_by_index(&g_session1, &held, info.index);
                item_info_assign(&info, &held);
                item_info_destruct(&held);
                game_mode_drag_item(mode, g_windowMgr.merchant_shop_make_wnd ? DRAG_MIRROR : DRAG_MIRROR_1, &info, info.count, -1);
                ui_window_send_msg(target, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
            }
        } else {
            show_item_window(&info);
        }
    }
    item_info_destruct(&info);
}

void ui_merchant_item_shop_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    if (drag->source != DRAG_VENDING_PURCHASE) {
        game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->drop_count == 1) {
        ui_merchant_item_shop_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, COUNT_FOR_SHOP, 0, 0, 0);
}

intptr_t ui_merchant_item_shop_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_RESIZE: {
        int width = val2, height = val3;

        if (val1 != 3)
            return 0;
        clamp_size(&width, &height, self->variant ? 0x1aa : 0x140, 0x150);
        if (width != wnd->width || height != wnd->height) {
            ui_window_resize_base(wnd, width, height);
            self->base.rows = (unsigned)(height - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
        }
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }

    case SHOP_MSG_REFRESH: {
        unsigned rows;

        refill(&self->base, session_shop_list_count(&g_session1), session_shop_list_item);
        rows = self->base.items.size > 4 ? self->base.items.size : 4;
        self->base.rows = rows;
        ui_window_resize_base(wnd, wnd->width, rows * ITEM_GRID_CELL + self->fixed_height);
        ui_window_invalidate_base(&self->base.frame.base);
        return 0;
    }

    case SHOP_MSG_SET_ACTOR:
        self->actor_id = val1;
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!shop_info_fits(self->info, self->variant != 0))
            shop_info_reset(self->info, self->variant != 0);
        take_info(&self->base, self->info, 3);
        return 0;

    case SHOP_MSG_TITLE:
        ui_static_text_set_text_fit(self->title, msg_str(MSG_SHOP_TITLE_PLAIN));
        return 0;

    case UI_MSG_ADD_DRAGGED: {
        struct Mode *mode = game_current_base_mode();

        if (self->variant == 0) {
            struct ItemInfo info;

            item_info_construct(&info);
            item_info_assign_from_drag(&info, game_current_drag());
            session_remove_store_purchase_item(&g_session1, &info);
            if (g_windowMgr.merchant_item_purchase_wnd)
                ui_window_send_msg(g_windowMgr.merchant_item_purchase_wnd, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
            ui_merchant_item_shop_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
            item_info_destruct(&info);
        }
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return 0;
    }

    case SHOP_MSG_NOTICE:
        self->notice_amount = val1;
        set_notice(wnd, self->notice, val1, 0x15);
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

static void my_shop_close(struct UIMerchantItemMyShopWnd *self)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct Mode *mode = client_current_mode();
    int variant = self->variant, assistant = self->assistant;
    int x = wnd->x, y = wnd->y;
    int own_wnd = variant ? WND_ID_MY_BUYING : WND_ID_MY_SHOP;

    if (!assistant) {
        if (mcstore_request_save(&g_session1.mcStoreBackup, variant))
            ui_chat_line(msg_str(variant ? 0xe66 : 0xe65), 0xffff);
        else
            ui_chat_line("Could not save local shop setup. Previous file preserved.", 0xffff);
    }

    if (assistant == 1) {
        ui_close_window(own_wnd);
        mode_send_message(mode, MODE_REQ_SHOP_CLOSE, 0, 0, 0, 0);
    } else {
        struct UIWindow *setup;

        mode_send_message(mode, variant ? MODE_REQ_CLOSE_BUYING : MODE_REQ_CLOSE_SHOP, 0, 0, 0, 0);
        mode_send_message(mode, MODE_REQ_SHOP_CLOSE, 0, 0, 0, 0);
        ui_close_window(own_wnd);
        setup = ui_window_open(variant ? WND_ID_BUYING_SETUP : WND_ID_SHOP_SETUP);
        if (setup) {
            ui_window_set_show(setup, 1);
            ui_window_move(setup, x, y);
        }
    }
    session_clear_own_store_items(&g_session1);
}

intptr_t ui_merchant_item_my_shop_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                               intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND: {
        struct Mode *mode = client_current_mode();

        if (val1 == UI_ID_MY_SHOP_CLOSE) {
            my_shop_close(self);
        } else if (val1 == UI_ID_CHECKBOX) {
            g_session1.noSellWndSound = g_session1.noSellWndSound == 0;
        } else if (val1 == UI_ID_MY_SHOP_END) {
            if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_MY_SHOP_END), UI_MSGBOX_QUESTION, 1, 0, 0,
                                       "메시지", 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES) {
                mode_send_message(mode, MODE_REQ_CLOSE_SHOP_KEEP, self->owner_aid, 0, 0, 0);
                mode_send_message(mode, MODE_REQ_SHOP_CLOSE, 0, 0, 0, 0);
                ui_close_window(self->variant == 1 ? WND_ID_MY_BUYING : WND_ID_MY_SHOP);
            }
        }
        return 0;
    }

    case UI_MSG_RESIZE: {
        int width = val2, height = val3;

        if (val1 != MY_SHOP_RESIZE_EDGE)
            return 0;
        clamp_size(&width, &height, 0x140, 0xf0);
        if (width != wnd->width || height != wnd->height) {
            ui_window_resize_base(wnd, width, height);
            self->base.rows = (unsigned)(height - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
            self->base.columns = (width - ITEM_GRID_X) / ITEM_GRID_CELL;
            ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
            ui_window_move_base(&self->base.scroll_bar->base, wnd->width - 0xf, 0xe);
            ui_scroll_bar_resize(&self->base.scroll_bar->base, ((struct UIWindow *)self->base.scroll_bar)->width,
                     wnd->height - self->bottom - ITEM_GRID_Y);
            ui_window_move_base(self->check, 0xc, wnd->height - (self->variant ? 0xc : 0x12));
            ui_window_move_base(&self->base.close->button.base, wnd->width - 0x3d, wnd->height - 0x18);
            if (self->variant)
                ui_window_move(self->notice, 0xc, wnd->height - 0x1a);
            ui_item_base_wnd_update_scroll(&self->base);
        }
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }

    case SHOP_MSG_REFRESH:
        refill(&self->base, (int)session_shop_item_count(&g_session1), session_shop_item_at);
        ui_window_invalidate_base(&self->base.frame.base);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;

    case SHOP_MSG_SET_KEEP:
        self->owner_aid = val1;
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!info_fits(self->info, 0x118, 0x78))
            own_info_reset(self->info);
        take_info(&self->base, self->info, MY_SHOP_RESIZE_EDGE);
        return 0;

    case SHOP_MSG_TITLE: {
        char text[0x80];

        sprintf(text, "%s : %s", msg_str(self->variant ? MSG_MY_SHOP_TITLE_VARIANT : MSG_MY_SHOP_TITLE_PLAIN),
                    string_cstr(&g_session1.shopTitle));
        ui_static_text_set_text_ellipsis(self->title, text);
        return 0;
    }

    case SHOP_MSG_NOTICE:
        set_notice(wnd, self->notice, val1, 0x1a);
        self->notice_amount = val1;
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

intptr_t ui_merchant_item_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                         intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE)
            ui_close_window(WND_ID_CART);
        return 0;

    case UI_MSG_RESIZE: {
        int width = val2, height = val3;

        if (val1 != MERCHANT_RESIZE_EDGE)
            return 0;
        clamp_size(&width, &height, 0x140, 0xf0);
        if (width != wnd->width || height != wnd->height) {
            ui_window_resize_base(wnd, width, height);
            self->base.columns = (width - ITEM_GRID_X) / ITEM_GRID_CELL;
            self->base.rows = (height - MERCHANT_BOTTOM) / ITEM_GRID_CELL;
            ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
            ui_window_move_base(&self->base.scroll_bar->base, wnd->width - 0xf, 0xe);
            ui_scroll_bar_resize(&self->base.scroll_bar->base, ((struct UIWindow *)self->base.scroll_bar)->width,
                     wnd->height - MERCHANT_BOTTOM);
            ui_item_base_wnd_update_scroll(&self->base);
        }
        ui_window_move(self->status, 0x10, wnd->height - 0x13);
        ui_window_move_base(self->close, wnd->width - 0xe, 3);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }

    case SHOP_MSG_REFRESH:
        refill(&self->base, session_get_cart_size(&g_session1), session_get_cart_item_at);
        ui_window_invalidate_base(&self->base.frame.base);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!info_fits(self->info, 0x118, 0x78))
            own_info_reset(self->info);
        take_info(&self->base, self->info, MERCHANT_RESIZE_EDGE);
        return 0;

    case UI_MSG_ADD_DRAGGED: {
        struct Mode *mode = game_current_base_mode();
        const struct DragInfo *drag = game_current_drag();

        if (drag->source == DRAG_INVENTORY)
            mode_send_message(mode, MODE_REQ_INVENTORY_TO_CART, drag->index, drag->drop_count, 0, 0);
        else if (drag->source == DRAG_STORAGE)
            mode_send_message(mode, MODE_REQ_STORAGE_TO_CART, drag->index, drag->drop_count, 0, 0);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return 0;
    }

    case CART_MSG_STATUS: {
        char text[0x80];

        sprintf(text, "Num: %d/%d Weight: %d/%d", g_session1.cartCount, g_session1.cartCountMax, g_session1.cartWeight, g_session1.cartWeightMax);
        ui_static_text_set_text(self->status, text, 0);
        ui_window_invalidate_base(&self->base.frame.base);
        return 0;
    }
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
