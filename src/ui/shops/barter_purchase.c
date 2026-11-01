#include "ui/controls/buttons.h"
#include "ui/shops/shops.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "ui/shops/shops_std.h"
#include "ui/ui_std.h"
#include "ui/shops/barter_internal.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "ui/shops/npc_shop_internal.h"
#include "ui/wndmgr/wndmgr.h"

struct DragInfo;
struct UIBitmapButton;
struct UIWindow;
void ui_purchase_button_destroy(struct UIWindow *window);
void ui_purchase_button_on_mouse_hover(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_purchase_button_ops = {
    .base = {
        .destroy = ui_purchase_button_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_window_set_show_base,
        .on_create = ui_window_on_create_base,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_bitmap_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_purchase_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_window_send_msg_base,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};
struct UIItemPurchaseWnd;
void ui_barter_item_purchase_wnd_destroy(struct UIWindow *window);
void ui_barter_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_barter_item_purchase_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_barter_item_purchase_wnd_on_draw(struct UIWindow *self);
void ui_barter_item_purchase_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_barter_item_purchase_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_barter_item_purchase_wnd_show_item_at(struct UIWindow *self, int x, int y);
int ui_barter_item_purchase_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_barter_item_purchase_wnd_ops = {
    .destroy = ui_barter_item_purchase_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_barter_item_purchase_wnd_drag_and_drop,
    .store_info = ui_item_purchase_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_barter_item_purchase_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_barter_item_purchase_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_barter_item_purchase_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_barter_item_purchase_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_barter_item_purchase_wnd_show_item_at,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_barter_item_purchase_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
BYTE barter_can_buy(struct UIItemPurchaseWnd *self);
int ui_barter_item_purchase_wnd_held_count(int item_id)
{
    int n = session_get_inventory_count(&g_session1);
    int i;

    for (i = 0; i < n; i++) {
        struct ItemInfo item;

        session_get_inventory_item_at(&g_session1, &item, i);
        if (atoi(string_cstr(&item.id_text)) == item_id) {
            int count = item.count;

            item_info_destruct(&item);
            return count;
        }
        item_info_destruct(&item);
    }
    return 0;
}

void barter_can_buy_refusal(struct UIWindow *wnd, int msg)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
    ui_chat_line(msg_str(msg), BARTER_REFUSAL_COLOUR);
    ui_window_send_msg(wnd, NULL, UI_MSG_COMMAND, UI_ID_CANCEL, 0, 0, 0);
}

static int count_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

void barter_count_add(struct Tree *counts, const struct String *id, int count)
{
    struct StrIntPair *entry = tree_find(counts, id, count_less);

    if (entry) {
        entry->value += count;
    } else {
        struct StrIntPair pair;

        string_copy(&pair.key, id);
        pair.value = count;
        tree_add(counts, &pair, sizeof pair, count_less, NULL);
    }
}

void barter_count_destroy(void *value)
{
    string_free(&((struct StrIntPair *)value)->key);
}

static void destroy_goods(void *value)
{
    barter_goods_destruct(value);
}

BYTE barter_can_buy(struct UIItemPurchaseWnd *self)
{
    struct Tree counts;
    const struct List *in_cart;
    struct List cart;
    const struct ListNode *node;
    int slots;
    BYTE ok;

    tree_init(&counts);
    in_cart = barter_cart(g_barter);
    list_init(&cart);
    for (node = in_cart->head->next; node != in_cart->head; node = node->next) {
        struct BarterGoods copy;

        barter_goods_copy(&copy, LIST_VALUE(node, const struct BarterGoods));
        list_push_back(&cart, &copy, sizeof copy);
    }
    slots = session_get_inventory_count(&g_session1) + session_count_worn_equips(&g_session1)
          + session_count_worn_costumes(&g_session1);
    for (node = cart.head->next; node != cart.head; node = node->next) {
        const struct BarterGoods *goods = LIST_VALUE(node, const struct BarterGoods);
        BYTE type = (BYTE)goods->type;

        barter_count_add(&counts, &goods->id_text, goods->count);
        if (session_is_equip_item_type(&g_session1, type) == 1 || session_is_cash_item_type(&g_session1, type) == 1
            || ui_barter_item_purchase_wnd_held_count(atoi(string_cstr(&goods->id_text))) == 0)
            slots++;
    }
    if (slots > g_session1.inventoryExtension + BARTER_INVENTORY_BASE) {
        barter_can_buy_refusal(&self->base.frame.base, MSG_BARTER_NO_ROOM);
        ok = 0;
    } else {
        struct Tree pay;
        struct TreeNode *it;

        tree_copy(&pay, barter_pay_map(g_barter), sizeof(struct BarterPay), NULL);
        ok = 1;
        for (it = tree_first(&pay); it != pay.head; it = tree_next(it)) {
            const struct BarterPay *pay_entry = TREE_VALUE(it, const struct BarterPay);

            if (pay_entry->count > ui_barter_item_purchase_wnd_held_count(pay_entry->id)) {
                barter_can_buy_refusal(&self->base.frame.base, MSG_BARTER_PAY_SHORT);
                ok = 0;
                break;
            }
        }
        for (it = tree_first(&counts); ok && it != counts.head; it = tree_next(it)) {
            const struct StrIntPair *entry = TREE_VALUE(it, const struct StrIntPair);
            int held = ui_barter_item_purchase_wnd_held_count(atoi(string_cstr(&entry->key)));

            if (entry->value + held > BARTER_MAX_COUNT) {
                struct String text;

                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
                string_init(&text);
                string_format(&text, msg_str(MSG_BARTER_HELD_OVER_MAX), BARTER_MAX_COUNT);
                ui_chat_line(string_cstr(&text), BARTER_REFUSAL_COLOUR);
                ui_window_send_msg(&self->base.frame.base, NULL, UI_MSG_COMMAND, UI_ID_CANCEL, 0, 0, 0);
                string_free(&text);
                ok = 0;
            }
        }
        tree_free(&pay, NULL);
    }
    list_free(&cart, destroy_goods);
    tree_free(&counts, barter_count_destroy);
    return ok;
}

struct UIBitmapButton *ui_purchase_button_construct(struct UIBitmapButton *self)
{
    ui_bitmap_button_construct(self);
    self->button.base.ops = &ui_purchase_button_ops.base;
    return self;
}

void ui_purchase_button_destruct(struct UIBitmapButton *self)
{
    self->button.base.ops = &ui_purchase_button_ops.base;
    ui_bitmap_button_destruct(self);
}

void ui_purchase_button_destroy(struct UIWindow *window)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    ui_purchase_button_destruct(self);
    mem_free(self);
}

void ui_purchase_button_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    struct Tree *pay = barter_pay_map(g_barter);
    struct TreeNode *node;
    struct String text;
    int lines = 0, tip_x, tip_y;
    struct Mode *mode;

    (void)x, (void)y;
    string_init(&text);
    for (node = tree_first(pay); node != pay->head;) {
        const struct BarterPay *pay_entry = TREE_VALUE(node, const struct BarterPay);
        int id = pay_entry->id, count = pay_entry->count;
        const char *name = item_table_get(id)->name;
        struct String line;

        node = tree_next(node);
        if (++lines == BARTER_TIP_MAX_LINES) {
            string_append_cstr(&text, "...");
            break;
        }
        string_init(&line);
        string_format(&line, msg_str(MSG_BARTER_PAY), name, count);
        if (node != pay->head)
            string_append_cstr(&line, "\n");
        string_append(&text, string_cstr(&line), line.size);
        string_free(&line);
    }
    tip_x = 0;
    tip_y = -4 - lines * 16;
    ui_window_to_screen(&self->button.base, &tip_x, &tip_y);
    mode = client_current_mode();
    if (mode)
        mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y, 0xffffffff, 0);
    string_free(&text);
}

struct UIItemPurchaseWnd *ui_barter_item_purchase_wnd_construct(struct UIItemPurchaseWnd *self)
{
    ui_item_purchase_wnd_construct(self);
    self->base.frame.base.ops = &ui_barter_item_purchase_wnd_ops;
    return self;
}

struct UIWindow *ui_barter_item_purchase_wnd_new(void)
{
    return (struct UIWindow *)ui_barter_item_purchase_wnd_construct(mem_alloc(sizeof(struct UIItemPurchaseWnd)));
}

void ui_barter_item_purchase_wnd_destruct(struct UIItemPurchaseWnd *self)
{
    self->base.frame.base.ops = &ui_barter_item_purchase_wnd_ops;
    ui_item_purchase_wnd_destruct(self);
}

void ui_barter_item_purchase_wnd_destroy(struct UIWindow *window)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    ui_barter_item_purchase_wnd_destruct(self);
    mem_free(self);
}

void ui_barter_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct UIWindow *count;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    if (drag->source != DRAG_BARTER_GOODS) {
        game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (session_is_equip_item_type(&g_session1, drag->type) == 1 || session_is_cash_item_type(&g_session1, drag->type) == 1) {
        ui_window_send_msg(&self->base.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    count = ui_window_find(WND_ID_COUNT_INPUT);
    if (count)
        ui_window_send_msg(count, NULL, COUNT_MSG_OPEN, COUNT_FOR_BARTER_PURCHASE, 0, 0, 0);
}

void ui_barter_item_purchase_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct UIItemBaseWnd *base = &self->base;
    struct UIWindow *wnd = &base->frame.base;
    struct UIBitmapButton **buttons = (struct UIBitmapButton **)&base->close;
    struct UIResizer *resizer = mem_alloc(sizeof *resizer);
    struct UIScrollBar *bar;
    struct UIBitmapButton *buy;

    if (resizer)
        resizer = ui_resizer_construct(resizer);
    base->resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(base->resizer, PURCHASE_RESIZE_EDGE);
    ui_resizer_set_bitmap(base->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&base->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(base->resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, &base->resizer->base);

    bar = mem_alloc(sizeof *bar);
    if (bar)
        bar = ui_scroll_bar_construct(bar);
    base->scroll_bar = bar;
    ui_scroll_bar_create(bar, wnd->width - 0xf, 0xe, 1, wnd->height - 0x26);
    ui_window_add_child(wnd, &base->scroll_bar->base);

    buy = mem_alloc(sizeof *buy);
    if (buy)
        buy = ui_purchase_button_construct(buy);
    buttons[0] = shop_place_button(wnd, buy, g_shopBuyButton, wnd->width - 0x6a, wnd->height - 0x18, UI_ID_OK);
    buttons[1] = shop_add_button(wnd, g_shopCancelButton, wnd->width - 0x3d, wnd->height - 0x18, UI_ID_CANCEL);

    base->columns = 1;
    base->rows = (height - 0x26) / ITEM_GRID_CELL;
    ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_barter_item_purchase_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct UIItemBaseWnd *base = &self->base;
    struct UIWindow *wnd = &base->frame.base;
    const struct List *list;
    const struct ListNode *node;
    int rows, i, cell = 0;

    ui_window_draw_title_bar(wnd, 0, msg_str(MSG_BARTER_PURCHASE_TITLE), 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    shop_draw_frame(base, wnd->height - 0x26, 2);
    rows = (wnd->height - 0x26) / ITEM_GRID_CELL;
    ui_window_fill_rect(wnd, ITEM_GRID_X, ITEM_GRID_Y, wnd->width - 0x32, rows * ITEM_GRID_CELL,
                      ui_skin_colour(2, 2));
    shop_draw_cells(wnd, rows);

    list = barter_cart(g_barter);
    for (node = list->head->next, i = 0; node != list->head; node = node->next, i++) {
        const struct BarterGoods *goods = LIST_VALUE(node, const struct BarterGoods);
        struct ItemInfo info;
        int row_y, col_x;

        if (i < base->scroll * base->columns)
            continue;
        row_y = cell / base->columns * ITEM_GRID_CELL;
        col_x = cell % base->columns * ITEM_GRID_CELL;
        if (self->hover == i)
            barter_draw_highlight(wnd, col_x, row_y, 0xbf);
        barter_goods_to_item_info(goods, &info);
        ui_window_draw_item_icon_count(wnd, col_x + 0x2c, row_y + 0x15, &info, 0, 0);
        ui_window_draw_item_name_short(wnd, col_x + 0x4c, row_y + 0x1d, 100, &info, 12);
        barter_draw_pay(wnd, row_y, goods, goods->count, 1);
        item_info_destruct(&info);
        if (++cell == base->rows * base->columns)
            break;
    }
}

void ui_barter_item_purchase_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct BarterFound at;
    int old;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);
    old = self->hover;
    self->hover = ui_item_base_wnd_index_at(&self->base, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(&self->base.frame.base);
    barter_goods_at(&self->base, barter_cart(g_barter), &at, x, y);
    if (at.found == 1) {
        struct GameMode *mode = game_current_mode();

        if (mode)
            game_mode_drag_barter_item(mode, DRAG_BARTER_CART, &at.goods, at.goods.count, -1);
    }
    barter_goods_destruct(&at.goods);
}

void ui_barter_item_purchase_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct BarterFound at;

    barter_goods_at(&self->base, barter_cart(g_barter), &at, x, y);
    if (at.found) {
        struct BarterGoods goods;

        barter_goods_copy(&goods, &at.goods);
        barter_show_pay_tooltip(&self->base.frame.base, x, y, &goods, goods.count);
        barter_goods_destruct(&goods);
    }
    barter_goods_destruct(&at.goods);
}

void ui_barter_item_purchase_wnd_show_item_at(struct UIWindow *window, int x, int y)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    barter_show_item_at(&self->base, barter_cart(g_barter), x, y);
}

static void add_listed(struct UIItemPurchaseWnd *self, struct Mode *mode, struct BarterFound *listed)
{
    const struct DragInfo *drag = game_current_drag();
    struct String id_text;
    struct UIWindow *market;
    int weight;

    if (!listed->found) {
        barter_refusal(MSG_BARTER_NOT_LISTED);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    listed->goods.count = drag->drop_count;
    if (session_is_wearing_item_id(&g_session1, listed->goods.pay_id) == 1) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
        barter_refusal(MSG_BARTER_PAY_ID_REFUSED);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    string_copy(&id_text, &drag->name);
    if (barter_cart_has_id(g_barter, id_text) == 1) {
        int msg = 0;

        if (session_is_equip_item_type(&g_session1, drag->type) == 1)
            msg = MSG_BARTER_TYPE_REFUSED;
        else if (session_is_cash_item_type(&g_session1, drag->type) == 1)
            msg = MSG_BARTER_CASH_TYPE_REFUSED;
        if (msg) {
            mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
            barter_refusal(msg);
            return;
        }
    }
    string_copy(&id_text, &drag->name);
    if (drag->drop_count + barter_cart_count_of(g_barter, id_text) > BARTER_MAX_COUNT) {
        char text[0xc8];

        memset(text, 0, sizeof text);
        _snprintf_s(text, sizeof text, (size_t)-1, msg_str(MSG_BARTER_OVER_MAX), BARTER_MAX_COUNT);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
        ui_chat_line(text, BARTER_REFUSAL_COLOUR);
        return;
    }
    weight = barter_goods_weight(g_barter, drag->shop_index);
    if (weight < 0) {
        barter_refusal(MSG_BARTER_NOT_LISTED);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (weight > 0) {
        float left = (float)(g_session1.maxWeight - g_session1.weight) - barter_cart_weight(g_barter);

        if ((int)left / weight < drag->drop_count) {
            barter_refusal(MSG_BARTER_TOO_HEAVY);
            mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
            return;
        }
    }
    barter_cart_add(g_barter, &listed->goods);
    ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
    market = ui_window_find(WND_ID_BARTER_MARKET);
    if (market)
        ui_window_send_msg(market, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

static void add_dragged(struct UIItemPurchaseWnd *self, struct Mode *mode)
{
    struct BarterFound listed;

    barter_find_goods(g_barter, &listed, game_current_drag()->shop_index);
    add_listed(self, mode, &listed);
    barter_goods_destruct(&listed.goods);
}

static void purchase_resize(struct UIItemPurchaseWnd *self, int width, int height)
{
    struct UIItemBaseWnd *base = &self->base;
    struct UIWindow *wnd = &base->frame.base;
    struct UIWindow **buttons = (struct UIWindow **)&base->close;

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
        base->columns = 1;
        base->rows = (wnd->height - 0x26) / ITEM_GRID_CELL;
        ui_window_move_base(&base->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
        ui_window_move_base(&base->scroll_bar->base, wnd->width - 0xf, 0xe);
        ui_scroll_bar_resize(&base->scroll_bar->base, base->scroll_bar->base.width, wnd->height - 0x26);
        ui_item_base_wnd_update_scroll(base);
    }
    ui_window_move(buttons[0], wnd->width - 0x6a, wnd->height - 0x18);
    ui_window_move(buttons[1], wnd->width - 0x3d, wnd->height - 0x18);
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

int ui_barter_item_purchase_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                               int val1, int val2, int val3, int val4)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND: {
        struct Mode *mode = game_current_base_mode();

        if (!mode)
            return 0;
        if (val1 == UI_ID_OK) {
            if (!barter_can_buy(self))
                return 0;
            mode_send_message(mode, MODE_MSG_BARTER_BUY, 0, 0, 0, 0);
        } else if (val1 != UI_ID_CANCEL) {
            return 0;
        }
        mode_send_message(mode, MODE_MSG_BARTER_CLOSE, 0, 0, 0, 0);
        barter_clear(g_barter);
        return 0;
    }

    case UI_MSG_RESIZE:
        if (val1 == PURCHASE_RESIZE_EDGE)
            purchase_resize(self, val2, val3);
        return 0;

    case SHOP_MSG_REFRESH:
        barter_refresh_items(&self->base, barter_cart(g_barter));
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!self->info)
            return 0;
        if (!purchase_info_usable(self->info))
            purchase_info_default(self->info);
        ui_window_send_msg(&self->base.frame.base, NULL, UI_MSG_RESIZE, MARKET_RESIZE_EDGE, self->info->width, self->info->height, 0);
        ui_frame_wnd_move(&self->base.frame.base, self->info->x, self->info->y);
        return 0;

    case UI_MSG_ADD_DRAGGED: {
        struct Mode *mode = game_current_base_mode();

        if (mode)
            add_dragged(self, mode);
        return 0;
    }
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
