#include "game/packet_handlers.h"
#include "ui/quests/tag_text_viewer.h"
#include "ui/window.h"
#include "ui/window_mgr.h"
#include "ui/controls/buttons.h"
#include "ui/controls/scroll.h"
#include "ui/draw.h"
#include "ui/game.h"
#include "navi/navi_manager.h"
#include "game/context.h"
#include "iteminfo/item_api.h"
#include "core/memory.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <shellapi.h>
#include "core/app.h"
#include "diag/perf.h"

void ui_tag_text_viewer_destroy(struct UIWindow *window);
void tag_text_viewer_ui_scroll_on_create(struct UIWindow *self, int width, int height);
void tag_text_viewer_ui_scroll_on_wheel(struct UIWindow *self, int delta);
void ui_tag_text_viewer_on_draw(struct UIWindow *self);
intptr_t ui_tag_text_viewer_send_msg(struct UIWindow *self, struct UIWindow *sender,
                          int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_tag_text_viewer_ops = {
    .destroy = ui_tag_text_viewer_destroy,
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
    .on_create = tag_text_viewer_ui_scroll_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_tag_text_viewer_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = tag_text_viewer_ui_scroll_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_tag_text_viewer_send_msg,
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
#define TAG_LINK_URL 0x1ad
#define TAG_LINK_NAVI 0x1ae
#define TAG_LINK_ITEM 0x1c8
#define TAG_LINK_QUEST 0x213
#define UI_MSG_TAG_BUTTON_CLICK 0x62

#define TAG_PAGE 10

#define ITEM_WND_ID 0xc
#define UI_MSG_SHOW_ITEM 0x18

struct TagTextPosition { struct UIWindow *child; float x, y; };

static struct UITagTextViewer *ui_tag_text_viewer_construct(struct UITagTextViewer *self)
{
    memset(self, 0, sizeof *self);
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_tag_text_viewer_ops;
    self->bg_argb = 0xffffff;
    self->line_spacing = 6;
    self->margin_x = 3;
    self->margin_y = 4;
    self->style.font_height = 12;
    self->outline_colour = 0xffffff;
    self->align_v = 3;
    return self;
}

struct UITagTextViewer *ui_tag_text_viewer_create(int width, int height)
{
    struct UITagTextViewer *self = malloc(sizeof *self);
    if (!self) abort();
    ui_tag_text_viewer_construct(self);
    ui_window_create(&self->base, width, height);
    return self;
}

static void ui_tag_text_viewer_destruct(struct UITagTextViewer *self)
{
    string_free(&self->text);
    free(self->items);
    self->items = NULL;
    self->item_count = self->item_capacity = 0;
    ui_window_destruct(&self->base);
}

void ui_tag_text_viewer_destroy(struct UIWindow *window)
{
    struct UITagTextViewer *self = (struct UITagTextViewer *)window;
    ui_tag_text_viewer_destruct(self);
    free(self);
}

void ui_tag_text_viewer_on_draw(struct UIWindow *window)
{
    struct UITagTextViewer *self = (struct UITagTextViewer *)window;
    ui_window_clear_canvas(&self->base, self->bg_argb);
    if (self->bg_argb == 0xff00ff && self->base.parent)
        ui_window_invalidate(self->base.parent);
}

void ui_tag_text_viewer_remove_all(struct UITagTextViewer *self)
{
    size_t i;
    for (i = 0; i < self->item_count; ++i)
        ui_window_remove_child(&self->base, self->items[i].child, 1);
    self->item_count = 0;
    self->scroll_pos = 0;
    ui_tag_text_viewer_update_scroll(self);
    string_clear(&self->text);
    self->has_text = 0;
}

static unsigned link_arg_count(struct UITextButton *link)
{
    return (unsigned)ui_text_button_line_count(link);
}

static struct TextView link_arg(struct UITextButton *link, unsigned i)
{
    return ui_text_button_get_line(link, i);
}

static int link_arg_number(struct UITextButton *link, unsigned i)
{
    return atoi(link_arg(link, i).data);
}

void ui_tag_text_viewer_on_url_link(struct UITagTextViewer *self, struct UITextButton *link)
{
    struct TextView url;
    (void)self;
    if (!link || !link_arg_count(link)) return;
    url = link_arg(link, 0);
    if (url.size) ShellExecuteA(g_hMainWnd, "open", url.data, NULL, NULL, SW_SHOWNORMAL);
}

void ui_tag_text_viewer_on_item_link(struct UITagTextViewer *self, struct UITextButton *link)
{
    struct TextView name;
    struct ItemInfo item;
    struct UIWindow *wnd;
    (void)self;
    if (!link || !link_arg_count(link)) return;
    name = link_arg(link, 0);
    item_info_construct(&item);
    item.identified = 1;
    item.type = link_arg_number(link, 1);
    string_assign(&item.id_text, name.data, name.size);
    wnd = ui_make_window(ITEM_WND_ID);
    if (wnd) ui_window_send_msg(wnd, NULL, UI_MSG_SHOW_ITEM, (intptr_t)&item, 0, 0, 0);
    item_info_destruct(&item);
}

void ui_tag_text_viewer_on_quest_link(struct UITagTextViewer *self, struct UITextButton *link)
{
    int number;

    (void)self;
    if (!link || !link_arg_count(link))
        return;
    number = link_arg_number(link, 0);
    game_mode_ui_open_v2(game_current_mode(), 6, number);
}

static void ui_tag_text_viewer_on_link(struct UITagTextViewer *self, struct UITextButton *link, int id)
{
    switch (id) {
    case TAG_LINK_NAVI:
        ui_tag_text_viewer_on_navi_link(self, link);
        break;
    case TAG_LINK_URL:
        ui_tag_text_viewer_on_url_link(self, link);
        break;
    case TAG_LINK_ITEM:
        ui_tag_text_viewer_on_item_link(self, link);
        break;
    case TAG_LINK_QUEST:
        ui_tag_text_viewer_on_quest_link(self, link);
        break;
    }
}

intptr_t ui_tag_text_viewer_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                       intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UITagTextViewer *self = (struct UITagTextViewer *)window;
    int old_pos;

    if (msg == UI_MSG_TAG_ADD_LINE) {
        ui_tag_text_viewer_append_text(self, (const char *)val1);
    } else if (msg == UI_MSG_TAG_BUTTON_CLICK) {
        switch (val1) {
        case TAG_LINK_URL:
        case TAG_LINK_NAVI:
        case TAG_LINK_ITEM:
        case TAG_LINK_QUEST:
            ui_tag_text_viewer_on_link(self, (struct UITextButton *)sender, val1);
            break;
        }
    }

    switch (msg) {
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old_pos = self->scroll_pos;
        if (msg == UI_MSG_PAGE_UP)
            self->scroll_pos = old_pos - TAG_PAGE;
        else if (msg == UI_MSG_PAGE_DOWN)
            self->scroll_pos = old_pos + TAG_PAGE;
        else
            self->scroll_pos = old_pos + val1;
        if (self->scroll_pos != old_pos)
            ui_tag_text_viewer_update_scroll(self);
        return self->scroll_pos - old_pos;
    case UI_MSG_BAR_PAGE_UP:
        ui_tag_text_viewer_send_msg(&self->base, self->vbar, UI_MSG_PAGE_UP, 0, 0, 0, 0);
        break;
    case UI_MSG_BAR_PAGE_DOWN:
        ui_tag_text_viewer_send_msg(&self->base, self->vbar, UI_MSG_PAGE_DOWN, 0, 0, 0, 0);
        break;
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}

static int tag_item_bottom(int total, const struct TagTextPosition *item)
{
    float low = (float)total;
    float bottom = (float)item->child->height + item->y;
    return (int)(low > bottom ? low : bottom);
}

static void tag_item_set(struct UITagTextViewer *self, struct UIWindow *child, float x, float y)
{
    size_t first = 0, last = self->item_count;
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        if ((uintptr_t)self->items[middle].child < (uintptr_t)child) first = middle + 1;
        else last = middle;
    }
    if (first == self->item_count || self->items[first].child != child) {
        if (self->item_count == self->item_capacity) {
            size_t limit = SIZE_MAX / sizeof *self->items;
            size_t capacity = self->item_capacity ? self->item_capacity : 8;
            struct TagTextPosition *items;
            if (self->item_count == limit) abort();
            if (capacity <= self->item_count) capacity = capacity > limit / 2 ? limit : capacity * 2;
            items = realloc(self->items, capacity * sizeof *items);
            if (!items) abort();
            self->items = items;
            self->item_capacity = capacity;
        }
        memmove(self->items + first + 1, self->items + first,
                (self->item_count - first) * sizeof *self->items);
        ++self->item_count;
        self->items[first].child = child;
    }
    self->items[first].x = x;
    self->items[first].y = y;
}

int ui_tag_text_viewer_content_height(struct UITagTextViewer *self)
{
    size_t i;
    int total = 0;
    for (i = 0; i < self->item_count; ++i) total = tag_item_bottom(total, self->items + i);
    return total;
}

void ui_tag_text_viewer_update_scroll(struct UITagTextViewer *self)
{
    size_t i;
    int total = self->base.height;
    if (!self->vbar) return;
    for (i = 0; i < self->item_count; ++i) total = tag_item_bottom(total, self->items + i);
    if (total > self->base.height) {
        int max_pos = total - self->base.height;
        int pos;
        ui_window_set_show_base(self->vbar, 1);
        ui_scroll_bar_set_range((struct UIScrollBar *)self->vbar, max_pos, self->base.height, total);
        pos = self->scroll_pos < 0 ? 0 : self->scroll_pos;
        if (pos > max_pos) pos = max_pos;
        self->scroll_pos = pos;
        ui_scroll_bar_set_pos((struct UIScrollBar *)self->vbar, pos);
        for (i = 0; i < self->item_count; ++i)
            ui_window_move(self->items[i].child, (int)self->items[i].x,
                     (int)(self->items[i].y - (float)self->scroll_pos));
    } else {
        ui_window_set_show_base(self->vbar, 0);
    }
    ui_window_invalidate_base(&self->base);
    if (self->base.parent) ui_window_invalidate(self->base.parent);
}

void ui_tag_text_viewer_set_align(struct UITagTextViewer *self, unsigned char align)
{
    if (align <= 2) self->align_h = align;
    else if (align <= 5) self->align_v = align;
}

void ui_tag_text_viewer_append_text(struct UITagTextViewer *self, const char *text)
{
    struct String joined = {0};
    if (!text) return;
    string_assign(&joined, string_cstr(&self->text), self->text.size);
    if (self->has_text) string_append_cstr(&joined, "\n");
    string_append_cstr(&joined, text);
    string_free(&self->text);
    self->text = joined;
    self->has_text = 1;
    ui_tag_text_viewer_layout(self);
}

void ui_tag_text_viewer_layout(struct UITagTextViewer *self)
{
    struct String text = {0};
    struct CreateLineInfo info;
    size_t i;
    int y0;

    string_assign(&text, string_cstr(&self->text), self->text.size);
    ui_tag_text_viewer_remove_all(self);
    string_assign(&self->text, string_cstr(&text), text.size);
    self->has_text = 1;
    if (!self->wrap_width) self->wrap_width = self->base.width - self->margin_x - 13;
    create_line_info_construct(&info, &self->base, string_cstr(&text), self->wrap_width,
                               self->line_spacing, self->style, self->outlined, self->outline_colour, self->align_h);

    y0 = self->margin_y > 0 ? self->margin_y : 0;
    if ((unsigned)self->base.height > (unsigned)(info.height + self->margin_y)) {
        BYTE valign = self->align_v;

        if (valign == 5)
            y0 = self->base.height - info.height - self->margin_y;
        else if (valign == 4)
            y0 = (int)((float)(unsigned)(self->base.height - info.height) * 0.5f);
    }

    for (i = 0; i < info.count; ++i) {
        struct UIWindow *window = info.windows[i];
        ui_window_move(window, self->margin_x + window->x, y0 + window->y);
        ui_window_add_child(&self->base, window);
        tag_item_set(self, window, (float)window->x, (float)window->y);
    }

    ui_tag_text_viewer_update_scroll(self);
    create_line_info_destruct(&info);
    string_free(&text);
}

#define NAVI_LINK_SEARCH (-0xde)

void ui_tag_text_viewer_on_navi_link(struct UITagTextViewer *self, struct UITextButton *link)
{
    struct TextView map;
    struct String text = {0};
    int x, y;

    (void)self;
    if (!link || !link_arg_count(link))
        return;
    map = link_arg(link, 0);
    x = link_arg_number(link, 1);
    y = link_arg_number(link, 2);
    if ((int)link_arg_count(link) > 3) {
        int kind = link_arg_number(link, 3);
        int link_kind = link_arg_number(link, 4);

        if (link_kind == NAVI_LINK_SEARCH) {
            string_assign_cstr(&text, "$$DB ");
            string_append(&text, map.data, map.size);
            navi_open_search(&g_navi, string_cstr(&text), text.size, kind, 0);
            string_free(&text);
        } else {
            int silent = link_arg_number(link, 5);
            int id = link_arg_number(link, 6);
            int flags = 0;

            if (link_kind >= 100) {
                flags = 1;
                link_kind -= 100;
            }
            if (link_kind >= 10) {
                flags += 2;
                link_kind -= 10;
            }
            if (link_kind >= 1)
                flags += 4;
            navi_navigate(&g_navi, map.data, kind, flags, silent == 0, x, y, id);
        }
    } else {
        const char *name = map.data;

        navi_navigate(&g_navi, name, 2, 5, 0, x, y, 0x3ea);
    }
}

void tag_text_viewer_ui_scroll_on_create(struct UIWindow *window, int width, int height)
{
    struct UITagTextViewer *self = (struct UITagTextViewer *)window;
    struct UIScrollBar *bar = (struct UIScrollBar *)mem_alloc(sizeof(struct UIScrollBar));

    if (!bar) abort();
    bar = ui_scroll_bar_construct(bar);
    self->vbar = (struct UIWindow *)bar;
    ui_scroll_bar_create(bar, width - 13, 0, 1, height);
    ui_window_add_child(&self->base, self->vbar);
    ui_window_set_show_base(self->vbar, 0);
    ui_tag_text_viewer_update_scroll(self);
}

void tag_text_viewer_ui_scroll_on_wheel(struct UIWindow *window, int delta)
{
    struct UITagTextViewer *self = (struct UITagTextViewer *)window;
    ui_tag_text_viewer_send_msg(&self->base, self->vbar, delta < 0 ? UI_MSG_PAGE_DOWN : UI_MSG_PAGE_UP, 0, 0, 0, 0);
}
