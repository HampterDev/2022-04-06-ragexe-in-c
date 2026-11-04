#include "ui/controls/buttons.h"
#include "world/tooltip.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/ui_std.h"
#include "ui/shops/barter_ex_internal.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "ui/wndmgr/wndmgr.h"
#include <stdint.h>

struct DragInfo;
void ui_expanded_barter_item_purchase_wnd_destroy(struct UIWindow *window);
void ui_expanded_barter_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_expanded_barter_item_purchase_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_expanded_barter_item_purchase_wnd_on_draw(struct UIWindow *self);
void ui_expanded_barter_item_purchase_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_expanded_barter_item_purchase_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_expanded_barter_item_purchase_wnd_show_item_at(struct UIWindow *self, int x, int y);
void ui_expanded_barter_item_purchase_wnd_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_expanded_barter_item_purchase_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_expanded_barter_item_purchase_wnd_ops = {
    .destroy = ui_expanded_barter_item_purchase_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_expanded_barter_item_purchase_wnd_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_expanded_barter_item_purchase_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_expanded_barter_item_purchase_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_expanded_barter_item_purchase_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_expanded_barter_item_purchase_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_expanded_barter_item_purchase_wnd_show_item_at,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_expanded_barter_item_purchase_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_expanded_barter_item_purchase_wnd_send_msg,
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
void ui_expanded_barter_purchase_button_destroy(struct UIWindow *window);
void ui_expanded_barter_purchase_button_on_mouse_hover(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_expanded_barter_purchase_button_ops = {
    .base = {
        .destroy = ui_expanded_barter_purchase_button_destroy,
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
        .on_mouse_hover = ui_expanded_barter_purchase_button_on_mouse_hover,
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

struct UIBitmapButton *ui_expanded_barter_purchase_button_construct(struct UIBitmapButton *self)
{
    ui_bitmap_button_construct(self);
    self->button.base.ops = &ui_expanded_barter_purchase_button_ops.base;
    return self;
}

void ui_expanded_barter_purchase_button_destruct(struct UIBitmapButton *self)
{
    self->button.base.ops = &ui_expanded_barter_purchase_button_ops.base;
    ui_bitmap_button_destruct(self);
}

void ui_expanded_barter_purchase_button_destroy(struct UIWindow *window)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    ui_expanded_barter_purchase_button_destruct(self);
    mem_free(self);
}

static void exb_pay_line(struct String *out, int refine, const char *name, int count)
{
    char line[0x400];

    string_init(out);
    if (refine > 0)
        exb_append_refine(out, refine);
    memset(line, 0, sizeof line);
    snprintf(line, sizeof line, msg_str(MSG_BARTER_PAY), name, count);
    string_append_cstr(out, line);
}

void ui_expanded_barter_purchase_button_on_mouse_hover(struct UIWindow *window, int x,
                                                            int y)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    const struct Vec *pays = expanded_barter_pay_items(g_expandedBarter);
    int n = (int)vec_count(pays, sizeof(struct BarterExPay)), lines = 0, tip_x, tip_y;
    const struct BarterExPay *pay;
    struct String text;
    int64_t amount;

    (void)x, (void)y;
    string_init(&text);
    for (pay = pays->first; pay != pays->last; pay++, lines++) {
        struct String line;

        if (lines == BARTER_TIP_MAX_LINES) {
            string_append_cstr(&text, "...");
            break;
        }
        exb_pay_line(&line, pay->refine, item_table_get(pay->id)->name, pay->count);
        string_append(&text, string_cstr(&line), line.size);
        string_free(&line);
        if (lines != n - 1)
            string_append_cstr(&text, "\n");
    }
    amount = expanded_barter_total(g_expandedBarter);
    if (amount > 0) {
        struct String line;

        string_init(&line);
        string_format(&line, msg_str(MSG_EXB_TIP_AMOUNT), amount);
        string_append_cstr(&text, "\n");
        string_append(&text, string_cstr(&line), line.size);
        string_free(&line);
    }
    if (text.size) {
        tip_x = 0;
        tip_y = -4 - lines * 16;
        ui_window_to_screen(&self->button.base, &tip_x, &tip_y);
        if (client_current_mode())
            mode_show_tooltip(client_current_mode(), string_cstr(&text), tip_x, tip_y, 0xffffffff, 0);
    }
    string_free(&text);
}

struct UIExpandedBarterItemPurchaseWnd *ui_expanded_barter_item_purchase_wnd_construct(
    struct UIExpandedBarterItemPurchaseWnd *self)
{
    exb_construct(&self->list, &ui_expanded_barter_item_purchase_wnd_ops);
    return self;
}

struct UIWindow *ui_expanded_barter_item_purchase_wnd_new(void)
{
    return (struct UIWindow *)ui_expanded_barter_item_purchase_wnd_construct(mem_alloc(sizeof(struct UIExpandedBarterItemPurchaseWnd)));
}

void ui_expanded_barter_item_purchase_wnd_destruct(struct UIExpandedBarterItemPurchaseWnd *self)
{
    exb_destruct(&self->list, &ui_expanded_barter_item_purchase_wnd_ops);
}

void ui_expanded_barter_item_purchase_wnd_destroy(struct UIWindow *window)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    ui_expanded_barter_item_purchase_wnd_destruct(self);
    mem_free(self);
}

void ui_expanded_barter_item_purchase_wnd_on_create(struct UIWindow *window, int width,
                                                         int height)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    struct UIWindow *wnd = &self->list.frame.base;
    struct UIBitmapButton *buy;

    (void)width;
    exb_add_resizer_and_bar(&self->list, PURCHASE_RESIZE_EDGE);
    buy = mem_alloc(sizeof *buy);
    if (buy)
        buy = ui_expanded_barter_purchase_button_construct(buy);
    self->buy = shop_place_button(wnd, buy, g_shopBuyButton, wnd->width - 0x69, wnd->height - 0x18, UI_ID_OK);
    self->cancel = shop_add_button(wnd, g_shopCancelButton, wnd->width - 0x3c, wnd->height - 0x18, UI_ID_CANCEL);
    exb_finish_create(&self->list, height);
}

void ui_expanded_barter_item_purchase_wnd_on_draw(struct UIWindow *window)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    exb_draw(&self->list, 0);
}

void ui_expanded_barter_item_purchase_wnd_on_l_btn_down(struct UIWindow *window, int x,
                                                           int y)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    struct BarterExFound at;

    exb_press(&self->list, x, y);
    exb_goods_at(&self->list, &at, x, y);
    if (at.found == 1) {
        struct GameMode *mode = game_current_mode();

        if (mode)
            game_mode_drag_expanded_barter_item(mode, DRAG_EXB_CART, &at.goods, at.goods.count, -1);
    }
    barter_goods_ex_destruct(&at.goods);
}

void ui_expanded_barter_item_purchase_wnd_on_mouse_hover(struct UIWindow *window, int x,
                                                             int y)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    exb_show_pay_tooltip(&self->list, x, y, 1);
}

void ui_expanded_barter_item_purchase_wnd_show_item_at(struct UIWindow *window, int x,
                                                           int y)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    exb_show_item_at(&self->list, x, y);
}

void ui_expanded_barter_item_purchase_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    exb_wheel(&self->list, delta);
}
BYTE ui_expanded_barter_item_purchase_wnd_can_buy(struct UIExpandedBarterItemPurchaseWnd *self)
{
    struct Tree counts;
    struct Vec cart;
    const struct BarterGoodsEx *goods;
    struct TreeNode *it;
    int slots, refusal = 0;
    BYTE ok = 1;

    tree_init(&counts);
    barter_goods_ex_vec_copy(&cart, expanded_barter_cart(g_expandedBarter));
    slots = session_get_inventory_count(&g_session1) + session_count_worn_equips(&g_session1)
          + session_count_worn_costumes(&g_session1);
    for (goods = cart.first; goods != cart.last; goods++) {
        barter_count_add(&counts, &goods->id_text, goods->count);
        if (session_is_equip_item_type(&g_session1, goods->type) == 1 || session_is_cash_item_type(&g_session1, goods->type) == 1)
            slots++;
        else
            session_get_inventory_index_by_id(&g_session1, atoi(string_cstr(&goods->id_text)));
    }
    if (slots > g_session1.inventoryExtension + BARTER_INVENTORY_BASE)
        refusal = MSG_BARTER_NO_ROOM;
    else if (!expanded_barter_can_pay(g_expandedBarter))
        refusal = MSG_BARTER_PAY_SHORT;
    if (refusal) {
        barter_can_buy_refusal(&self->list.frame.base, refusal);
        ok = 0;
    }
    for (it = tree_first(&counts); ok && it != counts.head; it = tree_next(it)) {
        const struct StrIntPair *entry = TREE_VALUE(it, const struct StrIntPair);
        int held = ui_barter_item_purchase_wnd_held_count(atoi(string_cstr(&entry->key)));

        if (entry->value + held > BARTER_MAX_COUNT) {
            struct String text;

            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
            string_init(&text);
            string_format(&text, msg_str(MSG_EXB_HELD_OVER_MAX), BARTER_MAX_COUNT);
            ui_chat_line(string_cstr(&text), BARTER_REFUSAL_COLOUR);
            ui_expanded_barter_item_purchase_wnd_send_msg(&self->list.frame.base, NULL, UI_MSG_COMMAND, UI_ID_CANCEL, 0, 0, 0);
            string_free(&text);
            ok = 0;
        }
    }
    barter_goods_ex_vec_free(&cart);
    tree_free(&counts, barter_count_destroy);
    return ok;
}

void ui_expanded_barter_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    struct Mode *mode = game_current_base_mode();

    (void)x, (void)y;
    if (drag->source != DRAG_EXB_GOODS) {
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (session_is_equip_item_type(&g_session1, drag->type) == 1 || session_is_cash_item_type(&g_session1, drag->type) == 1) {
        ui_expanded_barter_item_purchase_wnd_send_msg(&self->list.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send(ui_window_find(WND_ID_COUNT_INPUT), NULL, COUNT_MSG_OPEN, COUNT_FOR_EXB_PURCHASE, 0, 0, 0);
}

static void exb_drag_refusal(struct Mode *mode, int msg)
{
    ui_chat_line(msg_str(msg), BARTER_REFUSAL_COLOUR);
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

static void exb_add_dragged(struct UIExpandedBarterItemPurchaseWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();
    struct BarterExFound listed;
    const struct BarterExPay *pay;
    int weight;

    expanded_barter_find_goods(g_expandedBarter, &listed, drag->shop_index);
    if (!listed.found) {
        exb_drag_refusal(mode, MSG_BARTER_NOT_LISTED);
        goto done;
    }
    listed.goods.count = drag->drop_count;
    for (pay = (const struct BarterExPay *)listed.goods.pay.first; pay != (const struct BarterExPay *)listed.goods.pay.last; pay++) {
        struct ItemInfo item;
        int refused = 0;

        session_get_inventory_item(&g_session1, &item, pay->id);
        if (item.index != 0) {
            refused = 1;
        } else if (session_is_wearing_item_id_at(&g_session1, pay->id, pay->refine) == 1) {
            int n = session_get_inventory_count(&g_session1), i, count = 0;

            refused = 1;
            for (i = 0; i < n; i++) {
                struct ItemInfo held;

                session_get_inventory_item_at(&g_session1, &held, i);
                if (atoi(string_cstr(&held.id_text)) == pay->id && held.refine >= pay->refine) {
                    count = held.count;
                    item_info_destruct(&held);
                    refused = count <= 0;
                    break;
                }
                item_info_destruct(&held);
            }
        }
        if (refused) {
            mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
            ui_chat_line(msg_str(MSG_BARTER_PAY_ID_REFUSED), BARTER_REFUSAL_COLOUR);
            item_info_destruct(&item);
            goto done;
        }
        item_info_destruct(&item);
    }
    {
        int msg = 0;
        struct String id_text;

        if (session_is_equip_item_type(&g_session1, drag->type) == 1) {
            string_copy(&id_text, &drag->name);
            if (expanded_barter_cart_has_id(g_expandedBarter, id_text) == 1)
                msg = MSG_BARTER_TYPE_REFUSED;
        } else if (session_is_cash_item_type(&g_session1, drag->type) == 1) {
            string_copy(&id_text, &drag->name);
            if (expanded_barter_cart_has_id(g_expandedBarter, id_text) == 1)
                msg = MSG_BARTER_CASH_TYPE_REFUSED;
        }
        if (msg) {
            mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
            ui_chat_line(msg_str(msg), BARTER_REFUSAL_COLOUR);
            goto done;
        }
    }
    if (drag->drop_count + expanded_barter_cart_count(g_expandedBarter, drag->shop_index) > BARTER_MAX_COUNT) {
        char text[0xc8];

        memset(text, 0, sizeof text);
        snprintf(text, sizeof text, msg_str(MSG_EXB_OVER_MAX), BARTER_MAX_COUNT);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
        ui_chat_line(text, BARTER_REFUSAL_COLOUR);
        goto done;
    }
    weight = (int)expanded_barter_goods_weight(g_expandedBarter, drag->shop_index);
    if (weight < 0) {
        exb_drag_refusal(mode, MSG_BARTER_NOT_LISTED);
        goto done;
    }
    if (weight > 0) {
        int left = g_session1.maxWeight - g_session1.weight;
        float cart = expanded_barter_cart_weight(g_expandedBarter);

        if ((int)((float)left - cart) / weight < drag->drop_count) {
            exb_drag_refusal(mode, MSG_BARTER_TOO_HEAVY);
            goto done;
        }
    }
    if ((int64_t)(int)((unsigned)drag->drop_count * (unsigned)listed.goods.price) + expanded_barter_total(g_expandedBarter)
        > (int64_t)g_session1.zeny) {
        exb_drag_refusal(mode, MSG_EXB_NO_ZENY);
        goto done;
    }
    expanded_barter_cart_add(g_expandedBarter, &listed.goods);
    ui_expanded_barter_item_purchase_wnd_send_msg(&self->list.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
    ui_window_send(ui_window_find(WND_ID_EXB_MARKET), NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
done:
    barter_goods_ex_destruct(&listed.goods);
}

intptr_t ui_expanded_barter_item_purchase_wnd_send_msg(struct UIWindow *window,
                                                       struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                                       intptr_t val3, intptr_t val4)
{
    struct UIExpandedBarterItemPurchaseWnd *self = (struct UIExpandedBarterItemPurchaseWnd *)window;
    struct UIWindow *wnd = &self->list.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND: {
        struct Mode *mode = game_current_base_mode();

        if (val1 == UI_ID_OK) {
            if (!ui_expanded_barter_item_purchase_wnd_can_buy(self))
                return 0;
            mode_send_message(mode, MODE_MSG_EXB_BUY, 0, 0, 0, 0);
        } else if (val1 != UI_ID_CANCEL) {
            return 0;
        }
        mode_send_message(mode, MODE_MSG_BARTER_CLOSE, 0, 0, 0, 0);
        expanded_barter_clear(g_expandedBarter);
        return 0;
    }

    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        return exb_scroll(&self->list, msg, val1);

    case UI_MSG_RESIZE: {
        int height;

        if (val1 != EXB_PURCHASE_RESIZE_EDGE)
            return 0;
        height = exb_snap_height(val3);
        if (height != wnd->height)
            exb_resize(&self->list, wnd->width, height);
        ui_window_move_base(&self->buy->button.base, wnd->width - 0x69, wnd->height - 0x18);
        ui_window_move_base(&self->cancel->button.base, wnd->width - 0x3c, wnd->height - 0x18);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }

    case SHOP_MSG_REFRESH:
        exb_refresh(&self->list, expanded_barter_cart(g_expandedBarter));
        return 0;

    case UI_MSG_ADD_DRAGGED:
        exb_add_dragged(self);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->list.frame), sender, msg, val1, val2, val3, val4);
}
