#include "ui/controls/ui_string.h"
#include "platform/code_page.h"
#include "core/text_format.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "diag/perf.h"
#include "session/session_names.h"

const struct UIWindowOps ui_string_ops = {
    .destroy = ui_string_destroy,
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
    .on_draw = ui_string_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_string_on_mouse_shape,
    .on_l_btn_up = ui_string_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_string_send_msg,
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

#define BLANKS " \t"
#define QUOTE "\""
#define LINK_R 10
#define LINK_G 10
#define LINK_B 0xff
#define ICON_SIZE 0x18
#define TEXT_HEIGHT 0xc

#define WND_MONSTER_DISPLAY 0xc9
#define MSG_MONSTER_DISPLAY_NAME 0x3e
#define WND_QUEST 0x141
#define WND_ITEM_INFO 0xc
#define MSG_ITEM_INFO_SHOW 0x18
#define CURSOR_LINK 2

void ui_string_destruct(struct UIString *self)
{
    self->base.ops = &ui_string_ops;
    string_free(&self->text);
    string_free(&self->font);
    string_free(&self->image);
    string_free(&self->link);
    ui_window_destruct(&self->base);
}

void ui_string_destroy(struct UIWindow *window)
{
    struct UIString *self = (struct UIString *)window;
    ui_string_destruct(self);
    mem_free(self);
}

static void unquote(struct String *value)
{
    string_trim_left(value, BLANKS);
    string_trim_right(value, BLANKS);
    string_trim_left(value, QUOTE);
    string_trim_right(value, QUOTE);
}

static void quoted_value(struct String *tag, size_t from, struct String *out, size_t *end)
{
    size_t open = string_find_first_of(tag, QUOTE, from) + 1;

    *end = string_find_first_of(tag, QUOTE, open);
    string_init(out);
    string_substring(out, tag, open, *end - open);
}

static void blank_value(struct String *tag, size_t from, struct String *out, size_t *end)
{
    size_t start = string_find_first_of(tag, BLANKS, from) + 1;

    *end = string_find_first_of(tag, BLANKS, start);
    string_init(out);
    string_substring(out, tag, start, *end - start);
}

void ui_string_parse(struct UIString *self, struct TextView input)
{
    struct String text, tag, name, value;
    size_t open, close, pos, eq, end;

    string_init(&text);
    string_assign(&text, input.data, input.size);
    string_trim_left(&text, BLANKS);
    if (string_cstr(&text)[0] != '<') {
        string_assign(&self->text, string_cstr(&text), text.size);
        string_free(&text);
        return;
    }

    open = string_find_first_of(&text, "<", 0);
    close = string_find_first_of(&text, ">", open);
    string_init(&tag);
    string_substring(&tag, &text, open, close - open);

    for (pos = 0;;) {
        const char *s = string_cstr(&tag);

        eq = (size_t)-1;
        for (end = pos; end < tag.size; end++)
            if (s[end] == '=') {
                eq = end;
                break;
            }
        if (eq == (size_t)-1)
            break;
        string_init(&name);
        string_substring(&name, &tag, pos, eq - pos);
        if (!name.size) {
            string_free(&name);
            break;
        }
        string_trim_left(&name, BLANKS);
        string_trim_right(&name, BLANKS);

        end = eq;
        if (string_equal_cstr(&name, "image")) {
            quoted_value(&tag, eq, &value, &end);
            string_free(&self->image);
            string_move(&self->image, &value);
            unquote(&self->image);
        } else if (string_equal_cstr(&name, "color")) {
            struct String r, g, b;
            char *stop;

            blank_value(&tag, eq, &value, &end);
            string_init(&r);
            string_substring(&r, &value, 0, 2);
            string_init(&g);
            string_substring(&g, &value, 2, 2);
            string_init(&b);
            string_substring(&b, &value, 4, 2);
            self->r = strtol(string_cstr(&r), &stop, 16);
            self->g = strtol(string_cstr(&g), &stop, 16);
            self->b = strtol(string_cstr(&b), &stop, 16);
            string_free(&b);
            string_free(&g);
            string_free(&r);
            string_free(&value);
        } else if (string_equal_cstr(&name, "link")) {
            quoted_value(&tag, eq, &value, &end);
            string_free(&self->link);
            string_move(&self->link, &value);
            unquote(&self->link);
            self->r = LINK_R;
            self->g = LINK_G;
            self->b = LINK_B;
        } else if (string_equal_cstr(&name, "font")) {
            quoted_value(&tag, eq, &value, &end);
            string_free(&self->font);
            string_move(&self->font, &value);
            unquote(&self->font);
        } else if (string_equal_cstr(&name, "bold")) {
        } else if (string_equal_cstr(&name, "size")) {
            blank_value(&tag, eq, &value, &end);
            self->size = atoi(string_cstr(&value));
            string_free(&value);
        }
        pos = end + 1;
        string_free(&name);
    }

    open = close + 1;
    string_init(&value);
    string_substring(&value, &text, open, string_find_first_of(&text, "<", open) - open);
    string_free(&self->text);
    string_move(&self->text, &value);
    string_free(&tag);
    string_free(&text);
}

void ui_string_set_text(struct UIString *self, int width, int height, struct TextView text)
{
    int w, h;

    ui_window_create(&self->base, width, height);
    if (text.size == 1 && text.data[0] == '\n') {
        ui_window_resize_base(&self->base, 5, 5);
        string_assign(&self->text, text.data, text.size);
    } else {
        ui_string_parse(self, text);
        if (self->image.size) {
            w = ui_window_text_width(&self->base, string_cstr(&self->text), self->text.size, 0, TEXT_HEIGHT,
                                   0, 0);
            ui_window_resize_base(&self->base, w + ICON_SIZE, ICON_SIZE);
        } else {
            h = ui_window_text_height(&self->base, string_cstr(&self->text), self->text.size, 0,
                                    TEXT_HEIGHT, 0, 0);
            w = ui_window_text_width(&self->base, string_cstr(&self->text), self->text.size, 0, TEXT_HEIGHT,
                                   0, 0);
            ui_window_resize_base(&self->base, w, h);
        }
    }
}

void ui_string_on_draw(struct UIWindow *window)
{
    struct UIString *self = (struct UIString *)window;
    COLORREF colour;
    const char *p, *end;

    if (!self->text.size)
        return;
    colour = RGB(self->r, self->g, self->b);
    p = string_cstr(&self->text);
    end = p + self->text.size;
    if (self->image.size) {
        char path[0x80];
        struct BitmapRes *icon;

        memset(path, 0, sizeof path);
        session_get_item_bitmap_file_name(string_cstr(&self->image), path, 0);
        icon = (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
        if (!icon)
            return;
        ui_window_draw_bitmap(&self->base, 0, 0, icon, 1);
        while (p < end)
            p = text_read_colour_code(text_find_colour_code(p, end, g_codePage), end, &colour);
        ui_window_text_out_runs(&self->base, ICON_SIZE, 7, string_cstr(&self->text), self->text.size, &colour, 0,
                   TEXT_HEIGHT, 0);
    } else {
        ui_window_text_out_runs(&self->base, 0, 0, p, self->text.size, &colour, 0, TEXT_HEIGHT, 0);
    }
}

void ui_string_on_mouse_shape(struct UIWindow *window, int x, int y)
{
    struct UIString *self = (struct UIString *)window;
    (void)x;
    (void)y;
    if (self->link.size || self->image.size)
        mode_set_cursor_action(client_current_mode(), CURSOR_LINK);
}

void ui_string_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIString *self = (struct UIString *)window;
    (void)x;
    (void)y;
    if (self->link.size) {
        struct UIWindow *display = ui_window_open(WND_MONSTER_DISPLAY);
        struct UIWindow *quest, *parent;

        if (!display)
            return;
        ui_window_send_msg(display, NULL, MSG_MONSTER_DISPLAY_NAME, (intptr_t)string_cstr(&self->link), 1, 0, 0);
        quest = ui_window_find(WND_QUEST);
        parent = self->base.parent;
        if (quest && parent)
            ui_window_move(display, quest->x + parent->x + 100 + self->base.x, quest->y + parent->y - 60 + self->base.y);
    } else if (self->image.size) {
        struct ItemInfo info;
        struct UIWindow *wnd;

        item_info_construct(&info);
        string_assign(&info.id_text, string_cstr(&self->image), self->image.size);
        wnd = ui_window_open(WND_ITEM_INFO);
        ui_window_send(wnd, NULL, MSG_ITEM_INFO_SHOW, (intptr_t)&info, 0, 0, 0);
        item_info_destruct(&info);
    }
}

intptr_t ui_string_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3,
                                intptr_t val4)
{
    struct UIString *self = (struct UIString *)window;
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}

struct UIString *ui_string_construct(struct UIString *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_string_ops;
    self->joined = 1;
    string_init(&self->link);
    self->r = 0;
    self->g = 0;
    self->b = 0;
    string_init(&self->image);
    string_init(&self->font);
    string_init(&self->text);
    string_clear(&self->link);
    string_clear(&self->image);
    string_clear(&self->font);
    string_clear(&self->text);
    return self;
}

struct UIString *ui_string_create(int width, int height, struct TextView text)
{
    struct UIString *self = mem_alloc(sizeof *self);
    ui_string_construct(self);
    ui_string_set_text(self, width, height, text);
    return self;
}
