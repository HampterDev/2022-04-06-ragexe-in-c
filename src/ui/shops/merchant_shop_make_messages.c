#include "ui/controls/edit.h"
#include "ui/shops/merchant_shop_make.h"
#include "contents/mcstore_backup.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "game/context.h"
#include <stdlib.h>
#include <string.h>
#include "ui/shops/merchant_shop_make_internal.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "iteminfo/iteminfo.h"
#include "session/session_items.h"
#include "session/session_windows.h"

static void msm_add_dragged(struct UIMerchantShopMakeWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();
    struct ItemInfo item;
    int kind;

    item_info_construct(&item);
    item_info_assign_from_drag(&item, drag);
    kind = drag->source;
    if (kind == DRAG_VENDING_ITEM) {
        int index = drag->index, row, empty, full;
        struct String text;
        struct ItemInfo entry;
        struct UIWindow **edit;

        if (session_get_store_setup_count(&g_session1, index) + item.count > VENDING_STACK_MAX) {
            char line[0x100];

            item.count = VENDING_STACK_MAX - session_get_store_setup_count(&g_session1, index);
            sprintf(line, msg_str(MSG_STACK_LIMIT), VENDING_STACK_MAX);
            ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
            ui_chat_line(line, 0xff);
        }
        if (!item.count)
            goto done;
        full = self->slots <= session_get_store_setup_size(&g_session1);
        if (session_get_store_candidate_count(&g_session1, index)
            && (!full || session_get_store_setup_count(&g_session1, index))) {
            session_sell_list_add(&g_session1, &item, 0, 1);
            session_mirror_remove(&g_session1, &item, 0, 1);
        }

        row = session_get_store_setup_size(&g_session1) - 1;
        string_init(&text);
        empty = 1;
        string_clear(&text);
        session_get_store_setup_item_by_index(&g_session1, &entry, drag->index);
        if (entry.index) {
            struct ListNode *node;

            for (node = self->items.head->next, row = 0; node != self->items.head; node = node->next, row++)
                if (entry.index == LIST_VALUE(node, struct ItemInfo)->index) {
                    empty = 0;
                    string_assign_cstr(&text, ui_edit_ctrl_get_text(self->prices[row]));
                    break;
                }
        }
        if (row < 0)
            row = 0;
        if (row >= VENDING_SLOTS_MAX)
            row = VENDING_SLOTS_MAX - 1;
        edit = &self->prices[row];
        if (*edit) {
            if (self->buying) {
                if (!ui_edit_ctrl_get_text_size(*edit))
                    msm_edit_reset(*edit, "0");
            } else if (!full) {
                ui_edit_ctrl_set_back_colour(*edit, EDIT_LIGHT, EDIT_LIGHT, EDIT_LIGHT);
                ((struct UIEditCtrl *)*edit)->disabled = 0;
                ui_window_invalidate(*edit);
                msm_edit_reset(*edit, empty == 1 ? "0" : string_cstr(&text));
            }
        }
        item_info_destruct(&entry);
        string_free(&text);
    } else if (kind == DRAG_BUYING_ITEM) {
        int row;

        if (self->slots > session_get_store_setup_size(&g_session1)) {
            if (!session_sell_list_add(&g_session1, &item, 1, 1))
                ui_message_box(msg_str(MSG_BUYING_LIST_FULL));
            else
                session_mirror_remove(&g_session1, &item, 1, 1);
        }
        row = session_get_store_setup_size(&g_session1) - 1;
        if (row < 0)
            row = 0;
        if (row >= BUYING_SLOTS_MAX)
            row = BUYING_SLOTS_MAX - 1;
        if (self->prices[row]) {
            if (!ui_edit_ctrl_get_text_size(self->prices[row]))
                msm_edit_reset(self->prices[row], "0");
            else if (!ui_edit_ctrl_get_text_size(self->counts[row]))
                msm_edit_reset(self->counts[row], "0");
        }
    }
done:
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
    item_info_destruct(&item);
}

static void msm_refresh(struct UIMerchantShopMakeWnd *self, int removed)
{
    struct UIWindow *wnd = &self->frame.base;
    int i, n;

    list_clear(&self->items, item_info_destroy);
    n = session_get_store_setup_size(&g_session1);
    for (i = 0; i < n; i++) {
        struct ItemInfo item;

        session_get_store_setup_item_at(&g_session1, &item, i);
        if ((unsigned)item.index > 0)
            item_info_copy(list_push_back(&self->items, NULL, sizeof(struct ItemInfo)), &item);
        item_info_destruct(&item);
    }
    if (removed > 0 && !self->buying) {
        int row = removed - 1;
        struct UIWindow *last;

        if (self->focused >= row)
            self->focused--;
        if (row == (int)self->items.size) {
            msm_edit_amount_colour(wnd, self->prices[row], 0);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->prices[row], "0");
            ui_window_mgr_set_focus_edit(&g_windowMgr, row ? self->prices[row - 1] : self->shop_name);
        } else {
            ui_window_mgr_set_focus_edit(&g_windowMgr, self->prices[row]);
        }
        last = self->prices[self->items.size];
        ui_edit_ctrl_set_back_colour(last, EDIT_GREY, EDIT_GREY, EDIT_GREY);
        ((struct UIEditCtrl *)last)->disabled = 1;
        ui_window_invalidate(last);
        for (i = row; i < self->slots - 1; i++) {
            char buf[0x18];

            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->prices[i], ui_edit_ctrl_get_text(self->prices[i + 1]));
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->prices[i + 1], "0");
            sprintf(buf, "%s ", ui_edit_ctrl_get_text(self->prices[i]));
            session_is_number_text(&g_session1, buf);
            msm_edit_amount_colour(wnd, self->prices[i], _atoi64(buf));
            sprintf(buf, "%s ", ui_edit_ctrl_get_text(self->prices[i + 1]));
            session_is_number_text(&g_session1, buf);
            msm_edit_amount_colour(wnd, self->prices[i + 1], _atoi64(buf));
        }
        ui_window_invalidate_children(wnd);
    }
    ui_merchant_shop_make_wnd_update_scroll(self);
    ui_window_invalidate_base(wnd);
}

static int msm_scroll(struct UIMerchantShopMakeWnd *self, int msg, int lines)
{
    int old = self->first_row;

    if (msg == UI_MSG_PAGE_UP)
        self->first_row = old - self->rows + 1;
    else if (msg == UI_MSG_PAGE_DOWN)
        self->first_row = self->rows - 1 + old;
    else
        self->first_row = old + lines;
    if (old == self->first_row)
        return 0;
    if (self->scroll) {
        int size = self->items.size, over = size - self->fit_rows;

        if (over < 0)
            over = 0;
        if (over) {
            self->scrollable = 1;
            ui_scroll_bar_set_range(self->scroll, over, self->fit_rows, size);
            if (self->first_row < 0)
                self->first_row = 0;
            if (self->first_row > size - self->fit_rows)
                self->first_row = size - self->fit_rows;
            ui_scroll_bar_set_pos(self->scroll, self->first_row);
        } else {
            self->scrollable = 0;
            self->first_row = 0;
            ui_scroll_bar_set_range(self->scroll, 0, self->fit_rows, size);
        }
        ui_window_invalidate_base(&self->scroll->base);
    }
    ui_window_invalidate_base(&self->frame.base);
    return self->first_row - old;
}

static int edit_number(struct UIWindow *edit, char buf[0x18])
{
    sprintf(buf, "%s ", ui_edit_ctrl_get_text(edit));
    return session_is_number_text(&g_session1, buf);
}

static void msm_ok(struct UIMerchantShopMakeWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    struct ListNode *node;
    char buf[0x18];
    int zero_price = 0, over_price = 0, i;

    if (g_windowMgr.chat_room_wnd || g_windowMgr.chat_room_make_wnd) {
        ui_message_box(msg_str(MSG_SETUP_REFUSED));
        if (self->assistant == 1) {
            if (self->buying == 1)
                game_send_message(MODE_REQ_BUYING_SETUP_CANCELLED, 0, 0, 0, 0);
            else
                game_send_message(MODE_REQ_VENDING_SETUP_CANCELLED, 0, 0, 0, 0);
        } else {
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_VENDING_SETUP);
        }
        return;
    }
    if (!self->buying && !ui_merchant_shop_make_wnd_commit_price(self, -1))
        return;
    if (self->buying && !self->items.size) {
        ui_message_box(msg_str(MSG_BUYING_NO_ITEMS));
        return;
    }

    node = self->items.head->next;
    for (i = 0; i < self->slots; i++) {
        __int64 price;

        if (!edit_number(self->prices[i], buf)) {
            ui_message_box(msg_str(MSG_NOT_A_NUMBER));
            return;
        }
        price = _atoi64(buf);
        if (price >= 0x7fffffff) {
            price = 0x7fffffff;
        } else {
            price = _atoi64(buf);
            if (price <= 0)
                price = 0;
        }
        if (self->buying) {
            if (self->items.size <= (unsigned)i)
                break;
            if (price == 0) {
                msm_item_message_box(MSG_BUYING_NO_PRICE, LIST_VALUE(node, struct ItemInfo));
                return;
            }
            if (price > BUYING_PRICE_MAX) {
                msm_item_message_box(MSG_BUYING_PRICE_HIGH, LIST_VALUE(node, struct ItemInfo));
                return;
            }
            node = node->next;
        } else {
            struct ItemInfo item;

            if (!self->items.size) {
                ui_message_box(msg_str(MSG_EMPTY_LIST));
                return;
            }
            session_get_store_setup_item_at(&g_session1, &item, i);
            if ((unsigned)i < self->items.size) {
                if (!item.index) {
                    item_info_destruct(&item);
                    return;
                }
                if (!item.identified) {
                    ui_message_box(msg_str(MSG_NOT_IDENTIFIED));
                    item_info_destruct(&item);
                    return;
                }
                if (price == 0)
                    zero_price = 1;
            }
            if (!g_session1.noPriceCheckWarning && (unsigned)i < self->items.size && price > PRICE_MAX)
                over_price = 1;
            item_info_destruct(&item);
        }
    }
    if (!self->buying) {
        if (zero_price && msm_question_box(msg_str(MSG_ZERO_PRICE)) != UI_MSGBOX_ANSWER_YES)
            return;
        if (!g_session1.noPriceCheckWarning && over_price && msm_question_box(msg_str(MSG_PRICE_TOO_HIGH)) != UI_MSGBOX_ANSWER_YES)
            return;
    }
    if (self->buying) {
        __int64 total;

        node = self->items.head->next;
        for (i = 0; i < self->slots; i++) {
            int count;

            if (!edit_number(self->counts[i], buf)) {
                ui_message_box(msg_str(MSG_NOT_A_NUMBER));
                return;
            }
            if (self->items.size <= (unsigned)i)
                break;
            count = atoi(buf) > 0 ? atoi(buf) : 0;
            if (!count) {
                msm_item_message_box(MSG_BUYING_NO_COUNT, LIST_VALUE(node, struct ItemInfo));
                return;
            }
            if (atoi(buf) + LIST_VALUE(node, struct ItemInfo)->count > BUYING_COUNT_MAX) {
                msm_item_message_box(MSG_BUYING_COUNT_HIGH, LIST_VALUE(node, struct ItemInfo));
                return;
            }
            node = node->next;
        }
        total = _atoi64(ui_edit_ctrl_get_text(self->total));
        if (total > (__int64)g_session1.zeny) {
            ui_chat_line(msg_str(MSG_TOTAL_OVER_ZENY), CHAT_RED);
            return;
        }
        if (total > 0x7fffffff) {
            ui_chat_line(msg_str(MSG_TOTAL_TOO_HIGH), CHAT_RED);
            return;
        }
    }

    if (!ui_edit_ctrl_get_text_size(self->shop_name)) {
        ui_message_box(msg_str(MSG_NO_TITLE));
        return;
    }
    for (i = 0; i < self->slots; i++) {
        int price = 0;

        edit_number(self->prices[i], buf);
        if (ui_edit_ctrl_get_text_size(self->prices[i]))
            price = atoi(buf);
        if (g_serviceType == 2 && !ui_edit_ctrl_get_text_size(self->prices[i])
            && ui_edit_ctrl_get_text(self->prices[i])) {
            ui_message_box(msg_str(MSG_NOT_A_NUMBER));
            return;
        }
        session_set_store_setup_price_at(&g_session1, i, price);
    }
    if (self->buying) {
        for (i = 0; i < self->slots; i++) {
            int count = 0;

            edit_number(self->counts[i], buf);
            if (ui_edit_ctrl_get_text_size(self->counts[i]))
                count = atoi(buf);
            if (g_serviceType == 2 && !ui_edit_ctrl_get_text_size(self->counts[i])
                && ui_edit_ctrl_get_text(self->counts[i])) {
                ui_message_box(msg_str(MSG_NOT_A_NUMBER));
                return;
            }
            session_set_store_setup_count_at(&g_session1, i, count);
        }
        if (self->assistant == 1) {
            ui_merchant_shop_make_wnd_set_waiting(self, 1);
        } else {
            __int64 total = _atoi64(ui_edit_ctrl_get_text(self->total));

            mode_send_message(mode, MODE_REQ_OPEN_BUYING, (int)ui_edit_ctrl_get_text(self->shop_name), 1, (int)total,
                          0);
        }
    } else if (self->assistant == 1) {
        ui_merchant_shop_make_wnd_set_waiting(self, 1);
    } else {
        mode_send_message(mode, MODE_REQ_OPEN_VENDING, (int)ui_edit_ctrl_get_text(self->shop_name), 1, 0, 0);
    }

    owned_text_assign(&g_session1.mcStoreBackup.title,
        text_view_cstr(ui_edit_ctrl_get_text(self->shop_name)));
}

static void msm_cancel(struct UIMerchantShopMakeWnd *self, int val2, int val3, int val4)
{
    struct Mode *mode = game_current_base_mode();

    if (self->buying) {
        if (self->assistant == 1) {
            if (self->waiting == 1)
                ui_merchant_shop_make_wnd_set_waiting(self, 0);
            else
                mode_send_message(mode, MODE_REQ_BUYING_SETUP_CANCELLED, 0, 0, 0, 0);
        } else {
            mode_send_message(mode, MODE_REQ_OPEN_BUYING, 0, 0, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_BUYING_SETUP);
        }
    } else if (self->assistant == 1) {
        if (self->waiting == 1)
            ui_merchant_shop_make_wnd_set_waiting(self, 0);
        else
            mode_send_message(mode, MODE_REQ_VENDING_SETUP_CANCELLED, 0, 0, 0, 0);
    } else {
        mode_send_message(mode, MODE_REQ_OPEN_VENDING, (int)ui_edit_ctrl_get_text(self->shop_name), 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_VENDING_SETUP);
    }
}

int ui_merchant_shop_make_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    (void)sender;
    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_OK:
            msm_ok(self);
            break;
        case UI_ID_CANCEL:
            msm_cancel(self, val2, val3, val4);
            break;
        case ID_PRICE_CHECK:
            g_session1.noPriceCheckWarning = !g_session1.noPriceCheckWarning;
            break;
        case ID_SHOP_INFO:
            if (!ui_window_find(WND_ID_SHOP_INFO))
                ui_window_open(WND_ID_SHOP_INFO);
            break;
        case ID_LOAD_BACKUP: {
            int result = mcstore_request_load(&g_session1.mcStoreBackup, g_session1.ownAid,
                g_session1.ownGid, string_cstr(&g_session1.worldName), self->buying);
            if (result == 1)
                ui_merchant_shop_make_wnd_apply_backup(self);
            else
                ui_chat_line(result == 2 ? "No local shop setup has been saved yet." :
                    "Could not load local shop setup. Existing file preserved.", 0xffff);
            break;
        }
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        return msm_scroll(self, msg, val1);
    case MSM_MSG_REFRESH:
        msm_refresh(self, val1);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!merchant_shop_make_info_is_valid(self->info))
            merchant_shop_make_info_set_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    case UI_MSG_ADD_DRAGGED:
        msm_add_dragged(self);
        return 0;
    }
    return 0;
}

void ui_merchant_shop_make_wnd_print_sale_list(struct UIMerchantShopMakeWnd *self)
{
    struct ListNode *node = self->items.head->next;
    struct String line = {0}, name = {0}, number = {0};
    char money2[32], money[32], commission[64];
    unsigned i;

    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE,
        (int)"======================  SALE LIST  ======================", CHAT_SALE_FRAME, 0, 0);
    for (i = 0; i < self->items.size; i++, node = node->next) {
        const struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);
        __int64 fee;

        string_clear(&name);
        if (info->refine) item_name_append_refine(info, &name);
        if (item_info_has_card(info)) {
            item_name_append_card_prefixes(info, &name);
            string_append_cstr(&name, item_info_get_name(info));
            item_name_append_card_postfixes(info, &name);
            item_name_append_sockets(info, &name);
        } else if (item_info_has_maker_slot(info)) {
            string_append_cstr(&name, item_info_get_maker_star_text(info));
            string_append_cstr(&name, item_info_get_maker_name(info));
            string_append_cstr(&name, item_info_get_maker_element_text(info));
            string_append_cstr(&name, item_info_get_name(info));
        } else {
            string_append_cstr(&name, item_info_get_name(info));
            item_name_append_sockets(info, &name);
        }
        string_format(&number, "%s ", ui_edit_ctrl_get_text(self->prices[i]));
        session_is_number_text(&g_session1, number.data);
        if (atoi(number.data) > PRICE_MAX)
            strcpy(money, "1,000,000,000");
        else
            strcpy(money, format_money(atoi(number.data), money2));
        string_format(&line, "    %s  :  %s  Zeny", string_cstr(&name), money);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)string_cstr(&line), CHAT_SALE_LINE, 0, 0);
        fee = (__int64)((double)_atoi64(number.data) * ((double)zeny_fee_percent(_atoi64(number.data)) * 0.01));
        format_money64(fee, commission, sizeof commission);
        string_format(&line, "Commission : 1 EA = %s Zeny", commission);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)string_cstr(&line), CHAT_SALE_LINE, 0, 0);
    }
    strcpy(money, format_money(g_session1.zeny, money2));
    string_format(&line, "                                          Current Zeny : %s Z", money);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)string_cstr(&line), CHAT_SALE_ZENY, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE,
        (int)"============================ ============================", CHAT_SALE_FRAME, 0, 0);
    string_free(&line);
    string_free(&name);
    string_free(&number);
}

int merchant_shop_make_info_is_valid(struct WindowInfo *self)
{
    return self->width == 0x118 && self->height == 0xdc && self->x >= 0
           && self->x <= g_renderer->width - 0x118 && self->y >= 0
           && self->y <= g_renderer->height - 0xdc;
}

void merchant_shop_make_info_set_default(struct WindowInfo *self)
{
    self->x = 0;
    self->y = 0xf0;
    self->width = 0x118;
    self->height = 0xdc;
}

#define MODE_REQ_ASSISTANT_VENDING 0x128
#define MODE_REQ_ASSISTANT_BUYING 0x12a

void ui_merchant_shop_make_wnd_set_assistant_items(struct UIMerchantShopMakeWnd *self, int count,
                                                        const void *items)
{
    struct UIWindow *wnd;

    self->assistant = 1;
    if (self->buying == 1) {
        wnd = g_windowMgr.buying_store_mirror_item_wnd;
    } else {
        session_fill_store_candidates(&g_session1, count, items);
        wnd = g_windowMgr.merchant_mirror_item_wnd;
    }
    if (wnd)
        ui_window_send_msg(wnd, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

int ui_merchant_shop_make_wnd_is_waiting(const struct UIMerchantShopMakeWnd *self)
{
    return self->waiting == 1;
}

void ui_merchant_shop_make_wnd_set_chosen_cell(struct UIMerchantShopMakeWnd *self, int x, int y)
{
    self->cell_x = (short)x;
    self->cell_y = (short)y;
}

int ui_merchant_shop_make_wnd_get_chosen_cell(const struct UIMerchantShopMakeWnd *self, int *x, int *y)
{
    if (self->cell_x < 0 || self->cell_y < 0)
        return 0;
    *x = self->cell_x;
    *y = self->cell_y;
    return 1;
}

void ui_merchant_shop_make_wnd_request_assistant(struct UIMerchantShopMakeWnd *self)
{
    struct Mode *mode = game_current_base_mode();

    if (self->buying == 1)
        mode_send_message(mode, MODE_REQ_ASSISTANT_BUYING, (int)ui_edit_ctrl_get_text(self->shop_name),
                      atoi(ui_edit_ctrl_get_text(self->total)), 0, 0);
    else
        mode_send_message(mode, MODE_REQ_ASSISTANT_VENDING, (int)ui_edit_ctrl_get_text(self->shop_name), 0, 0, 0);
}

#define MSG_BUY_LIST_LINE 0x6c4

void ui_merchant_shop_make_wnd_apply_backup(struct UIMerchantShopMakeWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct { int price, count; } rows[VENDING_SLOTS_MAX];
    size_t row_count;
    int i, n;

    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->shop_name, owned_text_cstr(&g_session1.mcStoreBackup.title));
    ui_window_invalidate(self->shop_name);
    for (i = 0; i < self->slots; i++) {
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->prices[i], "0");
        if (self->buying) {
            ui_edit_ctrl_set_back_colour(self->prices[i], EDIT_LIGHT, EDIT_LIGHT, EDIT_LIGHT);
        } else {
            ((struct UIEditCtrl *)self->prices[i])->disabled = 1;
            ui_window_invalidate(self->prices[i]);
            ui_edit_ctrl_set_back_colour(self->prices[i], EDIT_GREY, EDIT_GREY, EDIT_GREY);
        }
    }

    n = session_get_store_setup_size(&g_session1);
    for (i = 0; i < n; i++) {
        struct ItemInfo item;

        session_get_store_setup_item_at(&g_session1, &item, i);
        session_mirror_add(&g_session1, &item, self->buying, 0);
        item_info_destruct(&item);
    }
    session_clear_store_setup(&g_session1);
    mcstore_restore_shop(&g_session1.mcStoreBackup, self->buying != 0, self->slots);

    row_count = g_session1.mcStoreBackup.item_count;
    if (row_count > VENDING_SLOTS_MAX) row_count = VENDING_SLOTS_MAX;
    for (i = 0; (size_t)i < row_count; ++i) {
        rows[i].price = g_session1.mcStoreBackup.items[i].price;
        rows[i].count = g_session1.mcStoreBackup.items[i].count;
    }
    for (i = 0; i < self->slots && (size_t)i < row_count; ++i) {
        int price = rows[i].price;
        char text[0x18];

        _itoa(price, text, 10);
        ((struct UIEditCtrl *)self->prices[i])->disabled = 0;
        ui_window_invalidate(self->prices[i]);
        ui_edit_ctrl_set_back_colour(self->prices[i], EDIT_LIGHT, EDIT_LIGHT, EDIT_LIGHT);
        ui_edit_ctrl_select_all(self->prices[i]);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->prices[i], text);
        if (self->buying && i < BUYING_SLOTS_MAX) {
            char count[0x18];

            _itoa(rows[i].count, count, 10);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->counts[i], count);
            ui_window_invalidate_base(self->counts[i]);
        } else {
            char money[0x18];

            msm_edit_amount_colour(wnd, self->prices[i], price);
            format_money64(price, money, sizeof money);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->prices[i], money);
            self->last_size = ui_edit_ctrl_get_text_size(self->prices[i]);
        }
    }
    session_notify_store_setup_window(&g_session1, self->buying != 0, 0);
    session_notify_store_candidate_window(&g_session1, self->buying);
    self->load_button->button.disabled = 0;
    ui_window_invalidate_base(&self->load_button->button.base);
    mcstore_clear_items(&g_session1.mcStoreBackup);
}

void ui_merchant_shop_make_wnd_print_buy_list(struct UIMerchantShopMakeWnd *self)
{
    struct ListNode *node = self->items.head->next;
    struct String line = {0}, number = {0};
    char money2[32], price_text[32], count_text[32], commission[64];
    unsigned i;

    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE,
        (int)"======================  BUY LIST  ======================", CHAT_SALE_FRAME, 0, 0);
    for (i = 0; i < self->items.size; i++, node = node->next) {
        const char *name = item_info_get_name(LIST_VALUE(node, struct ItemInfo));
        __int64 fee;
        double rate;

        string_format(&number, "%s ", ui_edit_ctrl_get_text(self->prices[i]));
        session_is_number_text(&g_session1, number.data);
        if (atoi(number.data) > BUYING_PRICE_MAX)
            strcpy(price_text, "99,990,000");
        else
            strcpy(price_text, format_money(atoi(number.data), money2));
        rate = (double)zeny_fee_percent(_atoi64(number.data)) * 0.01;
        fee = (__int64)((double)_atoi64(number.data) * rate);

        string_format(&number, "%s ", ui_edit_ctrl_get_text(self->counts[i]));
        session_is_number_text(&g_session1, number.data);
        if (atoi(number.data) > BUYING_COUNT_MAX)
            strcpy(count_text, "9,999");
        else
            strcpy(count_text, format_money(atoi(number.data), money2));
        string_format(&line, msg_str(MSG_BUY_LIST_LINE), name, price_text, count_text);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)string_cstr(&line), CHAT_SALE_LINE, 0, 0);
        format_money64(fee, commission, sizeof commission);
        string_format(&line, "Commission : 1 EA = %s Zeny", commission);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)string_cstr(&line), CHAT_SALE_LINE, 0, 0);
    }
    strcpy(price_text, format_money(g_session1.zeny, money2));
    string_format(&line, "                                          Current Zeny : %s Z", price_text);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)string_cstr(&line), CHAT_SALE_ZENY, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE,
        (int)"============================ ============================", CHAT_SALE_FRAME, 0, 0);
    string_free(&line);
    string_free(&number);
}
