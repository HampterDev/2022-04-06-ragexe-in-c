#include "ui/shops/shops.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/shops/npc_shop_internal.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"

struct DragInfo;
struct UIItemShopWnd;
struct UIWindow;
void ui_item_shop_wnd_destroy(struct UIWindow *window);
void ui_item_shop_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_item_shop_wnd_on_draw(struct UIWindow *self);
intptr_t ui_item_shop_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_item_shop_wnd_ops = {
    .destroy = ui_item_shop_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_shop_wnd_drag_and_drop,
    .store_info = ui_item_shop_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_shop_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_shop_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_shop_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_shop_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_shop_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_shop_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_shop_wnd_send_msg,
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

struct UIItemShopWnd2;
void ui_item_shop_wnd2_destroy(struct UIWindow *window);
void ui_item_shop_wnd2_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_item_shop_wnd2_store_info(struct UIWindow *self);
void ui_item_shop_wnd2_on_create(struct UIWindow *self, int width, int height);
void ui_item_shop_wnd2_on_draw(struct UIWindow *self);
void ui_item_shop_wnd2_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_shop_wnd2_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_item_shop_wnd2_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_item_shop_wnd2_on_r_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_item_shop_wnd2_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_item_shop_wnd2_ops = {
    .destroy = ui_item_shop_wnd2_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_shop_wnd2_drag_and_drop,
    .store_info = ui_item_shop_wnd2_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_shop_wnd2_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_shop_wnd2_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_shop_wnd2_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_shop_wnd2_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_shop_wnd2_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_shop_wnd2_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_shop_wnd2_send_msg,
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

static void price_unit_short(char *buf, int cash)
{
    sprintf(buf, "Z");
    if (!cash)
        return;
    sprintf(buf, "P");
}

static void shop_create(struct UIItemShopWnd2 *self, int height)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow *check;

    self->title = shop_add_title(wnd, 0x46, MSG_SHOP_TITLE);
    shop_add_resizer(&self->base, SHOP_RESIZE_EDGE);

    check = mem_alloc(sizeof(struct UICheckBox));
    if (check)
        check = &ui_check_box_construct(check)->base;
    self->check = check;
    ui_window_create(check, 0xc8, 0xc);
    ui_check_box_set_bitmaps(self->check, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp",
                          msg_str(MSG_SHOP_CHECK));
    ui_window_move_base(self->check, 0xc, wnd->height - 0x14);
    ui_check_box_set_check(self->check, g_windowMgr.drag_all);
    ui_window_add_child(wnd, self->check);

    shop_add_scroll_bar(&self->base, wnd->height - self->extra - self->bottom - ITEM_GRID_Y);
    self->base.columns = 1;
    self->base.rows = (height - self->extra - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

struct UIItemShopWnd2 *ui_item_shop_wnd2_construct(struct UIItemShopWnd2 *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_item_shop_wnd2_ops;
    self->hover = -1;
    self->base.show_counts = 0;
    self->with_count = 0;
    self->base.button_bar = 1;
    self->info = NULL;
    self->bottom = SHOP_BOTTOM;
    self->extra = 0;
    return self;
}

struct UIWindow *ui_item_shop_wnd2_new(void)
{
    return (struct UIWindow *)ui_item_shop_wnd2_construct(mem_alloc(sizeof(struct UIItemShopWnd2)));
}

void ui_item_shop_wnd2_destruct(struct UIItemShopWnd2 *self)
{
    self->base.frame.base.ops = &ui_item_shop_wnd2_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_item_shop_wnd2_destroy(struct UIWindow *window)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    ui_item_shop_wnd2_destruct(self);
    mem_free(self);
}

void ui_item_shop_wnd2_store_info(struct UIWindow *window)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

void ui_item_shop_wnd2_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    shop_create(self, height);
}

void ui_item_shop_wnd2_on_draw(struct UIWindow *window)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    int rows;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    rows = (wnd->height - self->extra - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    shop_draw_backdrop(&self->base, wnd->height - self->bottom - ITEM_GRID_Y, rows);
    shop_fill_names(wnd, rows);
    shop_draw_lines(&self->base, self->hover, 0, NULL);
}

void ui_item_shop_wnd2_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    npc_shop_show_price_tooltip(&self->base, x, y, NULL, self->with_count, 0);
}

void shop_resize(struct UIItemShopWnd2 *self, int width, int height, int with_check)
{
    struct UIWindow *wnd = &self->base.frame.base;

    if (width > SHOP_MAX_WIDTH)
        width = SHOP_MAX_WIDTH;
    if (height > SHOP_MAX_HEIGHT)
        height = SHOP_MAX_HEIGHT;
    if (width < SHOP_MIN_WIDTH)
        width = SHOP_MIN_WIDTH;
    if (height < SHOP_MIN_HEIGHT)
        height = SHOP_MIN_HEIGHT;
    width = (width - SHOP_WIDTH_BASE) / ITEM_GRID_CELL * ITEM_GRID_CELL + SHOP_WIDTH_BASE;
    height = (height - SHOP_MIN_HEIGHT) / ITEM_GRID_CELL * ITEM_GRID_CELL + SHOP_MIN_HEIGHT;
    if (width != wnd->width || height != wnd->height) {
        ui_window_resize_base(wnd, width, height);
        self->base.columns = 1;
        self->base.rows = (height - self->extra - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
        ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
        ui_window_move_base(&self->base.scroll_bar->base, wnd->width - 0xf, 0xe);
        ui_scroll_bar_resize(&self->base.scroll_bar->base, self->base.scroll_bar->base.width,
                 wnd->height - self->extra - self->bottom - ITEM_GRID_Y);
        if (with_check)
            ui_window_move_base(self->check, 0xc, wnd->height - 0x14);
        ui_item_base_wnd_update_scroll(&self->base);
    }
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

intptr_t ui_item_shop_wnd2_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                      intptr_t val3, intptr_t val4)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CHECKBOX)
            g_windowMgr.drag_all = val2;
        return 0;

    case UI_MSG_RESIZE:
        if (val1 == SHOP_RESIZE_EDGE)
            shop_resize(self, val2, val3, 1);
        return 0;

    case SHOP_MSG_REFRESH: {
        int count, i;

        list_clear(&self->base.items, item_info_destroy);
        count = session_sellable_count(&g_session1);
        for (i = 0; i < count; i++) {
            struct ItemInfo info;

            session_sellable_at(&g_session1, &info, i);
            if (info.index)
                item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), &info);
            item_info_destruct(&info);
        }
        ui_window_invalidate_base(&self->base.frame.base);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;
    }

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!shop_info_usable(self->info))
            shop_info_default(self->info);
        ui_window_send_msg(&self->base.frame.base, NULL, UI_MSG_RESIZE, SHOP_RESIZE_EDGE, self->info->width, self->info->height, 0);
        ui_frame_wnd_move(wnd, self->info->x, self->info->y);
        return 0;

    case SHOP_MSG_SET_MODE:
        self->with_count = 1;
        self->base.show_counts = 1;
        ui_static_text_set_text_fit(self->title, msg_str(MSG_SHOP2_CONVERT_TITLE));
        ((struct UICheckBox *)self->check)->disabled = 0;
        return 0;

    case UI_MSG_ADD_DRAGGED: {
        struct Mode *mode;
        struct ItemInfo info;
        struct UIWindow *list;

        if (val1 != 0 && val1 != 1)
            return 0;
        mode = game_current_base_mode();
        item_info_construct(&info);
        item_info_assign_from_drag(&info, game_current_drag());
        if (val1 == 0) {
            session_purchase_list_remove(&g_session1, &info);
            list = g_windowMgr.item_purchase_wnd;
        } else {
            session_sellable_add(&g_session1, &info);
            session_sell_list_remove(&g_session1, &info);
            list = g_windowMgr.item_sell_wnd2;
        }
        if (list)
            ui_window_send_msg(list, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        ui_window_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        item_info_destruct(&info);
        return 0;
    }
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

struct UIItemShopWnd *ui_item_shop_wnd_construct(struct UIItemShopWnd *self)
{
    ui_item_base_wnd_construct(&self->shop.base);
    self->shop.base.frame.base.ops = &ui_item_shop_wnd_ops;
    self->shop.hover = -1;
    self->shop.base.show_counts = 0;
    self->shop.with_count = 0;
    self->shop.base.button_bar = 1;
    self->shop.info = NULL;
    self->shop.bottom = SHOP_BOTTOM;
    self->shop.extra = 0;
    self->cash = 0;
    return self;
}

struct UIWindow *ui_item_shop_wnd_new(void)
{
    return (struct UIWindow *)ui_item_shop_wnd_construct(mem_alloc(sizeof(struct UIItemShopWnd)));
}

void ui_item_shop_wnd_destruct(struct UIItemShopWnd *self)
{
    self->shop.base.frame.base.ops = &ui_item_shop_wnd_ops;
    ui_item_base_wnd_destruct(&self->shop.base);
}

void ui_item_shop_wnd_destroy(struct UIWindow *window)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    ui_item_shop_wnd_destruct(self);
    mem_free(self);
}

void ui_item_shop_wnd_store_info(struct UIWindow *window)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    ui_item_shop_wnd2_store_info((struct UIWindow *)(&self->shop));
}

void ui_item_shop_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    shop_create(&self->shop, height);
}

void ui_item_shop_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIWindow *wnd = &self->shop.base.frame.base;
    int body, rows;
    char unit[8];

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    body = wnd->height - self->shop.bottom - ITEM_GRID_Y;
    rows = (wnd->height - self->shop.extra - self->shop.bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    shop_draw_backdrop(&self->shop.base, body, rows);
    if (self->cash)
        rows = body / ITEM_GRID_CELL;
    shop_fill_names(wnd, rows);
    if (self->cash)
        ui_window_fill_rect(wnd, ITEM_GRID_X, rows * ITEM_GRID_CELL - 0xf, ITEM_GRID_CELL, ITEM_GRID_CELL,
                          ui_skin_colour(2, 2));
    price_unit_short(unit, self->cash);
    shop_draw_lines(&self->shop.base, self->shop.hover, 1, unit);

    if (self->cash) {
        char line[0x148];
        struct String shortened = {0};
        const char *name = ui_window_shorten_text(wnd, &shortened, session_character_name(&g_session1),
                                               100, 0, 12, 0);
        int len;

        len = sprintf(line, msg_str(MSG_POINTS_CASH),
                          name,
                          g_session1.cashPoints);
        ui_window_text_out(wnd, 0xc, wnd->height - 0x15, line, len, 0, 12, 0, 0, 0);
        len = sprintf(line, msg_str(MSG_POINTS_FREE),
                          name,
                          g_session1.freePoints);
        ui_window_text_out(wnd, 0xc, wnd->height - self->shop.bottom - 0x15, line, len, 0, 12, 0, 0, 0);
        string_free(&shortened);
    }
}

void ui_item_shop_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    int old;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->shop.base.frame), x, y);
    old = self->shop.hover;
    self->shop.hover = ui_item_base_wnd_index_at(&self->shop.base, x, y);
    if (old != self->shop.hover)
        ui_window_invalidate_base(&self->shop.base.frame.base);
    {
        struct ItemInfo info;

        ui_item_base_wnd_get_item(&self->shop.base, &info, self->shop.hover);
        if (info.index) {
            struct GameMode *mode = game_current_mode();
            struct ItemInfo held;

            session_get_inventory_item(&g_session1, &held, info.index);
            game_mode_drag_item(mode, DRAG_SHOP_GOODS, &info, held.index ? info.count : 1, -1);
            item_info_destruct(&held);
        }
        item_info_destruct(&info);
    }
}

void ui_item_shop_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    char unit[8];

    npc_shop_price_unit_long(unit, self->cash);
    npc_shop_show_price_tooltip(&self->shop.base, x, y, unit, self->shop.with_count, 1);
}

static void guarded_drag_end(void)
{
    game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

void ui_item_shop_wnd2_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    if (drag->source == DRAG_SHOP_PURCHASE || drag->source == DRAG_SELL_LIST) {
        if (!self->with_count)
            return;
        npc_shop_drop_or_count(&self->base, drag->drop_count == 1 || g_windowMgr.drag_all, 1, COUNT_FOR_SHOP2);
        return;
    }
    guarded_drag_end();
}

void ui_item_shop_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIItemShopWnd2 *shop = &self->shop;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&shop->base.frame.base, x, y, drag);
        return;
    }
    if (drag->source == DRAG_SHOP_PURCHASE) {
        if (shop->with_count)
            return;
        npc_shop_drop_or_count(&shop->base, drag->drop_count == 1, 0, COUNT_FOR_SHOP_PURCHASE);
    } else if (drag->source == DRAG_SELL_LIST) {
        if (!shop->with_count)
            return;
        npc_shop_drop_or_count(&shop->base, drag->drop_count == 1 || g_windowMgr.drag_all, 1, COUNT_FOR_SHOP_SELL);
    } else {
        guarded_drag_end();
    }
}

void ui_item_shop_wnd2_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    struct UIWindow *target;

    if (self->with_count && g_windowMgr.drag_all && (target = g_windowMgr.item_sell_wnd2) != NULL)
        npc_shop_pass_on(&self->base, x, y, target, WND_ID_SELL2);
}

void ui_item_shop_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIWindow *target;

    if (self->shop.with_count && g_windowMgr.drag_all && (target = g_windowMgr.item_sell_wnd) != NULL)
        npc_shop_pass_on(&self->shop.base, x, y, target, WND_ID_SELL);
}

void ui_item_shop_wnd2_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    npc_shop_list_press(&self->base, &self->hover, x, y, 0);
}

void ui_item_shop_wnd2_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd2 *self = (struct UIItemShopWnd2 *)window;
    npc_shop_describe_at(&self->base, x, y, 1);
}

void ui_item_shop_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    npc_shop_describe_hovered(&self->shop.base, &self->shop.hover, x, y);
}

intptr_t ui_item_shop_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                     intptr_t val3, intptr_t val4)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIItemShopWnd2 *shop = &self->shop;
    struct UIWindow *wnd = &shop->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 != UI_ID_CHECKBOX)
            return 0;

        g_windowMgr.drag_all = val2;
        return 0;

    case UI_MSG_RESIZE:
        if (val1 == SHOP_RESIZE_EDGE)
            shop_resize(shop, val2, val3, 1);
        return 0;

    case SHOP_MSG_REFRESH:
        npc_shop_refill(&shop->base, session_sellable_count(&g_session1), session_sellable_at);
        ui_window_invalidate_base(&self->shop.base.frame.base);
        ui_item_base_wnd_update_scroll(&shop->base);
        return 0;

    case UI_MSG_SET_INFO:
        shop->info = (struct WindowInfo *)val1;
        npc_shop_list_set_info(&shop->base, shop->info, SHOP_RESIZE_EDGE, shop_info_usable, shop_info_default);
        return 0;

    case SHOP_MSG_SET_MODE: {
        int body;

        self->cash = val2;
        if (val1) {
            shop->with_count = 1;
            shop->base.show_counts = 1;
            ui_static_text_set_text_fit(shop->title, msg_str(MSG_SHOP_TITLE_COUNTED));
            ((struct UICheckBox *)shop->check)->disabled = 0;
        } else {
            shop->with_count = 0;
            shop->base.show_counts = 0;
            ui_static_text_set_text_fit(shop->title, msg_str(MSG_SHOP_TITLE_PLAIN));
            ((struct UICheckBox *)shop->check)->disabled = 1;
        }
        shop->extra = self->cash ? SHOP_CASH_EXTRA : 0;
        body = wnd->height - shop->bottom - shop->extra - ITEM_GRID_Y;
        shop->base.rows = body / ITEM_GRID_CELL;
        ui_scroll_bar_resize(&shop->base.scroll_bar->base, wnd->width - 0xf, body);
        ui_window_invalidate_base(&self->shop.base.frame.base);
        return 0;
    }

    case UI_MSG_ADD_DRAGGED: {
        struct Mode *mode;
        struct ItemInfo info;
        struct UIWindow *list;

        if (val1 != 0 && val1 != 1)
            return 0;
        mode = game_current_base_mode();
        item_info_construct(&info);
        item_info_assign_from_drag(&info, game_current_drag());
        if (val1 == 0) {
            session_purchase_list_remove(&g_session1, &info);
            list = g_windowMgr.item_purchase_wnd;
        } else {
            session_sellable_add(&g_session1, &info);
            session_sell_list_remove(&g_session1, &info);
            list = g_windowMgr.item_sell_wnd;
        }
        if (list)
            ui_window_send_msg(list, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        ui_window_send_msg(&self->shop.base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        item_info_destruct(&info);
        return 0;
    }
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&shop->base), sender, msg, val1, val2, val3, val4);
}
