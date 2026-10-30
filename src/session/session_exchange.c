#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_windows.h"
#include "ui/wndmgr/wndmgr.h"

int g_exchangeGiveBack;

static void exchange_clear(struct Session *self)
{
    int i;

    self->myTradeZeny = 0;
    for (i = 0; i < 10; i++)
        self->exchangeA[i].count = 0;
}

void session_end_exchange(struct Session *self)
{
    if (g_exchangeGiveBack) {
        int i;
        for (i = 0; i < 10; i++)
            if (self->exchangeA[i].count != 0)
                session_add_inventory_item(self, &self->exchangeA[i], 1);
        self->zeny += self->myTradeZeny;
        session_refresh_basic_info(self);
        ui_window_mgr_invalidate_once_per_frame(&g_windowMgr);
    }
    exchange_clear(self);
}

void session_end_exchange2(struct Session *self)
{
    if (g_exchangeGiveBack) {
        BYTE merged = 0;
        int i;
        for (i = 0; i < 10; i++) {
            struct ItemInfo *slot = &self->exchangeA[i];
            struct List *list = &self->itemList;
            struct ListNode *node;

            if (slot->count == 0)
                continue;
            for (node = list->head->next; node != list->head; node = node->next)
                if (LIST_VALUE(node, struct ItemInfo)->index == slot->index)
                    break;
            if (node != list->head) {
                LIST_VALUE(node, struct ItemInfo)->count += slot->count;
                merged = 1;
            } else {
                item_info_copy(list_push_back(list, NULL, sizeof(struct ItemInfo)), slot);
            }
        }
        if (merged)
            ui_window_mgr_refresh_item_windows(&g_windowMgr);
        self->zeny += self->myTradeZeny;
    }
    exchange_clear(self);
}

void session_clear_their_trade(struct Session *self)
{
    int i;

    self->partnerTradeZeny = 0;
    for (i = 0; i < 10; i++)
        self->partnerTradeItems[i].count = 0;
}

void session_clear_my_trade(struct Session *self)
{
    exchange_clear(self);
}

BYTE session_is_exist_index_in_my_change_list(struct Session *self, int index)
{
    int i;
    for (i = 0; i < 10; i++)
        if (self->exchangeA[i].count != 0 && self->exchangeA[i].index == index)
            return 1;
    return 0;
}

void session_set_my_exchange_money(struct Session *self, int money)
{
    struct UIWindow *wnd;

    self->cartType = money;
    wnd = g_windowMgr.equip_wnd;
    if (wnd)
        ui_window_send(wnd, NULL, 0x3c, 0, 0, 0, 0);
}

void session_set_other_exchange_money(struct Session *self, int money)
{
    self->myTradeZeny = money;
    notify_item_window(&g_windowMgr.exchange_wnd);
}

int session_is_exchange_a_full(struct Session *self)
{
    int i, n = 0;

    for (i = 0; i < 10; i++)
        if (self->exchangeA[i].count != 0)
            n++;
    return n == 10;
}

BYTE session_add_my_trade_item(struct Session *self, struct ItemInfo item)
{
    struct ItemInfo copy;
    BYTE result = 0;
    int i;

    if (!session_is_exchange_a_full(self)) {
        session_remove_inventory_item(self, item.index, item.count);
        item_info_copy(&copy, &item);
        for (i = 0; i < 10; i++)
            if (self->exchangeA[i].count != 0 && self->exchangeA[i].index == copy.index)
                break;
        if (i < 10) {
            self->exchangeA[i].count += copy.count;
        } else {
            for (i = 0; i < 10; i++)
                if (self->exchangeA[i].count == 0) {
                    item_info_assign(&self->exchangeA[i], &copy);
                    break;
                }
        }
        notify_item_window(&g_windowMgr.exchange_wnd);
        string_free(&copy.name);
        string_free(&copy.id_text);
        result = 1;
    }
    string_free(&item.name);
    string_free(&item.id_text);
    return result;
}

void session_set_their_trade_zeny(struct Session *self, int money)
{
    self->partnerTradeZeny = money;
    notify_item_window(&g_windowMgr.exchange_wnd);
}

void session_add_their_trade_item(struct Session *self, struct ItemInfo item)
{
    int i;

    for (i = 0; i < 10; i++)
        if (self->partnerTradeItems[i].count == 0) {
            item_info_assign(&self->partnerTradeItems[i], &item);
            break;
        }
    notify_item_window(&g_windowMgr.exchange_wnd);
    string_free(&item.name);
    string_free(&item.id_text);
}
