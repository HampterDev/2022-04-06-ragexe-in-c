#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include "ui/wndmgr/make_window_internal.h"
#include "contents/enchant_wnd.h"
#include "contents/item_reform_wnd.h"
#include "contents/lapine_box.h"
#include "contents/lapine_upgrade.h"
#include "core/msg_str.h"
#include "ui/items/item_list.h"
#include "ui/items/refine_controls.h"
#include "ui/items/refining_internal.h"
#include "ui/items/styling_shop.h"
#include "ui/quests/quests.h"
#include "ui/shops/barter_ex_internal.h"
#include "ui/shops/cash_shop.h"
#include "ui/shops/merchant.h"
#include "ui/shops/merchant_shop.h"
#include "ui/shops/merchant_shop_make_internal.h"
#include "ui/shops/ncash_wnd.h"
#include "ui/shops/roulette.h"
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
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)(INT_PTR)msg_str(msg), 0xff, 0, 0);
    return NULL;
}

static void wm_close_rodex(struct UIWindowMgr *self, int msg)
{
    if (self->rodex_inbox_wnd == NULL)
        return;
    ui_window_mgr_post_quit1(self, 0x108);
    ui_window_mgr_post_quit1(self, 0x109);
    ui_window_send_msg(self->rodex_inbox_wnd, NULL, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
    ui_window_mgr_send_msg_(self, UIWM_MSG_CHAT_LINE, (int)(INT_PTR)msg_str(msg), 0xff, 0, 0);
}

static void wm_close_bank(struct UIWindowMgr *self, int msg)
{
    if (self->bank_new_wnd == NULL)
        return;
    ui_window_mgr_post_quit1(&g_windowMgr, 0x113);
    ui_window_mgr_send_msg_(self, UIWM_MSG_CHAT_LINE, (int)(INT_PTR)msg_str(msg), 0xff, 0, 0);
}

static struct UIWindow *make_refining_wnd(struct UIWindowMgr *self)
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

struct UIWindow *wm_make_trade(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    switch (window_id) {
    case 0xb8:
        wnd = self->n_cash_wnd;
        if (wnd == NULL) {
            wnd = u_in_cash_wnd_new();
            self->n_cash_wnd = wnd;
            ui_window_create(wnd, 0x280, 0x21c);
            wm_move_640(wnd, 0, 0x1e);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(wnd, &self->n_cash_info);
            snap_mgr_register(&g_snapMgr, wnd);
        }
        wm_show(wnd);
        break;
    case 0x21:
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
        break;
    case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98:
    case 0x135:
        wnd = make_store_page(self, window_id);
        break;
    case 0x99:
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
        break;
    case 0x29:
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
        break;
    case 0x2a:
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
        break;
    case 0x28:
        if (!map_property_mgr_get(g_mapFlags, g_session1_state, 0x28)) {
            ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)(INT_PTR)msg_str(0x94c), 0xff, 0, 0);
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
        break;
    case 0x1f:
        wnd = WM_OPEN(exchange_wnd, ui_exchange_wnd_new, 0x230, 0x179, WM_AT_640, 0x64, 0x64);
        break;
    case 0x4a:
        wnd = WM_OPEN_INFO(item_composition_wnd, ui_item_composition_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8, item_composition_info);
        break;
    case 0x49:
        wnd = WM_OPEN_INFO(item_identify_wnd, ui_item_identify_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8, item_identify_info);
        break;
    case 0x19:
        wnd = WM_OPEN(choose_sell_buy_wnd, ui_choose_sell_buy_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c);
        break;
    case 0x18:
        wnd = WM_OPEN_INFO(item_sell_wnd, ui_item_sell_wnd_new, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, item_sell_info);
        break;
    case 0xa1:
        wnd = WM_OPEN_INFO(item_sell_wnd2, ui_item_sell_wnd2_new, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, item_sell_info);
        break;
    case 0x2c:
        wnd = WM_OPEN_INFO1(merchant_item_purchase_wnd, ui_merchant_item_purchase_wnd_new, 0, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, merchant_item_purchase_info);
        break;
    case 0x17:
        wnd = WM_OPEN_INFO(item_purchase_wnd, ui_item_purchase_wnd_new, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, item_purchase_info);
        ui_window_mgr_make_window(self, 0x32);
        break;
    case 0x2d:
        wnd = WM_OPEN_INFO1(merchant_item_my_shop_wnd, ui_merchant_item_my_shop_wnd_new, 0, 0x118, 0x78, WM_AT_XY, 0, 0xc8, merchant_item_my_shop_info);
        break;
    case 0x2b:
        wnd = WM_OPEN_INFO1(merchant_item_shop_wnd, ui_merchant_item_shop_wnd_new, 0, 0x118, 0x78, WM_AT_XY, 0, 0xc8, merchant_item_shop_info);
        break;
    case 0x16:
        wnd = WM_OPEN_INFO(item_shop_wnd, ui_item_shop_wnd_new, 0x118, 0x78, WM_AT_XY, 0, 0xc8, item_shop_info);
        break;
    case 0xa0:
        wnd = WM_OPEN_INFO(item_shop_wnd2, ui_item_shop_wnd2_new, 0x118, 0x78, WM_AT_XY, 0, 0xc8, item_shop_info);
        break;
    case 0xfe:
        wnd = WM_OPEN_INFO(para_item_shop_wnd, ui_para_item_shop_wnd_new, 0x118, 0x78, WM_AT_XY, 0, 0xc8, item_shop_info);
        break;
    case 0xff:
        wnd = WM_OPEN_INFO(para_item_purchase_wnd, ui_para_item_purchase_wnd_new, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, item_purchase_info);
        break;
    case 0x100:
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
        break;
    case 0x6b:
        wnd = WM_OPEN(item_repair_wnd, ui_item_repair_wnd_new, 0x118, 0x96, WM_AT_640_UP, 0xb9, 0x12c);
        break;
    case 0x6f:
        wnd = WM_OPEN(weapon_refine_wnd, ui_weapon_refine_wnd_new, 0x118, 0x96, WM_AT_640_UP, 0xb9, 0x12c);
        break;
    case 0xae:
        wnd = WM_OPEN_INFO1(buying_store_make_wnd, ui_merchant_shop_make_wnd_new, 1, 0x168, 0x121, WM_AT_640, 0xb9, 0xc8, buying_store_make_info);
        ui_window_mgr_make_window(self, 0xaf);
        break;
    case 0xaf:
        wnd = WM_OPEN_INFO1(buying_store_mirror_item_wnd, ui_merchant_mirror_item_wnd_new, 1, 0x118, 0x78, WM_AT_640, 0, 0x78, buying_store_mirror_item_info);
        break;
    case 0xb0:
        wnd = WM_OPEN_INFO1(buying_store_item_my_shop_wnd, ui_merchant_item_my_shop_wnd_new, 1, 0x118, 0x78, WM_AT_XY, 0, 0xc8, buying_store_item_my_shop_info);
        break;
    case 0xb1:
        wnd = WM_OPEN_INFO1(buying_store_item_shop_wnd, ui_merchant_item_shop_wnd_new, 1, 0x15e, 0x78, WM_AT_XY, 0, 0xc8, buying_store_item_shop_info);
        break;
    case 0xb2:
        wnd = WM_OPEN_INFO1(buying_store_item_sell_wnd, ui_merchant_item_purchase_wnd_new, 1, 0x118, 0x78, WM_AT_XY, 0x12c, 0xc8, buying_store_item_sell_info);
        break;
    case 0xb3:
        wnd = WM_OPEN_INFO1(buying_store_mirror_sell_list_wnd, ui_merchant_mirror_item_wnd_new, 2, 0x118, 0x78, WM_AT_XY, 0x12c, 0, buying_store_mirror_sell_list_info);
        break;
    case 0xe3:
        wnd = ui_window_mgr_get_window(self, 0xe3);
        if (wnd)
            break;
        wnd = merge_item_wnd_new();
        ui_window_create(wnd, 1, 1);
        wm_show(wnd);
        {
            int y = wm_times_half(g_renderer->height);

            wm_move(wnd, wm_times_half(g_renderer->width), y);
        }
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0xe6:
        if (ui_window_mgr_get_window(self, 0xe6) == NULL)
            ui_window_mgr_post_quit1(self, 0xe6);
        wnd = (struct UIWindow *)ui_merchant_item_purchase_msg_wnd_new();
        if (wnd == NULL)
            return NULL;
        ui_window_create(wnd, 0x12c, 0x96);
        wm_move(wnd, wm_center_x(wnd->width), wm_center_y(wnd->height));
        wm_show(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x113:
        if (ui_window_mgr_get_window(self, 0x127))
            return wm_refuse(0xbc6);
        if (ui_window_mgr_get_window(self, 0x157))
            return wm_refuse(0xe85);
        if (ui_window_mgr_get_window(self, 0x15b))
            return wm_refuse(0xf05);
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
        break;
    case 0x101:
    case 0x102:
        wnd = ui_window_mgr_get_window(self, window_id);
        if (wnd)
            break;
        wnd = ui_merchant_item_log_wnd_new(window_id == 0x102);
        ui_window_create(wnd, 0x1c2, 0x98);
        wm_move(wnd, 0, 0xc8);
        ui_window_set_show(wnd, 0);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x10b:
        wnd = ui_window_mgr_get_window(self, 0x10b);
        if (wnd == NULL) {
            struct BitmapRes *res;

            wnd = ui_mileage_wnd_new();
            res = ui_skin_bitmap(UI_SKIN "basic_interface\\mileage_bg1.bmp");
            if (res == NULL) {
                ui_window_create(wnd, 0x96, 0x46);
                wm_move(wnd, g_renderer->width - 0x1e0, 0xc);
            } else {
                ui_window_create(wnd, res->width, res->height);
                if (self->n_cash_call_wnd)
                    wm_move(wnd, g_renderer->width - 0x124, 0xb);
                else
                    wm_move(wnd, g_renderer->width - res->width - 0x154, 0xc);
            }
            ui_window_mgr_add_window(self, wnd);
        }
        ui_window_set_show(wnd, g_session1.showGoldPcIcon);
        break;
    case 0x10c:
        wnd = self->open_roullette_wnd;
        if (wnd == NULL) {
            wnd = ui_open_roullette_wnd_new();
            self->open_roullette_wnd = wnd;
            ui_window_create(wnd, 0x2b, 0x2b);
            wm_move(wnd, g_renderer->width - 0xbb, 0x47);
            ui_window_mgr_add_window(self, wnd);
        }
        ui_window_set_show(wnd, g_session1.showRouletteIcon);
        break;
    case 0x10d:
        wnd = self->roullette_wnd;
        if (wnd == NULL) {
            wnd = ui_roullette_wnd_new();
            self->roullette_wnd = wnd;
            ui_window_create(wnd, 0x280, 0x1f1);
            {
                int y = (int)((double)g_renderer->height * 0.5 - 240.0);

                wm_move(wnd, (int)((double)g_renderer->width * 0.5 - 320.0), y);
            }
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x119:
        wnd = ui_window_mgr_get_window(self, 0x119);
        if (wnd == NULL) {
            wnd = ui_styling_shop_wnd_new();
            ui_window_create(wnd, 0x190, 0xfa);
            wm_move(wnd, wm_center_x(0x190), wm_center_y(0xfa));
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x122:
        if (ui_window_mgr_get_window(self, 0x122))
            return NULL;
        wnd = lapine_box_new();
        ui_window_create(wnd, 0x190, 0xfa);
        wm_move(wnd, wm_center_x(0x190), wm_center_y(0xfa));
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x127:
        wnd = make_refining_wnd(self);
        break;
    case 0x12e:
        wnd = ui_window_mgr_get_window(self, 0x12e);
        if (wnd)
            break;
        wnd = lapine_upgrade_new();
        ui_window_create(wnd, 0x190, 0xfa);
        wm_move(wnd, wm_center_x(0x190), wm_center_y(0xfa));
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x13e:
        wnd = ui_window_mgr_get_window(self, 0x13e);
        if (wnd)
            break;
        wnd = ui_cash_shop_wnd_new();
        self->cash_shop_wnd = wnd;
        ui_window_create(wnd, 0x2d3, 0x21c);
        {
            int y = wm_times_half(g_renderer->height - 0x21c);

            wm_move(wnd, wm_times_half(g_renderer->width - 0x2d3), y);
        }
        wm_set_info(wnd, NULL);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x14e:
        wnd = ui_window_mgr_get_window(self, 0x14e);
        if (wnd)
            break;
        wnd = ui_bartermarket_wnd_new();
        self->bartermarket_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x190);
        wm_move(wnd, 0, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->item_shop_info);
        break;
    case 0x14f:
        wnd = ui_window_mgr_get_window(self, 0x14f);
        if (wnd)
            break;
        wnd = ui_barter_item_purchase_wnd_new();
        self->barter_item_purchase_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        wm_move(wnd, 0x12c, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->item_purchase_info);
        break;
    case 0x155:
        if (ui_window_mgr_get_window(self, 0x113)) {
            ui_window_mgr_post_quit1(&g_windowMgr, 0x113);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)(INT_PTR)msg_str(0xf7e), 0xff, 0,
                                  0);
        }
        wnd = ui_window_mgr_get_window(self, 0x155);
        if (wnd)
            break;
        wnd = ui_expanded_bartermarket_wnd_new();
        self->expanded_bartermarket_wnd = wnd;
        ui_window_create(wnd, 0x16e, 0x16e);
        wm_move(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x156:
        if (ui_window_mgr_get_window(self, 0x113))
            return NULL;
        if (ui_window_mgr_get_window(self, 0x156)) {
            wnd = ui_window_mgr_get_window(self, 0x155);
            break;
        }
        wnd = ui_expanded_barter_item_purchase_wnd_new();
        self->expanded_barter_item_purchase_wnd = wnd;
        ui_window_create(wnd, 0x16e, 0x16e);
        wm_move(wnd, 0x24a, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x14d:
        wnd = ui_window_mgr_get_window(self, 0x14d);
        if (wnd)
            break;
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
        break;
    case 0x157:
    {
        int x = (int)((float)(g_renderer->width - 0x190) * 0.5f - 100.0f);
        int y = (int)((float)(g_renderer->height - 0x14a) * 0.5f);

        ui_window_mgr_make_window(self, 0x8);
        if (self->item_wnd)
            wm_move(self->item_wnd, x + 0x190, y - self->item_wnd->height + 0x14a);
        wm_close_rodex(self, 0xe82);
        wm_close_bank(self, 0xe84);
        wnd = ui_window_mgr_get_window(self, 0x157);
        if (wnd)
            break;
        wnd = ui_grade_enchant_wnd_new();
        ui_window_create(wnd, 0x190, 0x14a);
        wm_move(wnd, x, y);
        ui_window_mgr_add_window(self, wnd);
        break;
    }
    case 0x15b:
    {
        int x = (int)((float)(g_renderer->width - 0x2b2) * 0.5f);
        int y = (int)((float)(g_renderer->height - 0x1d6) * 0.5f);

        wnd = ui_window_mgr_get_window(self, 0x15b);
        if (wnd == NULL) {
            wnd = enchant_wnd_new();
            ui_window_create(wnd, 0x2b2, 0x1d6);
            wm_move(wnd, x, y);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_close_rodex(self, 0xf02);
        wm_close_bank(self, 0xf04);
        break;
    }
    case 0x15c:
        wnd = ui_window_mgr_get_window(self, 0x15c);
        if (wnd)
            break;
        wnd = item_reform_wnd_new();
        ui_window_create(wnd, 0x1b8, 0x104);
        {
            int y = wm_half_y(0x104);

            wm_move(wnd, wm_half_x(0x1b8), y);
        }
        ui_window_mgr_add_window(self, wnd);
        break;
    default:
        return WM_NOT_MINE;
    }
    return wm_register(self, window_id, wnd);
}
