#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include "diag/perf.h"

struct UIPackageBoxNameList {
    struct UIListBox list;
};
SIZE_ASSERT(UIPackageBoxNameList, sizeof(struct UIListBox));

struct UIWindow;
void ui_package_box_name_list_destroy(struct UIWindow *window);
void ui_package_box_name_list_on_l_btn_down(struct UIWindow *self, int x, int y);
const struct UIListBoxOps ui_package_box_name_list_ops = {
    .base = {
        .destroy = ui_package_box_name_list_destroy,
        .resize = ui_list_box_resize,
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
        .on_create = ui_list_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_list_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_package_box_name_list_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_send_msg,
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
    .create = ui_list_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_list_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

void ui_package_box_name_list_destruct(struct UIPackageBoxNameList *self)
{
    self->list.base.ops = &ui_package_box_name_list_ops.base;
    self->list.base.ops = &ui_list_box_ops.base;
    string_vec_free(&self->list.items);
    ui_window_destruct(&self->list.base);
}

void ui_package_box_name_list_destroy(struct UIWindow *window)
{
    struct UIPackageBoxNameList *self = (struct UIPackageBoxNameList *)window;
    ui_package_box_name_list_destruct(self);
    mem_free(self);
}

void ui_package_box_name_list_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIPackageBoxNameList *self = (struct UIPackageBoxNameList *)window;
    self->list.selected = ui_list_box_hit_line(&self->list, x, y);
    if (self->list.selected != -1)
        ui_window_send_msg(self->list.base.parent, &self->list.base, UI_MSG_COMMAND, self->list.base.id,
                 self->list.selected, 0, 0);
    ui_window_invalidate_base(&self->list.base);
}
