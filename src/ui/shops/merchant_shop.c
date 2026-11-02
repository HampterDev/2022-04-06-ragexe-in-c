#include "ui/shops/shops.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/shops/merchant.h"
#include "ui/shops/merchant_shop.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

struct UIMerchantItemShopWnd;
struct UIWindow;
void ui_merchant_item_shop_wnd_destroy(struct UIWindow *window);
void ui_merchant_item_shop_wnd_store_info(struct UIWindow *self);
void ui_merchant_item_shop_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_merchant_item_shop_wnd_on_process(struct UIWindow *self);
void ui_merchant_item_shop_wnd_on_draw(struct UIWindow *self);
void ui_merchant_item_shop_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_merchant_item_shop_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);

const struct UIWindowOps ui_merchant_item_shop_wnd_ops = {
    .destroy = ui_merchant_item_shop_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_merchant_item_shop_wnd_drag_and_drop,
    .store_info = ui_merchant_item_shop_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_merchant_item_shop_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_merchant_item_shop_wnd_on_process,
    .on_draw = ui_merchant_item_shop_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_merchant_item_shop_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_merchant_item_shop_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_merchant_item_shop_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_merchant_item_shop_wnd_send_msg,
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

struct UIMerchantItemMyShopWnd;
void ui_merchant_item_my_shop_wnd_destroy(struct UIWindow *window);
void ui_merchant_item_my_shop_wnd_store_info(struct UIWindow *self);
void ui_merchant_item_my_shop_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_merchant_item_my_shop_wnd_on_process(struct UIWindow *self);
void ui_merchant_item_my_shop_wnd_on_draw(struct UIWindow *self);
void ui_merchant_item_my_shop_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_merchant_item_my_shop_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);

const struct UIWindowOps ui_merchant_item_my_shop_wnd_ops = {
    .destroy = ui_merchant_item_my_shop_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_base_wnd_drag_and_drop,
    .store_info = ui_merchant_item_my_shop_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_merchant_item_my_shop_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_merchant_item_my_shop_wnd_on_process,
    .on_draw = ui_merchant_item_my_shop_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_merchant_item_my_shop_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_merchant_item_my_shop_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_merchant_item_my_shop_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_merchant_item_my_shop_wnd_send_msg,
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

struct UIMerchantItemShopWnd *ui_merchant_item_shop_wnd_construct(struct UIMerchantItemShopWnd *self, int variant)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_merchant_item_shop_wnd_ops;
    self->notice_amount = 0;
    self->variant = variant;
    self->base.button_bar = 1;
    self->hover = -1;
    self->info = NULL;
    self->fixed_height = 0;
    self->timer_start = 0;
    self->timer_length = 0;
    self->bottom = SHOP_WND_BOTTOM;
    return self;
}

struct UIWindow *ui_merchant_item_shop_wnd_new(int arg)
{
    return (struct UIWindow *)ui_merchant_item_shop_wnd_construct(mem_alloc(sizeof(struct UIMerchantItemShopWnd)), arg);
}

void ui_merchant_item_shop_wnd_destruct(struct UIMerchantItemShopWnd *self)
{
    self->base.frame.base.ops = &ui_merchant_item_shop_wnd_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_merchant_item_shop_wnd_destroy(struct UIWindow *window)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    ui_merchant_item_shop_wnd_destruct(self);
    mem_free(self);
}

void ui_merchant_item_shop_wnd_store_info(struct UIWindow *window)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    merchant_store_info(&self->base.frame.base, self->info);
}

void ui_merchant_item_shop_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    (void)width;
    if (self->variant) {
        struct UIWindow *notice = mem_alloc(sizeof(struct UIStaticText));
        char text[0x50];

        if (notice)
            notice = ui_static_text_construct(notice);
        self->notice = notice;
        ui_window_create(notice, 0xa0, 0xf);
        sprintf(text, msg_str(MSG_SHOP_NOTICE), (const char *)NULL);
        ui_static_text_set_text_fit(self->notice, text);
        ui_window_move(self->notice, 0xc, wnd->height - 0x15);
        ui_window_add_child(wnd, self->notice);
    }
    self->base.rows = (unsigned)(height - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    self->base.columns = 1;
    self->fixed_height = wnd->height - self->base.rows * ITEM_GRID_CELL;
    ui_merchant_item_shop_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_merchant_item_shop_wnd_on_process(struct UIWindow *window)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    ui_window_invalidate_base(&self->base.frame.base);
}

static void shop_draw_time_left(struct UIWindow *wnd, int remaining, int x, int y, int size)
{
    struct String text;

    string_init(&text);
    string_format(&text, msg_str(MSG_SHOP_TIME_LEFT), remaining / 86400000, remaining / 3600000 % 24,
                  remaining / 60000 % 60, remaining / 1000 % 60);
    ui_window_text_out(wnd, x, y, string_cstr(&text), 0, 0, size, remaining <= 60000 ? 0xff : 0, 1, 0);
    string_free(&text);
}

static int shop_time_left(DWORD timer_start, DWORD timer_length)
{
    if (!timer_length)
        return 0;
    return (int)(timer_start - platform_ticks_ms() + timer_length);
}

static void shop_draw_item(struct UIMerchantItemShopWnd *self, const struct ItemInfo *info, int col_x, int row_y)
{
    struct UIWindow *wnd = &self->base.frame.base;
    char money[0x80], price[0x80];
    struct String text;
    int z_width, width, name_width;

    if (self->variant) {
        sprintf(money, msg_str(MSG_SHOP_COUNT), info->count);
        width = wnd->width - ui_window_text_width(wnd, "  100,000,000Z", 0, 0, 12, 0, 0);
        width -= ui_window_text_width(wnd, money, 0, 0, 12, 0, 0);
        ui_window_text_out(wnd, width - 0x12, row_y + 0x1d, money, 0, 0, 12, 0xff, 0, 0);
        ui_window_draw_item_icon(wnd, col_x + 0x2c, row_y + 0x15, info, 0);
    } else {
        ui_window_draw_item_icon_count(wnd, col_x + 0x2c, row_y + 0x15, info, 0, 0);
    }
    z_width = ui_window_text_width(wnd, "Z", 0, 0, 12, 0, 0);
    ui_window_text_out(wnd, wnd->width - z_width - 0x12, row_y + 0x1d, "Z", 0, 0, 12, 0, 0, 0);
    if (info->price != info->charged_price)
        sprintf(price, " %d -> %s ", info->price, format_money(info->charged_price, money));
    else
        sprintf(price, " %s ", format_money(info->price, money));
    width = z_width + ui_window_text_width(wnd, price, 0, 0, 12, 0, 0);
    string_init(&text);
    string_assign_cstr(&text, price);
    ui_window_text_out_price(wnd, info->price, text, wnd->width - width - 0x12, row_y + 0x1d);
    name_width = wnd->width - width - (col_x + ITEM_GRID_X) - 0x42;
    if (name_width > 0x82)
        name_width = 0x82;
    ui_window_draw_item_name_short(wnd, col_x + 0x4c, row_y + 0x1d, name_width, info, 12);
}

void ui_merchant_item_shop_wnd_on_draw(struct UIWindow *window)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    struct UIItemBaseWnd *base = &self->base;
    struct UIWindow *wnd = &base->frame.base;
    struct ListNode *node;
    struct String name;
    const char *label;
    char title[0x80];
    int body, rows, i, remaining, cell = 0;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    body = wnd->height - self->bottom - ITEM_GRID_Y;
    shop_draw_frame(base, body, base->button_bar ? 2 : 0);
    rows = (unsigned)body / ITEM_GRID_CELL;
    shop_draw_cells(wnd, rows);
    shop_fill_names(wnd, rows);

    game_mode_get_actor_name(game_current_mode(), &name, self->actor_id);
    label = msg_str(self->variant ? MSG_SHOP_TITLE_VARIANT : MSG_SHOP_TITLE);
    if (name.size) {
        sprintf(title, "%s - %s", label, string_cstr(&name));
    } else {
        ui_window_mgr_add_to_name_waiting_list(&g_windowMgr, wnd);
        sprintf(title, "%s", label);
    }
    ui_window_text_out(wnd, 0x11, 3, title, 0, 0, 12, 0xffffff, 0, 0);
    ui_window_text_out(wnd, 0x10, 2, title, 0, 0, 12, 0, 0, 0);
    string_free(&name);

    for (node = base->items.head->next, i = 0; node != base->items.head; node = node->next, i++) {
        int row_y, col_x;

        if (item_grid_hidden(LIST_VALUE(node, struct ItemInfo)))
            continue;
        row_y = cell / base->columns * ITEM_GRID_CELL;
        col_x = cell % base->columns * ITEM_GRID_CELL;
        if (self->hover == i)
            ui_window_fill_rect(wnd, col_x + 0x48, row_y + 0x13, 0xbf, 0x1c, ui_skin_colour(6, 2));
        shop_draw_item(self, LIST_VALUE(node, struct ItemInfo), col_x, row_y);
        cell++;
    }

    if (self->bottom == SHOP_WND_BOTTOM_BAR)
        ui_window_draw_button_bar(wnd, 3, 0, 1);
    remaining = shop_time_left(self->timer_start, self->timer_length);
    if (remaining > 0) {
        shop_draw_time_left(wnd, remaining, 0x14, wnd->height - 0x32, 12);
        if (self->variant == 1)
            ui_window_move(self->notice, 0xaa, wnd->height - 0x14);
    }
}

void ui_merchant_item_shop_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    struct ItemInfo info;
    int old;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);
    if (self->variant)
        return;
    old = self->hover;
    self->hover = ui_item_base_wnd_index_at(&self->base, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(&self->base.frame.base);
    ui_item_base_wnd_get_item(&self->base, &info, self->hover);
    if ((unsigned)info.index > 0)
        game_mode_drag_item(game_current_mode(), DRAG_SHOP_ITEM, &info, 1, -1);
    item_info_destruct(&info);
}

static void shop_item_tooltip(struct ItemInfo *info, int x, int y)
{
    struct String name = {0};
    struct Mode *mode;

    item_info_get_display_name(info, &name);
    mode = client_current_mode();
    if (info->identified == 1 && info->option_count) {
        struct String text;

        string_init(&text);
        string_format(&text, msg_str(MSG_TIP_SHOP_OPTIONS), string_cstr(&name), info->option_count);
        mode_show_tooltip(mode, string_cstr(&text), x, y,
                          session_get_item_dis_color(info), 0);
        string_free(&text);
    } else {
        mode_show_tooltip(mode, string_cstr(&name), x, y,
                          session_get_item_dis_color(info), 0);
    }
    string_free(&name);
}

void ui_merchant_item_shop_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemShopWnd *self = (struct UIMerchantItemShopWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct ItemInfo info;
    int tip_x, tip_y;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if ((unsigned)info.index > 0) {
        shop_tooltip_cell(wnd, x, y, &tip_x, &tip_y);
        shop_item_tooltip(&info, tip_x, tip_y);
    } else {
        struct ItemInfo line;

        ui_item_base_wnd_get_item(&self->base, &line, ui_item_base_wnd_index_at(&self->base, x, y));
        item_info_assign(&info, &line);
        item_info_destruct(&line);
        if ((unsigned)info.index > 0) {
            tip_x = 0x48;
            tip_y = (y - ITEM_GRID_Y) / ITEM_GRID_CELL * ITEM_GRID_CELL + 1;
            ui_window_to_screen(wnd, &tip_x, &tip_y);
            shop_item_tooltip(&info, tip_x - 2, tip_y + 0x18);
        }
    }
    item_info_destruct(&info);
}

static const char *const close_button[3] = { "btn_close", "btn_close_a", "btn_close_b" };

struct UIMerchantItemMyShopWnd *ui_merchant_item_my_shop_wnd_construct(struct UIMerchantItemMyShopWnd *self,
                                                                             int variant)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_merchant_item_my_shop_wnd_ops;
    self->variant = variant;
    self->bottom = SHOP_WND_BOTTOM;
    self->base.button_bar = 1;
    self->info = NULL;
    self->assistant = 0;
    self->owner_aid = 0;
    self->timer_start = 0;
    self->timer_length = 0;
    return self;
}

struct UIWindow *ui_merchant_item_my_shop_wnd_new(int arg)
{
    return (struct UIWindow *)ui_merchant_item_my_shop_wnd_construct(mem_alloc(sizeof(struct UIMerchantItemMyShopWnd)), arg);
}

void ui_merchant_item_my_shop_wnd_destruct(struct UIMerchantItemMyShopWnd *self)
{
    self->base.frame.base.ops = &ui_merchant_item_my_shop_wnd_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_merchant_item_my_shop_wnd_destroy(struct UIWindow *window)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    ui_merchant_item_my_shop_wnd_destruct(self);
    mem_free(self);
}

void ui_merchant_item_my_shop_wnd_store_info(struct UIWindow *window)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    merchant_store_info(&self->base.frame.base, self->info);
}

void ui_merchant_item_my_shop_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow *check;

    if (self->variant) {
        struct UIWindow *notice = mem_alloc(sizeof(struct UIStaticText));
        char text[0x50];

        if (notice)
            notice = ui_static_text_construct(notice);
        self->notice = notice;
        ui_window_create(notice, 0xa0, 0xf);
        sprintf(text, msg_str(MSG_SHOP_NOTICE), (const char *)NULL);
        ui_static_text_set_text_fit(self->notice, text);
        ui_window_move(self->notice, 0xc, wnd->height - 0x1a);
        ui_window_add_child(wnd, self->notice);
    }
    self->title = shop_add_title(wnd, 0xf0, self->variant ? MSG_MY_SHOP_TITLE_VARIANT : MSG_MY_SHOP_TITLE);
    shop_add_resizer(&self->base, MY_SHOP_RESIZE_EDGE);
    shop_add_scroll_bar(&self->base, wnd->height - self->bottom - ITEM_GRID_Y);

    check = mem_alloc(sizeof(struct UICheckBox));
    if (check)
        check = &ui_check_box_construct(check)->base;
    self->check = check;
    ui_window_create(check, 0xf0, 0xc);
    ui_check_box_set_bitmaps(self->check, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp",
                          msg_str(self->variant ? MSG_MY_SHOP_CHECK_VARIANT : MSG_MY_SHOP_CHECK));
    ui_check_box_fit_to_label((struct UICheckBox *)self->check);
    ui_window_move_base(self->check, 0xc, wnd->height - (self->variant ? 0xc : 0x12));
    ui_check_box_set_check(self->check, g_session1.noSellWndSound);
    ui_window_add_child(wnd, self->check);

    self->base.close = shop_add_button(wnd, close_button, wnd->width - 0x3d, wnd->height - 0x18,
                                       UI_ID_MY_SHOP_CLOSE);
    self->base.columns = (width - ITEM_GRID_X) / ITEM_GRID_CELL;
    self->base.rows = (unsigned)(height - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    ui_merchant_item_my_shop_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_merchant_item_my_shop_wnd_on_process(struct UIWindow *window)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    ui_window_invalidate_base(&self->base.frame.base);
}

void ui_merchant_item_my_shop_wnd_on_draw(struct UIWindow *window)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    int remaining;

    ui_item_base_wnd_on_draw((struct UIWindow *)(&self->base));
    if (self->bottom == SHOP_WND_BOTTOM_BAR)
        ui_window_draw_button_bar(wnd, 3, 0, 1);
    remaining = shop_time_left(self->timer_start, self->timer_length);
    if (remaining <= 0)
        return;
    if (self->variant == 1)
        shop_draw_time_left(wnd, remaining, 0xc, wnd->height - 0x2a, 0xb);
    else
        shop_draw_time_left(wnd, remaining, 0x14, wnd->height - 0x32, 12);
}

void ui_merchant_item_my_shop_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);
}

void ui_merchant_item_my_shop_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemMyShopWnd *self = (struct UIMerchantItemMyShopWnd *)window;
    struct ItemInfo info;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if ((unsigned)info.index > 0) {
        struct String text;
        struct String name = {0};
        char money[0x30];
        int tip_x, tip_y;

        shop_tooltip_cell(&self->base.frame.base, x, y, &tip_x, &tip_y);
        item_info_get_display_name(&info, &name);
        string_init(&text);
        if (self->variant)
            string_format(&text, msg_str(MSG_TIP_MY_SHOP_VARIANT), string_cstr(&name),
                          format_money(info.charged_price, money), info.count);
        else if (info.identified == 1 && info.option_count)
            string_format(&text, msg_str(MSG_TIP_MY_SHOP_OPTIONS), string_cstr(&name), info.option_count,
                          format_money(info.charged_price, money));
        else
            string_format(&text, "%s %s Zeny", string_cstr(&name), format_money(info.charged_price, money));
        string_free(&name);
        mode_show_tooltip(client_current_mode(), string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&info), 0);
        string_free(&text);
    }
    item_info_destruct(&info);
}

#define MSG_ASSISTANT_BUTTON 0xb6d
#define UI_ID_ASSISTANT_BUTTON 0x1f2
#define SHOP_RESIZE_EDGE 3

static void shop_grow_to_bar(struct UIWindow *wnd, int *bottom, int edge)
{
    int height = wnd->height - *bottom + SHOP_WND_BOTTOM_BAR;

    *bottom = SHOP_WND_BOTTOM_BAR;
    ui_window_resize(wnd, wnd->width, height);
    ui_window_send_msg(wnd, NULL, UI_MSG_RESIZE, edge, wnd->width, wnd->height, 0);
}

void ui_merchant_item_my_shop_wnd_add_assistant_bar(struct UIMerchantItemMyShopWnd *self)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIBasicButton *button;

    self->assistant = 1;
    button = mem_alloc(sizeof *button);
    if (button)
        button = ui_basic_button_construct(button);
    ui_basic_button_set_label(&button->button.base, 0x2a, msg_str(MSG_ASSISTANT_BUTTON));
    ui_window_move_base(&button->button.base, wnd->width - button->button.base.width - 0x42, wnd->height - 0x18);
    button->button.base.id = UI_ID_ASSISTANT_BUTTON;
    ui_window_add_child(wnd, &button->button.base);
    g_session1.noSellWndSound = 0;
    ui_window_set_show_base(self->check, 0);
    shop_grow_to_bar(wnd, &self->bottom, MY_SHOP_RESIZE_EDGE);
    ui_window_invalidate_base(wnd);
}

void ui_merchant_item_my_shop_wnd_set_timer(struct UIMerchantItemMyShopWnd *self, DWORD length)
{
    self->timer_start = platform_ticks_ms();
    self->timer_length = length;
}

void ui_merchant_item_shop_wnd_set_timer(struct UIMerchantItemShopWnd *self, DWORD length)
{
    self->timer_start = platform_ticks_ms();
    self->timer_length = length;
    shop_grow_to_bar(&self->base.frame.base, &self->bottom, SHOP_RESIZE_EDGE);
}

unsigned ui_merchant_shop_seller(const struct UIWindow *wnd)
{
    return (unsigned)((const struct UIMerchantItemShopWnd *)wnd)->actor_id;
}
