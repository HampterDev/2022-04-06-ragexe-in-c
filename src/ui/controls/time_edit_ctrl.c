#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "core/text_util.h"
#include "diag/perf.h"

#define TIME_EDIT_ID 0xb0
#define TIME_FIELD_MAX 0x3c

struct UIShowAndEditTimeCtrl {
    struct UIWindow base;
    struct UIWindow *edits[2];
};
const struct UIWindowOps ui_show_and_edit_time_ctrl_ops = {
    .destroy = ui_show_and_edit_time_ctrl_destroy,
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
    .on_create = ui_show_and_edit_time_ctrl_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_show_and_edit_time_ctrl_on_draw,
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
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_show_and_edit_time_ctrl_send_msg,
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

void ui_show_and_edit_time_ctrl_destroy(struct UIWindow *window)
{
    struct UIShowAndEditTimeCtrl *self = (struct UIShowAndEditTimeCtrl *)window;
    self->base.ops = &ui_show_and_edit_time_ctrl_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_show_and_edit_time_ctrl_on_create(struct UIWindow *window, int cx, int cy)
{
    struct UIShowAndEditTimeCtrl *self = (struct UIShowAndEditTimeCtrl *)window;
    int i, x;

    for (i = 0, x = 0; x < 0x4c; i++, x += 0x26) {
        struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));

        if (edit) {
            ui_edit_ctrl_construct(edit);
            edit->ops = &ui_edit_ctrl_var_id_ops.base;
        }
        self->edits[i] = edit;
        ui_window_create(edit, 0x14, 0x12);
        ui_edit_ctrl_set_back_colour(self->edits[i], 0xe8, 0xe8, 0xe8);
        ui_edit_ctrl_set_border_colour(self->edits[i], 0xc0, 0xc0, 0xc0);
        UI_EDIT_NO_BORDER(self->edits[i]) = 0;
        UI_EDIT_MAX_CHARS(self->edits[i]) = 2;
        ui_window_move_base(self->edits[i], x, 0);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edits[i], "0");
        self->edits[i]->id = TIME_EDIT_ID;
        ui_window_add_child(&self->base, self->edits[i]);
    }
}

void ui_show_and_edit_time_ctrl_on_draw(struct UIWindow *window)
{
    struct UIShowAndEditTimeCtrl *self = (struct UIShowAndEditTimeCtrl *)window;
    ui_window_clear_canvas(&self->base, 0xffff00ff);
    ui_window_text_out(&self->base, 0x16, 3, "m", 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(&self->base, 0x3c, 3, "s", 0, 0, 0xc, 0, 0, 0);
}

int ui_show_and_edit_time_ctrl_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UIShowAndEditTimeCtrl *self = (struct UIShowAndEditTimeCtrl *)window;
    int i;

    if (msg != UI_MSG_COMMAND || val1 != TIME_EDIT_ID)
        return 0;
    for (i = 0; i < 2; i++) {
        struct String text;
        int n;

        if (sender != self->edits[i])
            continue;
        n = atoi(ui_edit_ctrl_get_text(self->edits[i]));
        if (n > TIME_FIELD_MAX)
            n = TIME_FIELD_MAX;
        string_init(&text);
        string_format(&text, "%d", n);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edits[i], string_cstr(&text));
        string_free(&text);
    }
    return 0;
}
