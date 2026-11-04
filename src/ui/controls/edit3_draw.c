#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "platform/language.h"
#include "core/utf8.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include <stdlib.h>
#include <string.h>
#include "platform/clock.h"
#include "platform/window.h"
#include "core/numeric.h"

#define EDIT3_FONT_HEIGHT 0xc
#define EDIT3_MARGIN 3
#define CARET_BLINK_MS 500
#define HIGHLIGHT_HEIGHT 0xe
#define ID_RODEX_CONTENTS_BODY 0x1f0
#define ID_RODEX_COMPOSE_BODY 0x1e3
#define EDIT3_MSG_LINE_UP 0x12
#define EDIT3_MSG_LINE_DOWN 0x13

static unsigned line_count(const struct UIEditCtrl3 *self)
{
    return self->shown_lines.size;
}

static const char *line_at(const struct UIEditCtrl3 *self, int i)
{
    return text_array_at(&self->shown_lines, i).data;
}

static int line_len(const struct UIEditCtrl3 *self, int i)
{
    return (int)strlen(line_at(self, i));
}

static int chars_before(const struct UIEditCtrl3 *self, int line)
{
    int sum = 0, i;

    for (i = 0; i < line; i++)
        sum += line_len(self, i);
    return sum;
}

static int ends_in_break(const struct UIEditCtrl3 *self, unsigned line)
{
    return byte_array_at(&self->line_flags, line);
}

static int text_width(struct UIEditCtrl3 *self, const char *text, int len)
{
    return ui_window_text_width(&self->edit.base, text, len, 0, EDIT3_FONT_HEIGHT, 0, 0);
}

static int caret_blink_on(void)
{
    return !((platform_ticks_ms() / CARET_BLINK_MS) & 1);
}

static void draw_caret(struct UIEditCtrl3 *self, int x, int y, COLORREF colour)
{
    ui_window_text_out(&self->edit.base, x, y, "|", 0, 0, EDIT3_FONT_HEIGHT, colour, 0, 0);
}

static int break_bytes(const struct UIEditCtrl3 *self, unsigned line)
{
    int size = line_len(self, line);
    return ends_in_break(self, line) ? int_min(size, 2) : 0;
}

static void draw_selection(struct UIEditCtrl3 *self, int i, int line_end, int x0, int y)
{
    struct UIEditCtrl *edit = &self->edit;
    int lo = int_min(edit->sel_start, edit->sel_end), hi = int_max(edit->sel_start, edit->sel_end);
    int lo_x = 0, width = 0, x, right, len, col;
    const char *line = line_at(self, i);

    if (lo < line_end) {
        col = (int)strlen(line) - line_end + lo;
        if (col < 0)
            col = 0;
        lo_x = col ? text_width(self, line, col) : 0;
        len = (int)strlen(line);
        if (ends_in_break(self, i))
            len -= break_bytes(self, i);
        width = text_width(self, line, len);
    }
    if (hi <= line_end) {
        col = hi - line_end + (int)strlen(line);
        if (col < 0)
            col = 0;
        len = col;
        if ((int)strlen(line) == col && ends_in_break(self, i))
            len = col - break_bytes(self, i);
        width = len ? text_width(self, line, len) : 0;
    }
    x = x0 + lo_x;
    width -= lo_x;
    if (x < EDIT3_MARGIN) {
        width += x - EDIT3_MARGIN;
        x = EDIT3_MARGIN;
    }
    right = edit->base.width;
    if (x >= right || width <= 0)
        return;
    ui_window_fill_rect(&edit->base, x, y, x + width <= right ? width : right - x, HIGHLIGHT_HEIGHT,
                      ui_skin_colour(6, 2));
}

static void follow_caret(struct UIEditCtrl3 *self, int page, int at_break)
{
    int line = EDIT3_CARET_LINE(self), first;

    if (self->scrolled)
        return;
    if (self->stepped_up) {
        if (line - at_break < self->first_line) {
            if (line < self->line_count - page)
                self->first_line = at_break + line;
            else
                self->first_line = self->line_count - page;
        }
    } else if (self->stepped_down) {
        if (line > self->first_line - 1 + page)
            self->first_line = line < page ? 0 : line - page + 1;
    } else {
        first = self->line_count > page ? self->line_count - page + at_break : line;
        self->first_line = first;
        if (self->extra_line && first > 0)
            self->first_line = first - 1;
    }
}

static void place_caret(struct UIEditCtrl3 *self, int x0, int first, int page, int caret_end, int comp_end,
                        COLORREF colour)
{
    struct UIEditCtrl *edit = &self->edit;
    int line = EDIT3_CARET_LINE(self), before, col, len, caret_x, comp_x, at_break = 0;

    len = line_len(self, line);
    if (self->caret_line_len != len) {
        self->scrolled = 0;
        self->caret_line_len = line_len(self, line);
    }
    before = line > 0 ? chars_before(self, line) : 0;
    col = caret_end - before >= 0 ? caret_end - before : caret_end;
    if (line_len(self, line) == col && ends_in_break(self, line)) {
        at_break = 1;
        col = 0;
    }
    if (self->caret_rest != line_len(self, line) - col)
        self->caret_rest = line_len(self, line) - col;
    if (self->caret_col != col)
        self->caret_col = col;
    caret_x = col ? text_width(self, line_at(self, line), col) : 0;
    comp_end = comp_end - before >= 0 ? comp_end - before : comp_end;
    len = comp_end;
    if (line_len(self, line) == comp_end && ends_in_break(self, line))
        len -= break_bytes(self, line);
    comp_x = len ? text_width(self, line_at(self, line), len) : 0;
    if (g_language->composition_length) {
        int top = (at_break - first + line) * self->line_height;

        ui_edit_ctrl_place_ime_candidates(edit, x0 + caret_x, top, top + self->line_height);
        caret_x += int_max(comp_x - caret_x, 2) + 2;
        self->caret_col += 2;
    }
    if (self->caret_x != caret_x)
        self->caret_x = caret_x;
    if (caret_blink_on() && ui_edit_ctrl_caret_shown()) {
        draw_caret(self, x0 + caret_x - 2, (at_break - first + EDIT3_CARET_LINE(self)) * self->line_height,
                   colour);
        follow_caret(self, page, at_break);
    }
    if (at_break)
        EDIT3_CARET_LINE(self)++;
}

void ui_edit_ctrl3_draw_text(struct UIEditCtrl *base_edit)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)base_edit;
    struct UIEditCtrl *edit = &self->edit;
    struct String text = {0}, copy = {0};
    COLORREF colour = edit->text_colour;
    int x0 = EDIT3_MARGIN, width, count, page, first, end, line_end, i, caret_end, comp_end;

    if (!edit->multiline && edit->scroll)
        x0 = EDIT3_MARGIN - text_width(self, string_cstr(&edit->text), edit->scroll);
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == &edit->base)
        ui_edit_ctrl_sync_from_ime(edit);
    string_assign(&text, string_cstr(&edit->text), edit->text.size);
    width = edit->base.width - 6;
    if (self->scrolls)
        width += EDIT3_MARGIN - self->bar->base.width;
    text_array_resize(&self->shown_lines, 0);
    byte_array_resize(&self->line_flags, 0);
    ui_window_break_coloured_lines(&edit->base, string_cstr(&text), &self->shown_lines,
                                 &self->line_flags, width);
    ui_edit_ctrl3_rewrap(self, &self->shown_lines);
    self->line_count = (int)line_count(self);
    if (self->extra_line)
        self->line_count++;
    count = (int)line_count(self);
    if (!count) {
        if (caret_blink_on() && ui_edit_ctrl_caret_shown())
            draw_caret(self, 1, 0, colour);
        goto done;
    }

    page = edit->base.height / self->line_height;
    first = self->first_line;
    end = (unsigned)(first + page) < (unsigned)count ? first + page : count;
    line_end = first > 0 ? chars_before(self, first) : 0;
    for (i = first; i < end; i++) {
        struct TextView shown = text_array_at(&self->shown_lines, i);
        int y = (i - first) * self->line_height, lower;
        size_t len;

        line_end += line_len(self, i);
        if (ui_window_mgr_get_focus_edit(&g_windowMgr) == &edit->base &&
            abs(g_language->caret - g_language->anchor))
            draw_selection(self, i, line_end, x0, y);
        lower = edit->base.id == ID_RODEX_CONTENTS_BODY || edit->base.id == ID_RODEX_COMPOSE_BODY;
        if (self->lines.size < (unsigned)i + 1 && !text_array_resize(&self->lines, i + 1)) abort();
        if (!owned_text_assign(&self->lines.items[i], shown)) abort();
        ui_edit_ctrl3_place_navi_links(self, shown, y, i, first, end);
        len = shown.size;
        string_assign(&copy, shown.data, shown.size);
        if (len >= 2 && copy.data[len - 2] == '\r')
            string_truncate(&copy, len - 2);
        ui_window_text_out(&edit->base, x0, lower + y, string_cstr(&copy), 0, 0,
                          EDIT3_FONT_HEIGHT, colour, 0, 0);
        ui_edit_ctrl3_place_tags(self, &x0, &y, &first, &end, &i);
    }

    caret_end = edit->sel_end;
    comp_end = (int)g_language->composition_length + int_max(edit->sel_start, edit->sel_end);
    {
        int left = caret_end;
        unsigned li;

        for (li = 0; li < line_count(self); li++) {
            left -= line_len(self, (int)li);
            EDIT3_CARET_LINE(self) = (int)li;
            if (left < 0 || (self->caret_eol && left == 0))
                break;
        }
    }
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == &edit->base)
        place_caret(self, x0, first, page, caret_end, comp_end, colour);

    count = (int)line_count(self);
    if (count > 1 && EDIT3_CARET_LINE(self) > 0)
        string_assign_cstr(&self->line_above, line_at(self, EDIT3_CARET_LINE(self) - 1));
    else if ((count == 1 || (count > 1 && EDIT3_CARET_LINE(self) == 0)) && ends_in_break(self, 0))
        string_assign_cstr(&self->line_above, line_at(self, 0));
    if (count > 1 && (unsigned)EDIT3_CARET_LINE(self) < (unsigned)(count - 1))
        string_assign_cstr(&self->line_below, line_at(self, EDIT3_CARET_LINE(self) + 1));
    else if (count == 1 && ends_in_break(self, 0))
        string_assign_cstr(&self->line_below, line_at(self, 0));
    ui_edit_ctrl3_update_bar(self);
done:
    string_free(&copy);
    string_free(&text);
}

static int chars_to_x(struct UIEditCtrl3 *self, const char *text, int target)
{
    const char *p = text, *end = text + strlen(text);
    int chars = 0, x = 0;

    while (p < end) {
        const char *next = utf8_step(p, end);
        int clen = (int)(next - p), prev_chars = chars, prev_x = x;

        x += text_width(self, p, clen);
        chars += clen;
        if (clen == 1 && *p == '\n')
            return chars - 1;
        if (x > target) {
            if (x - target > target - prev_x)
                chars = prev_chars;
            return chars;
        }
        p = next;
    }
    return chars;
}

static void read_selection(struct UIEditCtrl3 *self)
{
    language_get_selection(g_language, &self->edit.sel_start, &self->edit.sel_end);
}

static void step_up(struct UIEditCtrl3 *self)
{
    unsigned count = line_count(self);

    self->caret_eol = 0;
    if (count <= 1 && !(count == 1 && ends_in_break(self, 0)))
        return;
    read_selection(self);
    if (self->step > 0 && self->step_caret == self->edit.sel_end && !self->stepped_up) {
        language_move_caret_back(g_language, self->step);
    } else {
        int chars = 0;

        self->stepped_down = 0;
        if (self->caret_col && self->line_above.size)
            chars = chars_to_x(self, string_cstr(&self->line_above), self->caret_x);
        self->step = self->caret_col - chars + (int)self->line_above.size;
        language_move_caret_back(g_language, self->step);
    }
    self->stepped_up = 1;
    self->caret_eol = 1;
    read_selection(self);
    self->step_caret = self->edit.sel_end;
}

static void step_down(struct UIEditCtrl3 *self)
{
    unsigned count = line_count(self);

    self->caret_eol = 0;
    if (count > 1) {
        if ((unsigned)EDIT3_CARET_LINE(self) >= count - 1)
            return;
    } else if (count != 1 || !ends_in_break(self, 0) || self->caret_rest <= 0) {
        return;
    }
    read_selection(self);
    if (self->step > 0 && self->step_caret == self->edit.sel_end && !self->stepped_down) {
        language_move_caret_forward(g_language, self->step);
    } else {
        int chars = 0;

        self->stepped_up = 0;
        count = line_count(self);
        if (count > 1 && (unsigned)EDIT3_CARET_LINE(self) < count - 1)
            chars = chars_to_x(self, string_cstr(&self->line_below), self->caret_x);
        self->step = self->caret_rest + chars;
        language_move_caret_forward(g_language, self->step);
    }
    self->stepped_down = 1;
    read_selection(self);
    self->step_caret = self->edit.sel_end;
}

static void on_key(struct UIEditCtrl3 *self, int key)
{
    int line = EDIT3_CARET_LINE(self);

    self->step = 0;
    self->step_caret = 0;
    switch (key) {
    case VK_BACK:
        if (!line_count(self))
            return;
        read_selection(self);
        if (chars_before(self, line) == self->edit.sel_end && ends_in_break(self, (unsigned)(line - 1)))
            language_on_char(g_language, VK_BACK);
        return;
    case VK_END:
        if (line_count(self)) {
            int len;

            read_selection(self);
            len = line_len(self, line) - (self->edit.sel_end - chars_before(self, line));
            language_move_caret_forward(g_language, len - break_bytes(self, line));
            read_selection(self);
        }
        self->caret_eol = 1;
        return;
    case VK_HOME:
        if (!line_count(self))
            return;
        read_selection(self);
        language_move_caret_back(g_language, self->edit.sel_end - chars_before(self, line));
        read_selection(self);
        return;
    case VK_LEFT:
        if (line > 0) {
            read_selection(self);
            if (chars_before(self, EDIT3_CARET_LINE(self)) == self->edit.sel_end &&
                ends_in_break(self, EDIT3_CARET_LINE(self) - 1))
                language_on_key_down(g_language, key);
        }
        language_on_key_down(g_language, key);
        return;
    case VK_RIGHT:
        read_selection(self);
        line = EDIT3_CARET_LINE(self);
        if ((unsigned)line < line_count(self)) {
            int through = line >= 0 ? chars_before(self, line + 1) : 0;

            if (through - 2 == self->edit.sel_end && ends_in_break(self, line)) {
                language_on_key_down(g_language, key);
                language_on_key_down(g_language, key);
                return;
            }
        }
        language_on_key_down(g_language, key);
        return;
    case VK_INSERT:
    case VK_DELETE:
    case 'C':
    case 'V':
    case 'X':
        language_on_key_down(g_language, key);
        return;
    }
}

static int scroll_view(struct UIEditCtrl3 *self, int msg, int lines)
{
    int old = self->first_line, page = self->edit.base.height / self->line_height, pos;

    self->scrolled = 1;
    if (msg == UI_MSG_PAGE_UP)
        pos = old - page + 1;
    else if (msg == UI_MSG_PAGE_DOWN)
        pos = old - 1 + page;
    else
        pos = lines + old;
    self->first_line = pos;
    if (pos < 0)
        self->first_line = pos = 0;
    if (pos > self->line_count - page)
        self->first_line = pos = self->line_count - page;
    ui_scroll_bar_set_pos(self->bar, pos);
    ui_window_invalidate(&self->bar->base);
    ui_window_invalidate_base(&self->edit.base);
    if (self->edit.base.parent)
        ui_window_invalidate(self->edit.base.parent);
    return self->first_line - old;
}

intptr_t ui_edit_ctrl3_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                   intptr_t val3, intptr_t val4)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    switch (msg) {
    case 0:
        if (self->extra_line && !(BYTE)(platform_key_state(VK_SHIFT) >> 8)) {
            ui_window_send_msg_base(&self->edit.base, sender, 0, val1, val2, val3, val4);
        } else {
            char crlf[3] = "\r\n";

            language_replace_selection(g_language, crlf);
        }
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        return scroll_view(self, msg, val1);
    case EDIT3_MSG_LINE_UP:
        step_up(self);
        return 0;
    case EDIT3_MSG_LINE_DOWN:
        step_down(self);
        return 0;
    case UI_MSG_KEY:
        self->scrolled = 0;
        on_key(self, val1);
        return 3;
    }
    ui_edit_ctrl_send_msg((struct UIWindow *)(&self->edit), sender, msg, val1, val2, val3, val4);
    return 0;
}
