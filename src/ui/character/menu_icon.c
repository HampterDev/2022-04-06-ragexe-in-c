#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"

struct UIMenuIcon {
    struct UIBitmapButton bmp;
};

struct UIWindow;
void ui_menu_icon_destroy(struct UIWindow *window);
void ui_menu_icon_on_mouse_move(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_menu_icon_ops = {
    .base = {
        .destroy = ui_menu_icon_destroy,
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
        .on_mouse_move = ui_menu_icon_on_mouse_move,
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

#define MENU_ICON_ID_OPTION 0xb9

static const struct {
    int id;
    int behavior;
    int name_msg;
} menu_icons[] = {
    { 0x0b8, 0x00b, 0x069 },
    { 0x0b9, 0x166, 0x109 },
    { 0x0ba, 0x008, 0x06a },
    { 0x0bb, 0x00a, 0x068 },
    { 0x0bc, 0x025, 0x11b },
    { 0x0bf, 0x045, 0x067 },
    { 0x0d3, 0x08c, 0xb2b },
    { 0x161, 0x086, 0x525 },
    { 0x16a, 0x166, 0xc38 },
    { 0x16d, 0x03b, 0xc3e },
    { 0x1a6, 0x0cb, 0x931 },
    { 0x1c5, 0x0fb, 0xc3f },
    { 0x1d1, 0x10e, 0xa56 },
    { 0x1d4, 0x107, 0xc40 },
    { 0x1f8, 0x13e, 0xc41 },
    { 0x214, 0x148, 0xd91 },
    { 0x218, 0x144, 0xdba },
};

static void set_tooltip_with_key(struct UIMenuIcon *self, int name_msg, const char *key)
{
    string_format(&self->bmp.button.tooltip, "%s (%s)", msg_str(name_msg), key);
}

void ui_menu_icon_destroy(struct UIWindow *window)
{
    struct UIMenuIcon *self = (struct UIMenuIcon *)window;
    self->bmp.button.base.ops = &ui_menu_icon_ops.base;
    ui_bitmap_button_destruct(&self->bmp);
    mem_free(self);
}

void ui_menu_icon_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIMenuIcon *self = (struct UIMenuIcon *)window;
    char *key;
    int name_msg = 0xf91;
    int i;

    (void)x;
    (void)y;
    for (i = 0; i < (int)(sizeof menu_icons / sizeof menu_icons[0]); i++) {
        if (menu_icons[i].id != self->bmp.button.base.id)
            continue;
        name_msg = menu_icons[i].name_msg;
        key = NULL;
        client_lua_call(g_session1.luaState, "GetBeHaviorOfWindow", "d>s", menu_icons[i].behavior, &key);
        if (strlen(key) != 0) {
            set_tooltip_with_key(self, name_msg, key);
        } else {
            const char *name = msg_str(name_msg);
            string_assign_cstr(&self->bmp.button.tooltip, name);
        }
        break;
    }
    if (self->bmp.button.base.id == MENU_ICON_ID_OPTION)
        set_tooltip_with_key(self, name_msg, "Esc");
}
