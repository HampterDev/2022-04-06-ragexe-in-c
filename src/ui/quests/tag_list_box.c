#include "ui/controls/listbox.h"
#include "ui/controls/buttons.h"
#include "core/text_array.h"
#include "platform/code_page.h"
#include "core/text_format.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "ui/quests/tag_list_box.h"
#include "diag/perf.h"

struct Vec;
struct UITagListBox;
struct UIWindow;
void ui_tag_list_box_destroy(struct UIWindow *window);
void ui_tag_list_box_on_draw(struct UIWindow *self);
void ui_tag_list_box_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_tag_list_box_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_tag_list_box_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_tag_list_box_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
static void ui_tag_list_box_clear(struct UIListBox *base_list);
const struct UIListBoxOps ui_tag_list_box_ops = {
    .base = {
        .destroy = ui_tag_list_box_destroy,
        .resize = ui_list_box_resize,
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
        .on_create = ui_list_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_tag_list_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_tag_list_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_tag_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_tag_list_box_on_l_btn_up,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_tag_list_box_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .create = ui_list_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_tag_list_box_clear,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

#define LINE_COUNT(lb) VEC_COUNT(&(lb)->items, struct String)
#define LINE(lb, i) (&((struct String *)(lb)->items.first)[i])

#define UI_MSG_TAG_BUTTON_CLICK 0x62
#define TAG_BUTTON_TOP 3
#define TAG_LINE_CHARS 35
#define TAG_TEXT_OUTLINED 0x8a
#define TAG_TEXT_OUTLINED_CHARS 0x26
#define TAG_LINK_COLOUR 0xf4934a
#define TAG_LINK_HOVER_COLOUR 0xff7864
#define TAG_NAVI_KIND_DEFAULT 2
#define TAG_NAVI_FLAGS_DEFAULT 5
#define TAG_NAVI_ID_DEFAULT 0x3ea
#define TAG_NAVI_SEARCH (-0xde)

static void ui_tag_list_box_destruct(struct UITagListBox *self)
{
    self->box.base.ops = &ui_tag_list_box_ops.base;
    vec_free(&self->buttons);
    ui_list_box_destruct(&self->box);
}

void ui_tag_list_box_destroy(struct UIWindow *window)
{
    struct UITagListBox *self = (struct UITagListBox *)window;
    ui_tag_list_box_destruct(self);
    mem_free(self);
}

static void ui_tag_list_box_clear(struct UIListBox *base_list)
{
    struct UITagListBox *self = (struct UITagListBox *)base_list;
    struct UIWindow **button;

    ui_list_box_clear_items(&self->box);
    for (button = self->buttons.first; button != self->buttons.last; button++)
        ui_window_remove_child(&self->box.base, *button, 1);
    self->buttons.last = self->buttons.first;
    ui_window_invalidate_base(&self->box.base);
}

void ui_tag_list_box_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_tag_list_box_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y)
{
}

void ui_tag_list_box_on_l_btn_up(struct UIWindow *self, int x, int y)
{
}

void ui_tag_list_box_on_draw(struct UIWindow *window)
{
    struct UITagListBox *self = (struct UITagListBox *)window;
    struct UIListBox *box = &self->box;
    struct UIWindow *wnd = &box->base;
    COLORREF colour = 0;
    int first = box->top, last, per_line, i;

    if (box->transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, RGB(box->r, box->g, box->b));
    }
    per_line = box->line_height - box->extra_lines;
    if (!per_line)
        per_line = 1;
    last = first + wnd->height / per_line;
    if ((unsigned)last >= LINE_COUNT(box))
        last = (int)LINE_COUNT(box);

    for (i = 0; i < first; i++) {
        const char *p = string_cstr(LINE(box, i));
        const char *end = p + LINE(box, i)->size;

        while (p < end)
            p = text_read_colour_code(text_find_colour_code(p, end, g_codePage), end, &colour);
    }
    for (i = 0; first + i < last; i++) {
        struct String *line = LINE(box, first + i);
        int y = box->line_height * i + 4;

        if (!box->outlined)
            ui_window_text_out_runs(wnd, 3, y, string_cstr(line), (int)line->size, &colour, 0, 12, 0);
        else
            ui_window_text_out_outlined(wnd, 3, y, string_cstr(line), (int)line->size, 0, 0xffffff, 0, 12,
                                     0, 0);
    }
}

void ui_tag_list_box_place_buttons(struct UITagListBox *self)
{
    struct UIListBox *box = &self->box;
    int x_offset = box->scroll_x;
    unsigned i;

    for (i = 0; i < VEC_COUNT(&self->buttons, struct UITagButton *); i++) {
        struct UITagButton *button = ((struct UITagButton **)self->buttons.first)[i];
        int line;

        if (!button)
            continue;
        line = button->button.value;
        if (line >= box->top && line < box->top + box->base.height / box->line_height - box->extra_lines) {
            ui_button_set_show((struct UIButton *)(button), 1);
            ui_window_move_base(&button->button.button.base, button->button.button.base.x - box->scroll_x + x_offset,
                     (line - box->top) * box->line_height + TAG_BUTTON_TOP);
        } else {
            ui_button_set_show((struct UIButton *)(button), 0);
        }
    }
}

static struct TagLinkPlace *ui_tag_list_box_link_place(struct UITagListBox *self, struct TagLinkPlace *out, int pos,
                                                   int new_lines)
{
    unsigned count = LINE_COUNT(&self->box), i;
    int sum = 0;

    out->x = 0;
    out->line = 0;
    for (i = count - new_lines; i < count; i++) {
        struct String *line = LINE(&self->box, i);
        struct String plain, head;
        int offset;

        sum += (int)line->size;
        if (sum <= pos)
            continue;
        offset = (int)line->size - sum + pos;
        tag_text_strip_codes(&plain, string_cstr(line), offset);
        out->line = (int)i;
        string_init(&head);
        string_substring(&head, &plain, 0, offset);
        out->x = ui_window_text_width(&self->box.base, string_cstr(&head), 0, 0, 12, 0, 0) + 2;
        string_free(&head);
        string_free(&plain);
        break;
    }
    return out;
}

static void ui_tag_button_split_args(struct UITagButton *button, struct TextView args)
{
    size_t start = 0;
    for (;;) {
        size_t end = start;
        while (end < args.size && args.data[end] != ',') ++end;
        ui_text_button_add_line(&button->button, text_view(args.data + start, end - start));
        if (end == args.size) break;
        start = end + 1;
    }
}

static void ui_tag_list_box_set_button_args(struct UITagButton *button, const struct String *args, int kind)
{
    if (!button || !args->size)
        return;
    switch (kind) {
    case TAG_KIND_ITEML: {
        struct Vec vals = { NULL, NULL, NULL };

        tag_item_link_decode(args, &vals);
        tag_item_link_fill_item(&vals, &button->item);
        vec_free(&vals);
        break;
    }
    case TAG_KIND_ITEM: {
        struct TextView name;

        ui_tag_button_split_args(button, string_view(args));
        button->item.type = atoi(ui_text_button_get_line(&button->button, 1).data);
        button->item.identified = 1;
        button->item.refine = 0;
        name = ui_text_button_get_line(&button->button, 0);
        string_assign(&button->item.id_text, name.data, name.size);
        break;
    }
    case TAG_KIND_NAVI:
        ui_tag_button_split_args(button, string_view(args));
        break;
    }
}

static void ui_tag_list_box_make_button(struct UITagListBox *self, const struct STagInfoV2 *link, int new_lines)
{
    struct UITagButton *button;
    struct TagLinkPlace place;
    const char *label;
    int width;

    if (!link->label.size)
        return;
    button = ui_tag_button_create();
    label = string_cstr(&link->label);
    ui_text_button_set_text(&button->button.button.base, label, TAG_LINK_COLOUR, 0, 12, 0, 1);
    width = ui_window_text_width(&self->box.base, label, 0, 0, 12, 0, 0);
    ui_text_button_fit_width(&button->button, width + 1, button->button.button.base.height);
    button->button.hover_colour = TAG_LINK_HOVER_COLOUR;
    button->button.button.base.id = link->kind;
    ui_tag_list_box_set_button_args(button, &link->args, link->kind);
    ui_tag_list_box_link_place(self, &place, link->start, new_lines);
    button->button.value = place.line;
    ui_window_move_base(&button->button.button.base, place.x, -100);
    ui_window_add_child(&self->box.base, &button->button.button.base);
    vec_push(&self->buttons, &button, sizeof button);
}

static void ui_tag_list_box_layout(struct UITagListBox *self, const struct String *text,
                                const struct Vec *links, int width)
{
    unsigned lines_before = LINE_COUNT(&self->box);
    const char *base = string_cstr(text);
    unsigned consumed = 0;
    const struct STagInfoV2 *link;

    while (text->size > consumed) {
        const char *line = base + consumed;
        struct String piece;
        int len;

        len = tag_text_line_length(line, (int)(text->size - consumed), width);
        consumed = tag_links_snap_to_link((int)consumed + len, links);
        string_init(&piece);
        if (line != base + consumed)
            string_assign(&piece, line, base + consumed - line);
        ui_list_box_add_item(&self->box, string_cstr(&piece));
        string_free(&piece);
    }
    for (link = links->first; link != links->last; link++)
        ui_tag_list_box_make_button(self, link, (int)(LINE_COUNT(&self->box) - lines_before));
    ui_window_send_msg(&self->box.base, NULL, UI_MSG_SCROLL_V, 0, 0, 0, 0);
}

void ui_tag_list_box_add_tagged(struct UITagListBox *self, const char *text, int width)
{
    struct Vec links = { NULL, NULL, NULL };
    struct String whole, plain;

    string_init(&whole);
    string_assign_cstr(&whole, text);
    tag_list_box_split_tags(&plain, &whole, &links);
    string_free(&whole);
    ui_tag_list_box_layout(self, &plain, &links, width);
    string_free(&plain);
    tag_links_free(&links);
}

void ui_tag_list_box_add_text(struct UITagListBox *self, const char *text, int width,
                                     int beside_bar, int wrap)
{
    int line_width = width ? width : TAG_LINE_CHARS;
    unsigned i;

    if (width == TAG_TEXT_OUTLINED) {
        struct TextArray lines = {0};
        text_split_lines(text, &lines, TAG_TEXT_OUTLINED_CHARS);
        for (i = 0; i < lines.size; i++)
            ui_list_box_add_item(&self->box, text_array_at(&lines, i).data);
        text_array_clear(&lines);
        self->box.outlined = 1;
    } else {
        if (wrap) {
            struct TextArray lines = {0};
            int bar = beside_bar ? self->box.vbar->base.width + 2 : 0;

            ui_window_break_lines(&self->box.base, text, &lines, self->box.base.width - bar - 4);
            for (i = 0; i < lines.size; i++)
                ui_list_box_add_item(&self->box, text_array_at(&lines, i).data);
            text_array_clear(&lines);
        } else {
            ui_tag_list_box_add_tagged(self, text, line_width);
        }
        self->box.outlined = 0;
    }
    ui_window_invalidate_base(&self->box.base);
}

void ui_tag_list_box_on_link_click(struct UITagListBox *self, int kind,
                                         struct UITagButton *button)
{
    struct TextView map;
    size_t count;
    int x, y;

    if (!button)
        return;
    switch (kind) {
    case TAG_KIND_ITEM:
    case TAG_KIND_ITEML: {
        struct UIWindow *wnd = ui_window_open(WND_ITEM_DESCRIBE);

        if (wnd)
            ui_window_send_msg(wnd, NULL, ITEM_DESCRIBE_MSG_SHOW, (int)&button->item, 0, 0, 0);
        return;
    }
    case TAG_KIND_NAVI:
        break;
    default:
        return;
    }

    count = ui_text_button_line_count(&button->button);
    if (!count)
        return;
    map = ui_text_button_get_line(&button->button, 0);
    x = atoi(ui_text_button_get_line(&button->button, 1).data);

    y = atoi(ui_text_button_get_line(&button->button, 2).data);

    if ((int)count <= 3) {
        navi_navigate(&g_navi, map.data, TAG_NAVI_KIND_DEFAULT, TAG_NAVI_FLAGS_DEFAULT, 0, x, y,
                             TAG_NAVI_ID_DEFAULT);
    } else {
        int navi_kind, flags_code;

        navi_kind = atoi(ui_text_button_get_line(&button->button, 3).data);

        flags_code = atoi(ui_text_button_get_line(&button->button, 4).data);

        if (flags_code == TAG_NAVI_SEARCH) {
            struct String search = {0};

            string_assign_cstr(&search, "$$DB ");
            string_append(&search, map.data, map.size);
            navi_open_search(&g_navi, string_cstr(&search), search.size, navi_kind, 0);
            string_free(&search);
        } else {
            int silent_arg, id, digits = flags_code;
            BYTE flags = 0;

            silent_arg = atoi(ui_text_button_get_line(&button->button, 5).data);

            id = atoi(ui_text_button_get_line(&button->button, 6).data);

            if (digits >= 100) {
                flags = 1;
                digits -= 100;
            }
            if (digits >= 10) {
                flags += 2;
                digits -= 10;
            }
            if (digits >= 1)
                flags += 4;
            navi_navigate(&g_navi, map.data, navi_kind, flags, silent_arg == 0, x, y, id);
        }
    }
}

int ui_tag_list_box_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                    int val2, int val3, int val4)
{
    struct UITagListBox *self = (struct UITagListBox *)window;
    switch (msg) {
    case 7: case 8: case 9: case 0xa: case 0xb: case 0xc:
        ui_list_box_send_msg((struct UIWindow *)(&self->box), sender, msg, val1, val2, val3, val4);
        ui_tag_list_box_place_buttons(self);
        return 0;
    case UI_MSG_BAR_PAGE_UP:
        if (self->box.has_vscroll)
            ui_window_send_msg(&self->box.base, &self->box.vbar->base, UI_MSG_SCROLL_V, -1, 0, 0, 0);
        return 0;
    case UI_MSG_BAR_PAGE_DOWN:
        if (self->box.has_vscroll)
            ui_window_send_msg(&self->box.base, &self->box.vbar->base, UI_MSG_SCROLL_V, 1, 0, 0, 0);
        return 0;
    case UI_MSG_TAG_ADD_LINE:
        ui_tag_list_box_add_text(self, (const char *)val1, val2, val3, val4);
        return 0;
    case UI_MSG_TAG_BUTTON_CLICK:
        ui_tag_list_box_on_link_click(self, val1, (struct UITagButton *)sender);
        return 0;
    }
    return ui_list_box_send_msg((struct UIWindow *)(&self->box), sender, msg, val1, val2, val3, val4);
}
