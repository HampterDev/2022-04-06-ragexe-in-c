#include "ui/controls/combo_box.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "diag/perf.h"

struct UINewComboBoxCtrl {
    struct UIWindow base;
    struct UIBitmapButton *button;
    struct UIWindow *text;
    BYTE no_border;
    BYTE open;
    int back_r, back_g, back_b;
    int border_r, border_g, border_b;
    int selected;
    struct Vec lines;
};
const struct UIWindowOps ui_new_combo_box_ctrl_ops = {
    .destroy = ui_new_combo_box_ctrl_destroy,
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
    .on_create = ui_new_combo_box_ctrl_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_new_combo_box_ctrl_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_new_combo_box_ctrl_on_l_btn_dbl_clk,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_new_combo_box_ctrl_send_msg,
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

#define NEW_COMBO_BUTTON 0x109
#define LINE_HEIGHT 16
#define BUTTON_BMP UI_SKIN "basic_interface\\txtbox_btn_"
#define NEW_COMBO_MSG_DBLCLK 0x7c

#define LINE(self, i) ((struct String *)(self)->lines.first + (i))
#define RGB_OF(r, g, b) ((COLORREF)(BYTE)(r) | (COLORREF)(BYTE)(g) << 8 | (COLORREF)(BYTE)(b) << 16)

void ui_new_combo_box_ctrl_destruct(struct UINewComboBoxCtrl *self)
{
    self->base.ops = &ui_new_combo_box_ctrl_ops;
    string_vec_free(&self->lines);
    ui_window_destruct(&self->base);
}

void ui_new_combo_box_ctrl_destroy(struct UIWindow *window)
{
    struct UINewComboBoxCtrl *self = (struct UINewComboBoxCtrl *)window;
    ui_new_combo_box_ctrl_destruct(self);
    mem_free(self);
}

void ui_new_combo_box_ctrl_on_create(struct UIWindow *window, int width, int height)
{
    struct UINewComboBoxCtrl *self = (struct UINewComboBoxCtrl *)window;
    static const char *const looks[3] = { "a.bmp", "b.bmp", "c.bmp" };
    struct String dir;
    struct UIBitmapButton *button;
    struct UIWindow *text;
    int state;

    string_init(&dir);
    string_assign(&dir, BUTTON_BMP, 0x2a);
    button = mem_alloc(sizeof *button);
    self->button = button ? ui_bitmap_button_construct(button) : NULL;
    for (state = 0; state < 3; state++) {
        struct String path;

        string_init(&path);
        string_reserve(&path, dir.size + 5);
        string_append(&path, string_cstr(&dir), dir.size);
        string_append(&path, looks[state], 5);
        ui_bitmap_button_set_bitmap(self->button, string_cstr(&path), state, 0);
        string_free(&path);
    }
    ui_window_create(&self->button->button.base, self->button->bitmap_width, self->button->bitmap_height);
    ui_window_move_base(&self->button->button.base, self->base.width - self->button->bitmap_width, 0);
    self->button->button.base.id = NEW_COMBO_BUTTON;
    ui_window_add_child(&self->base, &self->button->button.base);

    text = mem_alloc(sizeof(struct UIStaticText));
    text = text ? ui_static_text_construct(text) : NULL;
    self->text = text;
    ui_window_create(text, self->base.width - self->button->bitmap_width, self->button->bitmap_height);
    ui_static_text_set_text(self->text, "(No Select)", 0);
    ui_window_move(self->text, 2, 3);
    ((struct UIStaticText *)self->text)->white_shadow = 1;
    ui_window_add_child(&self->base, self->text);
    string_free(&dir);
}

void ui_new_combo_box_ctrl_on_draw(struct UIWindow *window)
{
    struct UINewComboBoxCtrl *self = (struct UINewComboBoxCtrl *)window;
    ui_window_fill_rect(&self->base, 0, 0, self->base.width, self->base.height,
                      RGB_OF(self->back_r, self->back_g, self->back_b));
    if (!self->no_border)
        ui_window_draw_outline(&self->base, 0, 0, self->base.width, self->base.height,
                   RGB_OF(self->border_r, self->border_g, self->border_b));
}

void ui_new_combo_box_ctrl_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UINewComboBoxCtrl *self = (struct UINewComboBoxCtrl *)window;
    if (self->base.parent)
        ui_window_send_msg(self->base.parent, &self->base, NEW_COMBO_MSG_DBLCLK, self->selected, 0, 0, 0);
}

int ui_new_combo_box_ctrl_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UINewComboBoxCtrl *self = (struct UINewComboBoxCtrl *)window;
    if (msg == UI_MSG_COMMAND) {
        struct UIWindow *list;
        int n, i, x, y;

        if (val1 != NEW_COMBO_BUTTON)
            return 0;
        self->open = !self->open;
        if (g_windowMgr.combo_box_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_COMBO_BOX);
        if (!self->open)
            return 0;
        list = ui_window_open(UI_WND_COMBO_BOX);
        n = (int)VEC_COUNT(&self->lines, struct String);
        for (i = 0; i < n; i++)
            ui_window_send_msg(list, NULL, COMBO_MSG_ADD_LINE, (int)string_cstr(LINE(self, i)), 0, 0, 0);
        ui_window_send_msg(list, NULL, COMBO_MSG_SELECT, self->selected, 0, 0, 0);
        ui_window_resize(list, self->base.width, n * LINE_HEIGHT);
        ui_combo_box_wnd_set_chosen_to((struct UIComboBoxWnd *)list, &self->base);
        x = 0;
        y = self->base.height;
        ui_window_to_screen(&self->base, &x, &y);
        ui_window_move(list, x, y);
        return 0;
    }
    if (msg == COMBO_MSG_CHOSEN) {
        int i = val1;

        self->open = 0;
        if (i < 0)
            return 0;
        if ((unsigned)i <= VEC_COUNT(&self->lines, struct String))
            ui_static_text_set_text(self->text, string_cstr(LINE(self, i)), 0);
        self->selected = i;
        ui_window_send_msg(self->base.parent, &self->base, COMBO_MSG_CHOSEN, i, 0, 0, 0);
        ui_window_invalidate_base(&self->base);
        ui_window_invalidate(self->base.parent);
        if (ui_window_get_ancestor(&self->base))
            ui_window_invalidate(ui_window_get_ancestor(&self->base));
        return 0;
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}
