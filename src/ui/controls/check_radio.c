#include "ui/controls/check_radio.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/controls/controls.h"
#include "ui/game.h"
#include <string.h>
#include "core/skin_path.h"
#include "diag/perf.h"

struct UIRadioBtn;
struct UIWindow;
const struct UIWindowOps ui_radio_btn_ops = {
    .destroy = ui_radio_btn_destroy,
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
    .on_draw = ui_radio_btn_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_radio_btn_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_radio_btn_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_radio_btn_send_msg,
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

void ui_check_box_destruct(struct UICheckBox *self)
{
    self->base.ops = &ui_check_box_ops;
    string_free(&self->unchecked_tooltip);
    string_free(&self->tooltip);
    string_free(&self->label);
    string_free(&self->unchecked_bitmap);
    string_free(&self->checked_bitmap);
    ui_window_destruct(&self->base);
}

void ui_check_box_destroy(struct UIWindow *window)
{
    struct UICheckBox *self = (struct UICheckBox *)window;
    ui_check_box_destruct(self);
    mem_free(self);
}

static struct BitmapRes *check_box_bitmap(struct UICheckBox *self)
{
    switch (self->base.state) {
    case 0:
        return ui_skin_bitmap(string_cstr(&self->unchecked_bitmap));
    case 1:
        return ui_skin_bitmap(string_cstr(&self->checked_bitmap));
    }
    return NULL;
}

void ui_check_box_on_draw(struct UIWindow *window)
{
    struct UICheckBox *self = (struct UICheckBox *)window;
    struct UIWindow *wnd = &self->base;
    struct BitmapRes *bitmap;
    int label_x = 0;

    if (!self->checked_bitmap.size || !self->unchecked_bitmap.size)
        return;
    if (self->disabled) {
        ui_window_clear_canvas(wnd, 0xffff00ff);
        return;
    }
    if (self->no_draw)
        return;
    if (wnd->state == 0 || wnd->state == 1) {
        bitmap = check_box_bitmap(self);
        ui_window_draw_bitmap(wnd, 0, (wnd->height - bitmap->height) / 2, bitmap, 0);
        label_x = bitmap->width;
    }
    if (self->label.size)
        ui_window_text_out(wnd, label_x + 3, 0, string_cstr(&self->label), 0, 0, self->label_height,
                         self->label_colour, 0, 0);
}

void ui_check_box_on_l_btn_down(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UICheckBox *self = (struct UICheckBox *)window;
    struct UIWindow *wnd = &self->base;

    if (self->disabled)
        return;
    wnd->state = 1 - wnd->state;
    ui_window_invalidate_base(wnd);
    if (ui_window_get_ancestor(wnd))
        ui_window_invalidate(ui_window_get_ancestor(wnd));
    if (wnd->parent)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, wnd->state, 0, 0);
    if (self->send_notice)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_CHECK_NOTICE, self->notice_value, 0, 0, 0);
}

void ui_check_box_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UICheckBox *self = (struct UICheckBox *)window;
    struct UIWindow *wnd = &self->base;
    struct String *tip;
    int tip_x, tip_y;

    if (self->disabled)
        return;
    ui_window_on_mouse_move_base(wnd, x, y);
    if (!self->tooltip.size)
        return;
    tip_x = 0;
    tip_y = -20;
    ui_window_to_screen(wnd, &tip_x, &tip_y);
    tip = wnd->state == 0 && self->unchecked_tooltip.size ? &self->unchecked_tooltip : &self->tooltip;
    mode_show_tooltip(client_current_mode(), string_cstr(tip), tip_x, tip_y, 0xffffffff, 0);
}

void ui_check_box_on_mouse_shape(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UICheckBox *self = (struct UICheckBox *)window;
    if (!self->disabled)
        mode_set_cursor_action(client_current_mode(), CURSOR_HAND);
}

void ui_check_box_fit_to_label(struct UICheckBox *self)
{
    struct UIWindow *wnd = &self->base;
    int text_width, width;

    if (!self->label.size)
        return;
    text_width = ui_window_text_width(wnd, string_cstr(&self->label), 0, 0, 12, 0, 0);
    width = check_box_bitmap(self)->width + text_width + 3;
    if (width != wnd->width)
        ui_window_resize_base(wnd, width, wnd->height);
}

void ui_check_box_set_bitmaps_fit(struct UICheckBox *self, const char *checked, const char *unchecked)
{
    struct BitmapRes *bitmap;
    const char *path;

    string_assign_cstr(&self->checked_bitmap, checked);
    string_assign_cstr(&self->unchecked_bitmap, unchecked);
    path = ui_skin_path(string_cstr(&self->checked_bitmap));
    bitmap = res_mgr_get(g_resMgr(), path);
    self->base.width = bitmap->width;
    self->base.height = bitmap->height;
    path = ui_skin_path(string_cstr(&self->unchecked_bitmap));
    res_mgr_get(g_resMgr(), path);
}

void ui_check_box_set_check_quiet(struct UICheckBox *self, int checked)
{
    struct UIWindow *wnd = &self->base;

    if (self->disabled)
        return;
    wnd->state = checked;
    ui_window_invalidate_base(wnd);
    if (ui_window_get_ancestor(wnd))
        ui_window_invalidate(ui_window_get_ancestor(wnd));
}

struct UIRadioBtn *ui_radio_btn_construct(struct UIRadioBtn *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_radio_btn_ops;
    string_init(&self->label);
    string_init(&self->tooltip);
    string_init(&self->on_bitmap);
    string_init(&self->off_bitmap);
    self->base.state = 0;
    self->enabled = 1;
    self->base.id = UI_ID_RADIO;
    self->alt_on_bitmap = 0;
    self->value = 0;
    self->label_height = 12;
    self->label_colour = RGB(0, 0, 0);
    return self;
}

void ui_radio_btn_destruct(struct UIRadioBtn *self)
{
    self->base.ops = &ui_radio_btn_ops;
    string_free(&self->off_bitmap);
    string_free(&self->on_bitmap);
    string_free(&self->tooltip);
    string_free(&self->label);
    ui_window_destruct(&self->base);
}

void ui_radio_btn_destroy(struct UIWindow *window)
{
    struct UIRadioBtn *self = (struct UIRadioBtn *)window;
    ui_radio_btn_destruct(self);
    mem_free(self);
}

void ui_radio_btn_on_draw(struct UIWindow *window)
{
    struct UIRadioBtn *self = (struct UIRadioBtn *)window;
    struct UIWindow *wnd = &self->base;
    struct BitmapRes *bitmap;
    COLORREF colour = self->enabled == 1 ? self->label_colour : UI_GREY_TEXT;
    int x, y;

    if (self->on_bitmap.size && self->off_bitmap.size) {
        ui_window_clear_canvas(wnd, 0xffff00ff);
        bitmap = ui_skin_bitmap(string_cstr(&self->on_bitmap));
        if (wnd->state == 0)
            bitmap = ui_skin_bitmap(string_cstr(&self->off_bitmap));
        if (bitmap)
            ui_window_draw_bitmap(wnd, 0, (int)((float)(wnd->height - bitmap->height) * 0.5f), bitmap, 1);
        y = ui_window_text_height(wnd, string_cstr(&self->label), 0, 0, self->label_height, 0, 0);
        y = (int)((float)(wnd->height - y) * 0.5f);
        x = bitmap->width + 3;
    } else {
        const char *path;

        ui_window_clear_canvas(wnd, 0xffff00ff);
        if (wnd->state == 0)
            path = UI_SKIN "radiobtn_off.bmp";
        else
            path = self->alt_on_bitmap ? UI_SKIN "radiobtn_on1.bmp" : UI_SKIN "radiobtn_on.bmp";
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(path), 0);
        colour = self->enabled ? self->label_colour : UI_GREY_TEXT;
        x = 13;
        y = 0;
    }
    ui_window_text_out(wnd, x, y, string_cstr(&self->label), 0, 0, self->label_height, colour, 0, 0);
}

void ui_radio_btn_on_l_btn_down(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIRadioBtn *self = (struct UIRadioBtn *)window;
    struct UIWindow *wnd = &self->base;

    if (wnd->parent && self->enabled)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, self->value, 0, 0);
}

void ui_radio_btn_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIRadioBtn *self = (struct UIRadioBtn *)window;
    int tip_x, tip_y;

    ui_window_on_mouse_move_base(&self->base, x, y);
    if (!self->tooltip.size)
        return;
    tip_x = 10;
    tip_y = -20;
    ui_window_to_screen(&self->base, &tip_x, &tip_y);
    mode_show_tooltip(client_current_mode(), string_cstr(&self->tooltip), tip_x, tip_y, 0xffffffff, 0);
}

int ui_radio_btn_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UIRadioBtn *self = (struct UIRadioBtn *)window;
    if (msg == UI_MSG_SET_STATE) {
        self->base.state = val1;
        ui_window_invalidate_base(&self->base);
    }
    return 0;
}

void ui_window_set_state(struct UIWindow *self, int state)
{
    self->state = state;
    ui_window_invalidate(self);
}

void ui_radio_btn_set_label(struct UIWindow *self, const char *label)
{
    string_assign_cstr(&((struct UIRadioBtn *)self)->label, label);
    ui_window_invalidate(self);
}

struct UICheckBox *ui_check_box_construct(struct UIWindow *wnd)
{
    struct UICheckBox *self = (struct UICheckBox *)wnd;

    ui_window_construct(wnd, 0);
    wnd->ops = &ui_check_box_ops;
    string_init(&self->checked_bitmap);
    string_init(&self->unchecked_bitmap);
    string_init(&self->label);
    string_init(&self->tooltip);
    string_init(&self->unchecked_tooltip);
    self->disabled = 0;
    wnd->state = 0;
    wnd->id = UI_ID_CHECKBOX;
    self->no_draw = 0;
    self->send_notice = 0;
    self->label_colour = RGB(0, 0, 0);
    self->label_height = 12;
    self->notice_value = -1;
    return self;
}

void ui_check_box_set_bitmaps(struct UIWindow *wnd, const char *checked,
                                      const char *unchecked, const char *label)
{
    struct UICheckBox *self = (struct UICheckBox *)wnd;

    string_assign_cstr(&self->checked_bitmap, checked);
    string_assign_cstr(&self->unchecked_bitmap, unchecked);
    res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->checked_bitmap)));
    res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->unchecked_bitmap)));
    if (!label)
        label = "";
    string_assign_cstr(&self->label, label);
    ui_check_box_fit_to_label(self);
}

void ui_check_box_set_check(struct UIWindow *wnd, int checked)
{
    struct UICheckBox *self = (struct UICheckBox *)wnd;

    if (self->disabled)
        return;
    ui_check_box_set_check_quiet(self, checked);
    if (wnd->parent)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, wnd->state, 0, 0);
}

void ui_check_box_set_hidden(struct UIWindow *wnd, BYTE hidden)
{
    struct UICheckBox *self = (struct UICheckBox *)wnd;

    self->no_draw = hidden;
    ui_window_set_show(wnd, hidden ^ 1);
    ui_window_invalidate(wnd);
}

void ui_check_box_set_tool_tip(struct UIWindow *self, const char *text)
{
    if (text)
        string_assign_cstr(&((struct UICheckBox *)self)->tooltip, text);
}

struct UICheckBox;
struct UIWindow;
const struct UIWindowOps ui_check_box_ops = {
    .destroy = ui_check_box_destroy,
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
    .on_draw = ui_check_box_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_check_box_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_check_box_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_check_box_on_mouse_shape,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
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
};
