#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_std.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"

struct UIEditCtrl;
struct UIWindow;
const struct UIEditCtrlOps ui_edit_ctrl2_ops = {
    .base = {
        .destroy = ui_edit_ctrl2_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl2_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};

struct UIEditCtrl4;
const struct UIEditCtrlOps ui_edit_ctrl4_ops = {
    .base = {
        .destroy = ui_edit_ctrl4_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl4_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl4_draw_text,
    .truncate = ui_edit_ctrl_truncate_default,
};

const struct UIEditCtrlOps ui_edit_ctrl5_ops = {
    .base = {
        .destroy = ui_edit_ctrl5_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl4_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl2_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl4_draw_text,
    .truncate = ui_edit_ctrl_truncate_default,
};

const struct UIEditCtrlOps ui_edit_ctrl_wheel_ops = {
    .base = {
        .destroy = ui_edit_ctrl_wheel_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_edit_ctrl_wheel_to_parent,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};

const struct UIEditCtrlOps ui_edit_ctrl_var_id_ops = {
    .base = {
        .destroy = ui_edit_ctrl_var_id_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_var_id_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_edit_ctrl_wheel_to_parent,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl_var_id_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_var_id_set_text,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};

struct UIWindow *ui_edit_ctrl2_construct(struct UIWindow *wnd)
{
    ui_edit_ctrl_construct(wnd);
    wnd->ops = &ui_edit_ctrl2_ops.base;
    return wnd;
}

void ui_edit_ctrl2_destruct(struct UIEditCtrl *self)
{
    self->base.ops = &ui_edit_ctrl2_ops.base;
    ui_edit_ctrl_destruct(self);
}

void ui_edit_ctrl2_destroy(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    self->base.ops = &ui_edit_ctrl2_ops.base;
    ui_edit_ctrl_destruct(self);
    mem_free(self);
}

int ui_edit_ctrl2_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    if (msg != UI_MSG_DEFAULT_BUTTON) {
        ui_edit_ctrl_send_msg((struct UIWindow *)(self), sender, msg, val1, val2, val3, val4);
        return 0;
    }
    if (ui_window_mgr_is_last_edit(&g_windowMgr, ui_window_mgr_get_focus_edit(&g_windowMgr)))
        ui_window_mgr_set_focus_edit_first(&g_windowMgr);
    else
        ui_window_mgr_focus_next_edit(&g_windowMgr);
    ui_window_send_msg_base(&self->base, sender, UI_MSG_DEFAULT_BUTTON, val1, val2, val3, val4);
    return 0;
}

struct UIEditCtrl4 *ui_edit_ctrl4_construct(struct UIEditCtrl4 *self)
{
    ui_edit_ctrl_construct(&self->edit.base);
    self->edit.base.ops = &ui_edit_ctrl4_ops.base;
    self->alt_colour = (COLORREF)-1;
    string_init(&self->empty_text);
    self->use_alt = 0;
    return self;
}

struct UIEditCtrl4 *ui_edit_ctrl5_construct(struct UIEditCtrl4 *self)
{
    ui_edit_ctrl4_construct(self);
    self->edit.base.ops = &ui_edit_ctrl5_ops.base;
    return self;
}

void ui_edit_ctrl4_destruct(struct UIEditCtrl4 *self)
{
    self->edit.base.ops = &ui_edit_ctrl4_ops.base;
    string_free(&self->empty_text);
    ui_edit_ctrl_destruct(&self->edit);
}

void ui_edit_ctrl5_destruct(struct UIEditCtrl4 *self)
{
    self->edit.base.ops = &ui_edit_ctrl5_ops.base;
    ui_edit_ctrl4_destruct(self);
}

void ui_edit_ctrl4_destroy(struct UIWindow *window)
{
    struct UIEditCtrl4 *self = (struct UIEditCtrl4 *)window;
    ui_edit_ctrl4_destruct(self);
    mem_free(self);
}

void ui_edit_ctrl5_destroy(struct UIWindow *window)
{
    struct UIEditCtrl4 *self = (struct UIEditCtrl4 *)window;
    ui_edit_ctrl5_destruct(self);
    mem_free(self);
}

void ui_edit_ctrl4_on_focus(struct UIWindow *window)
{
    struct UIEditCtrl4 *self = (struct UIEditCtrl4 *)window;
    string_clear(&self->edit.text);
    ui_edit_ctrl_on_focus((struct UIWindow *)(&self->edit));
}

void ui_edit_ctrl4_set_empty_text(struct UIWindow *self, const char *text)
{
    struct UIEditCtrl4 *edit = (struct UIEditCtrl4 *)self;

    string_assign_cstr(&edit->empty_text, text);
    ui_edit_ctrl_truncate_default(&edit->edit);
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == self) {
        language_cancel_input(g_language);
        language_cancel_input(g_language);
        language_replace_selection(g_language, string_cstr(&edit->empty_text));
    }
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == self) {
        language_set_selection(g_language, 0, (int)g_language->input_length);
    } else {
        edit->edit.sel_start = 0;
        edit->edit.sel_end = (int)edit->edit.text.size;
    }
    if (self->parent)
        ui_window_invalidate(self->parent);
}

void ui_edit_ctrl_var_id_destruct(struct UIEditCtrl *self)
{
    self->base.ops = &ui_edit_ctrl_var_id_ops.base;
    ui_edit_ctrl_destruct(self);
}

void ui_edit_ctrl_var_id_destroy(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    ui_edit_ctrl_var_id_destruct(self);
    mem_free(self);
}

void ui_edit_ctrl_var_id_on_focus_lost(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    ui_edit_ctrl_sync_from_ime(self);
    ui_window_invalidate_base(&self->base);
    ui_window_send_msg(&self->base, &self->base, UI_MSG_DEFAULT_BUTTON, 0, 0, 0, 0);
}

void ui_edit_ctrl_wheel_to_parent(struct UIWindow *window, int delta)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    if (self->base.parent)
        ui_window_on_wheel(self->base.parent, delta);
}

int ui_edit_ctrl_var_id_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                       int val3, int val4)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    if (msg != UI_MSG_DEFAULT_BUTTON)
        return ui_edit_ctrl_send_msg((struct UIWindow *)(self), sender, msg, val1, val2, val3, val4);
    if (self->base.parent)
        ui_window_send_msg(self->base.parent, &self->base, UI_MSG_COMMAND, self->base.id, 0, 0, 0);
    return 0;
}

void ui_edit_ctrl_var_id_set_text(struct UIEditCtrl *self, const char *text)
{
    ui_edit_ctrl_set_text_default(self, text);
    ui_window_invalidate_base(&self->base);
}

void ui_edit_ctrl_wheel_destruct(struct UIEditCtrl *self)
{
    self->base.ops = &ui_edit_ctrl_wheel_ops.base;
    ui_edit_ctrl_destruct(self);
}

void ui_edit_ctrl_wheel_destroy(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    ui_edit_ctrl_wheel_destruct(self);
    mem_free(self);
}

#define LINE_COUNT(e) ((e)->lines.size)

void ui_bitmap_edit_ctrl_destroy(struct UIWindow *window)
{
    struct UIBitmapEditCtrl *self = (struct UIBitmapEditCtrl *)window;
    string_free(&self->bitmap);
    ui_edit_ctrl_destruct(&self->edit);
    mem_free(self);
}

void ui_bitmap_edit_ctrl_set_bitmap(struct UIBitmapEditCtrl *self, const char *name)
{
    struct BitmapRes *bitmap;

    string_assign_cstr(&self->bitmap, name);
    bitmap = ui_skin_bitmap(string_cstr(&self->bitmap));
    self->edit.base.width = bitmap->width;
    self->edit.base.height = bitmap->height;
}

void ui_bitmap_edit_ctrl_on_draw(struct UIWindow *window)
{
    struct UIBitmapEditCtrl *self = (struct UIBitmapEditCtrl *)window;
    ui_window_clear_canvas(&self->edit.base, 0xffffffff);
    ui_window_draw_bitmap(&self->edit.base, 0, 0, res_mgr_get(g_resMgr(), string_cstr(&self->bitmap)),
                        0);
    self->edit.text_y = 3;
    ui_edit_ctrl_draw_text_default(&self->edit);
}
