#include "ui/controls/line_info.h"
#include "ui/window.h"
#include "ui/controls/buttons.h"
#include "ui/draw.h"
#include "ui/chat/markup_links.h"
#include "iteminfo/item_api.h"
#include "platform/code_page.h"
#include "core/text_format.h"
#include "core/text_markup.h"
#include "core/memory.h"
#include "std/std.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/object_type_tables.h"
#include "diag/perf.h"

const struct UIWindowOps line_cutter_text_ops = {
    .destroy = line_cutter_text_destroy,
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
    .on_draw = line_cutter_text_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = line_cutter_text_on_l_btn_down,
    .on_l_btn_dbl_clk = line_cutter_text_on_l_btn_dbl_clk,
    .on_mouse_move = line_cutter_text_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = line_cutter_text_on_mouse_shape,
    .on_l_btn_up = line_cutter_text_on_l_btn_up,
    .on_r_btn_down = line_cutter_text_on_r_btn_down,
    .on_r_btn_up = line_cutter_text_on_r_btn_up,
    .on_wheel = line_cutter_text_on_wheel,
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
};

#define LINK_URL_COLOUR 0xe6a300
#define LINK_URL_HOVER 0xefb700
#define LINK_COLOUR 0xf4934a
#define LINK_HOVER 0xff7864
#define LINK_FONT_HEIGHT 0xc
#define TEXT_DEFAULT_HEIGHT 0xc
#define TEXT_OUTLINE_DEFAULT 0xffffff
enum LineItemKind {
    LINE_ITEM_URL = 0x1ad,
    LINE_ITEM_NAVI = 0x1ae,
    LINE_ITEM_ITEM = 0x1c8,
    LINE_ITEM_ITEML = 0x1ed,
    LINE_ITEM_QUEST = 0x213,
    LINE_ITEM_NAVIL = 0x22a,
    LINE_ITEM_MSG = 0x22b,
    LINE_ITEM_TEXT = 0x239,
};

struct LineItem {
    struct String text;
    struct LineStyle style;
    int kind;
};

struct LineFont { int font_height; COLORREF colour; };
struct LineRow { size_t begin, count; };
struct LineBuilder {
    struct CreateLineInfo *result;
    struct UIWindow *wnd;
    int width, line_spacing, bold, italic, default_font_height, font_height;
    COLORREF default_colour, colour, outline_colour;
    BYTE outlined, align, style_changed;
    struct String text;
    struct LineFont *fonts;
    size_t font_count, font_capacity;
    struct LineItem *items;
    size_t item_count, item_capacity;
    struct LineRow *rows;
    size_t row_count, row_capacity, window_capacity;
};

static void *reserve(void *data, size_t *capacity, size_t count, size_t element_size)
{
    size_t limit = (size_t)-1 / element_size, next;
    void *grown;
    if (count <= *capacity) return data;
    if (count > limit) abort();
    next = *capacity ? *capacity : 8;
    while (next < count) next = next > limit / 2 ? limit : next * 2;
    grown = realloc(data, next * element_size);
    if (!grown) abort();
    *capacity = next;
    return grown;
}

static struct TextView suffix(struct TextView text, size_t pos)
{
    return text_view(text.data + pos, text.size - pos);
}

static int str_is(struct TextView text, const char *literal)
{
    size_t length = strlen(literal);
    return text.size == length && !memcmp(text.data, literal, length);
}

static int str_tag_matches(struct TextView text, const char *key, int whole)
{
    size_t i, length = strlen(key);
    if (text.size < length || (whole && text.size != length)) return 0;
    for (i = 0; i < length; ++i)
        if (toupper((unsigned char)text.data[i]) != (unsigned char)key[i]) return 0;
    return 1;
}

static size_t str_find_key(struct TextView text, const char *key)
{
    size_t i;
    for (i = 0; i < text.size; ++i)
        if (str_tag_matches(suffix(text, i), key, 0)) return i;
    return (size_t)-1;
}

static size_t first_char_size(struct TextView text)
{
    char bytes[3] = {0};
    size_t count;
    if (!text.size) return 0;
    memcpy(bytes, text.data, text.size < 2 ? text.size : 2);
    count = (size_t)(CharNextExA((WORD)g_codePage, bytes, 0) - bytes);
    return count && count <= text.size ? count : 1;
}

static BYTE parse_hex_colour(const char *hex, COLORREF *colour)
{
    char *end;
    unsigned long value = (unsigned long)strtol(hex, &end, 16);
    *colour = 0;
    if (end - hex != 6) return 0;
    *colour = (value & 0xff) << 16 | (value & 0xff00) | (value >> 16 & 0xff);
    return 1;
}

static SIZE measure(struct UIWindow *wnd, struct TextView text, const struct LineStyle *style)
{
    SIZE size = {0};
    if (wnd)
        ui_window_text_extent(wnd, &size, text.data, (int)text.size, 0,
                            style->font_height, style->bold, style->italic);
    return size;
}

static size_t fit_prefix(struct UIWindow *wnd, struct TextView text,
                         const struct LineStyle *style, int width)
{
    size_t pos = 0;
    int total = 0;
    while (pos < text.size) {
        size_t count = first_char_size(suffix(text, pos));
        total += measure(wnd, text_view(text.data + pos, count), style).cx;
        if (total > width) return pos;
        pos += count;
    }
    return pos;
}

static size_t split_line(struct LineBuilder *self, struct TextView text, const struct LineStyle *style)
{
    size_t count;
    if (measure(self->wnd, text, style).cx < self->width) return text.size;
    count = fit_prefix(self->wnd, text, style, self->width);
    return count ? count : first_char_size(text);
}

struct TagPair { const char *open, *close; int kind; };
static const struct TagPair links[] = {
    {"<URL>", "</URL>", LINE_ITEM_URL}, {"<ITEM>", "</ITEM>", LINE_ITEM_ITEM},
    {"<ITEML>", "</ITEML>", LINE_ITEM_ITEML}, {"<NAVI>", "</NAVI>", LINE_ITEM_NAVI},
    {"<NAVIL>", "</NAVIL>", LINE_ITEM_NAVIL}, {"<MSG>", "</MSG>", LINE_ITEM_MSG},
    {"<QUEST>", "</QUEST>", LINE_ITEM_QUEST}, {"<TIPBOX>", "</TIPBOX>", LINE_ITEM_TEXT}
};

static const struct TagPair *link_tags(int kind)
{
    size_t i;
    for (i = 0; i < sizeof links / sizeof links[0]; ++i)
        if (links[i].kind == kind) return links + i;
    return NULL;
}

struct LinkParts { struct TextView args, shown; int ok; };
static struct LinkParts link_parts(struct TextView text, int kind)
{
    struct LinkParts parts = {0};
    const struct TagPair *tags = link_tags(kind);
    size_t open_at, info_at, end_at, shown_at;
    if (!tags) return parts;
    open_at = text_view_find(text, text_view_cstr(tags->open), 0);
    if (open_at == (size_t)-1 || text_view_find(text, text_view_cstr(tags->close), open_at) == STRING_NPOS)
        return parts;
    info_at = text_view_find(text, text_view_cstr("<INFO>"), open_at);
    if (info_at == (size_t)-1) return parts;
    end_at = text_view_find(text, text_view_cstr("</INFO>"), info_at);
    if (end_at == (size_t)-1) return parts;
    shown_at = open_at + strlen(tags->open);
    parts.shown = text_view(text.data + shown_at, info_at - shown_at);
    parts.args = text_view(text.data + info_at + 6, end_at - info_at - 6);
    parts.ok = 1;
    return parts;
}

static void push_item(struct LineBuilder *self, int kind)
{
    struct LineItem *item;
    self->items = reserve(self->items, &self->item_capacity, self->item_count + 1, sizeof *self->items);
    item = self->items + self->item_count++;
    memset(item, 0, sizeof *item);
    item->style.colour = self->colour;
    item->style.font_height = self->font_height;
    item->style.bold = self->bold > 0;
    item->style.italic = self->italic > 0;
    item->kind = kind;
    self->style_changed = 0;
}

static void append_text(struct LineBuilder *self, struct TextView text)
{
    if (!self->item_count) push_item(self, LINE_ITEM_TEXT);
    string_append(&self->items[self->item_count - 1].text, text.data, text.size);
}

static void new_row(struct LineBuilder *self)
{
    struct LineRow row = {self->result->count, 0};
    self->rows = reserve(self->rows, &self->row_capacity, self->row_count + 1, sizeof *self->rows);
    self->rows[self->row_count++] = row;
}

static void row_add(struct LineBuilder *self, struct UIWindow *window)
{
    struct CreateLineInfo *result = self->result;
    if (!self->row_count) new_row(self);
    result->windows = reserve(result->windows, &self->window_capacity, result->count + 1, sizeof *result->windows);
    result->windows[result->count++] = window;
    ++self->rows[self->row_count - 1].count;
}

static size_t parse_tagged(struct LineBuilder *self, size_t pos, const struct TagPair *tags)
{
    struct TextView text = string_view(&self->text);
    size_t close_at = text_view_find(text, text_view_cstr(tags->close), pos);
    if (close_at != (size_t)-1) {
        size_t end = close_at + strlen(tags->close);
        push_item(self, tags->kind);
        append_text(self, text_view(text.data + pos, end - pos));
        if (tags->kind == LINE_ITEM_TEXT) {
            /* Retired guide links keep their labels, without a clickable target. */
            struct String *label = &self->items[self->item_count - 1].text;
            label->size = text_unwrap_tag(label->data, label->size, tags->open, tags->close);
            label->size = text_strip_info(label->data, label->size);
        }
        self->style_changed = 1;
        return end;
    }
    return pos;
}

static size_t parse_link(struct LineBuilder *self, size_t pos)
{
    struct TextView rest = suffix(string_view(&self->text), pos);
    size_t i;
    for (i = 0; i < sizeof links / sizeof links[0]; ++i) {
        size_t length = strlen(links[i].open);
        if (rest.size >= length && !memcmp(rest.data, links[i].open, length))
            return parse_tagged(self, pos, links + i);
    }
    return pos;
}

static void open_font(struct LineBuilder *self, struct TextView tag)
{
    struct String part = {0};
    size_t at = str_find_key(tag, "SIZE");
    int found = 0;
    if (at != (size_t)-1) {
        int size = 0;
        struct TextView rest = suffix(tag, at);
        string_assign(&part, rest.data, rest.size);
        sscanf(string_cstr(&part), "%*[^=]=%d", &size);
        if (size) self->font_height = size;
        found = 1;
    }
    at = str_find_key(tag, "COLOR");
    if (at != (size_t)-1) {
        char hex[7] = {0};
        COLORREF colour;
        struct TextView rest = suffix(tag, at);
        string_assign(&part, rest.data, rest.size);
        sscanf(string_cstr(&part), "%*[^=]=%6s", hex);
        if (parse_hex_colour(hex, &colour)) { found = 1; self->colour = colour; }
    }
    string_free(&part);
    if (found) {
        struct LineFont font = {self->font_height, self->colour};
        self->fonts = reserve(self->fonts, &self->font_capacity, self->font_count + 1, sizeof *self->fonts);
        self->fonts[self->font_count++] = font;
        self->style_changed = 1;
    }
}

static void close_font(struct LineBuilder *self)
{
    if (!self->font_count) return;
    if (!--self->font_count) {
        self->font_height = self->default_font_height;
        self->colour = self->default_colour;
    } else {
        self->font_height = self->fonts[self->font_count - 1].font_height;
        self->colour = self->fonts[self->font_count - 1].colour;
    }
    self->style_changed = 1;
}

static size_t parse_style_tag(struct LineBuilder *self, size_t pos)
{
    const char *text = string_cstr(&self->text), *gt;
    struct TextView tag;
    if (text[pos] != '<') return pos;
    gt = memchr(text + pos, '>', self->text.size - pos);
    if (!gt) return pos;
    tag = text_view(text + pos + 1, (size_t)(gt - text) - pos - 1);
    if (str_tag_matches(tag, "FONT", 0)) open_font(self, tag);
    else if (str_tag_matches(tag, "/FONT", 1)) close_font(self);
    else if (str_tag_matches(tag, "B", 1)) {
        if (!self->bold) self->style_changed = 1;
        ++self->bold;
    } else if (str_tag_matches(tag, "/B", 1)) {
        if (!--self->bold) self->style_changed = 1;
    } else if (str_tag_matches(tag, "I", 1)) {
        if (!self->italic) self->style_changed = 1;
        ++self->italic;
    } else if (str_tag_matches(tag, "/I", 1)) {
        if (!--self->italic) self->style_changed = 1;
    } else return pos;
    return (size_t)(gt - text) + 1;
}

static size_t parse_item_name(struct LineBuilder *self, size_t pos)
{
    struct TextView rest = suffix(string_view(&self->text), pos);
    size_t end;
    const size_t prefix = sizeof "^nItemID^" - 1;
    if (rest.size < prefix || memcmp(rest.data, "^nItemID^", prefix)) return pos;
    for (end = prefix; end < rest.size && isxdigit((unsigned char)rest.data[end]); ++end) {}
    if (end > prefix) {
        struct ItemInfo info;
        struct String digits = {0};
        string_assign(&digits, rest.data + prefix, end - prefix);
        item_info_construct(&info);
        item_info_set_item_id(&info, atoi(string_cstr(&digits)));
        string_free(&digits);
        push_item(self, LINE_ITEM_TEXT);
        append_text(self, text_view_cstr(item_info_get_name(&info)));
        item_info_destruct(&info);
        self->style_changed = 1;
        return pos + end;
    }
    return pos;
}

static size_t parse_char(struct LineBuilder *self, size_t pos)
{
    struct TextView rest = suffix(string_view(&self->text), pos);
    struct TextView ch = text_view(rest.data, first_char_size(rest));
    if (str_is(ch, "\n")) {
        push_item(self, LINE_ITEM_TEXT);
        append_text(self, ch);
        self->style_changed = 1;
    } else if (str_is(ch, "\r")) {
    } else if (str_is(ch, " ")) {
        if (pos && self->text.data[pos - 1] == '\n') push_item(self, LINE_ITEM_TEXT);
        append_text(self, ch);
        self->style_changed = 1;
    } else {
        if (self->style_changed) push_item(self, LINE_ITEM_TEXT);
        append_text(self, ch);
    }
    return pos + ch.size;
}

static size_t parse_colour(struct LineBuilder *self, size_t pos)
{
    char hex[7] = {0};
    COLORREF colour;
    size_t count = self->text.size - pos - 1;
    if (self->text.data[pos] != '^') return pos;
    memcpy(hex, self->text.data + pos + 1, count < 6 ? count : 6);
    if (!parse_hex_colour(hex, &colour)) return pos;
    self->style_changed = 1;
    self->default_colour = self->colour = colour;
    return pos + 7;
}

static void parse(struct LineBuilder *self)
{
    size_t pos = 0;
    while (pos < self->text.size) {
        size_t next = parse_colour(self, pos);
        if (next == pos) next = parse_item_name(self, pos);
        if (next == pos) next = parse_link(self, pos);
        if (next == pos) next = parse_style_tag(self, pos);
        if (next == pos) next = parse_char(self, pos);
        pos = next;
    }
}

static struct UIWindow *make_text(struct LineBuilder *self, struct TextView text, const struct LineStyle *style)
{
    struct LineCutterText *run;
    SIZE size;
    if (!text.size) return NULL;
    size = measure(self->wnd, text, style);
    run = malloc(sizeof *run);
    if (!run) return NULL;
    line_cutter_text_construct(run);
    ui_window_create(&run->base, size.cx, size.cy);
    line_cutter_text_set_text(run, text, *style, self->outlined, self->outline_colour);
    return &run->base;
}

static struct UIWindow *make_link(struct LineBuilder *self, struct TextView text,
                                  struct TextView args, int kind, struct LineStyle style)
{
    struct UITextButton *button;
    struct String label = {0};
    SIZE size;
    size_t start = 0;
    if (!text.size || !args.size) return NULL;
    size = measure(self->wnd, text, &style);
    button = malloc(sizeof *button);
    if (!button) return NULL;
    ui_text_button_construct(button);
    string_assign(&label, text.data, text.size);
    ui_text_button_set_text((struct UIWindow *)button, string_cstr(&label),
                        kind == LINE_ITEM_URL ? LINK_URL_COLOUR : LINK_COLOUR, 0, LINK_FONT_HEIGHT, 0, 1);
    string_free(&label);
    button->hover_colour = kind == LINE_ITEM_URL ? LINK_URL_HOVER : LINK_HOVER;
    button->button.base.id = kind;
    ui_text_button_fit_width(button, size.cx + 1, button->button.base.height);
    for (;;) {
        size_t end = start;
        while (end < args.size && args.data[end] != ',') ++end;
        ui_text_button_add_line(button, text_view(args.data + start, end - start));
        if (end == args.size) break;
        start = end + 1;
    }
    return (struct UIWindow *)button;
}

static int add_text(struct LineBuilder *self, int x, struct TextView text, struct LineStyle style)
{
    struct UIWindow *window;
    SIZE size = measure(self->wnd, text, &style);
    if (size.cx > self->width) {
        size_t pos = fit_prefix(self->wnd, text, &style, self->width - x);
        window = make_text(self, text_view(text.data, pos), &style);
        if (window) row_add(self, window);
        if (pos < text.size) x = 0;
        while (pos < text.size) {
            struct TextView rest = suffix(text, pos);
            size_t count = split_line(self, rest, &style);
            new_row(self);
            window = make_text(self, text_view(rest.data, count), &style);
            if (window) row_add(self, window);
            pos += count;
        }
        if (window) x += window->width;
        return x;
    }
    if (size.cx + x > self->width) { new_row(self); x = 0; }
    window = make_text(self, text, &style);
    if (window) { row_add(self, window); x += window->width; }
    return x;
}

static int add_link(struct LineBuilder *self, int x, struct TextView text, int kind, struct LineStyle style)
{
    struct LinkParts parts = link_parts(text, kind);
    struct String shown = {0};
    struct UIWindow *button;
    SIZE size;
    if (!parts.ok) return x;
    string_assign(&shown, parts.shown.data, parts.shown.size);
    if (shown.size) shown.size = text_strip_colour_codes(shown.data, shown.size, g_codePage);
    size = measure(self->wnd, string_view(&shown), &style);
    if (size.cx > self->width) {
        size_t pos = 0;
        while (pos < shown.size) {
            struct TextView rest = suffix(string_view(&shown), pos);
            size_t count = split_line(self, rest, &style);
            new_row(self);
            x = 0;
            button = make_link(self, text_view(rest.data, count), parts.args, kind, style);
            if (button) { row_add(self, button); x = button->width; }
            pos += count;
        }
    } else {
        if (size.cx + x > self->width) { new_row(self); x = 0; }
        button = make_link(self, string_view(&shown), parts.args, kind, style);
        if (button) { row_add(self, button); x += button->width; }
    }
    string_free(&shown);
    return x;
}

static void layout(struct LineBuilder *self)
{
    size_t i;
    int x = 0;
    for (i = 0; i < self->item_count; ++i) {
        struct LineItem *item = self->items + i;
        struct TextView text = string_view(&item->text);
        switch (item->kind) {
        case LINE_ITEM_TEXT:
            if (str_is(text, "\n")) { new_row(self); x = 0; }
            else x = add_text(self, x, text, item->style);
            break;
        case LINE_ITEM_URL: case LINE_ITEM_NAVI: case LINE_ITEM_ITEM:
        case LINE_ITEM_QUEST:
            x = add_link(self, x, text, item->kind, item->style);
            break;
        case LINE_ITEM_MSG: {
            struct String expanded = {0};
            navi_links_expand_msg_tags(&expanded, text);
            x = add_text(self, x, string_view(&expanded), item->style);
            string_free(&expanded);
            break;
        }
        default: break;
        }
    }
}

static void place(struct LineBuilder *self)
{
    size_t r;
    int y = 0;
    for (r = 0; r < self->row_count; ++r) {
        struct LineRow row = self->rows[r];
        size_t i;
        int x = 0, width = 0;
        unsigned height = 0;
        for (i = 0; i < row.count; ++i) {
            struct UIWindow *window = self->result->windows[row.begin + i];
            if ((unsigned)window->height > height) height = (unsigned)window->height;
            width += window->width;
        }
        if (self->align == 1) x = self->width;
        else if (self->align == 2) x = (int)((unsigned)(self->width - width) >> 1);
        for (i = 0; i < row.count && self->align <= 2; ++i) {
            size_t at = self->align == 1 ? row.count - i - 1 : i;
            struct UIWindow *window = self->result->windows[row.begin + at];
            if (self->align == 1) x -= window->width;
            ui_window_move(window, x, (int)height - window->height + y);
            if (self->align != 1) x += window->width;
        }
        y += (int)height + self->line_spacing;
    }
    self->result->height = y - self->line_spacing;
}

struct CreateLineInfo *create_line_info_construct(struct CreateLineInfo *self,
    struct UIWindow *wnd, const char *text, int width, int line_spacing,
    struct LineStyle style, BYTE outlined, COLORREF outline_colour, BYTE align)
{
    struct LineBuilder builder = {0};
    size_t i;
    memset(self, 0, sizeof *self);
    if (!wnd || !text) return self;
    builder.result = self;
    builder.wnd = wnd;
    builder.width = width;
    builder.line_spacing = line_spacing;
    builder.colour = builder.default_colour = style.colour;
    builder.font_height = builder.default_font_height = style.font_height;
    builder.bold = style.bold != 0;
    builder.italic = style.italic != 0;
    builder.outlined = outlined;
    builder.outline_colour = outline_colour;
    builder.align = align;
    builder.style_changed = 1;
    string_assign_cstr(&builder.text, text);
    builder.text.size = text_unescape(builder.text.data, builder.text.size);
    parse(&builder);
    layout(&builder);
    place(&builder);
    for (i = 0; i < builder.item_count; ++i) string_free(&builder.items[i].text);
    free(builder.items);
    free(builder.rows);
    free(builder.fonts);
    string_free(&builder.text);
    return self;
}

void create_line_info_destruct(struct CreateLineInfo *self)
{
    free(self->windows);
    memset(self, 0, sizeof *self);
}

struct LineCutterText *line_cutter_text_construct(struct LineCutterText *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &line_cutter_text_ops;
    self->colour = 0;
    self->font_height = TEXT_DEFAULT_HEIGHT;
    self->bold = 0;
    self->italic = 0;
    self->outlined = 0;
    self->outline_colour = TEXT_OUTLINE_DEFAULT;
    string_init(&self->text);
    return self;
}

void line_cutter_text_destruct(struct LineCutterText *self)
{
    self->base.ops = &line_cutter_text_ops;
    string_free(&self->text);
    ui_window_destruct(&self->base);
}

void line_cutter_text_destroy(struct UIWindow *window)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    line_cutter_text_destruct(self);
    free(self);
}

void line_cutter_text_set_text(struct LineCutterText *self, struct TextView text,
                                       struct LineStyle style, BYTE outlined, COLORREF outline_colour)
{
    string_assign(&self->text, text.data, text.size);
    self->colour = style.colour;
    self->font_height = style.font_height;
    self->bold = style.bold;
    self->italic = style.italic;
    self->outlined = outlined;
    self->outline_colour = outline_colour;
    if (outlined == 1) {
        int width = ui_window_text_width(&self->base, string_cstr(&self->text), 0, 0, style.font_height, style.bold,
                                       style.italic);

        ui_window_resize_base(&self->base, width + 2, style.font_height + 2);
    }
    ui_window_invalidate_base(&self->base);
}

void line_cutter_text_on_draw(struct UIWindow *window)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    ui_window_clear_canvas(&self->base, UI_TRANSPARENT);
    if (self->outlined == 1)
        ui_window_text_out_outlined(&self->base, 1, 1, string_cstr(&self->text), (int)self->text.size, self->colour,
                                 self->outline_colour, 0, self->font_height, self->bold, self->italic);
    else
        ui_window_text_out(&self->base, 0, 0, string_cstr(&self->text), (int)self->text.size, 0, self->font_height,
                         self->colour, self->bold, self->italic);
}

#define FORWARD_TO_PARENT(slot)                                                                 \
    do {                                                                                        \
        struct UIWindow *parent = self->base.parent;                                            \
        if (parent)                                                                             \
            parent->ops->slot(parent, x + self->base.x, y + self->base.y);                         \
    } while (0)

void line_cutter_text_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    FORWARD_TO_PARENT(on_l_btn_down);
}

void line_cutter_text_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    FORWARD_TO_PARENT(on_l_btn_dbl_clk);
}

void line_cutter_text_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    FORWARD_TO_PARENT(on_mouse_move);
}

void line_cutter_text_on_mouse_shape(struct UIWindow *window, int x, int y)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    FORWARD_TO_PARENT(on_mouse_shape);
}

void line_cutter_text_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    FORWARD_TO_PARENT(on_l_btn_up);
}

void line_cutter_text_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    FORWARD_TO_PARENT(on_r_btn_down);
}

void line_cutter_text_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    FORWARD_TO_PARENT(on_r_btn_up);
}

void line_cutter_text_on_wheel(struct UIWindow *window, int delta)
{
    struct LineCutterText *self = (struct LineCutterText *)window;
    struct UIWindow *parent = self->base.parent;

    if (parent)
        ui_window_on_wheel(parent, delta);
}
