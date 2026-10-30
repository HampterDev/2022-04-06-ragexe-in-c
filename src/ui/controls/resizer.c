#include "ui/controls/resizer.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include <string.h>
#include "core/skin_path.h"
#include "diag/perf.h"

struct UIResizer;
struct UIWindow;
const struct UIWindowOps ui_resizer_ops = {
    .destroy = ui_resizer_destroy,
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
    .on_draw = ui_resizer_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_resizer_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_resizer_on_mouse_move,
    .on_mouse_hover = ui_resizer_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_resizer_on_mouse_shape,
    .on_l_btn_up = ui_resizer_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
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

struct UIResizer *ui_resizer_construct(struct UIResizer *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_resizer_ops;
    string_init(&self->bitmap);
    self->edge = UI_EDGE_LEFT;
    self->parent_width = 0;
    self->parent_height = 0;
    self->line_colour = UI_TRANSPARENT;
    self->fill_colour = UI_TRANSPARENT;
    return self;
}

void ui_resizer_destroy(struct UIWindow *window)
{
    struct UIResizer *self = (struct UIResizer *)window;
    string_free(&self->bitmap);
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_resizer_set_bitmap(struct UIResizer *self, const char *name)
{
    struct BitmapRes *bitmap;
    const char *path;

    string_assign_cstr(&self->bitmap, name);
    path = string_cstr(&self->bitmap);
    if (g_serviceType)
        path = ui_skin_path(path);
    bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
    self->base.width = bitmap->width;
    self->base.height = bitmap->height;
}

void ui_resizer_on_draw(struct UIWindow *window)
{
    struct UIResizer *self = (struct UIResizer *)window;
    struct UIWindow *wnd = &self->base;

    if (self->bitmap.size) {
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(string_cstr(&self->bitmap)), 0);
        return;
    }
    ui_window_clear_canvas(wnd, self->fill_colour);
    ui_window_fill_rect(wnd, 0, 0, wnd->width, 2, self->line_colour);
}

void ui_resizer_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIResizer *self = (struct UIResizer *)window;
    struct UIWindow *parent;

    ui_window_mgr_set_capture(&g_windowMgr, &self->base);
    self->press_x = x;
    self->press_y = y;
    parent = self->base.parent;
    if (parent) {
        self->parent_width = parent->width;
        self->parent_height = parent->height;
    }
    ui_window_to_screen(&self->base, &self->press_x, &self->press_y);
}

void ui_resizer_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIResizer *self = (struct UIResizer *)window;
    int dx, dy, width, height;

    if (ui_window_mgr_get_capture(&g_windowMgr) != &self->base)
        return;
    ui_window_to_screen(&self->base, &x, &y);
    if (!self->base.parent)
        return;
    dx = x - self->press_x;
    dy = y - self->press_y;
    width = self->parent_width;
    height = self->parent_height;
    switch (self->edge) {
    case UI_EDGE_LEFT:         width -= dx; break;
    case UI_EDGE_RIGHT:        width += dx; break;
    case UI_EDGE_TOP:          height -= dy; break;
    case UI_EDGE_BOTTOM:       height += dy; break;
    case UI_EDGE_TOP_LEFT:     width -= dx; height -= dy; break;
    case UI_EDGE_TOP_RIGHT:    width += dx; height -= dy; break;
    case UI_EDGE_BOTTOM_LEFT:  width -= dx; height += dy; break;
    case UI_EDGE_BOTTOM_RIGHT: width += dx; height += dy; break;
    }
    ui_window_send_msg(self->base.parent, &self->base, UI_MSG_RESIZE, self->edge, width, height, 0);
}

void ui_resizer_on_mouse_hover(struct UIWindow *self, int x_unused, int y_unused)
{
}

void ui_resizer_on_mouse_shape(struct UIWindow *window, int x, int y)
{
    struct UIResizer *self = (struct UIResizer *)window;
    struct UIWindow *wnd = &self->base;

    if (ui_window_mgr_get_capture(&g_windowMgr) == wnd
        || (x >= 0 && x < wnd->width && y >= 0 && y <= wnd->height)) {
        mode_set_cursor_action(client_current_mode(), CURSOR_HAND);
        return;
    }
    if (ui_window_mgr_get_capture(&g_windowMgr) != wnd)
        mode_set_cursor_action(client_current_mode(), CURSOR_ARROW);
}

void ui_resizer_on_l_btn_up(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIResizer *self = (struct UIResizer *)window;
    if (ui_window_mgr_get_capture(&g_windowMgr) != &self->base)
        return;
    if (self->base.parent)
        ui_window_snap(self->base.parent);
    ui_window_mgr_release_capture(&g_windowMgr);
}

void ui_resizer_set_edge(struct UIResizer *self, int edge)
{
    self->edge = edge;
}

void ui_resizer_set_colours(struct UIResizer *self, DWORD line, DWORD fill)
{
    ui_window_invalidate_base(&self->base);
    self->line_colour = line;
    self->fill_colour = fill;
}
