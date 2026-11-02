#include "ui/controls/tabs.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "diag/perf.h"

struct UITab;
struct UIWindow;
const struct UIWindowOps ui_tab_ops = {
    .destroy = ui_tab_destroy,
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
    .on_draw = ui_tab_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_tab_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_tab_send_msg,
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

struct UITab *ui_tab_construct(struct UITab *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_tab_ops;
    self->base.state = 0;
    self->count = 3;
    self->kind = UI_TAB_ITM;
    return self;
}

void ui_tab_destroy(struct UIWindow *window)
{
    struct UITab *self = (struct UITab *)window;
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_tab_on_draw(struct UIWindow *window)
{
    struct UITab *self = (struct UITab *)window;
    char path[0x80];
    const char *fmt;

    switch (self->kind) {
    case UI_TAB_ITM:  fmt = UI_SKIN "basic_interface\\tab_itm_%02d.bmp"; break;
    case UI_TAB_CMD:  fmt = UI_SKIN "basic_interface\\tab_cmd_%02d.bmp"; break;
    case UI_TAB_QUE:  fmt = UI_SKIN "basic_interface\\tab_que_%02d.bmp"; break;
    case UI_TAB_ITEM: fmt = UI_SKIN "basic_interface\\tab_item_%02d.bmp"; break;
    default:
        return;
    }
    sprintf(path, fmt, self->base.state + 1);
    ui_window_draw_bitmap(&self->base, 0, 0, ui_skin_bitmap(path), 0);
}

void ui_tab_on_l_btn_down(struct UIWindow *window, int x_unused, int y)
{
    struct UITab *self = (struct UITab *)window;
    int tab;

    if (!self->base.parent)
        return;
    tab = y / (self->base.height / self->count);
    if (tab < 0)
        tab = 0;
    if (tab > self->count - 1)
        tab = self->count - 1;
    if (self->base.state == tab)
        return;
    self->base.state = tab;
    ui_window_invalidate_base(&self->base);
    ui_window_send_msg(self->base.parent, &self->base, UI_MSG_TAB_CHANGED, self->base.state, 0, 0, 0);
}

intptr_t ui_tab_send_msg(struct UIWindow *window, struct UIWindow *sender_unused, int msg, intptr_t val1,
                             intptr_t val2_unused, intptr_t val3_unused, intptr_t val4_unused)
{
    struct UITab *self = (struct UITab *)window;
    if (msg == UI_MSG_SET_STATE) {
        self->base.state = val1;
        ui_window_invalidate_base(&self->base);
    }
    return 0;
}
