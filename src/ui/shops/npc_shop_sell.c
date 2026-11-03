#include "ui/shops/shops.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "ui/shops/npc_shop_internal.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"
#include "ui/wndmgr/wndmgr.h"

struct DragInfo;
struct UIWindow;
struct UIItemSellWnd;
void ui_item_sell_wnd_destroy(struct UIWindow *window);
void ui_item_sell_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_item_sell_wnd_store_info(struct UIWindow *self);
void ui_item_sell_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_sell_wnd_on_draw(struct UIWindow *self);
void ui_item_sell_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_sell_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_item_sell_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_item_sell_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_item_sell_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_item_sell_wnd_ops = {
    .destroy = ui_item_sell_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_sell_wnd_drag_and_drop,
    .store_info = ui_item_sell_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_sell_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_sell_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_sell_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_sell_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_sell_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_sell_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_sell_wnd_send_msg,
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

struct UIItemSellWnd2;
void ui_item_sell_wnd2_destroy(struct UIWindow *window);
void ui_item_sell_wnd2_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_item_sell_wnd2_store_info(struct UIWindow *self);
void ui_item_sell_wnd2_on_create(struct UIWindow *self, int width, int height);
void ui_item_sell_wnd2_on_draw(struct UIWindow *self);
void ui_item_sell_wnd2_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_sell_wnd2_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_item_sell_wnd2_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_item_sell_wnd2_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_item_sell_wnd2_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_item_sell_wnd2_ops = {
    .destroy = ui_item_sell_wnd2_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_sell_wnd2_drag_and_drop,
    .store_info = ui_item_sell_wnd2_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_sell_wnd2_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_sell_wnd2_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_sell_wnd2_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_sell_wnd2_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_sell_wnd2_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_sell_wnd2_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_sell_wnd2_send_msg,
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

static void drop_goods(struct UIItemBaseWnd *base, int x, int y, const struct DragInfo *drag, int count_for)
{
    struct Mode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&base->frame.base, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    if (drag->source != DRAG_SHOP_GOODS) {
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->drop_count == 1 || g_windowMgr.drag_all) {
        ui_window_send_msg(&base->frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, count_for, 0, 0, 0);
}
static const char *const sell_button[3] = { "btn_sell", "btn_sell_a", "btn_sell_b" };
static const char *const exchange_button[3] = { "btn_exchange", "btn_exchange_a", "btn_exchange_b" };

static void sell_create(struct UIItemSellWnd *self, int height, const char *total_text, int title_msg,
                        const char *const first[3])
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow *total = mem_alloc(sizeof(struct UIStaticText));
    struct UIBitmapButton **buttons = (struct UIBitmapButton **)&self->base.close;

    if (total)
        total = ui_static_text_construct(total);
    self->total = total;
    ui_window_create(total, 0xa0, 0xf);
    ui_static_text_set_text_fit(self->total, total_text);
    ui_window_move(self->total, 0xc, wnd->height - 0x15);
    ui_window_add_child(wnd, self->total);

    shop_add_title(wnd, 0x5a, title_msg);
    shop_add_resizer(&self->base, 3);
    shop_add_scroll_bar(&self->base, wnd->height - 0x26);
    buttons[0] = shop_add_button(wnd, first, wnd->width - 0x6a, wnd->height - 0x18, UI_ID_OK);
    buttons[1] = shop_add_button(wnd, g_shopCancelButton, wnd->width - 0x3d, wnd->height - 0x18, UI_ID_CANCEL);

    self->base.columns = 1;
    self->base.rows = (height - 0x26) / ITEM_GRID_CELL;
    ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

static void sell_draw(struct UIItemSellWnd *self, int priced)
{
    struct UIWindow *wnd = &self->base.frame.base;
    int rows;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    rows = (wnd->height - 0x26) / ITEM_GRID_CELL;
    shop_draw_backdrop(&self->base, wnd->height - 0x26, rows);
    shop_fill_names(wnd, rows);
    shop_draw_lines(&self->base, self->hover, priced, NULL);
}

struct UIItemSellWnd *ui_item_sell_wnd_construct(struct UIItemSellWnd *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_item_sell_wnd_ops;
    self->hover = -1;
    self->base.button_bar = 1;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_item_sell_wnd_new(void)
{
    return (struct UIWindow *)ui_item_sell_wnd_construct(mem_alloc(sizeof(struct UIItemSellWnd)));
}

void ui_item_sell_wnd_destruct(struct UIItemSellWnd *self)
{
    self->base.frame.base.ops = &ui_item_sell_wnd_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_item_sell_wnd_destroy(struct UIWindow *window)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    ui_item_sell_wnd_destruct(self);
    mem_free(self);
}

void ui_item_sell_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    drop_goods(&self->base, x, y, drag, COUNT_FOR_SELL);
}

void ui_item_sell_wnd_store_info(struct UIWindow *window)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    npc_shop_store_info(&self->base.frame.base, self->info);
}

void ui_item_sell_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    char total[0x40];

    sprintf(total, "Total : %d Zeny", 0);
    sell_create(self, height, total, MSG_SELL_TITLE, sell_button);
}

void ui_item_sell_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    sell_draw(self, 1);
}

void ui_item_sell_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    npc_shop_show_price_tooltip(&self->base, x, y, NULL, 1, 1);
}

struct UIItemSellWnd2 *ui_item_sell_wnd2_construct(struct UIItemSellWnd2 *self)
{
    ui_item_base_wnd_construct(&self->sell.base);
    self->sell.base.frame.base.ops = &ui_item_sell_wnd2_ops;
    self->sell.hover = -1;
    self->sell.base.button_bar = 1;
    self->sell.info = NULL;
    self->window_type = 0;
    return self;
}

struct UIWindow *ui_item_sell_wnd2_new(void)
{
    return (struct UIWindow *)ui_item_sell_wnd2_construct(mem_alloc(sizeof(struct UIItemSellWnd2)));
}

void ui_item_sell_wnd2_destruct(struct UIItemSellWnd2 *self)
{
    self->sell.base.frame.base.ops = &ui_item_sell_wnd2_ops;
    ui_item_base_wnd_destruct(&self->sell.base);
}

void ui_item_sell_wnd2_destroy(struct UIWindow *window)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    ui_item_sell_wnd2_destruct(self);
    mem_free(self);
}

void ui_item_sell_wnd2_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    drop_goods(&self->sell.base, x, y, drag, COUNT_FOR_SELL2);
}

void ui_item_sell_wnd2_store_info(struct UIWindow *window)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    npc_shop_store_info(&self->sell.base.frame.base, self->sell.info);
}

void ui_item_sell_wnd2_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    sell_create(&self->sell, height, "", MSG_SELL2_TITLE, exchange_button);
}

void ui_item_sell_wnd2_on_draw(struct UIWindow *window)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    sell_draw(&self->sell, 0);
}

void ui_item_sell_wnd2_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    npc_shop_show_price_tooltip(&self->sell.base, x, y, NULL, 1, 0);
}

void ui_item_sell_wnd2_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    struct UIWindow *target;

    if (g_windowMgr.drag_all && (target = g_windowMgr.item_shop_wnd2) != NULL)
        npc_shop_pass_on(&self->sell.base, x, y, target, WND_ID_SELL2_SHOP);
}

void ui_item_sell_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    struct UIWindow *target;

    if (g_windowMgr.drag_all && (target = g_windowMgr.item_shop_wnd) != NULL)
        npc_shop_pass_on(&self->base, x, y, target, WND_ID_SHOP2);
}

void ui_item_sell_wnd2_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    npc_shop_list_press(&self->sell.base, &self->sell.hover, x, y, 1);
}

void ui_item_sell_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    npc_shop_list_press(&self->base, &self->hover, x, y, 1);
}

void ui_item_sell_wnd2_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    npc_shop_describe_at(&self->sell.base, x, y, 0);
}

void ui_item_sell_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    npc_shop_describe_at(&self->base, x, y, 0);
}

static void sell_add_dragged(struct UIItemBaseWnd *base, struct UIWindow *other)
{
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();
    struct ItemInfo info;

    item_info_construct(&info);
    item_info_assign_from_drag(&info, drag);
    if (session_has_shop_goods(&g_session1, drag->index)) {
        session_add_shop_sell_item(&g_session1, &info);
        session_remove_shop_goods(&g_session1, &info);
        if (other)
            ui_window_send_msg(other, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        ui_window_send_msg(&base->frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
    }
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
    item_info_destruct(&info);
}

int ui_item_sell_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIItemSellWnd *self = (struct UIItemSellWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND: {
        struct Mode *mode;

        mode = client_current_mode();
        if (val1 != UI_ID_OK && val1 != UI_ID_CANCEL)
            return 0;
        if (val1 == UI_ID_OK && session_has_shop_sell_items(&g_session1))
            mode_send_message(mode, MODE_REQ_SELL_CHECKED, 0, 0, 0, 0);
        ui_close_window(WND_ID_SHOP2);
        ui_close_window(WND_ID_SELL);
        mode_send_message(mode, MODE_REQ_SHOP_CLOSE, 0, 0, 0, 0);
        session_clear_lists(&g_session1);
        return 0;
    }

    case UI_MSG_RESIZE:
        if (val1 != 3)
            return 0;
        npc_shop_list_resize(&self->base, val2, val3, 0x26);
        ui_window_move(self->total, 0xc, wnd->height - 0x14);
        npc_shop_list_place_buttons(&self->base);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;

    case SHOP_MSG_REFRESH: {
        char text[0x40], money[0x40];
        int total = npc_shop_refill(&self->base, session_get_shop_sell_list_size(&g_session1), session_get_shop_sell_item_at);

        sprintf(text, "Total : %s Zeny", format_money(total, money));
        ui_static_text_set_text_fit(self->total, text);
        ui_window_invalidate_base(&self->base.frame.base);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;
    }

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        npc_shop_list_set_info(&self->base, self->info, 3, sell_info_usable, sell_info_default);
        return 0;

    case UI_MSG_ADD_DRAGGED:
        sell_add_dragged(&self->base, g_windowMgr.item_shop_wnd);
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

int ui_item_sell_wnd2_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UIItemSellWnd2 *self = (struct UIItemSellWnd2 *)window;
    struct UIItemBaseWnd *base = &self->sell.base;
    struct UIWindow *wnd = &base->frame.base;

    switch (msg) {
    case UI_MSG_COMMAND: {
        int count = session_get_shop_sell_list_size(&g_session1);
        struct Mode *mode = client_current_mode();

        if (val1 == UI_ID_OK) {
            if (count <= 0)
                return 0;
            if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_SELL2_CONFIRM), UI_MSGBOX_QUESTION, 1, 0, 0,
                                       UI_MSGBOX_TITLE, 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES)
                mode_send_message(mode, MODE_REQ_SELL2_YES, self->window_type, 0, 0, 0);
            else
                mode_send_message(mode, MODE_REQ_SELL2_NO, self->window_type, 0, 0, 0);
        } else if (val1 == UI_ID_CANCEL) {
            mode_send_message(mode, MODE_REQ_SELL2_NO, self->window_type, 0, 0, 0);
        } else {
            return 0;
        }
        ui_close_window(WND_ID_SELL2_SHOP);
        ui_close_window(WND_ID_SELL2);
        session_clear_lists(&g_session1);
        return 0;
    }

    case UI_MSG_RESIZE:
        if (val1 != 3)
            return 0;
        npc_shop_list_resize(base, val2, val3, 0x26);
        npc_shop_list_place_buttons(base);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;

    case SHOP_MSG_REFRESH:
        if (val1)
            self->window_type = val1;
        npc_shop_refill(base, session_get_shop_sell_list_size(&g_session1), session_get_shop_sell_item_at);
        ui_window_invalidate_base(&self->sell.base.frame.base);
        ui_item_base_wnd_update_scroll(base);
        return 0;

    case UI_MSG_SET_INFO:
        self->sell.info = (struct WindowInfo *)val1;
        npc_shop_list_set_info(base, self->sell.info, 3, sell_info_usable, sell_info_default);
        return 0;

    case UI_MSG_ADD_DRAGGED:
        sell_add_dragged(base, g_windowMgr.item_shop_wnd2);
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(base), sender, msg, val1, val2, val3, val4);
}
