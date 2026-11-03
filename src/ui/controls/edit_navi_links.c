#include "ui/controls/edit.h"
#include <stdlib.h>
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "core/markup_tags.h"
#include "platform/language.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/chat/markup_links.h"
#include <string.h>
#include "ui/controls/ui_hooks.h"

struct ItemInfo;

#define NAVI_LINK_COLOUR 0x9e68f0
#define LINK_FONT_HEIGHT 0xc
#define OUT_OF_SIGHT (-1000)

static int find_str(struct TextView text, struct TextView needle)
{
    return text_view_find(text_view(text.data, text.size), text_view(needle.data, needle.size), 0);
}

static void bracket(struct String *out, const struct String *name)
{
    string_assign_cstr(out, "<");
    string_append(out, string_cstr(name), name->size);
    string_append_cstr(out, ">");
}

static int text_width(struct UIEditCtrl *self, const char *text, int len)
{
    return ui_window_text_width(&self->base, text, len, 0, LINK_FONT_HEIGHT, 0, 0);
}

static void link_name(struct String *out, const struct UINaviLinkButton *btn)
{
    navi_links_readable_link(out, string_view(&btn->link), 0);
}

static void fit_name(struct UIEditCtrl *self, struct String *out, const struct String *name)
{
    ui_window_fit_plain_text(&self->base, out, string_view(name), self->base.width / 2);
}

static int caret_ours(void)
{
    return !g_uiHooks.may_move_ime_caret || g_uiHooks.may_move_ime_caret();
}

static void caret_out_of_link(int start, int name_len)
{
    int end = start + 2 + name_len, caret;

    if (!caret_ours())
        return;
    caret = g_language->caret;
    if (caret <= start || caret >= end)
        return;
    if (name_len / 2 + 1 + start > caret)
        language_set_selection(g_language, end, end);
    else
        language_set_selection(g_language, start, start);
}

static void drop_button(struct UIEditCtrl *self)
{
    struct UINaviLinkButton *button = self->navigation_link;

    self->navigation_link = NULL;
    if (button)
        ui_window_remove_child(&self->base, &button->text.button.base, 1);
}

enum LinkMatch { LINK_MISSING, LINK_WHOLE, LINK_DELETE };
struct LinkSpan { size_t start, size; };

static enum LinkMatch find_link_span(struct TextView text, struct TextView name,
                                    int caret, int may_delete, struct LinkSpan *span)
{
    size_t pos;

    if (!name.size || name.size > text.size)
        return LINK_MISSING;
    for (pos = 0; pos <= text.size - name.size; ++pos) {
        int open, close;
        size_t end = pos + name.size;

        if (memcmp(text.data + pos, name.data, name.size))
            continue;
        open = pos > 0 && text.data[pos - 1] == '<';
        close = end < text.size && text.data[end] == '>';
        if ((open && close) || (open != close && may_delete && caret >= 0 &&
                               (size_t)caret == (open ? end : pos))) {
            span->start = pos - open;
            span->size = name.size + open + close;
            return open && close ? LINK_WHOLE : LINK_DELETE;
        }
    }
    return LINK_MISSING;
}

static enum LinkMatch check_link_button(struct UIEditCtrl *self, struct LinkSpan *span)
{
    struct String name = {0};
    struct String fit = {0};
    enum LinkMatch match;

    link_name(&name, self->navigation_link);
    fit_name(self, &fit, &name);
    match = find_link_span(string_view(&self->text),
                           text_view_cstr(string_cstr(&fit)),
                           g_language->caret, caret_ours(), span);
    string_free(&fit);
    string_free(&name);
    if (match == LINK_WHOLE)
        return LINK_WHOLE;
    if (match == LINK_DELETE) {
        struct String replacement = {0};
        size_t end = span->start + span->size;

        string_assign(&replacement, string_cstr(&self->text), span->start);
        string_append(&replacement, string_cstr(&self->text) + end, self->text.size - end);
        language_cancel_input(g_language);
        language_replace_selection(g_language, string_cstr(&replacement));
        string_free(&replacement);
    }
    drop_button(self);
    return match;
}

static void serialize_link(struct String *out, struct TextView text,
                           const struct LinkSpan *span, const struct UINaviLinkButton *button)
{
    size_t end = span->start + span->size;

    string_assign(out, text.data, span->start);
    string_append(out, string_cstr(&button->link), button->link.size);
    string_append(out, text.data + end, text.size - end);
}

void ui_edit_ctrl_layout_navi_links(struct UIEditCtrl *self)
{
    struct LinkSpan span;
    enum LinkMatch match;
    struct String before = {0};
    size_t scroll;

    if (!self->navigation_link)
        return;
    match = check_link_button(self, &span);
    if (match != LINK_WHOLE) {
        const char *input = match == LINK_DELETE ? language_input_text(g_language) : string_cstr(&self->text);
        string_assign_cstr(&self->raw_text, input);
        return;
    }
    scroll = self->scroll > 0 ? (size_t)self->scroll : 0;
    if (scroll && span.start <= scroll) {
        ui_window_move_base(&self->navigation_link->text.button.base, 1000, self->text_y - 1);
    } else {
        string_assign(&before, string_cstr(&self->text) + scroll, span.start - scroll);
        ui_window_move_base(&self->navigation_link->text.button.base, text_width(self, string_cstr(&before), 0) + 2,
                 self->text_y - 1);
        string_free(&before);
    }
    caret_out_of_link((int)span.start, (int)span.size - 2);
    serialize_link(&self->raw_text, string_view(&self->text), &span, self->navigation_link);
}

void ui_edit_ctrl3_place_navi_links(struct UIEditCtrl3 *self, struct TextView line,
                              int y, int index, int first, int last)
{
    struct UIEditCtrl *edit = &self->edit;
    struct UINaviLinkButton *button = edit->navigation_link;
    struct LinkSpan full, local;
    struct TextView name;
    struct String serialized = {0};
    int x = 3;

    if (!button)
        return;
    if (button->line < first || button->line >= last)
        ui_window_move_base(&button->text.button.base, OUT_OF_SIGHT, OUT_OF_SIGHT);
    if (!memchr(line.data, '<', line.size) && !memchr(line.data, '>', line.size))
        return;
    if (check_link_button(edit, &full) != LINK_WHOLE)
        return;
    name = text_view(string_cstr(&edit->text) + full.start + 1, full.size - 2);
    if (find_link_span(line, name, 0, 0, &local) != LINK_WHOLE)
        return;
    if (!edit->multiline && edit->scroll)
        x -= text_width(edit, string_cstr(&edit->text), edit->scroll);
    button->line = index;
    ui_window_move_base(&button->text.button.base, (local.start ? text_width(edit, line.data, (int)local.start) : 0) + x - 1, y);
    caret_out_of_link((int)full.start, (int)full.size - 2);
    serialize_link(&serialized, line, &local, button);
    if (index < 0) abort();
    if (self->lines.size <= (unsigned)index && !text_array_resize(&self->lines, (unsigned)index + 1)) abort();
    if (!owned_text_assign(&self->lines.items[index], string_view(&serialized))) abort();
    string_free(&serialized);
}

static void add_navi_link(struct UIEditCtrl *self, const char *text)
{
    struct String name = {0}, shown = {0}, merged = {0}, readable = {0};
    BYTE had_button = 0;
    int last_pos = 0, last_len = 0;

    navi_links_readable_link(&name, text_view_cstr(text), 0);
    bracket(&shown, &name);
    if (self->navigation_link) {
        struct UINaviLinkButton *btn = self->navigation_link;
        struct String old_name = {0}, old_shown = {0};

        had_button = 1;
        link_name(&old_name, btn);
        bracket(&old_shown, &old_name);
        last_len = (int)strlen(string_cstr(&old_shown));
        last_pos = find_str(string_view(&self->text), string_view(&old_shown));
        if (last_pos != -1)
            string_assign(&merged, string_cstr(&self->text), last_pos);
        drop_button(self);
        string_free(&old_shown);
        string_free(&old_name);
    }
    if (!strstr(text, string_cstr(&text_tag_NAVIL)) &&
        !strstr(text, string_cstr(&text_tag_NAVIL_END))) {
        ui_edit_ctrl_set_text(self, text);
        goto done;
    }
    navi_links_readable_links(&readable, text_view_cstr(text));
    if (self->text.size <= (unsigned)self->max_chars &&
        readable.size <= (unsigned)self->max_chars - self->text.size) {
        struct UINaviLinkButton *btn;
        struct TextView link;

        if (!had_button) {
            string_append(&self->text, string_cstr(&readable), readable.size);
        } else if (last_pos != -1) {
            string_append(&merged, string_cstr(&shown), shown.size);
            string_append(&merged, string_cstr(&self->text) + last_pos + last_len, self->text.size - last_pos - last_len);
            string_assign(&self->text, string_cstr(&merged), merged.size);
        }
        ui_edit_ctrl_set_text(self, string_cstr(&self->text));
        self->sel_start = self->sel_end = (int)self->text.size;
        language_set_selection(g_language, (int)self->text.size, (int)self->text.size);
        btn = ui_navi_link_button_create();
        if (!btn)
            goto done;
        ui_text_button_set_text(&btn->text.button.base, string_cstr(&shown), NAVI_LINK_COLOUR, 0,
                             LINK_FONT_HEIGHT, 0, 0);
        btn->text.button.base.id = UI_ID_NAVI_LINK;
        link = navi_links_extract_link(text_view_cstr(text));
        string_assign(&btn->link, link.data, link.size);
        self->navigation_link = btn;
        ui_window_add_child(&self->base, &btn->text.button.base);
    }
done:
    string_free(&readable);
    string_free(&merged);
    string_free(&shown);
    string_free(&name);
}

int ui_edit_ctrl_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UIEditCtrl *self = (struct UIEditCtrl *)window;
    switch (msg) {
    case 0:
        ui_window_send_msg_base(&self->base, sender, 0, val1, val2, val3, val4);
        if (!self->keep_ime_on_enter)
            language_cancel_input(g_language);
        return 0;
    case UI_MSG_TEXT_BUTTON:
        if (val3 && g_uiHooks.show_item_link)
            g_uiHooks.show_item_link((struct ItemInfo *)val3, val4);
        break;
    case UI_MSG_PARSE_LINKS:
        add_navi_link(self, (const char *)val1);
        break;
    }
    ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    return 0;
}
