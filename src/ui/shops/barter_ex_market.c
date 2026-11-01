#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/shops/barter_ex_internal.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"

struct DragInfo;
void ui_expanded_bartermarket_wnd_destroy(struct UIWindow *window);
void ui_expanded_bartermarket_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_expanded_bartermarket_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_expanded_bartermarket_wnd_on_draw(struct UIWindow *self);
void ui_expanded_bartermarket_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_expanded_bartermarket_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_expanded_bartermarket_wnd_show_item_at(struct UIWindow *self, int x, int y);
void ui_expanded_bartermarket_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_expanded_bartermarket_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_expanded_bartermarket_wnd_ops = {
    .destroy = ui_expanded_bartermarket_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_expanded_bartermarket_wnd_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_expanded_bartermarket_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_expanded_bartermarket_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_expanded_bartermarket_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_expanded_bartermarket_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_expanded_bartermarket_wnd_show_item_at,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_expanded_bartermarket_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_expanded_bartermarket_wnd_send_msg,
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

struct ExpandedBarterWnd *ui_expanded_bartermarket_wnd_construct(struct ExpandedBarterWnd *self)
{
    exb_construct(self, &ui_expanded_bartermarket_wnd_ops);
    return self;
}

struct UIWindow *ui_expanded_bartermarket_wnd_new(void)
{
    return (struct UIWindow *)ui_expanded_bartermarket_wnd_construct(mem_alloc(sizeof(struct ExpandedBarterWnd)));
}

void ui_expanded_bartermarket_wnd_destruct(struct ExpandedBarterWnd *self)
{
    exb_destruct(self, &ui_expanded_bartermarket_wnd_ops);
}

void ui_expanded_bartermarket_wnd_destroy(struct UIWindow *window)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    ui_expanded_bartermarket_wnd_destruct(self);
    mem_free(self);
}

void ui_expanded_bartermarket_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    (void)width;
    exb_add_resizer_and_bar(self, MARKET_RESIZE_EDGE);
    exb_finish_create(self, height);
}

void ui_expanded_bartermarket_wnd_on_draw(struct UIWindow *window)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    exb_draw(self, 1);
}

void ui_expanded_bartermarket_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    struct BarterExFound at;

    exb_press(self, x, y);
    exb_goods_at(self, &at, x, y);
    if (at.found == 1) {
        if (at.goods.count == 0) {
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
            barter_refusal(MSG_EXB_COUNT_0_REFUSED);
        } else {
            struct GameMode *mode = game_current_mode();

            if (mode)
                game_mode_drag_expanded_barter_item(mode, DRAG_EXB_GOODS, &at.goods, 1, -1);
        }
    }
    barter_goods_ex_destruct(&at.goods);
}

void ui_expanded_bartermarket_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    exb_show_pay_tooltip(self, x, y, 0);
}

void ui_expanded_bartermarket_wnd_show_item_at(struct UIWindow *window, int x, int y)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    exb_show_item_at(self, x, y);
}

void ui_expanded_bartermarket_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    exb_wheel(self, delta);
}
void ui_expanded_bartermarket_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    (void)x, (void)y;
    if (drag->source != DRAG_EXB_CART) {
        game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->drop_count == 1) {
        ui_window_send_msg(&self->frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send(ui_window_find(WND_ID_COUNT_INPUT), NULL, COUNT_MSG_OPEN, COUNT_FOR_EXB_MARKET, 0, 0, 0);
}

int ui_expanded_bartermarket_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                                 int val1, int val2, int val3, int val4)
{
    struct ExpandedBarterWnd *self = (struct ExpandedBarterWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        return exb_scroll(self, msg, val1);

    case UI_MSG_RESIZE: {
        int width = val2, height;

        if (val1 != EXB_MARKET_RESIZE_EDGE)
            return 0;
        if (width < 0x16e)
            width = 0x16e;
        else if ((float)width > 439.2f)
            width = 0x1b7;
        height = exb_snap_height(val3);
        if (width != wnd->width || height != wnd->height)
            exb_resize(self, width, height);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }

    case SHOP_MSG_REFRESH:
        exb_refresh(self, expanded_barter_goods(g_expandedBarter));
        return 0;

    case UI_MSG_ADD_DRAGGED: {
        struct Mode *mode = game_current_base_mode();
        const struct DragInfo *drag = game_current_drag();
        struct BarterExFound in_cart;

        expanded_barter_find_in_cart(g_expandedBarter, &in_cart, drag->shop_index);
        if (in_cart.found) {
            in_cart.goods.count = drag->drop_count;
            expanded_barter_cart_remove(g_expandedBarter, &in_cart.goods);
            ui_window_send(ui_window_find(WND_ID_EXB_PURCHASE), NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
            ui_window_send_msg(&self->frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        }
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        barter_goods_ex_destruct(&in_cart.goods);
        return 0;
    }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
