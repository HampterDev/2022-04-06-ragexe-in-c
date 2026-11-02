#include "ui/window_access.h"
#include "ui/controls/edit.h"
#include "ui/window_mgr.h"
#include <stdlib.h>

#define EDIT_FONT_HEIGHT 12

static int has_keyboard(struct UIEditCtrl *self)
{
    return ui_window_mgr_get_focus_edit(&g_windowMgr) == &self->base;
}

static int text_width_flag(struct UIEditCtrl *self, int font_flag, const char *text, int len)
{
    return ui_window_text_width(&self->base, text, len, 0, EDIT_FONT_HEIGHT, font_flag, 0);
}

static void select_all(struct UIEditCtrl *self)
{
    if (has_keyboard(self)) {
        language_set_selection(g_language, 0, (int)g_language->input_length);
    } else {
        self->sel_start = 0;
        self->sel_end = (int)self->text.size;
    }
}

void ui_edit_ctrl_set_text_default(struct UIEditCtrl *self, const char *text)
{
    string_assign_cstr(&self->text, text);
    ui_edit_ctrl_truncate(self);
    if (has_keyboard(self)) {
        language_cancel_input(g_language);
        language_cancel_input(g_language);
        language_replace_selection(g_language, string_cstr(&self->text));
    }
    select_all(self);
    if (self->base.parent)
        ui_window_invalidate(self->base.parent);
}

void ui_edit_ctrl_truncate_text(struct UIEditCtrl *self, int to_language)
{
    unsigned size = self->text.size;
    const char *start, *p, *keep_end;
    unsigned keep;

    if ((int)size <= self->max_chars)
        return;
    start = string_cstr(&self->text);
    p = platform_code_page_next(g_codePage, start);
    if (size - strlen(p) <= (unsigned)self->max_chars) {
        do
            p = platform_code_page_next(g_codePage, p);
        while (size - strlen(p) <= (unsigned)self->max_chars);
    }
    keep_end = platform_code_page_prev(g_codePage, string_cstr(&self->text), p);
    keep = size - strlen(keep_end);
    string_truncate(&self->text, keep);
    if (to_language) {
        language_cancel_input(g_language);
        language_cancel_input(g_language);
        language_replace_selection(g_language, string_cstr(&self->text));
    }
    while (self->text.size < (unsigned)self->max_chars)
        string_append_cstr(&self->text, " ");
}

void ui_edit_ctrl_truncate_default(struct UIEditCtrl *self)
{
    ui_edit_ctrl_truncate_text(self, 0);
}

void ui_edit_ctrl_sync_from_ime_with(struct UIEditCtrl *self, int font_flag, int right_margin)
{
    int old_caret = self->sel_end;
    int view = self->base.width - right_margin;
    BYTE clamped = 0;
    unsigned size;

    struct TextView text;

    if (!language_get_text(g_language, &text))
        return;
    string_assign(&self->text, text.data, text.size);
    language_get_selection(g_language, &self->sel_start, &self->sel_end);
    ui_edit_ctrl_truncate(self);
    size = self->text.size;
    if ((unsigned)self->sel_end > size || (unsigned)self->sel_start > size) {
        if ((unsigned)self->sel_start >= size)
            self->sel_start = (int)size;
        if ((unsigned)self->sel_end >= size)
            self->sel_end = (int)size;
        language_set_selection(g_language, self->sel_start, self->sel_end);
        clamped = 1;
    }
    if (!self->sel_end) {
        self->scroll = 0;
        return;
    }
    if ((unsigned)abs(g_language->caret - g_language->anchor) >= self->text.size)
        self->scroll = 0;

    if (self->sel_end <= old_caret && !clamped) {
        if (self->sel_end < old_caret) {
            int w = text_width_flag(self, font_flag, string_cstr(&self->text), self->sel_end);

            if (w + 3 < view && self->scroll > self->sel_end)
                self->scroll = self->scroll - old_caret + self->sel_end;
            if (self->scroll <= 0)
                self->scroll = 0;
        }
        return;
    } else {
        const char *text = string_cstr(&self->text);
        int scrolled = self->scroll ? text_width_flag(self, font_flag, text, self->scroll) : 0;
        unsigned rest, scroll_on, fits;
        const char *end, *p;

        if (text_width_flag(self, font_flag, text, self->sel_end) - scrolled + 3 <= view)
            return;
        end = text + self->text.size;
        p = platform_code_page_next(g_codePage, text);
        if (text_width_flag(self, font_flag, p, 0) + 3 > view) {
            while (p < end) {
                p = platform_code_page_next(g_codePage, p);
                if (text_width_flag(self, font_flag, p, 0) + 3 <= view)
                    break;
            }
        }
        rest = strlen(p);
        scroll_on = (unsigned)(self->sel_end - old_caret + self->scroll);
        fits = (unsigned)self->sel_end - rest;
        self->scroll = (int)(scroll_on < fits ? scroll_on : (unsigned)self->sel_end - strlen(p));
    }
}

void ui_edit_ctrl_sync_from_ime(struct UIEditCtrl *self)
{
    ui_edit_ctrl_sync_from_ime_with(self, 0, 7);
}

const char *ui_edit_ctrl_get_text(struct UIWindow *wnd)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    if (has_keyboard(self))
        ui_edit_ctrl_sync_from_ime(self);
    return string_cstr(&self->text);
}

int ui_edit_ctrl_get_text_size(struct UIWindow *wnd)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    if (has_keyboard(self))
        return (int)g_language->input_length;
    return (int)self->text.size;
}

void ui_edit_ctrl_select_all(struct UIWindow *wnd)
{
    select_all((struct UIEditCtrl *)wnd);
}

void ui_edit_ctrl_set_caret(struct UIWindow *wnd, int pos)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)wnd;

    if ((unsigned)pos > self->text.size)
        pos = (int)self->text.size;
    self->sel_end = pos;
    self->sel_start = pos;
    language_set_selection(g_language, pos, pos);
}

struct TextView ui_edit_ctrl_link_text(const struct UIEditCtrl *self)
{
    return string_view(&self->raw_text);
}

void ui_edit_ctrl_truncate_to_ime(struct UIEditCtrl *edit)
{
    ui_edit_ctrl_truncate_text(edit, 1);
}
