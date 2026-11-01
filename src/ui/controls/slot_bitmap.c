#include "ui/controls/static.h"
#include "ui/controls/buttons.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include "base.h"
#include "diag/perf.h"

#define SLOT_ADD_BUTTON_ID 0xc3
#define SLOT_VAL_R_BTN_DOWN 0xda

void ui_slot_bitmap_destruct(struct UISlotBitmap *self)
{
    self->bitmap.base.ops = &ui_slot_bitmap_ops;
    ui_static_bitmap_destruct(&self->bitmap);
}

void ui_slot_bitmap_destroy(struct UIWindow *window)
{
    struct UISlotBitmap *self = (struct UISlotBitmap *)window;
    ui_slot_bitmap_destruct(self);
    mem_free(self);
}

void ui_slot_bitmap_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UISlotBitmap *self = (struct UISlotBitmap *)window;
    struct UIWindow *wnd = &self->bitmap.base;
    struct UIBitmapButton *button;
    struct UIWindow *bwnd;

    if (self->has_add_button != 1)
        return;
    button = mem_alloc(sizeof *button);
    self->add_button = button = button ? ui_bitmap_button_construct(button) : NULL;
    ui_bitmap_button_set_bitmap(self->add_button, UI_SKIN "select_character\\btn_add_out.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(self->add_button, UI_SKIN "select_character\\btn_add_over.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(self->add_button, UI_SKIN "select_character\\btn_add_press.bmp", UI_BTN_DOWN, 0);
    bwnd = &self->add_button->button.base;
    ui_window_create(bwnd, self->add_button->bitmap_width, self->add_button->bitmap_height);
    ui_window_move_base(bwnd, (int)((wnd->width - bwnd->width) * 0.5), (int)((wnd->height - bwnd->height) * 0.5));
    bwnd->id = SLOT_ADD_BUTTON_ID;
    ui_window_add_child(wnd, bwnd);
}

void ui_slot_bitmap_on_l_btn_down(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UISlotBitmap *self = (struct UISlotBitmap *)window;
    struct UIWindow *wnd = &self->bitmap.base;

    if (!wnd->parent)
        return;
    ui_window_send_msg(wnd->parent, wnd, UI_MSG_SET_STATE, self->slot, 0, 0, 0);
    ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, 0, 0, 0);
}

void ui_slot_bitmap_on_l_btn_dbl_clk(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UISlotBitmap *self = (struct UISlotBitmap *)window;
    struct UIWindow *wnd = &self->bitmap.base;

    if (wnd->parent)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, UI_ID_OK, 0, 0, 0);
}

void ui_slot_bitmap_on_r_btn_down(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UISlotBitmap *self = (struct UISlotBitmap *)window;
    struct UIWindow *wnd = &self->bitmap.base;

    if (!wnd->parent)
        return;
    ui_window_send_msg(wnd->parent, wnd, UI_MSG_SET_STATE, self->slot, 0, 0, 0);
    ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, SLOT_VAL_R_BTN_DOWN, 0, 0);
}

int ui_slot_bitmap_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UISlotBitmap *self = (struct UISlotBitmap *)window;
    struct UIWindow *wnd = &self->bitmap.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == SLOT_ADD_BUTTON_ID && wnd->parent)
            ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, SLOT_ADD_BUTTON_ID, self->slot, 0, 0);
        return 0;
    case UI_MSG_SET_STATE:
        wnd->state = val1;
        return 0;
    }
    return ui_window_send_msg_base(wnd, sender, msg, val1, val2, val3, val4);
}

const struct UIWindowOps ui_slot_bitmap_ops = {
    .destroy = ui_slot_bitmap_destroy,
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
    .on_create = ui_slot_bitmap_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_static_bitmap_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_slot_bitmap_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_slot_bitmap_on_l_btn_dbl_clk,
    .on_mouse_move = ui_static_bitmap_on_mouse_move,
    .on_mouse_hover = ui_static_bitmap_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_static_bitmap_on_mouse_shape,
    .on_l_btn_up = ui_static_bitmap_on_l_btn_up,
    .on_r_btn_down = ui_slot_bitmap_on_r_btn_down,
    .on_r_btn_up = ui_static_bitmap_on_r_btn_up,
    .on_wheel = ui_static_bitmap_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_slot_bitmap_send_msg,
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
