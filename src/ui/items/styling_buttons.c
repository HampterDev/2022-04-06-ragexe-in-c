#include "ui/controls/buttons.h"
#include "world/tooltip.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/items/styling_shop.h"
#include "diag/perf.h"

struct UIBitmapButton;
void ui_styling_shop_button_destroy(struct UIWindow *window);
void ui_styling_shop_button_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_styling_shop_button_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_styling_shop_button_on_l_btn_up(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_styling_shop_button_ops = {
    .base = {
        .destroy = ui_styling_shop_button_destroy,
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
        .on_draw = ui_bitmap_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_styling_shop_button_on_mouse_move,
        .on_mouse_hover = ui_styling_shop_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_styling_shop_button_on_l_btn_up,
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

struct UIAccBitmapButton;
void ui_acc_bitmap_button_destroy(struct UIWindow *window);
void ui_acc_bitmap_button_on_r_btn_up(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_acc_bitmap_button_ops = {
    .base = {
        .destroy = ui_acc_bitmap_button_destroy,
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
        .on_draw = ui_bitmap_button_on_draw,
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
        .on_r_btn_up = ui_acc_bitmap_button_on_r_btn_up,
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

const char g_styleNoColourText[] = "No Color";

void ui_styling_shop_button_destroy(struct UIWindow *window)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    self->button.base.ops = &ui_styling_shop_button_ops.base;
    ui_bitmap_button_destruct(self);
    mem_free(self);
}

void ui_styling_shop_button_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    struct UIWindow *wnd = &self->button.base;

    if (self->button.disabled || ui_window_mgr_get_capture(&g_windowMgr) != wnd)
        return;
    if (x < 0 || x >= wnd->width || y < 0 || y >= wnd->height || wnd->state == UI_BTN_DOWN)
        return;
    wnd->state = UI_BTN_DOWN;
    ui_window_invalidate_base(wnd);
}

void ui_styling_shop_button_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    int tip_x, tip_y;

    (void)x;
    (void)y;
    if (self->bitmaps[UI_BTN_HOVER].size != 0)
        return;
    tip_x = 0;
    tip_y = -0x14;
    ui_window_to_screen(&self->button.base, &tip_x, &tip_y);
    mode_show_tooltip(client_current_mode(), g_styleNoColourText, tip_x, tip_y, 0xffffffff, 0);
}

void ui_styling_shop_button_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    struct UIWindow *wnd = &self->button.base;
    struct UIWindow *parent;

    (void)x;
    (void)y;
    if (self->button.disabled || ui_window_mgr_get_capture(&g_windowMgr) != wnd)
        return;
    parent = wnd->parent;
    wnd->state = UI_BTN_DOWN;
    if (parent) {
        if (ui_window_mgr_get_window_flag(&g_windowMgr, 3, parent->id)) {
            if (wnd->id == UI_ID_SEND_EDITS_A || wnd->id == UI_ID_SEND_EDITS_B)
                ui_window_send_edit_fields(wnd->parent);
            ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, self->button.cmd_value, 0, 0);
        }
    } else {
        mode_send_message(client_current_mode(), MODE_MSG_UI_COMMAND, wnd->id, self->button.cmd_value, 0, 0);
    }
    ui_window_invalidate_base(wnd);
    ui_window_mgr_release_capture(&g_windowMgr);
}

void ui_acc_bitmap_button_destroy(struct UIWindow *window)
{
    struct UIAccBitmapButton *self = (struct UIAccBitmapButton *)window;
    self->btn.button.base.ops = &ui_acc_bitmap_button_ops.base;
    ui_bitmap_button_destruct(&self->btn);
    mem_free(self);
}

void ui_acc_bitmap_button_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIAccBitmapButton *self = (struct UIAccBitmapButton *)window;
    struct UIWindow *wnd = &self->btn.button.base;

    (void)x;
    (void)y;
    ui_window_send_msg(wnd->parent, wnd, STYLE_MSG_ACCESSORY, wnd->id, self->btn.button.cmd_value, 0, 0);
}
