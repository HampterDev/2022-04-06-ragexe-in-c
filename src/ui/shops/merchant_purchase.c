#include "ui/window_records.h"
#include "ui/shops/shops.h"
#include "world/tooltip.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/shops/merchant.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"
#include "ui/wndmgr/wndmgr.h"

struct UIMerchantItemPurchaseWnd;
void ui_merchant_item_purchase_wnd_destroy(struct UIWindow *window);
void ui_merchant_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_merchant_item_purchase_wnd_store_info(struct UIWindow *self);
void ui_merchant_item_purchase_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_merchant_item_purchase_wnd_on_draw(struct UIWindow *self);
void ui_merchant_item_purchase_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_merchant_item_purchase_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
int ui_merchant_item_purchase_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_merchant_item_purchase_wnd_ops = {
    .destroy = ui_merchant_item_purchase_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_merchant_item_purchase_wnd_drag_and_drop,
    .store_info = ui_merchant_item_purchase_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_merchant_item_purchase_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_merchant_item_purchase_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_merchant_item_purchase_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_merchant_item_purchase_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_merchant_item_purchase_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_merchant_item_purchase_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_merchant_item_purchase_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

static const char *const sell_button[3] = { "btn_sell", "btn_sell_a", "btn_sell_b" };

struct UIMerchantItemPurchaseWnd *ui_merchant_item_purchase_wnd_construct(struct UIMerchantItemPurchaseWnd *self,
                                                                                 int variant)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_merchant_item_purchase_wnd_ops;
    self->variant = variant;
    self->base.button_bar = 1;
    self->request_arg1 = 0;
    self->info = NULL;
    self->hover = -1;
    session_clear_purchase_list(&g_session1);
    self->fixed_height = 0;
    return self;
}

struct UIWindow *ui_merchant_item_purchase_wnd_new(int arg)
{
    return (struct UIWindow *)ui_merchant_item_purchase_wnd_construct(mem_alloc(sizeof(struct UIMerchantItemPurchaseWnd)), arg);
}

void ui_merchant_item_purchase_wnd_destruct(struct UIMerchantItemPurchaseWnd *self)
{
    self->base.frame.base.ops = &ui_merchant_item_purchase_wnd_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_merchant_item_purchase_wnd_destroy(struct UIWindow *window)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    ui_merchant_item_purchase_wnd_destruct(self);
    mem_free(self);
}

void ui_merchant_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    struct Mode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    if (drag->source == DRAG_SHOP_ITEM) {
        switch (drag->type) {
        case 4: case 5: case 8: case 9: case 0xb: case 0xc: case 0xd: case 0xe: case 0xf:
            if (session_get_store_purchase_count(&g_session1, drag->index)) {
                mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
                ui_chat_line(msg_str(MSG_ALREADY_LISTED), REFUSAL_COLOUR);
                return;
            }
            ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
            return;
        default:
            ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
            ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, COUNT_FOR_PURCHASE, 0, 0, 0);
            return;
        }
    }
    if (drag->source == DRAG_MIRROR_2) {
        if (drag->drop_count > 1) {
            ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
            ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, COUNT_FOR_PURCHASE_MIRROR, 0, 0, 0);
            return;
        }
        ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

void ui_merchant_item_purchase_wnd_store_info(struct UIWindow *window)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    merchant_store_info(&self->base.frame.base, self->info);
}

void ui_merchant_item_purchase_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIBitmapButton **buttons = (struct UIBitmapButton **)&self->base.close;
    struct UIWindow *total = mem_alloc(sizeof(struct UIStaticText));
    char text[0x50];

    (void)width;
    if (total)
        total = ui_static_text_construct(total);
    self->total = total;
    ui_window_create(total, 0xa0, 0xf);
    sprintf(text, "Total : %d Zeny", 0);
    ui_static_text_set_text_fit(self->total, text);
    ui_window_move(self->total, 0xc, wnd->height - 0x15);
    ui_window_add_child(wnd, self->total);

    shop_add_title(wnd, 0xa0, self->variant ? MSG_PURCHASE_TITLE_VARIANT : MSG_PURCHASE_TITLE);
    buttons[0] = shop_add_button(wnd, self->variant ? sell_button : g_shopBuyButton, wnd->width - 0x6a,
                                 wnd->height - 0x18, UI_ID_OK);
    buttons[1] = shop_add_button(wnd, g_shopCancelButton, wnd->width - 0x3d, wnd->height - 0x18, UI_ID_CANCEL);

    self->base.rows = (height - 0x26) / ITEM_GRID_CELL;
    self->base.columns = 1;
    self->fixed_height = wnd->height - self->base.rows * ITEM_GRID_CELL;
    ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_merchant_item_purchase_wnd_on_draw(struct UIWindow *window)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    struct UIItemBaseWnd *base = &self->base;
    struct UIWindow *wnd = &base->frame.base;
    struct ListNode *node;
    int rows, i, cell = 0;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    shop_draw_frame(base, wnd->height - 0x26, base->button_bar ? 2 : 0);
    rows = (wnd->height - 0x26) / ITEM_GRID_CELL;
    shop_draw_cells(wnd, rows);
    shop_fill_names(wnd, rows);

    for (node = base->items.head->next, i = 0; node != base->items.head; node = node->next, i++) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);
        char text[0x40], money[0x40];
        int row_y, col_x, len;

        if (item_grid_hidden(info))
            continue;
        row_y = cell / base->columns * ITEM_GRID_CELL;
        col_x = cell % base->columns * ITEM_GRID_CELL;
        if (self->hover == i)
            ui_window_fill_rect(wnd, col_x + 0x48, row_y + 0x13, 0xbf, 0x1c, ui_skin_colour(6, 2));
        ui_window_draw_item_icon_count(wnd, col_x + 0x2c, row_y + 0x15, info, 0, 0);
        ui_window_draw_item_name_short(wnd, col_x + 0x4c, row_y + 0x1d, 0x5a, info, 12);
        if (info->grade > 0) {
            struct String path;

            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", (int)info->grade);
            ui_window_draw_bitmap(wnd, col_x + 0x2c, row_y + 0x21,
                                res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
            string_free(&path);
        }
        if (info->price != info->charged_price)
            len = sprintf(text, " %d -> %s Zeny", info->price, format_money(info->charged_price, money));
        else
            len = sprintf(text, " %s Zeny", format_money(info->price, money));
        ui_window_text_out_right(wnd, wnd->width - 0x12, row_y + 0x1d, text, len, 0, 12, 0, 0);
        cell++;
    }
}

void ui_merchant_item_purchase_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    struct ItemInfo info;
    int old;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);
    old = self->hover;
    self->hover = ui_item_base_wnd_index_at(&self->base, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(&self->base.frame.base);
    if (g_replayPlaying)
        return;
    ui_item_base_wnd_get_item(&self->base, &info, self->hover);
    if ((unsigned)info.index > 0) {
        struct GameMode *mode = game_current_mode();
        struct ItemInfo listed;

        session_purchase_item_by_id(&g_session1, &listed, string_cstr(&info.id_text));
        item_info_assign(&info, &listed);
        item_info_destruct(&listed);
        game_mode_drag_item(mode, self->variant ? DRAG_BUYING_STORE_SELL : DRAG_VENDING_PURCHASE, &info,
                                info.count, -1);
    }
    item_info_destruct(&info);
}

void ui_merchant_item_purchase_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    struct ItemInfo info;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if ((unsigned)info.index > 0) {
        struct String text;
        struct String name = {0};
        char money[0x30];
        int tip_x, tip_y;
        struct Mode *mode;

        shop_tooltip_cell(&self->base.frame.base, x, y, &tip_x, &tip_y);
        mode = client_current_mode();
        item_info_get_display_name(&info, &name);
        string_init(&text);
        string_format(&text, "%s %d %s %s Zeny", string_cstr(&name), info.count, msg_str(MSG_TIP_COUNT_WORD),
                      format_money(info.charged_price, money));
        mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&info), 0);
        string_free(&text);
        string_free(&name);
    }
    item_info_destruct(&info);
}

static void purchase_refuse(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
    ui_chat_line(text, REFUSAL_COLOUR);
}

static void purchase_command(struct UIMerchantItemPurchaseWnd *self, int id)
{
    struct Mode *mode = client_current_mode();

    if (id == UI_ID_OK) {
        if (self->variant) {
            const struct ListNode *node;
            int sum = 0;

            for (node = self->base.items.head->next; node != self->base.items.head; node = node->next) {
                const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);

                if ((unsigned)item->index > 0)
                    sum += item->charged_price * item->count;
            }
            if (sum > ((const struct UIMerchantItemShopWnd *)g_windowMgr.buying_store_item_shop_wnd)->notice_amount) {
                ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_OVER_LIMIT), UI_MSGBOX_OK, 1, 0, 0,
                                       "메시지", 0x118, 0x78, 0);
                return;
            }
            mode_send_message(mode, MODE_REQ_PURCHASE_VARIANT, self->request_arg1, self->request_arg2, 0, 0);
        } else {
            ui_merchant_item_purchase_msg_wnd_set_purchase((struct UIMerchantItemPurchaseMSGWnd *)ui_window_open(WND_ID_PURCHASE_CONFIRM), self->request_arg1, self->request_arg2);
        }
        mode_send_message(mode, MODE_REQ_SHOP_CLOSE, 0, 0, 0, 0);
        if (self->variant) {
            ui_close_window(WND_ID_BUYING_STORE_SHOP);
            ui_close_window(WND_ID_BUYING_STORE_SELL);
            ui_close_window(WND_ID_BUYING_STORE_MIRROR);
        } else {
            ui_close_window(WND_ID_VENDING_SHOP);
            ui_close_window(WND_ID_VENDING_PURCHASE);
        }
        return;
    }
    if (id == UI_ID_CANCEL) {
        if (self->variant) {
            ui_close_window(WND_ID_BUYING_STORE_SHOP);
            ui_close_window(WND_ID_BUYING_STORE_SELL);
            ui_close_window(WND_ID_BUYING_STORE_MIRROR);
        } else {
            ui_close_window(WND_ID_VENDING_SHOP);
            ui_close_window(WND_ID_VENDING_PURCHASE);
        }
        mode_send_message(mode, MODE_REQ_SHOP_CLOSE, 0, 0, 0, 0);
        session_clear_purchase_lists(&g_session1);
    }
}

static void purchase_resize(struct UIMerchantItemPurchaseWnd *self, int width, int height)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow **buttons = (struct UIWindow **)&self->base.close;

    if (width > 0x140)
        width = 0x140;
    if (height > 0x150)
        height = 0x150;
    if (width < 0xe8)
        width = 0xe8;
    if (height < 0x78)
        height = 0x78;
    width = (width - 0x118) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x118;
    height = (height - 0x78) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x78;
    if (width != wnd->width || height != wnd->height) {
        ui_window_resize_base(wnd, width, height);
        self->base.rows = (height - 0x26) / ITEM_GRID_CELL;
    }
    ui_window_move(self->total, 0xc, wnd->height - 0x14);
    ui_window_move(buttons[0], wnd->width - 0x6a, wnd->height - 0x18);
    ui_window_move(buttons[1], wnd->width - 0x3d, wnd->height - 0x18);
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

static void purchase_refresh(struct UIMerchantItemPurchaseWnd *self)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow **buttons = (struct UIWindow **)&self->base.close;
    char text[0x40], money[0x40];
    int count, i, total = 0;
    unsigned rows;

    list_clear(&self->base.items, item_info_destroy);
    count = session_purchase_count(&g_session1);
    for (i = 0; i < count; i++) {
        struct ItemInfo info;

        session_purchase_item(&g_session1, &info, i);
        if ((unsigned)info.index > 0) {
            total += info.count * info.charged_price;
            item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), &info);
        }
        item_info_destruct(&info);
    }
    sprintf(text, "Total : %s Zeny", format_money(total, money));
    ui_static_text_set_text_fit(self->total, text);
    rows = self->base.items.size > PURCHASE_MIN_ROWS ? self->base.items.size : PURCHASE_MIN_ROWS;
    self->base.rows = rows;
    ui_window_resize_base(wnd, wnd->width, rows * ITEM_GRID_CELL + self->fixed_height);
    for (i = 0; i < 2; i++)
        ui_window_move(buttons[i], buttons[i]->x, wnd->height - 0x18);
    ui_window_move(self->total, 0xc, wnd->height - 0x15);
    ui_window_invalidate_base(&self->base.frame.base);
}

static void purchase_add_dragged(struct UIMerchantItemPurchaseWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();
    struct ItemInfo dragged, listed, stocked;
    char text[0x100];
    int count, i, already = 0, stock = 0, wanted;

    item_info_construct(&dragged);
    item_info_assign_from_drag(&dragged, drag);
    item_info_construct(&listed);
    count = session_purchase_count(&g_session1);
    for (i = 0; i < count; i++) {
        struct ItemInfo tmp;

        session_purchase_item(&g_session1, &tmp, i);
        item_info_assign(&listed, &tmp);
        item_info_destruct(&tmp);
        if (listed.index == dragged.index) {
            already = listed.count;
            break;
        }
    }
    item_info_construct(&stocked);
    count = session_shop_list_count(&g_session1);
    for (i = 0; i < count; i++) {
        struct ItemInfo tmp;
        int same;

        session_shop_list_item(&g_session1, &tmp, i);
        item_info_assign(&stocked, &tmp);
        item_info_destruct(&tmp);
        if (self->variant)
            same = atoi(string_cstr(&stocked.id_text)) == atoi(string_cstr(&dragged.id_text));
        else
            same = stocked.index == dragged.index;
        if (same) {
            stock = stocked.count;
            break;
        }
    }

    wanted = dragged.count + already;
    if (stock < wanted) {
        dragged.count = stock - already;
        wanted = stock;
        sprintf(text, msg_str(self->variant ? MSG_ONLY_IN_STOCK_VARIANT : MSG_ONLY_IN_STOCK), stock);
        purchase_refuse(text);
    }
    if (wanted > PURCHASE_MAX_COUNT) {
        dragged.count = PURCHASE_MAX_COUNT - already;
        sprintf(text, msg_str(self->variant ? MSG_AT_MOST_VARIANT : MSG_AT_MOST), PURCHASE_MAX_COUNT);
        purchase_refuse(text);
    }
    if (dragged.count > 0) {
        __int64 listed_total = session_purchase_total(&g_session1);

        if (self->variant) {
            int price = (int)((unsigned)drag->charged_price * (unsigned)dragged.count);

            if ((__int64)g_session1.zeny + price + listed_total > 0x7fffffff) {
                purchase_refuse(msg_str(MSG_ZENY_OVERFLOW));
            } else {
                session_purchase_add(&g_session1, &dragged);
                session_mirror_remove(&g_session1, &dragged, 2, 1);
                ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
                if (g_windowMgr.buying_store_mirror_sell_list_wnd)
                    ui_window_send_msg(g_windowMgr.buying_store_mirror_sell_list_wnd, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
            }
        } else if ((__int64)g_session1.zeny < (__int64)dragged.count * drag->charged_price + listed_total) {
            purchase_refuse(msg_str(MSG_NOT_ENOUGH_ZENY));
        } else {
            session_purchase_add(&g_session1, &dragged);
            ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        }
    }
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
    item_info_destruct(&stocked);
    item_info_destruct(&listed);
    item_info_destruct(&dragged);
}

static void purchase_load_records(struct UIMerchantItemPurchaseWnd *self, const char *buf, unsigned size)
{
    struct UIRecordHeader hdr;
    unsigned pos = sizeof hdr;

    memcpy(&hdr, buf, sizeof hdr);
    if (hdr.tag != REC_MERCHANT_PURCHASE_BEGIN && hdr.tag != REC_PURCHASE_VARIANT_BEGIN)
        return;
    while (size > pos) {
        memcpy(&hdr, buf + pos, sizeof hdr);
        pos += sizeof hdr;
        if (hdr.tag == REC_PURCHASE_VARIANT_END) {
            ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, PURCHASE_MSG_SET_REQUEST, 0, 0, 0, 0);
            ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
            return;
        }
        if (hdr.tag == REC_MERCHANT_PURCHASE_END) {
            ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
            return;
        }
        pos += hdr.len;
    }
}

int ui_merchant_item_purchase_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                 int msg, int val1, int val2, int val3, int val4)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        ui_window_record_command(&self->base.frame.base, val1, val2, val3, val4);
        purchase_command(self, val1);
        return 0;

    case UI_MSG_RESIZE:
        if (val1 == PURCHASE_RESIZE_EDGE)
            purchase_resize(self, val2, val3);
        return 0;

    case SHOP_MSG_REFRESH:
        purchase_refresh(self);
        return 0;

    case PURCHASE_MSG_SET_REQUEST:
        self->request_arg2 = val2;
        self->request_arg1 = val1;
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!shop_info_usable(self->info))
            shop_info_default(self->info);
        ui_merchant_item_purchase_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_RESIZE, PURCHASE_RESIZE_EDGE, self->info->width, self->info->height, 0);
        ui_frame_wnd_move(&self->base.frame.base, self->info->x, self->info->y);
        return 0;

    case UI_MSG_ADD_DRAGGED:
        purchase_add_dragged(self);
        return 0;

    case UI_MSG_LOAD_RECORDS:
        {
            int record_bytes = ui_window_records_validate((const void *)val1, (size_t)(unsigned)val2);
            if (record_bytes <= 0) return 0;
            val2 = record_bytes;
        }
        purchase_load_records(self, (const char *)val1, (unsigned)val2);
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

int ui_merchant_item_purchase_wnd_get_records(struct UIWindow *window,
                                                                  struct ReplayBuffer *out)
{
    struct UIMerchantItemPurchaseWnd *self = (struct UIMerchantItemPurchaseWnd *)window;
    struct SnapshotWriter writer = {0};
    if (self->variant) {
        snapshot_write(&writer, REC_PURCHASE_VARIANT_BEGIN, NULL, 0);
        snapshot_write(&writer, REC_PURCHASE_VARIANT_END, NULL, 0);
    } else {
        snapshot_write(&writer, REC_MERCHANT_PURCHASE_BEGIN, NULL, 0);
        snapshot_write(&writer, REC_MERCHANT_PURCHASE_END, NULL, 0);
    }
    return snapshot_finish(&writer, out);
}
