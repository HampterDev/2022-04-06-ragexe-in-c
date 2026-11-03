#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include <mmsystem.h>
#include <stdlib.h>
#include "ui/ui_std.h"
#include "diag/perf.h"

struct UIEditCtrlCustom;
struct UIWindow;
const struct UIEditCtrlOps ui_edit_ctrl_custom_ops = {
    .base = {
        .destroy = ui_edit_ctrl_custom_destroy,
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
    .draw_text = ui_edit_ctrl_custom_draw_text,
    .truncate = ui_edit_ctrl_truncate_to_ime,
};

struct UIEditCtrlCurrency;
void ui_edit_ctrl_currency_on_process(struct UIWindow *self);
const struct UIEditCtrlOps ui_edit_ctrl_currency_ops = {
    .base = {
        .destroy = ui_edit_ctrl_currency_destroy,
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
        .on_process = ui_edit_ctrl_currency_on_process,
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
        .send_msg = ui_edit_ctrl_currency_send_msg,
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
    .draw_text = ui_edit_ctrl_custom_draw_text,
    .truncate = ui_edit_ctrl_truncate_to_ime,
};

#define EDIT_FONT_HEIGHT 12
#define EDIT_MARGIN 3
#define CARET_BLINK_MS 500
#define UI_MSG_KEY 0x100

#include "render/textdc.h"

static int text_width(struct UIEditCtrlCustom *self, const char *text, int len, int font_flag)
{
    return ui_window_text_width(&self->edit.base, text, len, 0, EDIT_FONT_HEIGHT, font_flag, 0);
}

struct UIWindow *ui_edit_ctrl_custom_construct(struct UIWindow *wnd)
{
    struct UIEditCtrlCustom *self = (struct UIEditCtrlCustom *)wnd;

    ui_edit_ctrl2_construct(wnd);
    wnd->ops = &ui_edit_ctrl_custom_ops.base;
    self->font_flag = 0;
    self->offset_x = 0;
    self->offset_y = 0;
    self->align = UI_EDIT_ALIGN_LEFT;
    return wnd;
}

void ui_edit_ctrl_custom_destruct(struct UIEditCtrlCustom *self)
{
    self->edit.base.ops = &ui_edit_ctrl_custom_ops.base;
    ui_edit_ctrl2_destruct(&self->edit);
}

void ui_edit_ctrl_custom_destroy(struct UIWindow *window)
{
    struct UIEditCtrlCustom *self = (struct UIEditCtrlCustom *)window;
    ui_edit_ctrl_custom_destruct(self);
    mem_free(self);
}

static int aligned_x(struct UIEditCtrlCustom *self, const struct String *shown)
{
    int x = self->offset_x + EDIT_MARGIN;

    if (self->align == UI_EDIT_ALIGN_RIGHT)
        return self->edit.base.width - x - text_width(self, string_cstr(shown), 0, self->font_flag);
    if (self->align == UI_EDIT_ALIGN_CENTRE)
        return (int)((float)self->edit.base.width * 0.5f -
                     (float)text_width(self, string_cstr(shown), 0, self->font_flag) * 0.5f);
    return x;
}

static void sync_from_ime(struct UIEditCtrlCustom *self)
{
    ui_edit_ctrl_sync_from_ime_with(&self->edit, self->font_flag, 12);
}

void ui_edit_ctrl_custom_draw_text(struct UIEditCtrl *base_edit)
{
    struct UIEditCtrlCustom *self = (struct UIEditCtrlCustom *)base_edit;
    struct UIEditCtrl *edit = &self->edit;
    struct UIWindow *wnd = &edit->base;
    int y = self->offset_y + edit->text_y;
    struct String shown;
    int x, lo, hi, lo_x, hi_x;

    string_init(&shown);
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) != wnd) {
        if (edit->mask_char)
            string_resize(&shown, edit->text.size, (char)edit->mask_char);
        else
            string_assign(&shown, string_cstr(&edit->text), edit->text.size);
        x = aligned_x(self, &shown);
        ui_edit_ctrl_custom_draw_string(self, x, y, string_cstr(&shown));
    } else {
        sync_from_ime(self);
        if (edit->mask_char) {
            string_resize(&shown, edit->text.size, (char)edit->mask_char);
            lo = min(edit->sel_start, edit->sel_end);
            hi = max(edit->sel_start, edit->sel_end);
        } else {
            const char *tag = language_charset_tag(g_language, 0);

            string_assign_cstr(&shown, tag);
            string_append(&shown, string_cstr(&edit->text), edit->text.size);
            lo = min(edit->sel_start, edit->sel_end) + (int)strlen(tag);
            hi = max(edit->sel_start, edit->sel_end) + (int)strlen(tag) + (int)g_language->composition_length;
        }
        x = aligned_x(self, &shown);
        lo_x = lo ? text_width(self, string_cstr(&shown), lo, 0) : 0;
        hi_x = hi ? text_width(self, string_cstr(&shown), hi, 0) : 0;
        if (abs(g_language->caret - g_language->anchor)) {
            ui_edit_ctrl_draw_highlight(edit, lo_x + x, hi_x - lo_x);
            ui_edit_ctrl_custom_draw_string(self, x, y, string_cstr(&shown));
        } else if (g_language->composition_length) {
            ui_edit_ctrl_place_candidate_wnd(edit, lo_x);
            if (!((timeGetTime() / CARET_BLINK_MS) & 1))
                ui_edit_ctrl_draw_highlight(edit, lo_x + x, max(hi_x - lo_x, 2));
            ui_edit_ctrl_custom_draw_string(self, x, y, string_cstr(&shown));
        } else {
            ui_edit_ctrl_custom_draw_string(self, x, y, string_cstr(&shown));
            if (ui_edit_ctrl_caret_shown() == 1 && !((timeGetTime() / CARET_BLINK_MS) & 1))
                ui_window_text_out(wnd, lo_x + x - 2, y, "|", 0, 0, EDIT_FONT_HEIGHT, edit->text_colour, 0, 0);
        }
    }
    if (edit->multiline && wnd->parent)
        ui_window_invalidate(wnd->parent);
    string_free(&shown);
}

struct UIEditCtrlCurrency *ui_edit_ctrl_currency_construct(struct UIEditCtrlCurrency *self)
{
    ui_edit_ctrl_custom_construct(&self->custom.edit.base);
    self->custom.edit.base.ops = &ui_edit_ctrl_currency_ops.base;
    self->custom.align = UI_EDIT_ALIGN_RIGHT;
    self->formatted_size = 0;
    return self;
}

void ui_edit_ctrl_currency_destroy(struct UIWindow *window)
{
    struct UIEditCtrlCurrency *self = (struct UIEditCtrlCurrency *)window;
    ui_edit_ctrl_custom_destruct(&self->custom);
    mem_free(self);
}

int ui_edit_ctrl_currency_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                          int val2, int val3, int val4)
{
    struct UIEditCtrlCurrency *self = (struct UIEditCtrlCurrency *)window;
    struct UIEditCtrl *edit = &self->custom.edit;
    int anchor, caret;

    if (msg == UI_MSG_KEY && (val1 == VK_BACK || val1 == VK_DELETE)) {
        language_get_selection(g_language, &anchor, &caret);
        if (anchor == caret) {
            const char *text = string_cstr(&edit->text);

            if (val1 == VK_BACK ? caret <= 2 : caret == 0) {
                if (text[1] == ',')
                    language_set_selection(g_language, 0, 2);
            } else if (val1 == VK_BACK) {
                if (text[caret - 1] == ',')
                    language_set_selection(g_language, caret - 1, caret - 1);
            } else if (text[caret] == ',') {
                language_set_selection(g_language, caret + 1, caret + 1);
            }
        }
    }
    return ui_edit_ctrl2_send_msg((struct UIWindow *)(edit), sender, msg, val1, val2, val3, val4);
}

void ui_edit_ctrl_currency_on_process(struct UIWindow *window)
{
    struct UIEditCtrlCurrency *self = (struct UIEditCtrlCurrency *)window;
    struct String *text = &self->custom.edit.text;
    struct String digits;
    char buf[0x104];
    int anchor, caret, commas_before = 0, len, shift;
    size_t i = 0;
    const char *p;

    if (self->formatted_size == (int)text->size)
        return;
    language_get_selection(g_language, &anchor, &caret);
    string_copy(&digits, text);
    p = string_cstr(&digits);
    while (p != string_cstr(&digits) + digits.size) {
        if (string_cstr(text)[i] == ',' && anchor > (int)i)
            commas_before++;
        if (*p < '0' || *p > '9') {
            size_t at = (size_t)(p - string_cstr(&digits));

            string_erase(&digits, at, 1);
            p = string_cstr(&digits) + at;
            if (p == string_cstr(&digits) + digits.size)
                break;
        }
        i++;
        p++;
    }
    memset(buf, 0, sizeof buf);
    format_money64(_atoi64(string_cstr(&digits)), buf, sizeof buf);
    string_assign_cstr(text, buf);
    ui_edit_ctrl_truncate_to_ime(&self->custom.edit);
    language_cancel_input(g_language);
    language_cancel_input(g_language);
    language_replace_selection(g_language, string_cstr(text));
    len = (int)self->custom.edit.text.size;
    shift = len / 4 - (len - anchor) / 4 - commas_before;
    anchor += shift;
    shift = len / 4 - (len - caret) / 4 - commas_before;
    caret += shift;
    if (!digits.size) {
        caret = 1;
        anchor = 1;
    }
    language_set_selection(g_language, anchor, caret);
    self->formatted_size = (int)self->custom.edit.text.size;
    ui_window_invalidate_base(&self->custom.edit.base);
    string_free(&digits);
}
