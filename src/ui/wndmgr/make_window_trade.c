#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include "ui/wndmgr/make_window_internal.h"
#include "ui/wndmgr/window_registry.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "core/msg_str.h"
#include "ui/items/item_list.h"
#include "ui/items/refine_controls.h"
#include "ui/items/refining_internal.h"
#include "ui/items/styling_shop.h"
#include "ui/quests/quests.h"
#include "ui/shops/barter_ex_internal.h"
#include "ui/shops/merchant.h"
#include "ui/shops/merchant_shop.h"
#include "ui/shops/merchant_shop_make_internal.h"
#include "ui/shops/storage.h"
#include "ui/shops/storage_internal.h"
#include "ui/social/social.h"

static struct UIWindow *make_store_page(struct UIWindowMgr *self, int window_id)
{
    int page = window_id == 0x135 ? 7 : window_id - 0x92;
    struct UIWindow *wnd = self->item_store_sub_wnd[page];
    struct String title;
    int title_id;

    if (wnd == NULL) {
        wnd = ui_item_store_sub_wnd_new();
        self->item_store_sub_wnd[page] = wnd;
        ui_item_store_sub_wnd_set_category((struct UIItemStoreSubWnd *)wnd, page);
        if (window_id == 0x135)
            title_id = 0x5bf;
        else if (window_id == 0x96)
            title_id = 0xc4f;
        else if (window_id == 0x97 || window_id == 0x98)
            title_id = page + 0x5b8;
        else
            title_id = page + 0x5b9;
        ui_window_create(wnd, 0x8c, 0x91);
        string_init(&title);
        string_assign_cstr(&title, msg_str(title_id));
        ui_item_store_sub_wnd_set_title((struct UIItemStoreSubWnd *)self->item_store_sub_wnd[page], title);
        if (window_id == 0x135)
            wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->item_store_sub_wnd[page], &self->item_store_info.sub_info[page]);
    }
    wm_show(wnd);
    return wnd;
}

static struct UIWindow *wm_refuse(int msg)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(msg), 0xff, 0, 0);
    return NULL;
}

static void wm_close_rodex(struct UIWindowMgr *self, int msg)
{
    if (self->rodex_inbox_wnd == NULL)
        return;
    ui_window_mgr_post_quit1(self, 0x108);
    ui_window_mgr_post_quit1(self, 0x109);
    ui_window_send_msg(self->rodex_inbox_wnd, NULL, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
    ui_window_mgr_send_msg_(self, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(msg), 0xff, 0, 0);
}

static void wm_close_bank(struct UIWindowMgr *self, int msg)
{
    if (self->bank_new_wnd == NULL)
        return;
    ui_window_mgr_post_quit1(&g_windowMgr, 0x113);
    ui_window_mgr_send_msg_(self, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(msg), 0xff, 0, 0);
}

static struct UIWindow *make_refining_wnd(struct UIWindowMgr *self, int window_id)
{
    int x = (int)((float)(g_renderer->width - 0x108) * 0.5f);
    int y = (int)((float)(g_renderer->height - 0x15a) * 0.5f);
    struct UIWindow *wnd;

    ui_window_mgr_make_window(self, 0x8);
    if (self->item_wnd)
        wm_move(self->item_wnd, x + 0x108, y - self->item_wnd->height + 0x15a);
    wm_close_rodex(self, 0xbaf);
    wm_close_bank(self, 0xbc5);
    wnd = ui_window_mgr_get_window(self, 0x127);
    if (wnd)
        return wnd;
    wnd = ui_refining_wnd_new();
    ui_window_create(wnd, 0x108, 0x15a);
    wm_move(wnd, x, y);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x17: post_quit1 */
static int close_item_purchase_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->item_purchase_wnd) {
        mode_send_message(client_current_mode(), 0x123, 0, 0, 0, 0);
        closed = wm_close_slot(self, &self->item_purchase_wnd);
    }
    return closed;
}

/* 0x18: post_quit1 */
static int close_item_sell_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->item_sell_wnd) {
        mode_send_message(client_current_mode(), 0x123, 0, 0, 0, 0);
        closed = wm_close_slot(self, &self->item_sell_wnd);
    }
    return closed;
}

/* 0x19: post_quit1 */
static int close_choose_sell_buy_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->choose_sell_buy_wnd) {
        mode_send_message(client_current_mode(), 0x123, 0, 0, 0, 0);
        closed = wm_close_slot(self, &self->choose_sell_buy_wnd);
    }
    return closed;
}

/* 0x21 */
static struct UIWindow *make_item_store_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->item_store_wnd;
    if (wnd == NULL) {
        wnd = ui_item_store_wnd_new();
        self->item_store_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x1ca);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->item_store_wnd, &self->item_store_info);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x28 */
static struct UIWindow *make_merchant_item_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (!map_property_mgr_get(g_mapFlags, g_session1.mapType, 0x28)) {
        ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(0x94c), 0xff, 0, 0);
        return NULL;
    }
    wnd = self->merchant_item_wnd;
    if (wnd == NULL) {
        wnd = ui_merchant_item_wnd_new();
        self->merchant_item_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640(wnd, 0xc8, 0x78);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->merchant_item_info);
    }
    wm_show(wnd);
    self->merchant_item_info.show = 1;
    return wnd;
}

/* 0x28: post_quit1 */
static int close_merchant_item_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->merchant_item_wnd)) {
        self->merchant_item_info.show = 0;
        closed = 1;
    }
    return closed;
}

/* 0x29 */
static struct UIWindow *make_merchant_shop_make_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->merchant_shop_make_wnd;
    if (wnd == NULL) {
        wnd = ui_merchant_shop_make_wnd_new(0);
        self->merchant_shop_make_wnd = wnd;
        ui_window_create(wnd, 0x190, 0x104);
        wm_move_640(wnd, 0xb9, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->merchant_shop_make_info);
    }
    wm_show(wnd);
    ui_window_mgr_make_window(self, 0x2a);
    return wnd;
}

/* 0x29: post_quit1 */
static int close_merchant_shop_make_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    ui_window_mgr_post_quit1(self, 0x2a);
    closed = wm_close_slot(self, &self->merchant_shop_make_wnd);
    return closed;
}

/* 0x2a */
static struct UIWindow *make_merchant_mirror_item_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->merchant_mirror_item_wnd;
    if (wnd == NULL) {
        wnd = ui_merchant_mirror_item_wnd_new(0);
        self->merchant_mirror_item_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640(wnd, 0, 0x78);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->merchant_mirror_item_info);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98: post_quit1 */
static int close_store_page(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->item_store_sub_wnd[window_id - 0x92])) {
        closed = 1;
        if (self->item_store_wnd)
            ui_item_store_wnd_set_page_check((struct UIItemStoreWnd *)self->item_store_wnd, window_id - 0x92, 0);
    }
    return closed;
}

/* 0x99 */
static struct UIWindow *make_item_store_find_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->item_store_find_wnd;
    if (wnd == NULL) {
        struct String title;

        wnd = ui_item_store_find_wnd_new();
        self->item_store_find_wnd = wnd;
        ui_window_create(wnd, 0x8c, 0x91);
        string_init(&title);
        string_assign_cstr(&title, msg_str(0x447));
        ui_item_store_find_wnd_set_title((struct UIItemStoreFindWnd *)self->item_store_find_wnd, title);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->item_store_find_wnd, &self->item_store_find_info);
    }
    wm_show(wnd);
    return wnd;
}

/* 0xae */
static struct UIWindow *make_buying_store_make_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = WM_OPEN_INFO1(buying_store_make_wnd, ui_merchant_shop_make_wnd_new, 1, 0x168, 0x121, WM_AT_640, 0xb9, 0xc8, buying_store_make_info);
    ui_window_mgr_make_window(self, 0xaf);
    return wnd;
}

/* 0xae: post_quit1 */
static int close_buying_store_make_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    ui_window_mgr_post_quit1(self, 0xaf);
    closed = wm_close_slot(self, &self->buying_store_make_wnd);
    return closed;
}

/* 0xe3 */
static struct UIWindow *make_merge_item_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0xe3);
    if (wnd)
        return wnd;
    wnd = merge_item_wnd_new();
    ui_window_create(wnd, 1, 1);
    wm_show(wnd);
    {
        int y = wm_times_half(g_renderer->height);

        wm_move(wnd, wm_times_half(g_renderer->width), y);
    }
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0xe6 */
static struct UIWindow *make_merchant_item_purchase_msg_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (ui_window_mgr_get_window(self, 0xe6) == NULL)
        ui_window_mgr_post_quit1(self, 0xe6);
    wnd = (struct UIWindow *)ui_merchant_item_purchase_msg_wnd_new();
    if (wnd == NULL)
        return NULL;
    ui_window_create(wnd, 0x12c, 0x96);
    wm_move(wnd, wm_center_x(wnd->width), wm_center_y(wnd->height));
    wm_show(wnd);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0xff: post_quit1 */
static int close_para_item_purchase_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->para_item_purchase_wnd) {
        mode_send_message(client_current_mode(), 0x124, 0, 0, 0, 0);
        closed = wm_close_slot(self, &self->para_item_purchase_wnd);
    }
    return closed;
}

/* 0x100 */
static struct UIWindow *make_para_result_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->para_result_wnd;
    if (wnd == NULL) {
        wnd = ui_para_result_wnd_new();
        self->para_result_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        wm_move(wnd, 0x12c, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, NULL);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x100: post_quit1 */
static int close_para_result_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->para_result_wnd) {
        struct Mode *game = game_current_base_mode();
        if (game)
            mode_send_message(game, 0x124, 0, 0, 0, 0);
        closed = wm_close_slot(self, &self->para_result_wnd);
    }
    return closed;
}

/* 0x101, 0x102 */
static struct UIWindow *make_merchant_item_log_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, window_id);
    if (wnd)
        return wnd;
    wnd = ui_merchant_item_log_wnd_new(window_id == 0x102);
    ui_window_create(wnd, 0x1c2, 0x98);
    wm_move(wnd, 0, 0xc8);
    ui_window_set_show(wnd, 0);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x113 */
static struct UIWindow *make_bank_new_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (ui_window_mgr_get_window(self, 0x127))
        return wm_refuse(0xbc6);
    if (ui_window_mgr_get_window(self, 0x157))
        return wm_refuse(0xe85);
    if (ui_window_mgr_get_window(self, 0x155))
        return wm_refuse(0xf7f);
    wnd = self->bank_new_wnd;
    if (wnd == NULL) {
        wnd = ui_bank_new_wnd_new();
        self->bank_new_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x99);
        {
            int y = (int)((double)g_renderer->height * 0.5 - 77.0);

            wm_move(wnd, (int)((double)g_renderer->width * 0.5 - 140.0), y);
        }
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x119 */
static struct UIWindow *make_styling_shop_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x119);
    if (wnd == NULL) {
        wnd = ui_styling_shop_wnd_new();
        ui_window_create(wnd, 0x190, 0xfa);
        wm_move(wnd, wm_center_x(0x190), wm_center_y(0xfa));
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x135: post_quit1 */
static int close_last_store_page(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->item_store_sub_wnd[7])) {
        closed = 1;
        if (self->item_store_wnd)
            ui_item_store_wnd_set_page_check((struct UIItemStoreWnd *)self->item_store_wnd, 7, 0);
    }
    return closed;
}

/* 0x14d */
static struct UIWindow *make_note_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x14d);
    if (wnd)
        return wnd;
    wnd = ui_note_wnd_new();
    ui_window_create(wnd, 0x166, 0x113);
    if (self->merchant_shop_make_wnd) {
        struct UIWindow *shop = self->merchant_shop_make_wnd;

        wm_move(wnd, shop->x + 5 + shop->width, shop->y);
    } else {
        int y = wm_times_half(g_renderer->height - 0x113);

        wm_move(wnd, wm_times_half(g_renderer->width - 0x166), y);
    }
    ui_window_send_msg(wnd, NULL, UI_MSG_SET_INFO, 0xc95, 0, 0, 0);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x14e */
static struct UIWindow *make_bartermarket_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x14e);
    if (wnd)
        return wnd;
    wnd = ui_bartermarket_wnd_new();
    self->bartermarket_wnd = wnd;
    ui_window_create(wnd, 0x118, 0x190);
    wm_move(wnd, 0, 0xc8);
    ui_window_mgr_add_window(self, wnd);
    wm_set_info(wnd, &self->item_shop_info);
    return wnd;
}

/* 0x14f */
static struct UIWindow *make_barter_item_purchase_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_get_window(self, 0x14f);
    if (wnd)
        return wnd;
    wnd = ui_barter_item_purchase_wnd_new();
    self->barter_item_purchase_wnd = wnd;
    ui_window_create(wnd, 0x118, 0x78);
    wm_move(wnd, 0x12c, 0xc8);
    ui_window_mgr_add_window(self, wnd);
    wm_set_info(wnd, &self->item_purchase_info);
    return wnd;
}

/* 0x14f: post_quit1 */
static int close_barter_item_purchase_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->barter_item_purchase_wnd) {
        if (client_current_mode())
            mode_send_message(client_current_mode(), 0x135, 0, 0, 0, 0);
        closed = wm_close_slot(self, &self->barter_item_purchase_wnd);
    }
    return closed;
}

/* 0x155 */
static struct UIWindow *make_expanded_bartermarket_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (ui_window_mgr_get_window(self, 0x113)) {
        ui_window_mgr_post_quit1(&g_windowMgr, 0x113);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(0xf7e), 0xff, 0,
                              0);
    }
    wnd = ui_window_mgr_get_window(self, 0x155);
    if (wnd)
        return wnd;
    wnd = ui_expanded_bartermarket_wnd_new();
    self->expanded_bartermarket_wnd = wnd;
    ui_window_create(wnd, 0x16e, 0x16e);
    wm_move(wnd, 0xc8, 0xc8);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x155: post_quit1 */
static int close_expanded_bartermarket_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->expanded_bartermarket_wnd)) {
        closed = 1;
        if (self->expanded_barter_item_purchase_wnd)
            ui_window_mgr_post_quit1(self, 0x156);
    }
    return closed;
}

/* 0x156 */
static struct UIWindow *make_expanded_barter_item_purchase_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (ui_window_mgr_get_window(self, 0x113))
        return NULL;
    if (ui_window_mgr_get_window(self, 0x156)) {
        wnd = ui_window_mgr_get_window(self, 0x155);
        return wnd;
    }
    wnd = ui_expanded_barter_item_purchase_wnd_new();
    self->expanded_barter_item_purchase_wnd = wnd;
    ui_window_create(wnd, 0x16e, 0x16e);
    wm_move(wnd, 0x24a, 0xc8);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x156: post_quit1 */
static int close_expanded_barter_item_purchase_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->expanded_barter_item_purchase_wnd) {
        if (client_current_mode())
            mode_send_message(client_current_mode(), 0x142, 0, 0, 0, 0);
        ui_window_mgr_post_quit2(self, self->expanded_barter_item_purchase_wnd);
        self->expanded_barter_item_purchase_wnd = NULL;
        closed = 1;
        if (self->expanded_bartermarket_wnd)
            ui_window_mgr_post_quit1(self, 0x155);
    }
    return closed;
}

/* The rows of the storage, shop, vending, buying-store, bank, barter and refining windows.
   window_table.h describes the columns. In ID order. */
static const struct WmWindowRow k_trade_rows[] = {
    WM_ROW_OPEN_INFO(0x16, item_shop_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_item_shop_wnd_new, 0x118, 0x78, WM_AT_XY, 0, 0xc8, item_shop_info),
    WM_ROW_OPEN_INFO(0x17, item_purchase_wnd, WM_UNTRACKED, close_item_purchase_wnd, ui_item_purchase_wnd_new, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, item_purchase_info),
    WM_ROW_OPEN_INFO(0x18, item_sell_wnd, WM_UNTRACKED, close_item_sell_wnd, ui_item_sell_wnd_new, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, item_sell_info),
    WM_ROW_OPEN(0x19, choose_sell_buy_wnd, 0, close_choose_sell_buy_wnd, ui_choose_sell_buy_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c),
    WM_ROW_OPEN(0x1f, exchange_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_exchange_wnd_new, 0x230, 0x179, WM_AT_640, 0x64, 0x64),
    WM_ROW(0x21, WM_SLOT(item_store_wnd), WM_UNTRACKED | WM_GET_SLOT, make_item_store_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x28, WM_SLOT(merchant_item_wnd), 0, make_merchant_item_wnd, close_merchant_item_wnd),
    WM_ROW(0x29, WM_SLOT(merchant_shop_make_wnd), 0, make_merchant_shop_make_wnd, close_merchant_shop_make_wnd),
    WM_ROW(0x2a, WM_SLOT(merchant_mirror_item_wnd), WM_UNTRACKED, make_merchant_mirror_item_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN_INFO1(0x2b, merchant_item_shop_wnd, 0, WM_CLOSE_SLOT, ui_merchant_item_shop_wnd_new, 0, 0x118, 0x78, WM_AT_XY, 0, 0xc8, merchant_item_shop_info),
    WM_ROW_OPEN_INFO1(0x2c, merchant_item_purchase_wnd, 0, WM_CLOSE_SLOT, ui_merchant_item_purchase_wnd_new, 0, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, merchant_item_purchase_info),
    WM_ROW_OPEN_INFO1(0x2d, merchant_item_my_shop_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_merchant_item_my_shop_wnd_new, 0, 0x118, 0x78, WM_AT_XY, 0, 0xc8, merchant_item_my_shop_info),
    WM_ROW_OPEN_INFO(0x49, item_identify_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_item_identify_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8, item_identify_info),
    WM_ROW_OPEN_INFO(0x4a, item_composition_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_item_composition_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8, item_composition_info),
    WM_ROW_OPEN(0x6b, item_repair_wnd, WM_UNTRACKED, WM_CLOSE_UNCOUNTED, ui_item_repair_wnd_new, 0x118, 0x96, WM_AT_640_UP, 0xb9, 0x12c),
    WM_ROW_OPEN(0x6f, weapon_refine_wnd, WM_UNTRACKED, WM_CLOSE_UNCOUNTED, ui_weapon_refine_wnd_new, 0x118, 0x96, WM_AT_640_UP, 0xb9, 0x12c),
    WM_ROW(0x92, WM_SLOT(item_store_sub_wnd[0]), 0, make_store_page, close_store_page),
    WM_ROW(0x93, WM_SLOT(item_store_sub_wnd[1]), 0, make_store_page, close_store_page),
    WM_ROW(0x94, WM_SLOT(item_store_sub_wnd[2]), 0, make_store_page, close_store_page),
    WM_ROW(0x95, WM_SLOT(item_store_sub_wnd[3]), 0, make_store_page, close_store_page),
    WM_ROW(0x96, WM_SLOT(item_store_sub_wnd[4]), 0, make_store_page, close_store_page),
    WM_ROW(0x97, WM_SLOT(item_store_sub_wnd[5]), 0, make_store_page, close_store_page),
    WM_ROW(0x98, WM_SLOT(item_store_sub_wnd[6]), 0, make_store_page, close_store_page),
    WM_ROW(0x99, WM_SLOT(item_store_find_wnd), 0, make_item_store_find_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN_INFO(0xa0, item_shop_wnd2, WM_UNTRACKED, WM_CLOSE_SLOT, ui_item_shop_wnd2_new, 0x118, 0x78, WM_AT_XY, 0, 0xc8, item_shop_info),
    WM_ROW_OPEN_INFO(0xa1, item_sell_wnd2, WM_UNTRACKED, WM_CLOSE_SLOT, ui_item_sell_wnd2_new, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, item_sell_info),
    WM_ROW(0xae, WM_SLOT(buying_store_make_wnd), 0, make_buying_store_make_wnd, close_buying_store_make_wnd),
    WM_ROW_OPEN_INFO1(0xaf, buying_store_mirror_item_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_merchant_mirror_item_wnd_new, 1, 0x118, 0x78, WM_AT_640, 0, 0x78, buying_store_mirror_item_info),
    WM_ROW_OPEN_INFO1(0xb0, buying_store_item_my_shop_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_merchant_item_my_shop_wnd_new, 1, 0x118, 0x78, WM_AT_XY, 0, 0xc8, buying_store_item_my_shop_info),
    WM_ROW_OPEN_INFO1(0xb1, buying_store_item_shop_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_merchant_item_shop_wnd_new, 1, 0x15e, 0x78, WM_AT_XY, 0, 0xc8, buying_store_item_shop_info),
    WM_ROW_OPEN_INFO1(0xb2, buying_store_item_sell_wnd, 0, WM_CLOSE_SLOT, ui_merchant_item_purchase_wnd_new, 1, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, buying_store_item_sell_info),
    WM_ROW_OPEN_INFO1(0xb3, buying_store_mirror_sell_list_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_merchant_mirror_item_wnd_new, 2, 0x118, 0x78, WM_AT_XY, 0x12c, 0, buying_store_mirror_sell_list_info),
    WM_ROW(0xe3, WM_NO_SLOT, WM_UNTRACKED, make_merge_item_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0xe6, WM_NO_SLOT, 0, make_merchant_item_purchase_msg_wnd, WM_CLOSE_WINDOW),
    WM_ROW_OPEN_INFO(0xfe, para_item_shop_wnd, 0, WM_CLOSE_SLOT, ui_para_item_shop_wnd_new, 0x118, 0x78, WM_AT_XY, 0, 0xc8, item_shop_info),
    WM_ROW_OPEN_INFO(0xff, para_item_purchase_wnd, 0, close_para_item_purchase_wnd, ui_para_item_purchase_wnd_new, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, item_purchase_info),
    WM_ROW(0x100, WM_SLOT(para_result_wnd), WM_KEPT_BY_CLEAR, make_para_result_wnd, close_para_result_wnd),
    WM_ROW(0x101, WM_NO_SLOT, WM_UNTRACKED, make_merchant_item_log_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x102, WM_NO_SLOT, WM_UNTRACKED, make_merchant_item_log_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x113, WM_SLOT(bank_new_wnd), WM_GET_SLOT, make_bank_new_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x119, WM_NO_SLOT, 0, make_styling_shop_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x127, WM_NO_SLOT, WM_UNTRACKED, make_refining_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x135, WM_SLOT(item_store_sub_wnd[7]), 0, make_store_page, close_last_store_page),
    WM_ROW(0x14d, WM_NO_SLOT, 0, make_note_wnd, WM_CLOSE_WINDOW),
    WM_ROW(0x14e, WM_SLOT(bartermarket_wnd), 0, make_bartermarket_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x14f, WM_SLOT(barter_item_purchase_wnd), 0, make_barter_item_purchase_wnd, close_barter_item_purchase_wnd),
    WM_ROW(0x155, WM_SLOT(expanded_bartermarket_wnd), 0, make_expanded_bartermarket_wnd, close_expanded_bartermarket_wnd),
    WM_ROW(0x156, WM_SLOT(expanded_barter_item_purchase_wnd), 0, make_expanded_barter_item_purchase_wnd, close_expanded_barter_item_purchase_wnd),
};

const struct WmWindowRows wm_trade_windows = WM_WINDOW_ROWS(k_trade_rows);
