#include "ui/window.h"
#include "ui/controls/controls.h"

BYTE ui_window_query_false(struct UIWindow *self)
{
    (void)self;
    return 0;
}

BYTE ui_window_query_true(struct UIWindow *self)
{
    (void)self;
    return 1;
}

void ui_window_abstract_on_create(struct UIWindow *self, int width, int height)
{
    (void)self;
    (void)width;
    (void)height;
    abstract_method_called();
}

void ui_window_abstract_on_draw(struct UIWindow *self)
{
    (void)self;
    abstract_method_called();
}

BYTE ui_window_is_focusable_base(struct UIWindow *self)
{
    (void)self;
    return 0;
}

BYTE ui_window_get_backdrop_box_base(struct UIWindow *self, struct UIBackdropBox *out)
{
    (void)self;
    (void)out;
    return 0;
}

BYTE ui_window_is_click_through_base(struct UIWindow *self)
{
    (void)self;
    return 0;
}

BYTE ui_window_is_hit_testable_base(struct UIWindow *self)
{
    (void)self;
    return 1;
}

void ui_window_store_info_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_save_pos_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_move_delta_base(struct UIWindow *self, int dx, int dy)
{
    (void)self;
    (void)dx;
    (void)dy;
}

void ui_window_set_show_base(struct UIWindow *self, int visible)
{
    self->visible = visible;
}

void ui_window_on_create_base(struct UIWindow *self, int width, int height)
{
    (void)self;
    (void)width;
    (void)height;
}

void ui_window_on_destroy_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_on_process_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_on_focus_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_on_focus_lost_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_on_l_btn_down_base(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_window_on_l_btn_dbl_clk_base(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_window_on_mouse_move_base(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_window_on_mouse_hover_base(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_window_track_mouse_base(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_window_on_l_btn_up_base(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_window_on_r_btn_down_base(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_window_on_r_btn_up_base(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_window_on_wheel_base(struct UIWindow *self, int delta)
{
    (void)self;
    (void)delta;
}

void ui_window_snap_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_draw_backdrop_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_on_hover_enter_base(struct UIWindow *self)
{
    (void)self;
}

void ui_window_on_hover_leave_base(struct UIWindow *self)
{
    (void)self;
}

BYTE ui_edit_ctrl_is_focusable(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    return self->disabled == 0;
}

void ui_list_box_on_r_btn_down(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

void ui_list_box_navi_search_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y)
{
    (void)self;
    (void)x;
    (void)y;
}

BYTE ui_balloon_text_is_hit_testable(struct UIWindow *self)
{
    (void)self;
    return 0;
}

void ui_balloon_text_on_process(struct UIWindow *self)
{
    self->state_count++;
}

void ui_window_on_draw_base(struct UIWindow *self)
{
    (void)self;
}
