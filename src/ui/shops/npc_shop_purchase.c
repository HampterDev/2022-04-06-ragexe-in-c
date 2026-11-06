#include "ui/controls/edit.h"
#include "ui/window_records.h"
#include "ui/shops/shops.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/shops/npc_shop_internal.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"

struct DragInfo;
struct UIWindow;
struct UIItemPurchaseWnd;
void ui_item_purchase_wnd_destroy(struct UIWindow *window);
void ui_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
int ui_item_purchase_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_item_purchase_wnd_ops = {
    .destroy = ui_item_purchase_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_purchase_wnd_drag_and_drop,
    .store_info = ui_item_purchase_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_purchase_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_purchase_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_purchase_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_purchase_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_purchase_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_purchase_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_item_purchase_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIItemPurchaseWnd *ui_item_purchase_wnd_construct(struct UIItemPurchaseWnd *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_item_purchase_wnd_ops;
    self->hover = -1;
    self->base.button_bar = 1;
    self->cash = 0;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_item_purchase_wnd_new(void)
{
    return (struct UIWindow *)ui_item_purchase_wnd_construct(mem_alloc(sizeof(struct UIItemPurchaseWnd)));
}

void ui_item_purchase_wnd_destruct(struct UIItemPurchaseWnd *self)
{
    self->base.frame.base.ops = &ui_item_purchase_wnd_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_item_purchase_wnd_destroy(struct UIWindow *window)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    ui_item_purchase_wnd_destruct(self);
    mem_free(self);
}

void ui_item_purchase_wnd_store_info(struct UIWindow *window)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    npc_shop_store_info(&self->base.frame.base, self->info);
}

int ui_item_purchase_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out)
{
    (void)self;
    struct SnapshotWriter writer = {0};
    snapshot_write(&writer, REC_PURCHASE_BEGIN, NULL, 0);
    snapshot_write(&writer, REC_PURCHASE_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

void ui_item_purchase_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));
    struct UIWindow *total;
    struct UIBitmapButton **buttons = (struct UIBitmapButton **)&self->base.close;

    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->points = edit;
    ui_window_create(edit, 0x40, 0x10);
    ((struct UIEditCtrl *)self->points)->max_chars = 9;
    ui_window_move(self->points, -100, -100);
    ui_edit_ctrl_set_back_colour(self->points, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->points, "0");
    ui_window_add_child(wnd, self->points);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->points);
    self->base.frame.default_id = PURCHASE_DEFAULT_ID;

    total = mem_alloc(sizeof(struct UIStaticText));
    if (total)
        total = ui_static_text_construct(total);
    self->total = total;
    ui_window_create(total, 0xa0, 0xf);
    ui_window_move(self->total, 0xc, wnd->height - 0x15);
    ui_window_add_child(wnd, self->total);

    shop_add_title(wnd, 0xa0, MSG_PURCHASE_TITLE);
    shop_add_resizer(&self->base, 3);
    shop_add_scroll_bar(&self->base, wnd->height - 0x26);
    buttons[0] = shop_add_button(wnd, g_shopBuyButton, wnd->width - 0x6a, wnd->height - 0x18, UI_ID_OK);
    buttons[1] = shop_add_button(wnd, g_shopCancelButton, wnd->width - 0x3d, wnd->height - 0x18, UI_ID_CANCEL);

    self->base.columns = 1;
    self->base.rows = (height - 0x26) / ITEM_GRID_CELL;
    ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_item_purchase_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    int old;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);
    old = self->hover;
    self->hover = ui_item_base_wnd_index_at(&self->base, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(&self->base.frame.base);
    if (!g_replayPlaying) {
        struct ItemInfo info;

        ui_item_base_wnd_get_item(&self->base, &info, self->hover);
        if (info.index) {
            struct GameMode *mode = game_current_mode();
            struct ItemInfo listed;

            session_find_purchase_item(&g_session1, &listed, string_cstr(&info.id_text));
            item_info_assign(&info, &listed);
            item_info_destruct(&listed);
            game_mode_drag_item(mode, DRAG_SHOP_PURCHASE, &info, info.count, -1);
        }
        item_info_destruct(&info);
    }
}

void ui_item_purchase_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    char unit[8];

    npc_shop_price_unit_long(unit, self->cash);
    npc_shop_show_price_tooltip(&self->base, x, y, unit, 1, 0);
}

void ui_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct Mode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    if (drag->source != DRAG_SHOP_GOODS) {
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    switch (drag->type) {
    case 4: case 5: case 8: case 9: case 0xb: case 0xc: case 0xd: case 0xe: case 0xf:
        ui_window_send_msg(&self->base.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    npc_shop_drop_or_count(&self->base, 0, 0, COUNT_FOR_PURCHASE);
}

void ui_item_purchase_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    npc_shop_describe_hovered(&self->base, &self->hover, x, y);
}

void ui_item_purchase_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIItemBaseWnd *base = &self->base;
    struct ListNode *node;
    int body, rows, index, cell;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    body = wnd->height - 0x26;
    shop_draw_frame(base, body, base->button_bar ? 2 : 0);
    ui_window_fill_rect(wnd, ITEM_GRID_X, ITEM_GRID_Y, wnd->width - 0x32, body / ITEM_GRID_CELL * ITEM_GRID_CELL,
                      ui_skin_colour(2, 2));
    rows = (wnd->height - PURCHASE_ROWS_MARGIN) / ITEM_GRID_CELL;
    shop_draw_cells(wnd, rows);

    cell = 0;
    for (node = base->items.head->next, index = 0; node != base->items.head; node = node->next, index++) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);
        char unit[12], text[0x40], money[0x40];
        int row_y, col_x, name_width, len;

        if (index < base->scroll * base->columns || item_grid_hidden(info))
            continue;
        row_y = cell / base->columns * ITEM_GRID_CELL;
        col_x = cell % base->columns * ITEM_GRID_CELL;
        if (self->hover == index)
            ui_window_fill_rect(wnd, col_x + 0x48, row_y + 0x13, 0xbf, 0x1c, ui_skin_colour(6, 2));
        ui_window_draw_item_icon_count(wnd, col_x + 0x2c, row_y + 0x15, info, 0);
        name_width = 0x6e;
        if (info->price != info->charged_price) {
            if (info->price >= 10000)
                name_width = 0x2d;
            else if (info->price >= 1000)
                name_width = 0x37;
            else if (info->price >= 100)
                name_width = 0x46;
            else if (info->price >= 10)
                name_width = 0x50;
        }
        ui_window_draw_item_name_short(wnd, col_x + 0x4c, row_y + 0x1d, name_width, info, 12);
        if (info->grade > 0) {
            struct String path;

            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", (int)info->grade);
            ui_window_draw_bitmap(wnd, col_x + 0x2c, row_y + 0x21,
                                res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
            string_free(&path);
        }
        _snprintf_s(unit, 10, (size_t)-1, "Z");
        if (self->cash) {
            if (g_serviceType == 6)
                sprintf(unit, "Koin");
            else if (g_serviceType != 0)
                sprintf(unit, "Point");
            else if (g_session1.channelCopId == 1)
                sprintf(unit, CASH_HANCOIN);
            else
                sprintf(unit, msg_str(MSG_SHOP_UNIT_CASH));
        }
        if (info->price != info->charged_price)
            len = sprintf(text, " %d -> %s %s", info->price, format_money(info->charged_price, money), unit);
        else
            len = sprintf(text, " %s %s", format_money(info->price, money), unit);
        ui_window_text_out_right(wnd, wnd->width - 0x12, row_y + 0x1d, text, len, 0, 12, 0, 0);
        if (++cell == base->rows * base->columns)
            break;
    }
    if (self->cash) {
        ui_window_text_out(wnd, 0xc, wnd->height - 0x30, msg_str(MSG_PURCHASE_POINTS), 0, 0, 0xc, 0, 0, 0);
        ui_window_text_out(wnd, 0xb3, wnd->height - 0x30, msg_str(MSG_SHOP_UNIT_CASH), 0, 0, 0xc, 0, 0, 0);
        ui_window_fill_rect(wnd, 0x6d, wnd->height - 0x33, 0x42, 0x12, 0x8e8e8e);
    }
}

static void purchase_total_text(struct UIItemPurchaseWnd *self, int total)
{
    char text[0x60], money[0x60];
    const char *fmt = "Total : %s Zeny";

    if (self->cash) {
        if (g_serviceType == 6)
            fmt = "Total : %s Koin";
        else if (g_serviceType != 0)
            fmt = "Total : %s Point";
        else if (g_session1.channelCopId == 1)
            fmt = "Total : %s " CASH_HANCOIN;
        else
            fmt = "Total : %s " CASH_WORD;
    }
    sprintf(text, fmt, format_money(total, money));
    ui_static_text_set_text_fit(self->total, text);
}

static int purchase_points(struct UIItemPurchaseWnd *self)
{
    int points = atoi(ui_edit_ctrl_get_text(self->points));

    if (points < 0)
        points = 0;
    if (points > g_session1.freePoints)
        points = g_session1.freePoints;
    return points;
}

static void chat_refusal(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
    ui_chat_line(text, CHAT_RED);
}

static void purchase_buy(struct UIItemPurchaseWnd *self, struct Mode *mode)
{
    char buf[0x10];
    int points, total;

    if (!self->cash) {
        mode_send_message(mode, MODE_REQ_BUY, 0, 0, 0, 0);
        mode_send_message(mode, MODE_REQ_SHOP_CLOSE, 0, 0, 0, 0);
        session_clear_lists(&g_session1);
        return;
    }
    if (session_get_shop_buy_list_size(&g_session1) == 0)
        return;
    points = purchase_points(self);
    if (points > session_get_shop_buy_total(&g_session1))
        points = session_get_shop_buy_total(&g_session1);
    _itoa(points, buf, 10);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->points);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->points, buf);
    ui_window_mgr_set_focus_edit(&g_windowMgr, NULL);
    total = session_get_shop_buy_total(&g_session1);
    if (g_session1.freePoints + g_session1.cashPoints < total) {
        ui_chat_line(msg_str(MSG_NO_CASH), CHAT_RED);
        return;
    }
    if (points == 0 && g_session1.cashPoints < total
        && ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_CASH_NO_FREE), UI_MSGBOX_OK, 1, 0, 0, "Message",
                                  0x118, 0x78, 0) != 0)
        return;
    {
        struct String text, tmp;

        string_init(&text);
        string_init(&tmp);
        if (points > 0)
            string_format(&tmp, msg_str(MSG_CASH_CONFIRM_FREE), session_get_shop_buy_total(&g_session1) - points,
                          points);
        else
            string_format(&tmp, msg_str(MSG_CASH_CONFIRM), session_get_shop_buy_total(&g_session1));
        string_assign(&text, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        if (ui_window_mgr_message_box(&g_windowMgr, string_cstr(&text), UI_MSGBOX_QUESTION, 1, 1, 0,
                                   "Message", 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES) {
            ui_window_mgr_set_focus_edit(&g_windowMgr, self->points);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->points, "0");
            mode_send_message(mode, MODE_REQ_CASH_BUY, points, 0, 0, 0);
            ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        }
        string_free(&text);
    }
}

static void purchase_add_dragged(struct UIItemPurchaseWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();
    struct ItemInfo info;
    struct ListNode *node;
    int budget, total, held;

    switch (drag->type) {
    case 4: case 5: case 8: case 9: case 0xb: case 0xc: case 0xd: case 0xe: case 0xf:
        if (session_get_shop_buy_count_by_name(&g_session1, string_cstr(&drag->name))) {
            mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
            chat_refusal(msg_str(MSG_PURCHASE_HAVE_ONE));
            return;
        }
        break;
    }
    item_info_construct(&info);
    item_info_assign_from_drag(&info, drag);
    total = session_get_shop_buy_total(&g_session1);
    budget = g_session1.zeny;
    if (self->cash) {
        if (session_get_shop_buy_list_size(&g_session1) == CASH_PURCHASE_MAX) {
            chat_refusal("한 번에 10개 이상 구매 할 수 "
                         "없습니다.");
            item_info_destruct(&info);
            return;
        }
        budget = g_session1.freePoints + g_session1.cashPoints;
    }
    total += drag->drop_count * drag->charged_price;
    held = 0;
    for (node = self->base.items.head->next; node != self->base.items.head; node = node->next) {
        const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);

        if (string_equal(&info.id_text, &item->id_text)) {
            held = item->count;
            break;
        }
    }
    if (held + info.count > PURCHASE_STACK_LIMIT) {
        char text[0x100];

        info.count += PURCHASE_STACK_LIMIT - (held + info.count);
        sprintf(text, msg_str(MSG_PURCHASE_LIMIT), PURCHASE_STACK_LIMIT);
        chat_refusal(text);
    }
    if (info.count > 0) {
        if (budget >= total) {
            session_add_shop_buy_item(&g_session1, &info);
            ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        } else {
            chat_refusal(msg_str(self->cash ? MSG_NO_CASH : MSG_NO_ZENY));
        }
    }
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
    item_info_destruct(&info);
}

int ui_item_purchase_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND: {
        struct Mode *mode;

        ui_window_record_command(wnd, val1, val2, val3, val4);
        mode = client_current_mode();
        if (val1 == UI_ID_OK) {
            purchase_buy(self, mode);
        } else if (val1 == UI_ID_CANCEL) {
            ui_close_window(WND_ID_SHOP2);
            ui_close_window(WND_ID_PURCHASE);
            mode_send_message(mode, MODE_REQ_SHOP_CLOSE, 0, 0, 0, 0);
            session_clear_lists(&g_session1);
        } else if (val1 == PURCHASE_DEFAULT_ID && self->cash) {
            char buf[0x10];

            _itoa(purchase_points(self), buf, 10);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->points, buf);
            ui_window_mgr_set_focus_edit(&g_windowMgr, NULL);
        }
        return 0;
    }

    case UI_MSG_RESIZE:
        if (val1 != 3)
            return 0;
        npc_shop_list_resize(&self->base, val2, val3, PURCHASE_ROWS_MARGIN);
        ui_window_move(self->total, 0xc, wnd->height - 0x14);
        if (self->cash)
            ui_window_move(self->points, 0x6e, wnd->height - 0x32);
        else
            ui_window_move(self->points, -100, -100);
        npc_shop_list_place_buttons(&self->base);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;

    case SHOP_MSG_REFRESH:
        purchase_total_text(self, npc_shop_refill(&self->base, session_get_shop_buy_list_size(&g_session1), session_get_shop_buy_item_at));
        ui_window_invalidate_base(&self->base.frame.base);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        npc_shop_list_set_info(&self->base, self->info, 3, purchase_info_usable, purchase_info_default);
        return 0;

    case SHOP_MSG_SET_MODE: {
        char text[0x60];
        const char *fmt = "Total : %d Zeny";

        self->cash = val1;
        if (val1)
            ui_window_move(self->points, 0x6e, wnd->height - 0x32);
        else
            ui_window_move(self->points, -100, -100);
        if (self->cash) {
            if (g_serviceType == 6)
                fmt = "Total : %d Koin";
            else if (g_serviceType != 0)
                fmt = "Total : %d Point";
            else if (g_session1.channelCopId == 1)
                fmt = "Total : %d " CASH_HANCOIN;
            else
                fmt = "Total : %d Cash";
        }
        sprintf(text, fmt, 0);
        ui_static_text_set_text_fit(self->total, text);
        return 0;
    }

    case UI_MSG_ADD_DRAGGED:
        purchase_add_dragged(self);
        return 0;

    case UI_MSG_LOAD_RECORDS:
        {
            int record_bytes = ui_window_records_validate((const void *)val1, (size_t)(unsigned)val2);
            if (record_bytes <= 0) return 0;
            val2 = record_bytes;
        }
        if (npc_shop_records_reach((const char *)val1, (unsigned)val2, REC_PURCHASE_BEGIN, REC_PURCHASE_END)) {
            npc_shop_open_shop_list(WND_ID_SHOP2, 0);
            ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_SET_MODE, 0, 0, 0, 0);
            ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        }
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
