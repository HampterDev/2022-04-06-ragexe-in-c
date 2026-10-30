#include "ui/shops/shops.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "ui/shops/barter_internal.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"

struct DragInfo;
struct UIItemShopWnd;
struct UIWindow;
void ui_bartermarket_wnd_destroy(struct UIWindow *window);
void ui_bartermarket_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_bartermarket_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_bartermarket_wnd_on_draw(struct UIWindow *self);
void ui_bartermarket_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_bartermarket_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_bartermarket_wnd_show_item_at(struct UIWindow *self, int x, int y);
int ui_bartermarket_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_bartermarket_wnd_ops = {
    .destroy = ui_bartermarket_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_bartermarket_wnd_drag_and_drop,
    .store_info = ui_item_shop_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_bartermarket_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_bartermarket_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_bartermarket_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_shop_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_bartermarket_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_bartermarket_wnd_show_item_at,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_bartermarket_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void barter_refusal(int msg)
{
    ui_chat_line(msg_str(msg), BARTER_REFUSAL_COLOUR);
}

static int in_lines(struct UIItemBaseWnd *base, int x, int y)
{
    return x >= ITEM_GRID_X && x < base->frame.base.width - 0x14 && y >= ITEM_GRID_Y
        && y < base->rows * ITEM_GRID_CELL + ITEM_GRID_Y;
}

static const struct ListNode *node_in_row(struct UIItemBaseWnd *base, const struct List *list,
                                                 int row)
{
    const struct ListNode *node;
    int i = 0, cell = 0;

    for (node = list->head->next; node != list->head; node = node->next, i++) {
        if (i < base->scroll * base->columns)
            continue;
        if (cell == base->columns * row)
            return node;
        cell++;
    }
    return NULL;
}

struct BarterFound *barter_goods_at(struct UIItemBaseWnd *base, const struct List *list,
                                    struct BarterFound *out, int x, int y)
{
    const struct ListNode *node = NULL;

    if (in_lines(base, x, y))
        node = node_in_row(base, list, (y - ITEM_GRID_Y) / ITEM_GRID_CELL);
    out->found = node != NULL;
    if (node)
        barter_goods_copy(&out->goods, LIST_VALUE(node, const struct BarterGoods));
    else
        barter_goods_construct(&out->goods);
    return out;
}

static struct BarterFound *goods_or_pay_at(struct UIItemBaseWnd *base, const struct List *list,
                                           struct BarterFound *out, int x, int y)
{
    const struct ListNode *node = NULL;

    if (in_lines(base, x, y))
        node = node_in_row(base, list, (y - ITEM_GRID_Y) / ITEM_GRID_CELL);
    out->found = node != NULL;
    if (!node) {
        barter_goods_construct(&out->goods);
    } else if ((x - ITEM_GRID_X) / ITEM_GRID_CELL == 0) {
        barter_goods_copy(&out->goods, LIST_VALUE(node, const struct BarterGoods));
    } else {
        struct BarterGoods pay;
        char id[100];

        barter_goods_construct(&pay);
        memset(id, 0, sizeof id);
        _itoa(LIST_VALUE(node, const struct BarterGoods)->pay_id, id, 10);
        string_assign_cstr(&pay.id_text, id);
        pay.type = BARTER_PAY_ITEM_TYPE;
        barter_goods_copy(&out->goods, &pay);
        barter_goods_destruct(&pay);
    }
    return out;
}

void barter_show_item_at(struct UIItemBaseWnd *base, const struct List *list, int x, int y)
{
    struct BarterFound at;

    goods_or_pay_at(base, list, &at, x, y);
    if (at.found) {
        struct UIWindow *wnd = ui_window_open(ITEM_WND_ID);
        struct ItemInfo info;

        barter_goods_to_item_info(&at.goods, &info);
        if (wnd)
            ui_window_send_msg(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
        item_info_destruct(&info);
    }
    barter_goods_destruct(&at.goods);
}

void barter_show_pay_tooltip(struct UIWindow *wnd, int x, int y, const struct BarterGoods *goods, int count)
{
    struct String text, line;
    struct ItemInfo info;
    int tip_x, tip_y;
    struct Mode *mode;

    shop_tooltip_cell(wnd, x, y, &tip_x, &tip_y);
    string_init(&text);
    string_init(&line);
    string_format(&line, msg_str(MSG_BARTER_PAY), item_table_get(goods->pay_id)->name, goods->pay_count * count);
    string_free(&text);
    string_move(&text, &line);
    mode = client_current_mode();
    barter_goods_to_item_info(goods, &info);
    if (mode)
        mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&info), 0);
    item_info_destruct(&info);
    string_free(&text);
}

void barter_refresh_items(struct UIItemBaseWnd *base, const struct List *list)
{
    const struct ListNode *node;

    list_clear(&base->items, item_info_destroy);
    for (node = list->head->next; node != list->head; node = node->next) {
        struct ItemInfo info;

        barter_goods_to_item_info(LIST_VALUE(node, const struct BarterGoods), &info);
        item_info_copy(list_push_back(&base->items, NULL, sizeof(struct ItemInfo)), &info);
        item_info_destruct(&info);
    }
    ui_window_invalidate_base(&base->frame.base);
    ui_item_base_wnd_update_scroll(base);
}

void barter_draw_pay(struct UIWindow *wnd, int row_y, const struct BarterGoods *goods, int count, int clear)
{
    char id[0x40], number[0x40];

    if (clear)
        memset(id, 0, sizeof id);
    _snprintf_s(id, sizeof id, (size_t)-1, "%d", goods->pay_id);
    ui_window_draw_item_icon_by_name(wnd, wnd->width - 0x46, row_y + 0x15, id, 1);
    if (clear)
        memset(number, 0, sizeof number);
    if (goods->pay_count > 0) {
        int len = _snprintf_s(number, sizeof number, (size_t)-1, "%d", goods->pay_count * count);

        ui_window_text_out_right(wnd, wnd->width - 0x19, row_y + 0x1d, number, len, 0, 12, 0, 0);
    }
}

void barter_draw_highlight(struct UIWindow *wnd, int col_x, int row_y, int width)
{
    ui_window_fill_rect(wnd, col_x + 0x48, row_y + 0x13, width, 0x1c, ui_skin_colour(6, 2));
}

struct UIItemShopWnd *ui_bartermarket_wnd_construct(struct UIItemShopWnd *self)
{
    ui_item_shop_wnd_construct(self);
    self->shop.base.frame.base.ops = &ui_bartermarket_wnd_ops;
    return self;
}

struct UIWindow *ui_bartermarket_wnd_new(void)
{
    return (struct UIWindow *)ui_bartermarket_wnd_construct(mem_alloc(sizeof(struct UIItemShopWnd)));
}

void ui_bartermarket_wnd_destruct(struct UIItemShopWnd *self)
{
    self->shop.base.frame.base.ops = &ui_bartermarket_wnd_ops;
    ui_item_shop_wnd_destruct(self);
}

void ui_bartermarket_wnd_destroy(struct UIWindow *window)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    ui_bartermarket_wnd_destruct(self);
    mem_free(self);
}

void ui_bartermarket_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIWindow *count;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->shop.base.frame.base, x, y, drag);
        return;
    }
    if (drag->source != DRAG_BARTER_CART) {
        struct Mode *mode = game_current_base_mode();

        if (mode)
            mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->drop_count == 1) {
        ui_window_send_msg(&self->shop.base.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    count = ui_window_find(WND_ID_COUNT_INPUT);
    if (count)
        ui_window_send_msg(count, NULL, COUNT_MSG_OPEN, COUNT_FOR_BARTER_MARKET, 0, 0, 0);
}

void ui_bartermarket_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIItemBaseWnd *base = &self->shop.base;
    struct UIWindow *wnd = &base->frame.base;
    struct UIResizer *resizer = mem_alloc(sizeof *resizer);
    struct UIScrollBar *bar;

    if (resizer)
        resizer = ui_resizer_construct(resizer);
    base->resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(base->resizer, MARKET_RESIZE_EDGE);
    ui_resizer_set_bitmap(base->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&base->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(base->resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, &base->resizer->base);

    bar = mem_alloc(sizeof *bar);
    if (bar)
        bar = ui_scroll_bar_construct(bar);
    base->scroll_bar = bar;
    ui_scroll_bar_create(bar, wnd->width - 0xf, 0xe, 1, wnd->height - self->shop.bottom - ITEM_GRID_Y);
    ui_window_add_child(wnd, &base->scroll_bar->base);

    base->columns = 1;
    base->rows = (height - self->shop.bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    ui_window_send_msg(&self->shop.base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_bartermarket_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIItemBaseWnd *base = &self->shop.base;
    struct UIWindow *wnd = &base->frame.base;
    const struct List *list;
    const struct ListNode *node;
    int rows, i, cell = 0;

    ui_window_draw_title_bar(wnd, 0, msg_str(MSG_BARTER_TITLE), 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    shop_draw_frame(base, wnd->height - self->shop.bottom - ITEM_GRID_Y, 2);
    rows = (wnd->height - self->shop.extra - self->shop.bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    shop_draw_cells(wnd, rows);
    shop_fill_names(wnd, rows);

    list = barter_goods(g_barter);
    for (node = list->head->next, i = 0; node != list->head; node = node->next, i++) {
        const struct BarterGoods *goods = LIST_VALUE(node, const struct BarterGoods);
        struct ItemInfo info;
        int row_y, col_x;

        if (i < base->scroll * base->columns)
            continue;
        row_y = cell / base->columns * ITEM_GRID_CELL;
        col_x = cell % base->columns * ITEM_GRID_CELL;
        if (self->shop.hover == i)
            barter_draw_highlight(wnd, col_x, row_y, wnd->width - 0x58);
        barter_goods_to_item_info(goods, &info);
        if (goods->no_stock == 1)
            ui_window_draw_item_icon(wnd, col_x + 0x2c, row_y + 0x15, &info, 0);
        else
            ui_window_draw_item_icon_count(wnd, col_x + 0x2c, row_y + 0x15, &info, 0, 0);
        item_info_destruct(&info);
        barter_goods_to_item_info(goods, &info);
        ui_window_draw_item_name_short(wnd, col_x + 0x4c, row_y + 0x1d, 0x78, &info, 12);
        item_info_destruct(&info);
        barter_draw_pay(wnd, row_y, goods, 1, 0);
        if (++cell == base->rows * base->columns)
            break;
    }
}

void ui_bartermarket_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIItemBaseWnd *base = &self->shop.base;
    int old = self->shop.hover, hover = -1;
    struct BarterFound at;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&base->frame), x, y);
    if (in_lines(base, x, y)) {
        const struct List *list = barter_goods(g_barter);
        const struct ListNode *node;
        int column = (x - ITEM_GRID_X) / ITEM_GRID_CELL, row = (y - ITEM_GRID_Y) / ITEM_GRID_CELL;
        int i = 0, cell = 0;

        if (column > base->columns - 1)
            column = base->columns - 1;
        for (node = list->head->next; node != list->head; node = node->next, i++) {
            if (i < base->scroll * base->columns)
                continue;
            if (cell == base->columns * row + column) {
                hover = i;
                break;
            }
            cell++;
        }
    }
    self->shop.hover = hover;
    if (old != hover)
        ui_window_invalidate_base(&self->shop.base.frame.base);
    barter_goods_at(base, barter_goods(g_barter), &at, x, y);
    if (at.found) {
        struct GameMode *mode = game_current_mode();

        if (mode)
            game_mode_drag_barter_item(mode, DRAG_BARTER_GOODS, &at.goods, 1, -1);
    }
    barter_goods_destruct(&at.goods);
}

void ui_bartermarket_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct BarterFound at;

    barter_goods_at(&self->shop.base, barter_goods(g_barter), &at, x, y);
    if (at.found)
        barter_show_pay_tooltip(&self->shop.base.frame.base, x, y, &at.goods, 1);
    barter_goods_destruct(&at.goods);
}

void ui_bartermarket_wnd_show_item_at(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    barter_show_item_at(&self->shop.base, barter_goods(g_barter), x, y);
}

int ui_bartermarket_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    switch (msg) {
    case UI_MSG_RESIZE:
        if (val1 == MARKET_RESIZE_EDGE)
            shop_resize(&self->shop, val2, val3, 0);
        return 0;

    case SHOP_MSG_REFRESH:
        barter_refresh_items(&self->shop.base, barter_goods(g_barter));
        return 0;

    case UI_MSG_SET_INFO:
        self->shop.info = (struct WindowInfo *)val1;
        if (!self->shop.info)
            return 0;
        if (!shop_info_usable(self->shop.info))
            shop_info_default(self->shop.info);
        ui_window_send_msg(&self->shop.base.frame.base, NULL, UI_MSG_RESIZE, MARKET_RESIZE_EDGE, self->shop.info->width,
                 self->shop.info->height, 0);
        ui_frame_wnd_move(&self->shop.base.frame.base, self->shop.info->x, self->shop.info->y);
        return 0;

    case UI_MSG_ADD_DRAGGED: {
        struct Mode *mode = game_current_base_mode();
        const struct DragInfo *drag = game_current_drag();
        struct BarterFound in_cart;
        struct UIWindow *cart;

        if (!mode)
            return 0;
        barter_find_in_cart(g_barter, &in_cart, drag->shop_index);
        if (!in_cart.found) {
            barter_refusal(MSG_BARTER_NOT_LISTED);
            mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
            barter_goods_destruct(&in_cart.goods);
            return 0;
        }
        in_cart.goods.count = drag->drop_count;
        barter_cart_remove(g_barter, &in_cart.goods);
        cart = ui_window_find(WND_ID_BARTER_PURCHASE);
        if (cart)
            ui_window_send_msg(cart, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        ui_window_send_msg(&self->shop.base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        barter_goods_destruct(&in_cart.goods);
        return 0;
    }
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->shop.base), sender, msg, val1, val2, val3, val4);
}
