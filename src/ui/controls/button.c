#include "ui/controls/buttons.h"
#include "ui/controls/ui_hooks.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "ui/ui_std.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include <string.h>
#include <stdlib.h>
#include "ui/controls/controls.h"
#include "core/skin_path.h"
#include "diag/perf.h"

struct UIButton;
struct UIWindow;
const struct UIButtonOps ui_button_ops = {
    .base = {
        .destroy = ui_button_destroy,
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
        .on_draw = ui_button_on_draw,
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
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

struct UINoneVisibleButton;
const struct UIButtonOps ui_none_visible_button_ops = {
    .base = {
        .destroy = ui_none_visible_button_destroy,
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
        .on_draw = ui_none_visible_button_on_draw,
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
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

struct UIBitmapButton;
const struct UIButtonOps ui_bitmap_button_ops = {
    .base = {
        .destroy = ui_bitmap_button_destroy,
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
    },
    .set_show = ui_button_set_show,
};

struct UIBasicButton;
const struct UIButtonOps ui_basic_button_ops = {
    .base = {
        .destroy = ui_basic_button_destroy,
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
        .on_draw = ui_basic_button_on_draw,
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
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

struct UIBmpTextButton;
const struct UIButtonOps ui_bmp_text_button_ops = {
    .base = {
        .destroy = ui_bmp_text_button_destroy,
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
        .on_draw = ui_bmp_text_button_on_draw,
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
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

struct UIButton *ui_button_construct(struct UIButton *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_button_ops.base;
    string_init(&self->text);
    string_init(&self->tooltip);
    self->base.state = UI_BTN_UP;
    self->disabled = 0;
    self->skinned = 0;
    self->cmd_value = 0;
    return self;
}

void ui_button_destruct(struct UIButton *self)
{
    self->base.ops = &ui_button_ops.base;
    string_free(&self->tooltip);
    string_free(&self->text);
    ui_window_destruct(&self->base);
}

void ui_button_destroy(struct UIWindow *window)
{
    struct UIButton *self = (struct UIButton *)window;
    ui_button_destruct(self);
    mem_free(self);
}

void ui_button_on_draw(struct UIWindow *window)
{
    struct UIButton *self = (struct UIButton *)window;
    struct BitmapRes *bitmap;

    if (!self->skinned)
        return;
    switch (self->base.state) {
    case UI_BTN_UP:
        bitmap = ui_skin_bitmap(UI_SKIN "sml_button_up.bmp");
        break;
    case UI_BTN_HOVER:
        bitmap = ui_skin_bitmap(UI_SKIN "sml_button_act.bmp");
        break;
    case UI_BTN_DOWN:
        bitmap = ui_skin_bitmap(UI_SKIN "sml_button_down.bmp");
        break;
    default:
        bitmap = NULL;
        break;
    }
    ui_window_draw_bitmap(&self->base, 0, 0, bitmap, 0);
    ui_window_text_out(&self->base, 0, 0, string_cstr(&self->text), 0, 0, 12, RGB(0, 0, 0), 0, 0);
}

void ui_button_on_l_btn_down(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIButton *self = (struct UIButton *)window;
    if (self->disabled)
        return;
    ui_window_mgr_set_capture(&g_windowMgr, &self->base);
    self->base.state = UI_BTN_DOWN;
    ui_window_invalidate_base(&self->base);
}

void ui_button_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIButton *self = (struct UIButton *)window;
    ui_window_on_l_btn_down(&self->base, x, y);
}

void ui_button_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIButton *self = (struct UIButton *)window;
    struct UIWindow *wnd = &self->base;
    struct UIWindow *parent;
    int inside, want;

    if (self->disabled)
        return;
    inside = x >= 0 && x < wnd->width && y >= 0 && y < wnd->height;
    if (!inside)
        want = UI_BTN_UP;
    else if (ui_window_mgr_get_capture(&g_windowMgr) == wnd)
        want = UI_BTN_DOWN;
    else
        want = UI_BTN_HOVER;
    if (wnd->state != want) {
        wnd->state = want;
        ui_window_invalidate_base(wnd);
    }

    parent = wnd->parent;
    if (parent == g_windowMgr.basic_info_wnd || parent == ui_window_find(UI_WND_NOTE_TARGET))
        ui_window_send_msg(parent, wnd, UI_MSG_COMMAND, UI_ID_HOVER_NOTE, wnd->state != UI_BTN_UP, 0, 0);
}

void ui_button_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIButton *self = (struct UIButton *)window;
    struct UIWindow *wnd = &self->base;
    struct UIWindow *parent = wnd->parent;

    if (parent && (wnd->id == UI_ID_CLOSE || wnd->id == UI_ID_HELP) && parent->id != UI_WND_NO_TOOLTIPS) {
        if (wnd->id == UI_ID_HELP && parent->id != 0) {
            string_clear(&self->tooltip);
        } else {
            char *behavior = NULL;

            if (client_lua_call(g_session1.luaState, "GetBeHaviorOfWindow", "d>s", parent->id, &behavior))
                string_assign_cstr(&self->tooltip, behavior);
        }
    }
    if (self->tooltip.size) {
        int tip_x = 0, tip_y = -20;

        ui_window_to_screen(wnd, &tip_x, &tip_y);
        ui_show_tooltip(string_cstr(&self->tooltip), tip_x, tip_y, 0xffffffff, 0);
    }
    if (!self->disabled)
        ui_window_on_mouse_move(wnd, x, y);
}

void ui_button_track_mouse(struct UIWindow *window, int x, int y)
{
    struct UIButton *self = (struct UIButton *)window;
    struct UIWindow *wnd = &self->base;

    if (x >= wnd->x && x <= wnd->x + wnd->width && y >= wnd->y && y <= wnd->y + wnd->height)
        return;
    if (wnd->state != UI_BTN_UP) {
        wnd->state = UI_BTN_UP;
        ui_window_invalidate_base(wnd);
    }
}

void ui_button_on_mouse_shape(struct UIWindow *window, int x, int y)
{
    struct UIButton *self = (struct UIButton *)window;
    if (self->disabled) {
        ui_window_on_mouse_shape_base(&self->base, x, y);
        return;
    }
    ui_set_cursor_action(CURSOR_HAND);
}

void ui_button_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIButton *self = (struct UIButton *)window;
    struct UIWindow *wnd = &self->base;
    BYTE disabled = self->disabled;
    struct UIWindow *capture = ui_window_mgr_get_capture(&g_windowMgr);

    if (disabled) {
        if (capture == wnd)
            ui_window_mgr_release_capture(&g_windowMgr);
        return;
    }
    if (capture != wnd)
        return;
    if (x >= 0 && x < wnd->width && y >= 0 && y < wnd->height) {
        wnd->state = UI_BTN_HOVER;
        if (wnd->parent) {
            {
                ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, self->cmd_value, 0, 0);
            }
        } else {
            mode_send_message(client_current_mode(), MODE_MSG_UI_COMMAND, wnd->id, self->cmd_value, 0, 0);
        }
    } else {
        wnd->state = UI_BTN_UP;
    }
    ui_window_invalidate_base(wnd);
    ui_window_mgr_release_capture(&g_windowMgr);
}

void ui_button_set_show(struct UIButton *self, BYTE show)
{
    self->disabled = show ^ 1;
    ui_window_invalidate_base(&self->base);
    self->base.visible = show;
}

void ui_bitmap_button_destruct(struct UIBitmapButton *self)
{
    int i;

    self->button.base.ops = &ui_bitmap_button_ops.base;
    for (i = 3; i >= 0; i--)
        string_free(&self->bitmaps[i]);
    ui_button_destruct(&self->button);
}

void ui_bitmap_button_destroy(struct UIWindow *window)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    ui_bitmap_button_destruct(self);
    mem_free(self);
}

void ui_bitmap_button_on_draw(struct UIWindow *window)
{
    struct UIBitmapButton *self = (struct UIBitmapButton *)window;
    struct UIWindow *wnd = &self->button.base;
    struct BitmapRes *bitmap;

    if (wnd->state >= UI_BTN_UP && wnd->state <= UI_BTN_DOWN)
        bitmap = ui_skin_bitmap(string_cstr(&self->bitmaps[wnd->state]));
    else
        bitmap = NULL;
    if (self->button.disabled && self->bitmaps[UI_BMP_DISABLED].size)
        bitmap = ui_skin_bitmap(string_cstr(&self->bitmaps[UI_BMP_DISABLED]));
    ui_window_draw_bitmap(wnd, 0, 0, bitmap, 0);
    if (self->redraw_parent && wnd->parent)
        ui_window_invalidate(wnd->parent);
}

struct UIBasicButton *ui_basic_button_construct(struct UIBasicButton *self)
{
    ui_button_construct(&self->button);
    self->button.base.ops = &ui_basic_button_ops.base;
    string_init(&self->label);
    self->button.base.height = UI_BASIC_BUTTON_HEIGHT;
    self->redraw_parent = 0;
    return self;
}

void ui_basic_button_destroy(struct UIWindow *window)
{
    struct UIBasicButton *self = (struct UIBasicButton *)window;
    self->button.base.ops = &ui_basic_button_ops.base;
    string_free(&self->label);
    ui_button_destruct(&self->button);
    mem_free(self);
}

BYTE ui_basic_button_set_label(struct UIWindow *self, int width, const char *label)
{
    if (!label)
        return 0;
    ui_window_resize(self, width, self->height);
    string_assign_cstr(&((struct UIBasicButton *)self)->label, label);
    if (width < ui_window_text_width(self, string_cstr(&((struct UIBasicButton *)self)->label), 0, 0, 0xc, 0, 0))
        return 0;
    ui_window_invalidate(self);
    return 1;
}

struct BasicButtonLook {
    const char *left, *mid, *right;
};

#define BASIC_SKIN UI_SKIN "basic_interface\\"
static const struct BasicButtonLook basic_look_disabled = {
    BASIC_SKIN "btn_disable_left.bmp", BASIC_SKIN "btn_disable_mid.bmp", BASIC_SKIN "btn_disable_right.bmp"};
static const struct BasicButtonLook basic_look_pressed = {
    BASIC_SKIN "btn_press_left.bmp", BASIC_SKIN "btn_press_mid.bmp", BASIC_SKIN "btn_press_right.bmp"};
static const struct BasicButtonLook basic_look_over = {
    BASIC_SKIN "btn_over_left.bmp", BASIC_SKIN "btn_over_mid.bmp", BASIC_SKIN "btn_over_right.bmp"};
static const struct BasicButtonLook basic_look_out = {
    BASIC_SKIN "btn_out_left.bmp", BASIC_SKIN "btn_out_mid.bmp", BASIC_SKIN "btn_out_right.bmp"};
#define BASIC_LABEL_HEIGHT 0xc

static struct BitmapRes *basic_skin_bitmap(const char *path)
{
    return object_as_type(res_mgr_get(g_resMgr(), ui_skin_path(path)), OBJECT_BITMAP_RESOURCE);
}

static void basic_button_frame(struct UIWindow *wnd, const struct BasicButtonLook *look)
{
    int count, i, x;

    ui_window_draw_bitmap(wnd, 0, 0, basic_skin_bitmap(look->left), 0);
    count = (wnd->width - 12) / 6 + 1;
    for (i = 1, x = 6; i <= count; i++, x += 6) {
        struct BitmapRes *mid = basic_skin_bitmap(look->mid);

        ui_window_draw_bitmap(wnd, i == count ? wnd->width - 12 : x, 0, mid, 0);
    }
    ui_window_draw_bitmap(wnd, wnd->width - 6, 0, basic_skin_bitmap(look->right), 0);
}

static void basic_button_label(struct UIBasicButton *self, COLORREF first_colour, int first_shift,
                               COLORREF second_colour)
{
    struct UIWindow *wnd = &self->button.base;
    int x = (int)((double)(wnd->width - ui_window_text_width(wnd, string_cstr(&self->label), 0, 0,
                                                            BASIC_LABEL_HEIGHT, 0, 0)) * 0.5);
    int y = (int)((double)(wnd->height - ui_window_text_height(wnd, string_cstr(&self->label), 0, 0,
                                                              BASIC_LABEL_HEIGHT, 0, 0)) * 0.5);
    int second_shift = !first_shift;

    ui_window_text_out(wnd, x + first_shift, y + first_shift, string_cstr(&self->label), 0, 0,
                     BASIC_LABEL_HEIGHT, first_colour, 0, 0);
    ui_window_text_out(wnd, x + second_shift, y + second_shift, string_cstr(&self->label), 0, 0,
                     BASIC_LABEL_HEIGHT, second_colour, 0, 0);
}

void ui_basic_button_on_draw(struct UIWindow *window)
{
    struct UIBasicButton *self = (struct UIBasicButton *)window;
    struct UIWindow *wnd = &self->button.base;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    if (self->button.disabled == 1) {
        basic_button_frame(wnd, &basic_look_disabled);
        basic_button_label(self, 0xe2e2e2, 0, 0x6f6f6f);
    } else {
        switch (wnd->state) {
        case 0:
            basic_button_frame(wnd, &basic_look_out);
            basic_button_label(self, 0xd7d7d7, 1, 0);
            break;
        case 1:
            basic_button_frame(wnd, &basic_look_over);
            basic_button_label(self, 0xc7a796, 1, 0);
            break;
        case 2:
            basic_button_frame(wnd, &basic_look_pressed);
            basic_button_label(self, 0xffffff, 0, 0);
            break;
        }
    }
    if (self->redraw_parent == 1 && wnd->parent)
        ui_window_invalidate(wnd->parent);
}
struct UINoneVisibleButton *ui_none_visible_button_construct(struct UINoneVisibleButton *self)
{
    ui_button_construct(&self->button);
    self->button.base.ops = &ui_none_visible_button_ops.base;
    return self;
}

void ui_none_visible_button_destroy(struct UIWindow *window)
{
    struct UINoneVisibleButton *self = (struct UINoneVisibleButton *)window;
    self->button.base.ops = &ui_none_visible_button_ops.base;
    ui_button_destruct(&self->button);
    mem_free(self);
}

void ui_none_visible_button_on_draw(struct UIWindow *window)
{
    struct UINoneVisibleButton *self = (struct UINoneVisibleButton *)window;
    ui_window_clear_canvas(&self->button.base, UI_TRANSPARENT);
}

struct UIBmpTextButton *ui_bmp_text_button_construct(struct UIBmpTextButton *self)
{
    ui_bitmap_button_construct(&self->bmp);
    self->bmp.button.base.ops = &ui_bmp_text_button_ops.base;
    string_init(&self->caption);
    self->styles = NULL;
    self->style_count = self->style_capacity = 0;
    return self;
}

void ui_bmp_text_button_destroy(struct UIWindow *window)
{
    struct UIBmpTextButton *self = (struct UIBmpTextButton *)window;
    self->bmp.button.base.ops = &ui_bmp_text_button_ops.base;
    free(self->styles);
    string_free(&self->caption);
    ui_bitmap_button_destruct(&self->bmp);
    mem_free(self);
}

void ui_bmp_text_button_set_caption(struct UIBmpTextButton *self, struct TextView text)
{
    string_assign(&self->caption, text.data, text.size);
    ui_window_invalidate_base(&self->bmp.button.base);
}

void ui_bitmap_button_set_tool_tip(struct UIBitmapButton *self, const char *text)
{
    ui_button_set_tool_tip(&self->button, text);
}

struct UIBitmapButton *ui_bitmap_button_construct(struct UIBitmapButton *self)
{
    int i;

    ui_button_construct(&self->button);
    self->button.base.ops = &ui_bitmap_button_ops.base;
    for (i = 0; i < 4; i++)
        string_init(&self->bitmaps[i]);
    self->bitmap_width = 0;
    self->bitmap_height = 0;
    string_clear(&self->bitmaps[UI_BMP_DISABLED]);
    self->redraw_parent = 0;
    return self;
}

BYTE ui_bitmap_button_set_bitmap(struct UIBitmapButton *self, const char *path, int state,
                                         int flag)
{
    struct BitmapRes *bitmap = res_mgr_get(g_resMgr(), ui_skin_path(path));

    if (!bitmap)
        return 0;
    if (bitmap->width > self->bitmap_width)
        self->bitmap_width = bitmap->width;
    if (bitmap->height > self->bitmap_height)
        self->bitmap_height = bitmap->height;
    if ((BYTE)flag) {
        self->bitmap_width = bitmap->width;
        self->bitmap_height = bitmap->height;
    }
    if ((unsigned)state <= UI_BMP_DISABLED)
        string_assign_cstr(&self->bitmaps[state], path);
    return 1;
}
