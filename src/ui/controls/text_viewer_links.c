#include "game/packet_handlers.h"
#include "ui/controls/listbox.h"
#include "ui/controls/buttons.h"
#include "ui/controls/text_viewer.h"
#include "ui/chat/readable_links.h"
#include "ui/chat/navi_text.h"
#include "core/text_array.h"
#include "platform/code_page.h"
#include "core/text_format.h"
#include "core/text_markup.h"
#include "iteminfo/item_api.h"
#include "navi/navi_manager.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "core/fatal.h"
#include "ui/window_access.h"
#include "core/markup_tags.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>
#include <string.h>
#include "core/app.h"
#include "ui/controls/url_link.h"

struct TagLink {
    int pos;
    struct String label;
    struct String args;
};

#define LINK_URL 0x1ad
#define LINK_NAVI 0x1ae
#define LINK_ITEM 0x1c8
#define LINK_QUEST 0x213
#define LINK_COLOUR 0xf4934a
#define LINK_HOVER 0xff7864
#define URL_COLOUR 0xe6a300
#define URL_HOVER 0xefb700
#define LINK_FONT_HEIGHT 0xc
#define LINK_HIDDEN_Y (-100)
#define LAYOUT_DEFAULT 0x23
#define LAYOUT_AMPERSAND 0x8a
#define VIEWER_MSG_LINE_UP 0x12
#define VIEWER_MSG_LINE_DOWN 0x13
#define NAVI_DB_SEARCH (-0xde)
#define WND_ITEM 0xc
#define UI_MSG_SHOW_ITEM 0x18

#define LINE_COUNT(tv) ((int)VEC_COUNT(&(tv)->list.items, struct String))
#define LINE(tv, i) (&((struct String *)(tv)->list.items.first)[i])

static int link_arg_number(struct UITextButton *link, unsigned i)
{
    return atoi(ui_text_button_get_line(link, i).data);
}

static void tag_links_free(struct Vec *v)
{
    struct TagLink *link;

    for (link = v->first; link != (struct TagLink *)v->last; link++) {
        string_free(&link->args);
        string_free(&link->label);
    }
    vec_free(v);
}

static struct String *unescape_newlines(struct String *out, const struct String *in)
{
    int at;

    string_copy(out, in);
    while ((at = string_find(out, "\\n", 0)) != -1)
        string_replace(out, at, 2, "\n", 1);
    while ((at = string_find(out, "\\r", 0)) != -1)
        string_replace(out, at, 2, "\r", 1);
    return out;
}

static void link_args_split(struct UITextButton *button, const struct String *args)
{
    struct TextView text = string_view(args);
    size_t start = 0;
    if (!text.size) return;
    for (;;) {
        size_t end = start;
        while (end < text.size && text.data[end] != ',') ++end;
        ui_text_button_add_line(button, text_view(text.data + start, end - start));
        if (end == text.size) break;
        start = end + 1;
    }
}

static int *link_place(struct UITextViewer *self, int out[2], int pos, int lines_back)
{
    int count = LINE_COUNT(self), i, total = 0;

    out[0] = out[1] = 0;
    for (i = count - lines_back; (unsigned)i < (unsigned)count; i++) {
        struct String plain, head;

        string_init(&plain);
        string_assign_cstr(&plain, string_cstr(LINE(self, i)));
        if (plain.size)
            plain.size = text_strip_colour_codes(plain.data, plain.size, g_codePage);
        total += plain.size;
        if (total > pos) {
            out[0] = i;
            string_init(&head);
            string_substring(&head, &plain, 0, plain.size - total + pos);
            out[1] = ui_window_text_width(&self->list.base, string_cstr(&head), 0, 0, LINK_FONT_HEIGHT, 0, 0) +
                     2;
            string_free(&head);
            string_free(&plain);
            return out;
        }
        string_free(&plain);
    }
    return out;
}

static void apply(struct String *s, const char *open, const char *close)
{
    if (s->size)
        s->size = text_unwrap_tag(s->data, s->size, open, close);
}

static void apply_chat(struct String *s)
{
    struct String result = {0};

    if (!g_windowMgr.new_chat_wnd)
        return;
    ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, &result, string_view(s));
    string_assign(s, string_cstr(&result), result.size);
    string_free(&result);
}

static void apply_navil(struct String *s)
{
    struct String result = {0};

    navi_links_readable_links(&result, string_view(s));
    string_assign(s, string_cstr(&result), result.size);
    string_free(&result);
}

static struct UITextButton *new_link(struct UITextViewer *self, const struct String *label, DWORD colour)
{
    struct UITextButton *btn = ui_text_button_create();

    ui_text_button_set_text(&btn->button.base, string_cstr(label), colour, 0, LINK_FONT_HEIGHT, 0, 1);
    return btn;
}

static void fit_link(struct UITextViewer *self, struct UITextButton *btn, const struct String *label)
{
    ui_text_button_fit_width(btn,
                          ui_window_text_width(&self->list.base, string_cstr(label), 0, 0, LINK_FONT_HEIGHT,
                                             0, 0) + 1,
                          btn->button.base.height);
}

static void place_link(struct UITextViewer *self, struct UITextButton *btn, int pos, int lines_back)
{
    int at[2];

    link_place(self, at, pos, lines_back);
    btn->value = at[0];
    ui_window_move_base(&btn->button.base, at[1], LINK_HIDDEN_Y);
    ui_window_add_child(&self->list.base, &btn->button.base);
}

static void add_embed(struct UITextViewer *self, struct UITextButton *btn)
{
    vec_push(&self->embeds, &btn, sizeof btn);
}

static void add_link(struct UITextViewer *self, const struct TagLink *link, int id, int lines)
{
    struct UITextButton *btn = new_link(self, &link->label, LINK_COLOUR);

    fit_link(self, btn, &link->label);
    btn->hover_colour = LINK_HOVER;
    btn->button.base.id = id;
    link_args_split(btn, &link->args);
    place_link(self, btn, link->pos, lines);
    add_embed(self, btn);
}

static BYTE add_links(struct UITextViewer *self, struct Vec *links, int id, int lines)
{
    struct TagLink *link;

    if (links->first == links->last)
        return 0;
    for (link = links->first; link != (struct TagLink *)links->last; link++)
        add_link(self, link, id, lines);
    ui_window_send_msg(&self->list.base, NULL, UI_MSG_SCROLL_V, 0, 0, 0, 0);
    return 1;
}

static void plain_text(struct String *s, const struct String *text)
{
    string_init(s);
    string_assign_cstr(s, string_cstr(text));
    if (s->size)
        s->size = text_strip_colour_codes(s->data, s->size, g_codePage);
}

static BYTE add_item_links(struct UITextViewer *self, struct String text, int lines)
{
    struct String s;
    struct Vec links = { 0 };
    BYTE added;

    plain_text(&s, &text);
    apply(&s, "<URL>", "</URL>");
    apply(&s, "<NAVI>", "</NAVI>");
    apply_chat(&s);
    apply_navil(&s);
    apply(&s, "<TIPBOX>", "</TIPBOX>");
    ui_window_parse_tag_links(&self->list.base, string_from(string_cstr(&s), s.size), string_from(string_cstr(&text_tag_ITEM), text_tag_ITEM.size),
                           string_from(string_cstr(&text_tag_ITEM_END), text_tag_ITEM_END.size), &links);
    added = add_links(self, &links, LINK_ITEM, lines);
    tag_links_free(&links);
    string_free(&s);
    string_free(&text);
    return added;
}

static void add_navi_links(struct UITextViewer *self, struct String text, int lines)
{
    struct String s;
    struct Vec links = { 0 };

    plain_text(&s, &text);
    apply(&s, "<URL>", "</URL>");
    apply(&s, "<ITEM>", "</ITEM>");
    apply_chat(&s);
    apply(&s, "<TIPBOX>", "</TIPBOX>");
    apply_navil(&s);
    apply(&s, "<QUEST>", "</QUEST>");
    ui_window_parse_navi_links(&self->list.base, string_from(string_cstr(&s), s.size), &links);
    add_links(self, &links, LINK_NAVI, lines);
    tag_links_free(&links);
    string_free(&s);
    string_free(&text);
}

static BYTE add_quest_links(struct UITextViewer *self, struct String text, int lines)
{
    struct String s;
    struct Vec links = { 0 };
    BYTE added;

    plain_text(&s, &text);
    apply(&s, "<URL>", "</URL>");
    apply(&s, "<NAVI>", "</NAVI>");
    apply_chat(&s);
    apply_navil(&s);
    apply(&s, "<TIPBOX>", "</TIPBOX>");
    ui_window_parse_tag_links(&self->list.base, string_from(string_cstr(&s), s.size), string_from(string_cstr(&text_tag_QUEST), text_tag_QUEST.size),
                           string_from(string_cstr(&text_tag_QUEST_END), text_tag_QUEST_END.size), &links);
    added = add_links(self, &links, LINK_QUEST, lines);
    tag_links_free(&links);
    string_free(&s);
    string_free(&text);
    return added;
}

static void add_url_links(struct UITextViewer *self, struct String text, int lines)
{
    struct String s;
    struct Vec links = { 0 };
    struct TagLink *link;

    plain_text(&s, &text);
    apply(&s, "<NAVI>", "</NAVI>");
    apply(&s, "<ITEM>", "</ITEM>");
    apply_chat(&s);
    apply(&s, "<TIPBOX>", "</TIPBOX>");
    apply_navil(&s);
    apply(&s, "<QUEST>", "</QUEST>");
    ui_window_parse_url_links(&self->list.base, string_from(string_cstr(&s), s.size), &links);
    if (links.first != links.last) {
        for (link = links.first; link != (struct TagLink *)links.last; link++) {
            struct UITextButton *btn = new_link(self, &link->label, URL_COLOUR);

            btn->hover_colour = URL_HOVER;
            btn->button.base.id = LINK_URL;
            fit_link(self, btn, &link->label);
            if (s.size)
                s.size = text_strip_info(s.data, s.size);
            place_link(self, btn, link->pos, lines);
            link_args_split(btn, &link->args);
            add_embed(self, btn);
        }
        ui_window_send_msg(&self->list.base, NULL, UI_MSG_SCROLL_V, 0, 0, 0, 0);
    }
    tag_links_free(&links);
    string_free(&s);
    string_free(&text);
}

static int add_lines(struct UITextViewer *self, const char *text, int layout, int val3, int val4)
{
    struct TextArray lines = {0};
    int i, count;
    if (val4) {
        int margin = val3 ? self->list.vbar->base.width + 2 : 0;
        ui_window_break_lines(&self->list.base, text, &lines, self->list.base.width - margin - 4);
    } else {
        text_wrap_bytes(text, &lines, layout, val3 != 0);
    }
    count = (int)lines.size;
    for (i = 0; i < count; i++)
        ui_list_box_add_item(&self->list, text_array_at(&lines, i).data);
    text_array_clear(&lines);
    return count;
}

static void add_text(struct UITextViewer *self, char *text, int val2, int val3, int val4)
{
    int layout = val2 ? val2 : LAYOUT_DEFAULT;

    if (val2 == LAYOUT_AMPERSAND) {
        struct TextArray lines = {0};
        int i;

        text_split_lines(text, &lines, '&');
        for (i = 0; i < (int)lines.size; i++)
            ui_list_box_add_item(&self->list, text_array_at(&lines, i).data);
        text_array_clear(&lines);
        self->list.outlined = 1;
    } else {
        if (text_has_markup_tags(text, strlen(text))) {
            struct String in, unescaped, s;
            struct String result = {0};
            int count;

            in = string_from_cstr(text);
            unescape_newlines(&unescaped, &in);
            string_free(&in);
            strcpy_s(text, strlen(text) + 1, string_cstr(&unescaped));
            s = string_from_cstr(text);
            apply(&s, "<URL>", "</URL>");
            apply(&s, "<NAVI>", "</NAVI>");
            apply(&s, "<TIPBOX>", "</TIPBOX>");
            apply(&s, "<ITEM>", "</ITEM>");
            if (s.size)
                s.size = text_strip_info(s.data, s.size);
            if (g_windowMgr.new_chat_wnd) {
                ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, &result, string_view(&s));
                string_assign(&s, string_cstr(&result), result.size);
                string_free(&result);
            }
            apply_navil(&s);
            apply(&s, "<QUEST>", "</QUEST>");
            count = add_lines(self, string_cstr(&s), layout, val3, val4);
            add_item_links(self, string_from_cstr(text), count);
            add_url_links(self, string_from_cstr(text), count);
            add_navi_links(self, string_from_cstr(text), count);
            add_quest_links(self, string_from_cstr(text), count);
            string_free(&s);
            string_free(&unescaped);
        } else {
            add_lines(self, text, layout, val3, val4);
        }
        self->list.outlined = 0;
    }
    ui_window_invalidate_base(&self->list.base);
}

static void navi_link(struct UITextButton *link, unsigned count)
{
    struct TextView map = ui_text_button_get_line(link, 0);
    int x, y;

    x = link_arg_number(link, 1);
    y = link_arg_number(link, 2);
    if ((int)count > 3) {
        int kind = link_arg_number(link, 3);
        int how = link_arg_number(link, 4);

        if (how == NAVI_DB_SEARCH) {
            struct String search = {0};

            string_assign_cstr(&search, "$$DB ");
            string_append(&search, map.data, map.size);
            navi_open_search(&g_navi, string_cstr(&search), search.size, kind, 0);
            string_free(&search);
        } else {
            int silent = link_arg_number(link, 5);
            int id = link_arg_number(link, 6);
            BYTE flags = 0, f;
            int units;

            if (how >= 100) {
                flags = 1;
                how -= 100;
            }
            f = how < 10 ? flags : (BYTE)(flags + 2);
            units = how < 10 ? how : how - 10;
            f = units < 1 ? f : (BYTE)(f + 4);
            navi_navigate(&g_navi, map.data, kind, f, silent == 0, x, y, id);
        }
    } else {
        navi_navigate(&g_navi, map.data, 2, 5, 0, x, y, 0x3ea);
    }
}

static void item_link(struct UITextButton *link)
{
    struct TextView name = ui_text_button_get_line(link, 0);
    struct ItemInfo info;
    struct UIWindow *wnd;

    item_info_construct(&info);
    info.identified = 1;
    info.type = link_arg_number(link, 1);
    info.refine = 0;
    string_assign(&info.id_text, name.data, name.size);
    wnd = ui_window_open(WND_ITEM);
    if (wnd) ui_window_send_msg(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
    item_info_destruct(&info);
}

static void on_link(struct UITextButton *link, int id)
{
    struct TextView arg;

    switch (id) {
    case LINK_URL:
        if (!link)
            return;
        arg = ui_text_button_get_line(link, 0);
        ui_open_url_link(arg);
        break;
    case LINK_NAVI: {
        unsigned count;

        if (link && (count = ui_text_button_line_count(link)) != 0)
            navi_link(link, count);
        break;
    }
    case LINK_ITEM:
        if (link && ui_text_button_line_count(link))
            item_link(link);
        break;
    case LINK_QUEST:
        if (link)
            game_mode_ui_open_v2(game_current_mode(), 6, link_arg_number(link, 0));
        break;
    }
}

static void place_embeds(struct UITextViewer *self, int old_scroll_x)
{
    unsigned i;

    for (i = 0; i < VEC_COUNT(&self->embeds, struct UIWindow *); i++) {
        struct UIWindow *embed = ((struct UIWindow **)self->embeds.first)[i];
        int line;

        if (!embed)
            continue;
        line = ((struct UITextButton *)embed)->value;
        if (line >= self->list.top &&
            line < self->list.base.height / self->list.line_height - self->list.extra_lines + self->list.top) {
            ui_button_show((struct UIButton *)(embed), 1);
            ui_window_move(embed, embed->x - self->list.scroll_x + old_scroll_x,
                     (line - self->list.top) * self->list.line_height + 3);
        } else {
            ui_button_show((struct UIButton *)(embed), 0);
        }
    }
}

int ui_text_viewer_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UITextViewer *self = (struct UITextViewer *)window;
    switch (msg) {
    case UI_MSG_SCROLL_V:
    case UI_MSG_SCROLL_H:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
    case UI_MSG_PAGE_LEFT:
    case UI_MSG_PAGE_RIGHT: {
        int old_scroll_x = self->list.scroll_x;

        ui_list_box_send_msg((struct UIWindow *)(&self->list), sender, msg, val1, val2, val3, val4);
        place_embeds(self, old_scroll_x);
        return 0;
    }
    case VIEWER_MSG_LINE_UP:
        if (self->list.has_vscroll)
            ui_window_send_msg(&self->list.base, &self->list.vbar->base, UI_MSG_SCROLL_V, -1, 0, 0, 0);
        return 0;
    case VIEWER_MSG_LINE_DOWN:
        if (self->list.has_vscroll)
            ui_window_send_msg(&self->list.base, &self->list.vbar->base, UI_MSG_SCROLL_V, 1, 0, 0, 0);
        return 0;
    case UI_MSG_ADD_TEXT:
        add_text(self, (char *)val1, val2, val3, val4);
        return 0;
    case UI_MSG_TEXT_BUTTON:
        on_link((struct UITextButton *)sender, val1);
        return 0;
    }
    return ui_list_box_send_msg((struct UIWindow *)(&self->list), sender, msg, val1, val2, val3, val4);
}
