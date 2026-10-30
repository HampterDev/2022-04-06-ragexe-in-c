#include "ui/controls/buttons.h"
#include "ui/items/refine_controls.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/items/refining_internal.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/controls/static.h"

const struct UIButtonOps ui_refining_btn_refining_ops = {
    .base = {
        .destroy = ui_refining_btn_refining_destroy,
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
        .on_draw = ui_refining_btn_refining_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
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
    },
    .set_show = ui_refine_button_set_show,
};
const struct UIButtonOps ui_refining_btn_again_ops = {
    .base = {
        .destroy = ui_refining_btn_again_destroy,
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
        .on_draw = ui_refining_btn_again_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
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
    },
    .set_show = ui_refine_button_set_show,
};
const struct UIButtonOps ui_refining_btn_return_ops = {
    .base = {
        .destroy = ui_refining_btn_return_destroy,
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
        .on_draw = ui_refining_btn_return_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
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
    },
    .set_show = ui_button_set_show,
};
const struct UIWindowOps ui_refining_notice_ops = {
    .destroy = ui_refining_notice_destroy,
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
    .on_process = ui_refining_notice_on_process,
    .on_draw = ui_refining_notice_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_static_text_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_static_text_on_l_btn_dbl_clk,
    .on_mouse_move = ui_static_text_on_mouse_move,
    .on_mouse_hover = ui_static_text_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_static_text_on_mouse_shape,
    .on_l_btn_up = ui_static_text_on_l_btn_up,
    .on_r_btn_down = ui_static_text_on_r_btn_down,
    .on_r_btn_up = ui_static_text_on_r_btn_up,
    .on_wheel = ui_static_text_on_wheel,
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

void ui_refining_btn_refining_set_bitmaps(struct UIRefining_BtnRefining *self)
{
    struct UIBitmapButton *btn = &self->button;

    ui_bitmap_button_set_bitmap(btn, REFINE_DIR "bt_refining_normal.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(btn, REFINE_DIR "bt_refining_normal.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(btn, REFINE_DIR "bt_refining_press.bmp", 2, 0);
    ui_bitmap_button_set_bitmap(btn, REFINE_DIR "bt_refining_disable.bmp", 3, 0);
    ui_window_resize_base(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
}

static void set_result_bitmaps(struct UIBitmapButton *btn, const char *normal, const char *press, BYTE success)
{
    struct String path;
    const char *word = success == 1 ? "success" : "fail";

    string_init(&path);
    string_format(&path, normal, word);
    ui_bitmap_button_set_bitmap(btn, string_cstr(&path), 0, 0);
    string_free(&path);
    string_format(&path, normal, word);
    ui_bitmap_button_set_bitmap(btn, string_cstr(&path), 1, 0);
    string_free(&path);
    string_format(&path, press, word);
    ui_bitmap_button_set_bitmap(btn, string_cstr(&path), 2, 0);
    string_free(&path);
    ui_window_resize_base(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    ui_window_invalidate_base(&btn->button.base);
}

void ui_refining_btn_again_set_bitmaps(struct UIRefining_BtnAgain *self)
{
    set_result_bitmaps(&self->button, REFINE_DIR "bt_refining_again_%s_normal.bmp",
                       REFINE_DIR "bt_refining_again_%s_press.bmp", self->highlight);
}

void ui_refining_btn_return_set_bitmaps(struct UIRefining_BtnReturn *self)
{
    set_result_bitmaps(&self->button, REFINE_DIR "bt_refining_return_%s_normal.bmp",
                       REFINE_DIR "bt_refining_return_%s_press.bmp", self->highlight);
}

void ui_refining_notice_set_text(struct UIRefining_Notice *self, const char *text, int delay, BYTE red)
{
    self->red = red;
    string_assign_cstr(&self->base.text, text);
    if (!delay) {
        string_assign_cstr(&self->next_text, text);
        self->next_red = red;
    } else {
        self->due = timeGetTime() + delay;
        if (!self->base.base.visible) {
            self->hide_when_due = 1;
            self->base.base.visible = 1;
        }
    }
    ui_window_invalidate_base(&self->base.base);
    if (self->base.base.parent)
        ui_window_invalidate(self->base.base.parent);
}
