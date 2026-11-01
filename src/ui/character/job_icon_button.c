#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "ui/character/character.h"
#include "ui/character/job_icon_button.h"
#include "diag/perf.h"

#define UI_WND_MESSENGER_GROUP 0x22

struct UIWindow;
void ui_job_icon_button_destroy(struct UIWindow *window);
void ui_job_icon_button_on_draw(struct UIWindow *self);
void ui_job_icon_button_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_job_icon_button_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_job_icon_button_on_r_btn_up(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_job_icon_button_ops = {
    .base = {
        .destroy = ui_job_icon_button_destroy,
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
        .on_draw = ui_job_icon_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_job_icon_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_job_icon_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_job_icon_button_on_r_btn_up,
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

struct UIJobIconButton *ui_job_icon_button_construct(struct UIJobIconButton *self)
{
    ui_bitmap_button_construct(&self->bmp);
    self->bmp.button.base.ops = &ui_job_icon_button_ops.base;
    self->draw_mode = 0;
    return self;
}

void ui_job_icon_button_destroy(struct UIWindow *window)
{
    struct UIJobIconButton *self = (struct UIJobIconButton *)window;
    self->bmp.button.base.ops = &ui_job_icon_button_ops.base;
    ui_bitmap_button_destruct(&self->bmp);
    mem_free(self);
}

void ui_job_icon_button_on_draw(struct UIWindow *window)
{
    struct UIJobIconButton *self = (struct UIJobIconButton *)window;
    struct UIWindow *parent;
    struct BitmapRes *bitmap = NULL;

    switch (self->bmp.button.base.state) {
    case 0:
    case 1:
    case 2:
        bitmap = ui_skin_bitmap(string_cstr(&self->bmp.bitmaps[self->bmp.button.base.state]));
        break;
    }
    if (self->bmp.button.disabled && self->bmp.bitmaps[UI_BMP_DISABLED].size > 0)
        bitmap = ui_skin_bitmap(string_cstr(&self->bmp.bitmaps[UI_BMP_DISABLED]));

    if (self->draw_mode == 1)
        ui_window_draw_bitmap_tinted(&self->bmp.button.base, 0, 0, 0, bitmap, 1, 0);
    else
        ui_window_draw_bitmap(&self->bmp.button.base, 0, 0, bitmap, 1);

    if (self->bmp.redraw_parent) {
        parent = self->bmp.button.base.parent;
        if (parent)
            ui_window_invalidate(parent);
    }
    parent = self->bmp.button.base.parent;
    if (parent)
        ui_window_invalidate(parent);
}

void ui_job_icon_button_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIJobIconButton *self = (struct UIJobIconButton *)window;
    struct UIWindow *wnd = ui_window_find(UI_WND_MESSENGER_GROUP);

    if (wnd)
        ui_window_on_l_btn_down(wnd, self->bmp.button.base.x + x, self->bmp.button.base.y + y);
}

void ui_job_icon_button_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIJobIconButton *self = (struct UIJobIconButton *)window;
    struct UIWindow *wnd = ui_window_find(UI_WND_MESSENGER_GROUP);

    if (wnd)
        ui_window_on_l_btn_up(wnd, self->bmp.button.base.x + x, self->bmp.button.base.y + y);
}

void ui_job_icon_button_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIJobIconButton *self = (struct UIJobIconButton *)window;
    struct UIWindow *wnd = ui_window_find(UI_WND_MESSENGER_GROUP);

    if (wnd)
        ui_window_on_r_btn_up(wnd, self->bmp.button.base.x + x, self->bmp.button.base.y + y);
}
