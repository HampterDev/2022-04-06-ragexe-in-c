#include "ui/controls/buttons.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/ui_std.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include <string.h>
#include <stdlib.h>
#include "ui/controls/controls.h"
#include "diag/perf.h"

struct UIWindow;

struct UITextButton;
const struct UIButtonOps ui_text_button_ops = {
    .base = {
        .destroy = ui_text_button_destroy,
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
        .on_draw = ui_text_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_text_button_on_mouse_move,
        .on_mouse_hover = ui_text_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_text_button_on_l_btn_up,
        .on_r_btn_down = ui_text_button_on_r_btn_down,
        .on_r_btn_up = ui_text_button_on_r_btn_up,
        .on_wheel = ui_text_button_on_wheel,
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

struct UITextButton *ui_text_button_construct(struct UITextButton *self)
{
    ui_button_construct(&self->button);
    self->button.base.ops = &ui_text_button_ops.base;
    string_init(&self->label);
    text_array_init(&self->lines);
    self->value = 0;
    self->outline_colour = 0xffffff;
    return self;
}

struct UITextButton *ui_text_button_create(void)
{
    struct UITextButton *self = malloc(sizeof *self);
    if (!self) abort();
    return ui_text_button_construct(self);
}

void ui_text_button_destruct(struct UITextButton *self)
{
    self->button.base.ops = &ui_text_button_ops.base;
    text_array_clear(&self->lines);
    string_free(&self->label);
    ui_button_destruct(&self->button);
}

void ui_text_button_destroy(struct UIWindow *window)
{
    struct UITextButton *self = (struct UITextButton *)window;
    ui_text_button_destruct(self);
    free(self);
}

void ui_text_button_on_draw(struct UIWindow *window)
{
    struct UITextButton *self = (struct UITextButton *)window;
    struct UIWindow *wnd = &self->button.base;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (wnd->parent)
        ui_window_invalidate(wnd->parent);
    if (self->outlined)
        ui_window_text_out_outlined(wnd, 1, 1, string_cstr(&self->label), 0, self->text_colour,
                                 self->outline_colour, self->font_face, self->font_height, 1, 0);
    else
        ui_window_text_out(wnd, 1, 1, string_cstr(&self->label), 0, self->font_face, self->font_height,
                         self->text_colour, 0, 0);
    if (self->underlined)
        ui_window_fill_rect(wnd, 0, wnd->height - 1, wnd->width, 1, self->text_colour);
}

static void text_button_show(struct UITextButton *self, DWORD colour, int state)
{
    self->text_colour = colour;
    if (self->button.base.state != state) {
        self->button.base.state = state;
        ui_window_invalidate_base(&self->button.base);
    }
}

void ui_text_button_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UITextButton *self = (struct UITextButton *)window;
    struct UIWindow *wnd = &self->button.base;
    int inside;

    if (self->button.disabled)
        return;
    inside = x >= 0 && x < wnd->width && y >= 0 && y < wnd->height;
    if (ui_window_mgr_get_capture(&g_windowMgr) == wnd && inside)
        text_button_show(self, self->hover_colour, UI_BTN_DOWN);
    else if (inside)
        text_button_show(self, self->hover_colour, UI_BTN_HOVER);
    else
        text_button_show(self, self->normal_colour, UI_BTN_UP);
}

void ui_text_button_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UITextButton *self = (struct UITextButton *)window;
    struct UIWindow *wnd = &self->button.base;

    if (wnd->parent)
        ui_window_on_mouse_hover(wnd->parent, wnd->x + x, wnd->y + y);
    ui_button_on_mouse_hover((struct UIWindow *)(&self->button), x, y);
}

static void text_button_release(struct UITextButton *self, int x, int y, int msg)
{
    struct UIWindow *wnd = &self->button.base;

    if (self->button.disabled || ui_window_mgr_get_capture(&g_windowMgr) != wnd)
        return;
    if (x >= 0 && x < wnd->width && y >= 0 && y < wnd->height) {
        wnd->state = UI_BTN_HOVER;
        if (wnd->parent)
            ui_window_send_msg(wnd->parent, wnd, msg, wnd->id, self->value, 0, 0);
    } else {
        wnd->state = UI_BTN_UP;
    }
    ui_window_invalidate_base(wnd);
    ui_window_mgr_release_capture(&g_windowMgr);
}

void ui_text_button_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UITextButton *self = (struct UITextButton *)window;
    text_button_release(self, x, y, UI_MSG_TEXT_BUTTON);
}

void ui_text_button_on_r_btn_down(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UITextButton *self = (struct UITextButton *)window;
    struct UIWindow *wnd = &self->button.base;

    if (self->button.disabled)
        return;
    if (wnd->parent)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_TEXT_BUTTON_RCLICK, wnd->id, self->value, 0, 0);
    ui_window_invalidate_base(wnd);
}

void ui_text_button_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UITextButton *self = (struct UITextButton *)window;
    text_button_release(self, x, y, UI_MSG_TEXT_BUTTON_RCLICK);
}

void ui_text_button_on_wheel(struct UIWindow *window, int delta)
{
    struct UITextButton *self = (struct UITextButton *)window;
    if (self->button.base.parent)
        ui_window_on_wheel(self->button.base.parent, delta);
}

void ui_text_button_set_text(struct UIWindow *wnd, const char *text, DWORD colour,
                          int font_face, int font_height, BYTE outlined, BYTE underlined)
{
    struct UITextButton *self = (struct UITextButton *)wnd;
    struct String copy = {0};
    size_t length = strlen(text);
    int width, height;

    string_assign(&copy, text, length);
    string_assign(&self->button.text, string_cstr(&copy), length);
    string_assign(&self->label, string_cstr(&copy), copy.size);
    string_free(&copy);
    self->normal_colour = self->hover_colour = self->text_colour = colour;
    self->font_face = font_face;
    self->font_height = font_height;
    self->outlined = outlined;
    self->underlined = underlined;
    width = (int)((unsigned)(length * font_height) >> 1) + outlined * 4 + 12;
    height = font_height + 2;
    ui_window_resize_base(wnd, width, height);
    ui_window_on_create(wnd, width, height);
}

void ui_text_button_fit_width(struct UITextButton *self, int max_width, int height)
{
    struct UIWindow *wnd = &self->button.base;
    struct String *text = &self->button.text;
    const char *cut;

    if (!strcmp(string_cstr(text), ""))
        return;
    string_assign(&self->label, string_cstr(text), text->size);
    ui_window_resize_base(wnd,
                    ui_window_text_width(wnd, string_cstr(text), 0, self->font_face, self->font_height,
                                       self->outlined, 0) + 2,
                    self->font_height + 2);
    if (wnd->width > max_width) {
        struct String shortened = {0};
        cut = ui_window_shorten_text(wnd, &shortened, string_cstr(text), max_width, self->font_face, self->font_height,
                                   self->outlined);
        string_assign_cstr(&self->label, cut);
        string_free(&shortened);
        ui_window_resize_base(wnd, max_width, height);
    }
}

struct TextView ui_text_button_get_line(const struct UITextButton *self, size_t i)
{
    return text_array_at(&self->lines, i);
}

void ui_text_button_add_line(struct UITextButton *self, struct TextView text)
{
    if (!text_array_append(&self->lines, text)) abort();
}

size_t ui_text_button_line_count(const struct UITextButton *self)
{
    return self->lines.size;
}

void ui_text_button_fit_to_caption(struct UITextButton *self)
{
    struct String *caption = &self->button.text;
    int width;

    if (strcmp(string_cstr(caption), "") == 0)
        return;
    string_assign(&self->label, string_cstr(caption), caption->size);
    width = ui_window_text_width(&self->button.base, string_cstr(caption), 0, self->font_face,
                               self->font_height, self->outlined, 0);
    ui_window_resize_base(&self->button.base, width + 2, self->font_height + 2);
}
