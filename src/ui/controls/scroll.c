#include "ui/controls/scroll.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/skin.h"
#include "diag/perf.h"

struct UIScrollBar;
struct UIWindow;
const struct UIWindowOps ui_scroll_bar_ops = {
    .destroy = ui_scroll_bar_destroy,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_scroll_bar_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,
    .on_wheel = ui_scroll_bar_on_wheel,
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
};

const struct UIWindowOps ui_scroll_bar3_ops = {
    .destroy = ui_scroll_bar3_destroy,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_scroll_bar3_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,
    .on_wheel = ui_scroll_bar_on_wheel,
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
};

const struct UIWindowOps ui_scroll_bar_navi_ops = {
    .destroy = ui_scroll_bar_navi_destroy,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_scroll_bar_navi_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,
    .on_wheel = ui_scroll_bar_on_wheel,
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
};

static int bar_length(const struct UIScrollBar *self)
{
    return self->orientation == UI_SCROLL_HORZ ? self->base.width : self->base.height;
}

struct UIScrollBar *ui_scroll_bar_construct(struct UIScrollBar *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_scroll_bar_ops;
    self->button_size = 13;
    self->max_pos = 0;
    self->pos = 0;
    self->orientation = UI_SCROLL_VERT;
    self->hot = UI_SCROLL_HOT_NONE;
    self->press_x = 0;
    self->press_y = 0;
    self->drag_steps = 0;
    self->total = 0;
    self->page = 0;
    self->thickness = 13;
    self->min_thumb = 1;
    return self;
}

void ui_scroll_bar_destruct(struct UIScrollBar *self)
{
    self->base.ops = &ui_scroll_bar_ops;
    ui_window_destruct(&self->base);
}

void ui_scroll_bar_destroy(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_scroll_bar_destruct(self);
    mem_free(self);
}

void ui_scroll_bar_create(struct UIScrollBar *self, int x, int y, int orientation, int length)
{
    int width, height;

    self->orientation = orientation;
    switch (orientation) {
    case UI_SCROLL_HORZ:
        width = length;
        height = self->thickness;
        break;
    case UI_SCROLL_VERT:
        width = self->thickness;
        height = length;
        break;
    default:
        width = length;
        height = length;
        break;
    }
    ui_window_resize_base(&self->base, width, height);
    ui_window_move_base(&self->base, x, y);
    ui_window_on_create_base(&self->base, width, height);
}

void ui_scroll_bar_resize(struct UIWindow *window, int width, int height)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_window_invalidate_base(&self->base);
    switch (self->orientation) {
    case UI_SCROLL_HORZ:
        height = self->thickness;
        ui_window_resize_base(&self->base, width, height);
        break;
    case UI_SCROLL_VERT:
        width = self->thickness;
        break;
    }
    ui_window_resize_base(&self->base, width, height);
}

BYTE ui_scroll_bar_is_click_through(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    if (self->max_pos == 0 && self->base.parent)
        return ui_window_is_click_through(self->base.parent);
    return 0;
}

int ui_scroll_bar_get_thumb_size(struct UIScrollBar *self)
{
    int size = bar_length(self) - 2 * self->button_size;

    if (self->total)
        size = self->page * size / self->total;
    if (self->min_thumb && size < UI_SCROLL_MIN_THUMB)
        size = UI_SCROLL_MIN_THUMB;
    return size;
}

int ui_scroll_bar_thumb_travel(struct UIScrollBar *self)
{
    return bar_length(self) - 2 * self->button_size - ui_scroll_bar_get_thumb_size(self);
}

int ui_scroll_bar_thumb_pos(struct UIScrollBar *self)
{
    return self->pos * ui_scroll_bar_thumb_travel(self) / self->max_pos;
}

int ui_scroll_bar_hit_part(struct UIScrollBar *self, int x, int y)
{
    int along, length;

    if (!self->max_pos || x < 0 || x >= self->base.width || y < 0 || y >= self->base.height)
        return UI_SCROLL_OUTSIDE;
    switch (self->orientation) {
    case UI_SCROLL_HORZ:
        along = x;
        length = self->base.width;
        break;
    case UI_SCROLL_VERT:
        along = y;
        length = self->base.height;
        break;
    default:
        return UI_SCROLL_OUTSIDE;
    }
    if (along < self->button_size)
        return UI_SCROLL_ARROW_BACK;
    if (along < ui_scroll_bar_thumb_pos(self) + self->button_size)
        return UI_SCROLL_TRACK_BACK;
    if (along < ui_scroll_bar_get_thumb_size(self) + ui_scroll_bar_thumb_pos(self) + self->button_size)
        return UI_SCROLL_THUMB;
    if (along < length - self->button_size)
        return UI_SCROLL_TRACK_FWD;
    return UI_SCROLL_ARROW_FWD;
}

static int ask_parent(struct UIScrollBar *self, int msg, int val1)
{
    return ui_window_send_msg(self->base.parent, &self->base, msg, val1, 0, 0, 0);
}

void ui_scroll_bar_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;
    int prev_hot, part;

    if (!self->max_pos) {
        ui_window_on_l_btn_down(wnd->parent, wnd->x + x, wnd->y + y);
        return;
    }
    prev_hot = self->hot;
    part = ui_scroll_bar_hit_part(self, x, y);
    self->press_y = y;
    self->press_x = x;
    self->drag_steps = 0;
    switch (self->orientation) {
    case UI_SCROLL_HORZ:
        switch (part) {
        case UI_SCROLL_ARROW_BACK: ask_parent(self, UI_MSG_SCROLL_H, -1); break;
        case UI_SCROLL_TRACK_BACK: ask_parent(self, UI_MSG_PAGE_LEFT, 0); break;
        case UI_SCROLL_THUMB:
            ui_window_mgr_set_capture(&g_windowMgr, wnd);
            self->hot = UI_SCROLL_HOT_DRAG;
            break;
        case UI_SCROLL_TRACK_FWD:  ask_parent(self, UI_MSG_PAGE_RIGHT, 0); break;
        case UI_SCROLL_ARROW_FWD:  ask_parent(self, UI_MSG_SCROLL_H, 1); break;
        }
        break;
    case UI_SCROLL_VERT:
        switch (part) {
        case UI_SCROLL_ARROW_BACK: ask_parent(self, UI_MSG_SCROLL_V, -1); break;
        case UI_SCROLL_TRACK_BACK: ask_parent(self, UI_MSG_PAGE_UP, 0); break;
        case UI_SCROLL_THUMB:
            ui_window_mgr_set_capture(&g_windowMgr, wnd);
            self->hot = UI_SCROLL_HOT_DRAG;
            break;
        case UI_SCROLL_TRACK_FWD:  ask_parent(self, UI_MSG_PAGE_DOWN, 0); break;
        case UI_SCROLL_ARROW_FWD:  ask_parent(self, UI_MSG_SCROLL_V, 1); break;
        }
        break;
    }
    if (prev_hot != self->hot)
        ui_window_invalidate_base(wnd);
}

void ui_scroll_bar_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;

    ui_scroll_bar_on_l_btn_down(wnd, x, y);
    if (!self->max_pos)
        ui_window_on_l_btn_dbl_clk(wnd->parent, wnd->x + x, wnd->y + y);
}

void ui_scroll_bar_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;
    int prev_hot, steps;

    if (!self->max_pos) {
        ui_window_on_mouse_move(wnd->parent, wnd->x + x, wnd->y + y);
        return;
    }
    if (ui_scroll_bar_thumb_travel(self) == 0)
        return;
    prev_hot = self->hot;
    switch (ui_scroll_bar_hit_part(self, x, y)) {
    case UI_SCROLL_ARROW_BACK: self->hot = UI_SCROLL_HOT_BACK; break;
    case UI_SCROLL_THUMB:      self->hot = UI_SCROLL_HOT_THUMB; break;
    case UI_SCROLL_ARROW_FWD:  self->hot = UI_SCROLL_HOT_FWD; break;
    default:                   self->hot = UI_SCROLL_HOT_NONE; break;
    }
    if (prev_hot != self->hot)
        ui_window_invalidate_base(wnd);

    if (ui_window_mgr_get_capture(&g_windowMgr) != wnd || !wnd->parent)
        return;
    switch (self->orientation) {
    case UI_SCROLL_HORZ:
        steps = (x - self->press_x) * (self->max_pos + 1) / ui_scroll_bar_thumb_travel(self) - self->drag_steps;
        if (steps)
            self->drag_steps += ask_parent(self, UI_MSG_SCROLL_H, steps);
        break;
    case UI_SCROLL_VERT:
        steps = (y - self->press_y) * (self->max_pos + 1) / ui_scroll_bar_thumb_travel(self) - self->drag_steps;
        if (steps)
            self->drag_steps += ask_parent(self, UI_MSG_SCROLL_V, steps);
        break;
    }
}

void ui_scroll_bar_on_mouse_shape(struct UIWindow *window, int x, int y)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    if (!self->max_pos) {
        ui_window_on_mouse_shape_base(&self->base, x, y);
        return;
    }
    mode_set_cursor_action(client_current_mode(), CURSOR_HAND);
}

void ui_scroll_bar_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;

    if (!self->max_pos) {
        ui_window_on_l_btn_up(wnd->parent, wnd->x + x, wnd->y + y);
        return;
    }
    if (ui_window_mgr_get_capture(&g_windowMgr) == wnd)
        ui_window_mgr_release_capture(&g_windowMgr);
}

void ui_scroll_bar_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;

    if (!self->max_pos)
        ui_window_on_r_btn_up(wnd->parent, wnd->x + x, wnd->y + y);
}

void ui_scroll_bar_on_wheel(struct UIWindow *window, int delta)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_window_on_wheel(self->base.parent, delta);
}

static int tiles13(int span)
{
    return span / 13 + (span % 13 != 0);
}

void ui_scroll_bar_on_draw(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;
    struct BitmapRes *mid;
    int btn, n, i, top, end, p;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (!self->max_pos)
        return;
    switch (self->orientation) {
    case UI_SCROLL_VERT:
        ui_window_draw_bitmap(wnd, 0, 1, ui_skin_bitmap(UI_SKIN "scroll0up.bmp"), 0);
        mid = ui_skin_bitmap(UI_SKIN "scroll0mid.bmp");
        n = tiles13(wnd->height - self->button_size - 14);
        for (i = 0, p = 0; i < n; i++, p += 13)
            ui_window_draw_bitmap(wnd, 0, self->button_size + p, mid, 0);
        ui_window_draw_bitmap(wnd, 0, wnd->height - self->button_size - 13, mid, 0);
        ui_window_fill_rect(wnd, 0, wnd->height - self->button_size, wnd->width, 13, UI_TRANSPARENT);
        ui_window_draw_bitmap(wnd, 0, wnd->height - self->button_size - 1,
                            ui_skin_bitmap(UI_SKIN "scroll0down.bmp"), 1);

        btn = self->button_size;
        top = btn + ui_scroll_bar_thumb_pos(self);
        ui_window_draw_bitmap(wnd, 0, top, ui_skin_bitmap(UI_SKIN "scroll0bar_up.bmp"), 1);
        end = ui_scroll_bar_get_thumb_size(self) - 4 + top;
        for (p = top + 4; p < end; p += 4)
            ui_window_draw_bitmap(wnd, 0, p, ui_skin_bitmap(UI_SKIN "scroll0bar_mid.bmp"), 1);
        ui_window_draw_bitmap(wnd, 0, end, ui_skin_bitmap(UI_SKIN "scroll0bar_down.bmp"), 1);
        break;

    case UI_SCROLL_HORZ:
        ui_window_draw_bitmap(wnd, 1, 0, ui_skin_bitmap(UI_SKIN "scroll1left.bmp"), 0);
        mid = ui_skin_bitmap(UI_SKIN "scroll1mid.bmp");
        n = tiles13(wnd->width - self->button_size - 14);
        for (i = 0, p = 0; i < n; i++, p += 13)
            ui_window_draw_bitmap(wnd, self->button_size + p, 0, mid, 0);
        ui_window_draw_bitmap(wnd, wnd->width - self->button_size - 13, 0, mid, 0);
        ui_window_fill_rect(wnd, wnd->width - self->button_size, 0, 13, wnd->height, UI_TRANSPARENT);
        ui_window_draw_bitmap(wnd, wnd->width - self->button_size - 1, 0,
                            ui_skin_bitmap(UI_SKIN "scroll1right.bmp"), 1);

        btn = self->button_size;
        top = btn + ui_scroll_bar_thumb_pos(self);
        ui_window_draw_bitmap(wnd, top, 0, ui_skin_bitmap(UI_SKIN "scroll1bar_left.bmp"), 1);
        end = ui_scroll_bar_get_thumb_size(self) - 4 + top;
        for (p = top + 4; p < end; p += 4)
            ui_window_draw_bitmap(wnd, p, 0, ui_skin_bitmap(UI_SKIN "scroll1bar_mid.bmp"), 1);
        ui_window_draw_bitmap(wnd, end, 0, ui_skin_bitmap(UI_SKIN "scroll1bar_right.bmp"), 1);
        break;
    }
}

static struct BitmapRes *res_bitmap(const char *path)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
}

struct UIScrollBar *ui_scroll_bar3_construct(struct UIScrollBar *self)
{
    ui_scroll_bar_construct(self);
    self->base.ops = &ui_scroll_bar3_ops;
    self->button_size = 9;
    return self;
}

void ui_scroll_bar3_destroy(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_scroll_bar_destruct(self);
    mem_free(self);
}

void ui_scroll_bar3_on_draw(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;
    int top, size;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (!self->max_pos)
        return;
    if (self->orientation == UI_SCROLL_VERT) {
        ui_window_fill_rect(wnd, 0, 10, 9, wnd->height - 20, 0xf1f1f1);
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\dialscr_up1.bmp"), 0);
        top = ui_scroll_bar_thumb_pos(self) + self->button_size;
        if (top < 9)
            top = 9;
        size = ui_scroll_bar_get_thumb_size(self);
        if (size > wnd->height - 20)
            size = 1;
        ui_window_fill_rect(wnd, 0, top + 1, 9, size - 1, 0xe9e9e9);
        ui_window_draw_bitmap(wnd, 0, wnd->height - self->button_size,
                            ui_skin_bitmap(UI_SKIN "basic_interface\\dialscr_down1.bmp"), 0);
    }
    self->min_thumb = 0;
}

struct UIScrollBar *ui_scroll_bar_navi_construct(struct UIScrollBar *self)
{
    ui_scroll_bar_construct(self);
    self->base.ops = &ui_scroll_bar_navi_ops;
    self->button_size = 7;
    self->thickness = 7;
    return self;
}

void ui_scroll_bar_navi_destroy(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_scroll_bar_destruct(self);
    mem_free(self);
}

#define NAVI UI_SKIN "navigation_interface3\\"

void ui_scroll_bar_navi_on_draw(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;
    struct BitmapRes *up, *mid, *down, *bar_up, *bar_mid, *bar_down;
    int btn, span, n, i, top, size, y;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (!self->max_pos)
        return;
    if (self->orientation == UI_SCROLL_VERT) {
        up = ui_skin_bitmap(NAVI "Navi_scroll_up.bmp");
        mid = ui_skin_bitmap(NAVI "Navi_scroll_mid.bmp");
        down = ui_skin_bitmap(NAVI "Navi_scroll_down.bmp");
        btn = self->button_size;
        span = wnd->height - 2 * btn;
        n = span / btn;
        if (span > btn * n)
            n++;
        for (i = 1; i <= n; i++)
            ui_window_draw_bitmap(wnd, 0, self->button_size * i, mid, 0);
        ui_window_draw_bitmap(wnd, 0, 0, up, 0);
        ui_window_draw_bitmap(wnd, 0, wnd->height - self->button_size, down, 0);

        bar_up = res_bitmap(NAVI "Navi_bar_up.bmp");
        bar_mid = res_bitmap(NAVI "Navi_bar_mid.bmp");
        bar_down = res_bitmap(NAVI "Navi_bar_down.bmp");
        top = ui_scroll_bar_thumb_pos(self) + self->button_size;
        ui_window_draw_bitmap(wnd, 0, top, bar_up, 1);
        size = ui_scroll_bar_get_thumb_size(self);
        for (y = bar_up->height + top; y < size + bar_up->height + top; y += bar_down->height)
            ui_window_draw_bitmap(wnd, 0, y, bar_down, 1);
        ui_window_draw_bitmap(wnd, 0, size + bar_up->height + top, bar_mid, 1);
    }
    self->min_thumb = 0;
    ui_window_invalidate(self->base.parent);
}

void ui_scroll_bar_set_pos(struct UIScrollBar *self, int pos)
{
    self->pos = pos;
}

void ui_scroll_bar_set_range(struct UIScrollBar *self, int max_pos, int page, int total)
{
    self->max_pos = max_pos;
    self->page = page;
    self->total = total;
    ui_window_invalidate_base(&self->base);
    if (self->pos > self->max_pos)
        self->pos = self->max_pos;
}
