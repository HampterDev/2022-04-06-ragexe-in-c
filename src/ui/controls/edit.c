#include "ui/controls/edit.h"
#include "ui/window_keyboard.h"
#include "ui/window_access.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include <stdlib.h>
#include "diag/perf.h"
#include "ui/draw_key.h"
#include "platform/clock.h"

struct UIEditCtrl;
struct UIWindow;
const struct UIEditCtrlOps ui_edit_ctrl_ops = {
    .base = {
        .destroy = ui_edit_ctrl_destroy,
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

#define EDIT_FONT_HEIGHT 12
#define EDIT_MARGIN 3
#define CARET_BLINK_MS 500

static int text_width(struct UIEditCtrl *self, const char *text, int len)
{
    return ui_window_text_width(&self->base, text, len, 0, EDIT_FONT_HEIGHT, 0, 0);
}

static int has_keyboard(struct UIEditCtrl *self)
{
    return ui_window_mgr_get_focus_edit(&g_windowMgr) == &self->base;
}

struct UIWindow *ui_edit_ctrl_construct(struct UIWindow *wnd)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_edit_ctrl_ops.base;
    self->border_r = 0;
    self->border_g = 0;
    self->border_b = 0;
    string_init(&self->tooltip);
    self->disabled = 0;
    string_init(&self->text);
    self->keep_ime_on_enter = 0;
    self->navigation_link = NULL;
    string_init(&self->raw_text);
    self->max_chars = 0xff;
    string_reserve(&self->text, 0xff);
    self->sel_end = 0;
    self->sel_start = 0;
    self->mask_char = 0;
    self->no_border = 1;
    self->back_r = 0xff;
    self->back_g = 0xff;
    self->back_b = 0xff;
    self->text_colour = 0;
    self->double_colour = -1;
    self->scroll = 0;
    self->text_y = 0;
    self->multiline = 0;
    self->text_top = 0;
    return wnd;
}

void ui_edit_ctrl_destruct(struct UIEditCtrl *self)
{
    self->base.ops = &ui_edit_ctrl_ops.base;
    string_free(&self->raw_text);
    self->navigation_link = NULL;
    string_free(&self->text);
    string_free(&self->tooltip);
    ui_window_destruct(&self->base);
}

void ui_edit_ctrl_destroy(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    ui_edit_ctrl_destruct(self);
    mem_free(self);
}

void ui_edit_ctrl_on_draw(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    int r = self->back_r, g = self->back_g, b = self->back_b;
    RECT rect;

    if (self->disabled) {
        if (r > 0x1e)
            r -= 0x1e;
        if (g > 0x1e)
            g -= 0x1e;
        if (b > 0x1e)
            b -= 0x1e;
    }
    self->text_y = 0;
    if (!self->no_border) {
        rect.left = 0;
        rect.top = 0;
        rect.right = self->base.width;
        rect.bottom = self->base.height;
        ui_window_fill_canvas(&self->base, RGB(self->border_r, self->border_g, self->border_b), &rect);
        rect = (RECT){1, 1, self->base.width - 1, self->base.height - 1};
        ui_window_fill_canvas(&self->base, RGB(r, g, b), &rect);
    } else {
        ui_window_clear_canvas(&self->base, RGB(r, g, b));
    }
    self->text_y = self->text_top + 2;
    ui_edit_ctrl_draw_text(self);
}

void ui_edit_ctrl_on_focus(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    language_cancel_input(g_language);
    language_cancel_input(g_language);
    language_replace_selection(g_language, string_cstr(&self->text));
    language_set_masked(g_language, self->mask_char != 0);
    ui_edit_ctrl_select_all(&self->base);
}

void ui_edit_ctrl_on_focus_lost(struct UIWindow *window)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    ui_edit_ctrl_sync_from_ime(self);
    ui_window_invalidate_base(&self->base);
}

void ui_edit_ctrl_on_l_btn_down(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    if (!self->disabled)
        ui_window_mgr_set_focus_edit(&g_windowMgr, &self->base);
}

void ui_edit_ctrl_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    int tip_x, tip_y;

    if (self->disabled)
        return;
    ui_window_on_mouse_move_base(&self->base, x, y);
    if (!self->tooltip.size)
        return;
    tip_x = 10;
    tip_y = -20;
    ui_window_to_screen(&self->base, &tip_x, &tip_y);
    mode_show_tooltip(client_current_mode(), string_cstr(&self->tooltip), tip_x, tip_y, -1, 0);
}

static void draw_string(struct UIEditCtrl *self, int x, const struct String *shown)
{
    if (self->double_colour == -1)
        ui_window_text_out(&self->base, x, self->text_y, string_cstr(shown), 0, 0, EDIT_FONT_HEIGHT,
                         self->text_colour, 0, 0);
    else
        ui_window_text_out_doubled(&self->base, x, self->text_y, string_cstr(shown), self->text_colour,
                                self->double_colour, 0, 0, EDIT_FONT_HEIGHT);
}

static void draw_string_alt(struct UIEditCtrl *self, int x, const struct String *shown,
                            const struct UIEditCtrl4 *alt)
{
    const char *text = string_cstr(shown);
    COLORREF colour = self->text_colour;

    if (self->double_colour == -1 && alt->use_alt == 1) {
        if (!strcmp(text, ""))
            text = string_cstr(&alt->empty_text);
        colour = alt->alt_colour;
    }
    if (self->double_colour == -1)
        ui_window_text_out(&self->base, x, self->text_y, text, 0, 0, EDIT_FONT_HEIGHT, colour, 0, 0);
    else
        draw_string(self, x, shown);
}

static void draw_text(struct UIEditCtrl *self, const struct UIEditCtrl4 *alt)
{
    struct UIWindow *wnd = &self->base;
    struct String shown;
    int x = EDIT_MARGIN;
    int lo, hi, lo_x, hi_x;
    int plain = 1;

    if (!self->multiline && self->scroll)
        x = EDIT_MARGIN - text_width(self, string_cstr(&self->text), self->scroll);
    string_init(&shown);

    if (!has_keyboard(self)) {
        if (self->mask_char)
            string_resize(&shown, self->text.size, (char)self->mask_char);
        else
            string_assign(&shown, string_cstr(&self->text), self->text.size);
        if (alt)
            draw_string_alt(self, x, &shown, alt);
        else
            draw_string(self, x, &shown);
    } else {
        ui_edit_ctrl_sync_from_ime(self);
        if (self->mask_char) {
            string_resize(&shown, self->text.size, (char)self->mask_char);
            lo = min(self->sel_start, self->sel_end);
            hi = max(self->sel_start, self->sel_end);
        } else {
            const char *tag = language_charset_tag(g_language, 0);

            string_assign_cstr(&shown, tag);
            string_append(&shown, string_cstr(&self->text), self->text.size);
            lo = min(self->sel_start, self->sel_end) + (int)strlen(tag);
            hi = max(self->sel_start, self->sel_end) + (int)strlen(tag) + (int)g_language->composition_length;
        }
        lo_x = lo ? text_width(self, string_cstr(&shown), lo) : 0;
        hi_x = hi ? text_width(self, string_cstr(&shown), hi) : 0;

        if (abs(g_language->caret - g_language->anchor)) {
            ui_edit_ctrl_draw_highlight(self, lo_x + x, hi_x - lo_x);
            draw_string(self, x, &shown);
            plain = 0;
        } else if (g_language->composition_length) {
            ui_edit_ctrl_place_candidate_wnd(self, lo_x);
            ui_edit_ctrl_place_key_stroke_wnd(self, hi_x);
            if (!((platform_ticks_ms() / CARET_BLINK_MS) & 1))
                ui_edit_ctrl_draw_highlight(self, lo_x + x, max(hi_x - lo_x, 2));
            draw_string(self, x, &shown);
            plain = 0;
        } else {
            draw_string(self, x, &shown);
            if (!((platform_ticks_ms() / CARET_BLINK_MS) & 1) && ui_edit_ctrl_caret_shown())
                ui_window_text_out(wnd, lo_x + x - 2, self->text_y, "|", 0, 0, EDIT_FONT_HEIGHT,
                                 self->text_colour, 0, 0);
        }
    }
    if (plain && self->multiline && wnd->parent)
        ui_window_invalidate(wnd->parent);
    ui_edit_ctrl_layout_navi_links(self);
    string_free(&shown);
}

void ui_edit_ctrl_draw_text_default(struct UIEditCtrl *self)
{
    draw_text(self, NULL);
}

void ui_edit_ctrl4_draw_text(struct UIEditCtrl *base_edit)
{
    struct UIEditCtrl4 *self = (struct UIEditCtrl4 *)base_edit;
    draw_text(&self->edit, self);
}

void ui_edit_ctrl_set_back_colour(struct UIWindow *wnd, int red, int green, int blue)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    self->back_r = red;
    self->back_g = green;
    self->back_b = blue;
    ui_window_invalidate_base(&self->base);
}

void ui_edit_ctrl_set_border_colour(struct UIWindow *wnd, int red, int green, int blue)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    self->border_r = red;
    self->border_g = green;
    self->border_b = blue;
}

void ui_edit_ctrl_set_text_colour(struct UIWindow *wnd, int red, int green, int blue,
                                         int double_colour)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    self->double_colour = double_colour;
    self->text_colour = RGB((BYTE)red, (BYTE)green, (BYTE)blue);
    ui_window_invalidate_base(&self->base);
}

void ui_edit_ctrl_draw_highlight(struct UIEditCtrl *self, int x, int width)
{
    int right = self->base.width;

    if (x < EDIT_MARGIN) {
        width += x - EDIT_MARGIN;
        x = EDIT_MARGIN;
    }
    if (x >= right || width <= 0)
        return;
    ui_window_fill_rect(&self->base, x, 0, x + width > right ? right - x : width, self->base.height,
                      ui_skin_colour(6, 2));
}

static void place_ime_window(struct UIEditCtrl *self, struct UIWindow *ime_wnd, int x)
{
    int sx = x, sy = self->base.height + 6;

    ui_window_to_screen(&self->base, &sx, &sy);
    if (ime_wnd->width + sx > g_renderer->width)
        sx = g_renderer->width - ime_wnd->width;
    if (sy + ime_wnd->height > g_renderer->height)
        sy += -12 - self->base.height - ime_wnd->height;
    ui_window_move(ime_wnd, sx, sy);
}

void ui_edit_ctrl_place_candidate_wnd(struct UIEditCtrl *self, int x)
{
    if (g_windowMgr.candidate_wnd)
        place_ime_window(self, g_windowMgr.candidate_wnd, x);
}

void ui_edit_ctrl_place_key_stroke_wnd(struct UIEditCtrl *self, int x)
{
    if (g_windowMgr.key_stroke_wnd)
        place_ime_window(self, g_windowMgr.key_stroke_wnd, x);
}

BYTE ui_edit_ctrl_caret_shown(void)
{
    if (ui_window_mgr_is_chat_edit_focused(&g_windowMgr) && !g_windowMgr.chat_bar_shown)
        return 0;
    return 1;
}

unsigned long long ui_edit_ctrl_focus_key(void)
{
    struct Language *language = g_language;
    unsigned long long key = DRAW_KEY_START;

    key = draw_key_int(key, (int)language->input_length);
    key = draw_key_bytes(key, language->input, language->input_length);
    key = draw_key_int(key, language->caret);
    key = draw_key_int(key, language->anchor);
    key = draw_key_int(key, (int)language->reading_length);
    key = draw_key_bytes(key, language->reading, language->reading_length);
    key = draw_key_int(key, (int)language->composition_length);
    key = draw_key_bytes(key, language->composition, language->composition_length);
    key = draw_key_int(key, language->composition_cursor);
    key = draw_key_int(key, language->candidate_count);
    key = draw_key_int(key, language->candidate_page);
    key = draw_key_int(key, language->candidate_selection);
    key = draw_key_int(key, language->masked);
    key = draw_key_text(key, language_charset_tag(language, 0));
    key = draw_key_int(key, (int)((platform_ticks_ms() / CARET_BLINK_MS) & 1));
    key = draw_key_int(key, g_windowMgr.chat_bar_shown);
    return draw_key_int(key, ui_edit_ctrl_caret_shown());
}

void ui_edit_ctrl_make_select(struct UIWindow *wnd)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    if (has_keyboard(self)) {
        int end = (int)g_language->input_length;

        language_set_selection(g_language, end, end);
    } else {
        self->sel_start = (int)self->text.size;
        self->sel_end = (int)self->text.size;
    }
}
