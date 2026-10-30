#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <stdlib.h>
#include "ui/shops/cash_shop.h"
#include "browser/browser_containers.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"

#define CART_ROWS 6
#define CART_ROW_PITCH 0x39
#define CART_MAX_LINES 8
#define CART_MAX_COUNT 100

#define MSG_CASH_ADD_NONE 0xe50
#define MSG_CASH_CART_FULL 0xcf0
#define MSG_CASH_STOCK_LIMIT 0x9dc
#define MSG_CASH_COUNT_LIMIT 0x6a8

struct Vec;
struct UICashShopWnd_CartItem;
struct UIWindow;
void ui_cash_shop_wnd_cart_item_destroy(struct UIWindow *window);
void ui_cash_shop_wnd_cart_item_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_cash_shop_wnd_cart_item_on_create(struct UIWindow *self, int width, int height);
void ui_cash_shop_wnd_cart_item_on_draw(struct UIWindow *self);
void ui_cash_shop_wnd_cart_item_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_cash_shop_wnd_cart_item_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_cash_shop_wnd_cart_item_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_cash_shop_wnd_cart_item_show_info(struct UIWindow *self, int x, int y);
void ui_cash_shop_wnd_cart_item_on_wheel(struct UIWindow *self, int delta);
int ui_cash_shop_wnd_cart_item_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_cash_shop_wnd_cart_item_ops = {
    .destroy = ui_cash_shop_wnd_cart_item_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_cash_shop_wnd_cart_item_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_cash_shop_wnd_cart_item_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_cash_shop_wnd_cart_item_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_cash_shop_wnd_cart_item_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_cash_shop_wnd_cart_item_on_l_btn_dbl_clk,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_cash_shop_wnd_cart_item_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_cash_shop_wnd_cart_item_show_info,
    .on_wheel = ui_cash_shop_wnd_cart_item_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_cash_shop_wnd_cart_item_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
struct UICashShopWnd_Cart;
struct DragInfo;
void ui_cash_shop_wnd_cart_destroy(struct UIWindow *window);
void ui_cash_shop_wnd_cart_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_cash_shop_wnd_cart_on_create(struct UIWindow *self, int width, int height);
void ui_cash_shop_wnd_cart_on_draw(struct UIWindow *self);
void ui_cash_shop_wnd_cart_on_wheel(struct UIWindow *self, int delta);
int ui_cash_shop_wnd_cart_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_cash_shop_wnd_cart_ops = {
    .destroy = ui_cash_shop_wnd_cart_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_cash_shop_wnd_cart_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_cash_shop_wnd_cart_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_cash_shop_wnd_cart_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_cash_shop_wnd_cart_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_cash_shop_wnd_cart_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UICashShopWnd_CartItem {
    struct UIWindow base;
    struct ItemInfo info;
};
OFFSET_ASSERT(UICashShopWnd_CartItem, info, 0x7c);

struct UICashShopWnd_CartItem *ui_cash_shop_wnd_cart_item_construct(struct UICashShopWnd_CartItem *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_cash_shop_wnd_cart_item_ops;
    item_info_construct(&self->info);
    return self;
}

void ui_cash_shop_wnd_cart_item_destruct(struct UICashShopWnd_CartItem *self)
{
    self->base.ops = &ui_cash_shop_wnd_cart_item_ops;
    item_info_destruct(&self->info);
    ui_window_destruct(&self->base);
}

void ui_cash_shop_wnd_cart_item_destroy(struct UIWindow *window)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    ui_cash_shop_wnd_cart_item_destruct(self);
    mem_free(self);
}

void ui_cash_shop_wnd_cart_item_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    if (self->base.parent)
        ui_window_drag_and_drop(self->base.parent, x, y, drag);
}

static void add_button(struct UICashShopWnd_CartItem *self, const char *up, const char *hover, const char *down,
                       int x, int y, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, hover, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, down, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(&self->base, &button->button.base);
}

void ui_cash_shop_wnd_cart_item_on_create(struct UIWindow *window, int width, int height)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    add_button(self, UI_SKIN "cashshop\\btn_close.bmp", UI_SKIN "cashshop\\btn_close.bmp",
               UI_SKIN "cashshop\\btn_close.bmp", 0xa1, 2, CASH_ID_REMOVE);
    add_button(self, UI_SKIN "cashshop\\btn_plus_normal.bmp", UI_SKIN "cashshop\\btn_plus_over.bmp",
               UI_SKIN "cashshop\\btn_plus_press.bmp", 0x52, 0x1d, CASH_ID_PLUS);
    add_button(self, UI_SKIN "cashshop\\btn_minus_normal.bmp", UI_SKIN "cashshop\\btn_minus_over.bmp",
               UI_SKIN "cashshop\\btn_minus_press.bmp", 0x52, 0x26, CASH_ID_MINUS);
}

void ui_cash_shop_wnd_cart_item_on_draw(struct UIWindow *window)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    struct UIWindow *wnd = &self->base;
    struct String name, text, path;
    struct String fitted = {0};
    struct BitmapRes *picture;

    ui_window_draw_bitmap(wnd, 0, 0,
                        res_mgr_get(g_resMgr(), UI_SKIN "cashshop\\img_shop_itemBg2.bmp"), 0);
    string_copy(&name, &self->info.name);
    ui_window_fit_text(wnd, &fitted, 0x70, string_cstr(&name), 0, 0xb, 0);
    string_free(&name);
    ui_window_text_out(wnd, 0x30, 0xc, string_cstr(&fitted), 0, 0, 0xb, 0x902418, 0, 0);

    string_init(&path);
    string_format(&path, UI_SKIN "collection\\%s.bmp", item_info_get_res_name(&self->info));
    picture = res_mgr_get_string(g_resMgr(), path);
    if (picture)
        ui_window_draw_bitmap_scaled(wnd, 4, 0, (int)(picture->width * 0.5), (int)(picture->height * 0.5),
                                  picture);

    string_init(&text);
    string_format(&text, "%d C", CASH_GOODS_PRICE(&self->info));
    ui_window_text_out_right(wnd, 0xa0, 0x22, string_cstr(&text), 0, 0, 0xc, 0, 0);
    string_free(&text);
    string_format(&text, "%d", self->info.count);
    ui_window_text_out_centred(wnd, 0x32, 0x21, string_cstr(&text), 0x20, 0, 0xb, 0, 0);
    string_free(&text);
    string_free(&fitted);
}

void ui_cash_shop_wnd_cart_item_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    if (self->info.index)
        game_mode_drag_item(game_current_mode(), DRAG_CASH_CART, &self->info,
                                self->info.count, -1);
}

void ui_cash_shop_wnd_cart_item_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    ui_cash_shop_wnd_cart_item_send_msg(&self->base, &self->base, UI_MSG_COMMAND, CASH_ID_REMOVE, 0, 0, 0);
}

void ui_cash_shop_wnd_cart_item_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    struct String name;
    int tip_x, tip_y;

    if ((unsigned)(x - 0x30) > 0x70 || (unsigned)(y - 0xc) > 0xb)
        return;
    tip_x = 0;
    tip_y = -10;
    ui_window_to_screen(&self->base, &tip_x, &tip_y);
    string_copy(&name, &self->info.name);
    mode_show_tooltip(client_current_mode(), string_cstr(&name), tip_x, tip_y, 0xffffffff, 0);
    string_free(&name);
}

void ui_cash_shop_wnd_cart_item_show_info(struct UIWindow *window, int x, int y)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    struct UIWindow *wnd;

    if ((unsigned)(x - 4) > 0x25)
        return;
    wnd = ui_make_window(ITEM_WND_ID);
    if (wnd)
        ui_window_send_msg(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&self->info, 0, 0, 0);
}

void ui_cash_shop_wnd_cart_item_on_wheel(struct UIWindow *window, int delta)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    if (self->base.parent)
        ui_window_on_wheel(self->base.parent, delta);
}

int ui_cash_shop_wnd_cart_item_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                              int val1, int val2, int val3, int val4)
{
    struct UICashShopWnd_CartItem *self = (struct UICashShopWnd_CartItem *)window;
    struct UIWindow *cart = self->base.parent;

    if (msg == CASH_MSG_SET_GOODS) {
        item_info_assign(&self->info, (const struct ItemInfo *)val1);
        ui_window_set_show_base(&self->base, 1);
        ui_window_invalidate_base(&self->base);
    } else if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case CASH_ID_REMOVE:
            if (!cart)
                break;
            ui_window_send_msg(cart, &self->base, CASH_MSG_REMOVE, atoi(string_cstr(&self->info.id_text)), 0, 0,
                     0);
            ui_window_invalidate_base(&self->base);
            break;
        case CASH_ID_PLUS:
            if (cart)
                ui_window_send_msg(cart, &self->base, CASH_MSG_ADD, (int)&self->info, 1, 0, 0);
            ui_window_invalidate_base(&self->base);
            break;
        case CASH_ID_MINUS:
            if (cart)
                ui_window_send_msg(cart, &self->base, CASH_MSG_ADD, (int)&self->info, -1, 0, 0);
            ui_window_invalidate_base(&self->base);
            break;
        }
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}

struct UICashShopWnd_Cart {
    struct UIWindow base;
    struct Vec rows;
    struct Vec lines;
    struct UIScrollBar_Custom *scroll;
    int row_count;
    int first;
};
OFFSET_ASSERT(UICashShopWnd_Cart, lines, 0x88);
OFFSET_ASSERT(UICashShopWnd_Cart, first, 0x9c);
SIZE_ASSERT(UICashShopWnd_Cart, 0xa0);

struct Vec *cash_shop_cart_lines(struct UIWindow *cart)
{
    return &((struct UICashShopWnd_Cart *)cart)->lines;
}

#define CART_LINE_COUNT(cart) VEC_COUNT(&(cart)->lines, struct ItemInfo)
#define CART_LINE(cart, i) (&VEC_AT(&(cart)->lines, struct ItemInfo, i))

static struct UICashShopWnd_Cart *ui_cash_shop_wnd_cart_construct(struct UICashShopWnd_Cart *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_cash_shop_wnd_cart_ops;
    self->rows.first = self->rows.last = self->rows.end = NULL;
    self->lines.first = self->lines.last = self->lines.end = NULL;
    self->scroll = NULL;
    self->row_count = CART_ROWS;
    self->first = 0;
    return self;
}

struct UICashShopWnd_Cart *ui_cash_shop_wnd_cart_new(void)
{
    struct UICashShopWnd_Cart *self = mem_alloc(sizeof *self);

    return self ? ui_cash_shop_wnd_cart_construct(self) : NULL;
}

void ui_cash_shop_wnd_cart_destruct(struct UICashShopWnd_Cart *self)
{
    self->base.ops = &ui_cash_shop_wnd_cart_ops;
    item_info_vec_free(&self->lines);
    vec_free(&self->rows);
    ui_window_destruct(&self->base);
}

void ui_cash_shop_wnd_cart_destroy(struct UIWindow *window)
{
    struct UICashShopWnd_Cart *self = (struct UICashShopWnd_Cart *)window;
    ui_cash_shop_wnd_cart_destruct(self);
    mem_free(self);
}

void ui_cash_shop_wnd_cart_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UICashShopWnd_Cart *self = (struct UICashShopWnd_Cart *)window;
    struct Mode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    if (!mode)
        return;
    if (drag->source != DRAG_CASH_GOODS) {
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    if (g_windowMgr.item_drop_cnt_wnd)
        ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, COUNT_FOR_CASH_CART, 0, 0, 0);
}

void ui_cash_shop_wnd_cart_on_create(struct UIWindow *window, int width, int height)
{
    struct UICashShopWnd_Cart *self = (struct UICashShopWnd_Cart *)window;
    struct UIScrollBar_Custom *scroll = mem_alloc(sizeof *scroll);
    int i, y;

    if (scroll)
        scroll = ui_scroll_bar_custom_construct(scroll);
    self->scroll = scroll;
    ui_scroll_bar_create_sized(&scroll->bar, self->base.width - 7, 0, self->base.height, 1, 7, 7);
    ui_scroll_bar_custom_set_arrows(self->scroll, string_from_cstr(UI_SKIN "cashshop\\shop_scroll_up.bmp"),
                                 string_from_cstr(UI_SKIN "cashshop\\shop_scroll_down.bmp"));
    ui_scroll_bar_custom_set_thumb(self->scroll, string_from_cstr(UI_SKIN "cashshop\\shop_bar_up.bmp"),
                                string_from_cstr(UI_SKIN "cashshop\\shop_bar_mid.bmp"),
                                string_from_cstr(UI_SKIN "cashshop\\shop_bar_down.bmp"));
    ui_window_add_child(&self->base, &self->scroll->bar.base);

    for (i = 0, y = 5; i < self->row_count; i++, y += CART_ROW_PITCH) {
        struct UICashShopWnd_CartItem *row = mem_alloc(sizeof *row);
        struct UIWindow *wnd;

        if (row)
            row = ui_cash_shop_wnd_cart_item_construct(row);
        wnd = &row->base;
        ui_window_create(wnd, 0xac, 0x36);
        ui_window_move(wnd, 3, y);
        ui_window_set_show(wnd, 0);
        ui_window_add_child(&self->base, wnd);
        vec_push(&self->rows, &wnd, sizeof(struct UIWindow *));
    }
}

void ui_cash_shop_wnd_cart_on_draw(struct UIWindow *window)
{
    struct UICashShopWnd_Cart *self = (struct UICashShopWnd_Cart *)window;
    unsigned line = self->first;
    int i;

    ui_window_draw_bitmap(&self->base, 0, 0,
                        res_mgr_get(g_resMgr(), UI_SKIN "cashshop\\img_shop_cart_bg.bmp"), 0);
    for (i = 0; i < self->row_count; i++, line++) {
        struct UIWindow *row = VEC_AT(&self->rows, struct UIWindow *, i);

        if (line >= CART_LINE_COUNT(self))
            ui_window_set_show(row, 0);
        else
            ui_window_send_msg(row, &self->base, CASH_MSG_SET_GOODS, (int)CART_LINE(self, line), 0, 0, 0);
    }
}

void ui_cash_shop_wnd_cart_on_wheel(struct UIWindow *window, int delta)
{
    struct UICashShopWnd_Cart *self = (struct UICashShopWnd_Cart *)window;
    ui_cash_shop_wnd_cart_send_msg(&self->base, &self->scroll->bar.base, UI_MSG_SCROLL_V, delta < 0 ? 1 : -1, 0, 0, 0);
}

static void report_total(struct UICashShopWnd_Cart *self)
{
    struct ItemInfo *line;
    int total = 0;

    if (!self->base.parent)
        return;
    for (line = self->lines.first; line != self->lines.last; line++)
        total += CASH_GOODS_PRICE(line) * line->count;
    ui_window_send_msg(self->base.parent, &self->base, CASH_MSG_CART_TOTAL, total, 0, 0, 0);
}

static void limit_box(int msg, int limit)
{
    struct String text;

    string_init(&text);
    string_format(&text, msg_str(msg), limit);
    ui_message_box(string_cstr(&text));
    string_free(&text);
}

static void update_scroll(struct UICashShopWnd_Cart *self, int max_pos)
{
    ui_scroll_bar_set_range(&self->scroll->bar, max_pos, self->row_count, (int)CART_LINE_COUNT(self));
    ui_scroll_bar_set_pos(&self->scroll->bar, self->first);
}

int ui_cash_shop_wnd_cart_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UICashShopWnd_Cart *self = (struct UICashShopWnd_Cart *)window;
    switch (msg) {
    case UI_MSG_SCROLL_V: {
        int count = (int)CART_LINE_COUNT(self);
        int max_pos, first;

        if (!count)
            break;
        max_pos = count - self->row_count;
        if (max_pos < 0)
            max_pos = 0;
        if (!max_pos) {
            if (self->first > 0) {
                self->first = 0;
                update_scroll(self, max_pos);
            }
            break;
        }
        first = self->first + val1;
        if (first >= 0) {
            self->first = first > max_pos ? max_pos : first;
            update_scroll(self, max_pos);
        }
        ui_window_invalidate_base(&self->base);
        break;
    }

    case CASH_MSG_ADD: {
        const struct ItemInfo *info = (const struct ItemInfo *)val1;
        struct ItemInfo *line;
        int id;

        if (!info)
            return -1;
        if (!val2) {
            ui_message_box(msg_str(MSG_CASH_ADD_NONE));
            return -1;
        }
        id = atoi(string_cstr(&info->id_text));
        for (line = self->lines.first; line != self->lines.last; line++)
            if (atoi(string_cstr(&line->id_text)) == id)
                break;

        if (line == self->lines.last) {
            struct ItemInfo copy;
            int limit;

            if (CART_LINE_COUNT(self) >= CART_MAX_LINES) {
                limit_box(MSG_CASH_CART_FULL, CART_MAX_LINES);
                return -1;
            }
            item_info_copy(&copy, info);
            limit = CASH_GOODS_STOCK(&copy);
            if (limit && val2 > limit) {
                limit_box(MSG_CASH_STOCK_LIMIT, limit);
                val2 = limit;
            }
            copy.count = val2;
            vec_push(&self->lines, &copy, sizeof copy);
            ui_cash_shop_wnd_cart_send_msg(&self->base, &self->base, UI_MSG_SCROLL_V, 1, 0, 0, 0);
        } else {
            int limit = CASH_GOODS_STOCK(line), count;

            if (limit && line->count + val2 > limit) {
                limit_box(MSG_CASH_STOCK_LIMIT, limit);
                val2 = limit - line->count;
            }
            if (!val2)
                return -1;
            count = line->count + val2;
            if (count > CART_MAX_COUNT) {
                limit_box(MSG_CASH_COUNT_LIMIT, CART_MAX_COUNT);
                line->count = CART_MAX_COUNT;
            } else {
                line->count = count > 0 ? count : 1;
            }
            ui_cash_shop_wnd_cart_send_msg(&self->base, &self->base, UI_MSG_SCROLL_V, 0, 0, 0, 0);
        }
        report_total(self);
        ui_window_invalidate_base(&self->base);
        break;
    }

    case CASH_MSG_REMOVE: {
        struct ItemInfo *line, *last;

        if (!sender)
            break;
        for (line = self->lines.first; line != self->lines.last; line++)
            if (atoi(string_cstr(&line->id_text)) == val1)
                break;
        if (line == self->lines.last)
            break;
        last = self->lines.last;
        for (; line + 1 != last; line++)
            item_info_assign(line, line + 1);
        item_info_destruct(last - 1);
        self->lines.last = last - 1;
        report_total(self);
        ui_cash_shop_wnd_cart_send_msg(&self->base, &self->base, UI_MSG_SCROLL_V, 0, 0, 0, 0);
        ui_window_invalidate_base(&self->base);
        break;
    }
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}
