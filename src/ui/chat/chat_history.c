#include "ui/controls/listbox.h"
#include "ui/controls/scroll.h"
#include "ui/chat/markup_links.h"
#include "ui/items/item_tags.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/markup_tags.h"
#include "ui/chat/chat_history.h"
#include <stdio.h>
#include <string.h>
#include "platform/code_page.h"
#include "diag/perf.h"
#include "ui/ui_std.h"
#include <time.h>

struct Vec;
struct UIChatHisBox;
struct UIWindow;
void ui_chat_his_box_destroy(struct UIWindow *window);
BYTE ui_chat_his_box_is_click_through(struct UIWindow *self);
void ui_chat_his_box_on_create(struct UIWindow *self, int width, int height);
void ui_chat_his_box_on_draw(struct UIWindow *self);
void ui_chat_his_box_on_l_btn_down(struct UIWindow *self, int x, int y);
static void ui_chat_his_box_create(struct UIListBox *base_list, int x, int y, int width, int height, BYTE transparent_unused);
const struct UIListBoxOps ui_chat_his_box_ops = {
    .base = {
        .destroy = ui_chat_his_box_destroy,
        .resize = ui_chat_his_box_resize,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_chat_his_box_is_click_through,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_chat_his_box_set_show,
        .on_create = ui_chat_his_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_chat_his_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_chat_his_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_chat_his_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_chat_his_box_on_l_btn_up,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_chat_his_box_send_msg,
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
    },
    .create = ui_chat_his_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_chat_his_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

struct UIChatHisBox2;
void ui_chat_his_box2_destroy(struct UIWindow *window);
void ui_chat_his_box2_on_draw(struct UIWindow *self);
void ui_chat_his_box2_on_r_btn_up(struct UIWindow *self, int x, int y);
const struct UIListBoxOps ui_chat_his_box2_ops = {
    .base = {
        .destroy = ui_chat_his_box2_destroy,
        .resize = ui_chat_his_box_resize,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_chat_his_box2_is_click_through,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_chat_his_box_set_show,
        .on_create = ui_chat_his_box2_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_chat_his_box2_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_chat_his_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_chat_his_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_chat_his_box_on_l_btn_up,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_chat_his_box2_on_r_btn_up,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_chat_his_box_send_msg,
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
    },
    .create = ui_chat_his_box2_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_chat_his_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

struct UIChatHisBox3;
void ui_chat_his_box3_destroy(struct UIWindow *window);
void ui_chat_his_box3_resize(struct UIWindow *self, int width, int height);
void ui_chat_his_box3_on_create(struct UIWindow *self, int width, int height);
void ui_chat_his_box3_on_draw(struct UIWindow *self);
const struct UIListBoxOps ui_chat_his_box3_ops = {
    .base = {
        .destroy = ui_chat_his_box3_destroy,
        .resize = ui_chat_his_box3_resize,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_chat_his_box2_is_click_through,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_chat_his_box_set_show,
        .on_create = ui_chat_his_box3_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_chat_his_box3_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_chat_his_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_chat_his_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_chat_his_box_on_l_btn_up,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_chat_his_box2_on_r_btn_up,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_chat_his_box_send_msg,
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
    },
    .create = ui_chat_his_box2_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_chat_his_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

#define LINE_COUNT(box) ((unsigned)VEC_COUNT(&(box)->list.items, struct String))
#define LINE(box, i) (&((struct String *)(box)->list.items.first)[i])
#define STRINGS(v) ((struct String *)(v)->first)
#define STRING_COUNT(v) ((unsigned)VEC_COUNT(v, struct String))
#define DWORDS(v) ((DWORD *)(v)->first)
#define DWORD_COUNT(v) ((unsigned)VEC_COUNT(v, DWORD))
#define WINDOWS(v) ((struct UIWindow **)(v)->first)
#define WINDOW_COUNT(v) ((unsigned)VEC_COUNT(v, struct UIWindow *))

static void push_string(struct Vec *v, const char *text)
{
    string_vec_push_text(v, text, strlen(text));
}

static int has_item_link(const char *text)
{
    return platform_code_page_find(text, string_cstr(&text_tag_ITEML)) ||
           platform_code_page_find(text, string_cstr(&text_tag_ITEML_END));
}

static int rows_in_view(struct UIChatHisBox *self)
{
    return self->list.base.height / self->list.line_height;
}

static int view_end(struct UIChatHisBox *self)
{
    unsigned end = (unsigned)(rows_in_view(self) - self->list.extra_lines + self->list.top);

    return (int)(end < LINE_COUNT(self) ? end : LINE_COUNT(self));
}

static int bottom_top(struct UIChatHisBox *self)
{
    return (int)LINE_COUNT(self) - rows_in_view(self) + self->list.extra_lines;
}

static void scroll_to_bottom(struct UIChatHisBox *self)
{
    int top = bottom_top(self);

    self->list.top = top;
    self->list.vbar->pos = top;
    ui_window_invalidate_base(&self->list.vbar->base);
}

static void destroy_windows(void *value)
{
    vec_free(value);
}

static void remove_all_item_tags(struct UIChatHisBox *self)
{
    struct ListNode *node = self->item_tags.head->next;
    unsigned i;

    while (node != self->item_tags.head) {
        struct Vec *windows = LIST_VALUE(node, struct Vec);

        for (i = 0; i < WINDOW_COUNT(windows); i++)
            ui_window_remove_child(&self->list.base, WINDOWS(windows)[i], 1);
        node = list_erase(&self->item_tags, node, destroy_windows);
    }
}

static void drop_item_tags_first_half(struct UIChatHisBox *self)
{
    int half = (int)(LINE_COUNT(self) >> 1);
    struct ListNode *node = self->item_tags.head->next;
    struct UIWindow *tag;
    unsigned i;
    BYTE removed;

    while (node != self->item_tags.head) {
        struct Vec *windows = LIST_VALUE(node, struct Vec);

        removed = 0;
        for (i = 0; i < WINDOW_COUNT(windows); i++) {
            tag = WINDOWS(windows)[i];
            ((struct UIItemTagButton *)tag)->line -= half;
            if (((struct UIItemTagButton *)WINDOWS(windows)[i])->line < 0) {
                removed = 1;
                ui_window_remove_child(&self->list.base, WINDOWS(windows)[i], 1);
            }
        }
        if (removed)
            node = list_erase(&self->item_tags, node, destroy_windows);
        else
            node = node->next;
    }
}

static void drop_navi_links_first_half(struct UIChatHisBox *self)
{
    int half = (int)(LINE_COUNT(self) >> 1);
    struct ListNode *node = self->navi_links.head->next;
    struct UIWindow *btn;
    unsigned i;
    BYTE removed;

    while (node != self->navi_links.head) {
        struct Vec *windows = LIST_VALUE(node, struct Vec);

        removed = 0;
        for (i = 0; i < WINDOW_COUNT(windows); i++) {
            btn = WINDOWS(windows)[i];
            if (btn->id != UI_ID_NAVI_LINK)
                continue;
            ((struct UINaviLinkButton *)btn)->line -= half;
            if (((struct UINaviLinkButton *)btn)->line < 0) {
                removed = 1;
                ui_window_remove_child(&self->list.base, WINDOWS(windows)[i], 1);
            }
        }
        if (removed)
            node = list_erase(&self->navi_links, node, destroy_windows);
        else
            node = node->next;
    }
}

static void place_item_tags(struct UIChatHisBox *self, int line)
{
    int top = self->list.top;
    int end = view_end(self);
    struct ListNode *node;
    struct UIWindow *tag;
    unsigned i;

    for (node = self->item_tags.head->next; node != self->item_tags.head; node = node->next) {
        struct Vec *windows = LIST_VALUE(node, struct Vec);

        for (i = 0; i < WINDOW_COUNT(windows); i++) {
            tag = WINDOWS(windows)[i];
            if (((struct UIItemTagButton *)tag)->line < top || ((struct UIItemTagButton *)tag)->line > end - 1)
                ui_window_move(tag, -100, -100);
            else if (((struct UIItemTagButton *)tag)->line == line)
                ui_window_move(tag, ((struct UIItemTagButton *)tag)->line_x + 1, (line - top) * self->list.line_height + 2);
        }
    }
}

static void place_navi_links(struct UIChatHisBox *self, int line)
{
    int top = self->list.top;
    int end = view_end(self);
    struct ListNode *node;
    struct UIWindow *btn;
    unsigned i;

    for (node = self->navi_links.head->next; node != self->navi_links.head; node = node->next) {
        struct Vec *windows = LIST_VALUE(node, struct Vec);

        for (i = 0; i < WINDOW_COUNT(windows); i++) {
            btn = WINDOWS(windows)[i];
            if (btn->id != UI_ID_NAVI_LINK)
                continue;
            if (((struct UINaviLinkButton *)btn)->line < top || ((struct UINaviLinkButton *)btn)->line > end - 1)
                ui_window_move(btn, -100, -100);
            else if (((struct UINaviLinkButton *)btn)->line == line)
                ui_window_move(btn, ((struct UINaviLinkButton *)btn)->line_x + 1, (line - top) * self->list.line_height + 2);
        }
    }
}

static void keep_newer_strings(struct Vec *v)
{
    unsigned half = STRING_COUNT(v) >> 1;
    unsigned i;

    for (i = 0; i < half; i++)
        string_assign(&STRINGS(v)[i], string_cstr(&STRINGS(v)[half + i]), (&STRINGS(v)[half + i])->size);
    string_vec_resize(v, half);
}

static void keep_newer_dwords(struct Vec *v)
{
    int half = (int)(DWORD_COUNT(v) >> 1);
    int i;

    for (i = 0; i < half; i++)
        DWORDS(v)[i] = DWORDS(v)[half + i];
    vec_resize(v, (size_t)half, sizeof(DWORD));
}

static void drop_lines_first_half(struct UIChatHisBox *self)
{
    keep_newer_strings(&self->list.items);
    keep_newer_dwords(&self->colours);
    keep_newer_strings(&self->line_extra);
    ui_list_box_update_scroll_bar(&self->list);
}

static void drop_log_first_half(struct UIChatHisBox *self)
{
    keep_newer_strings(&self->log_text);
    keep_newer_dwords(&self->log_colour);
    keep_newer_strings(&self->log_extra);
    ui_chat_his_box_rebuild(self);
}

struct UIChatHisBox *ui_chat_his_box_construct(struct UIChatHisBox *self)
{
    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_chat_his_box_ops.base;
    list_init(&self->item_tags);
    list_init(&self->navi_links);
    memset(&self->colours, 0, sizeof self->colours);
    memset(&self->line_extra, 0, sizeof self->line_extra);
    memset(&self->log_text, 0, sizeof self->log_text);
    memset(&self->log_colour, 0, sizeof self->log_colour);
    memset(&self->log_extra, 0, sizeof self->log_extra);
    self->max_lines = CHAT_HISTORY_MAX_LINES;
    self->list.line_height = 0xe;
    vec_reserve(&self->list.items, CHAT_HISTORY_MAX_LINES, sizeof(struct String));
    vec_reserve(&self->colours, CHAT_HISTORY_MAX_LINES, sizeof(DWORD));
    vec_reserve(&self->line_extra, CHAT_HISTORY_MAX_LINES, sizeof(struct String));
    vec_reserve(&self->log_text, CHAT_HISTORY_MAX_LINES, sizeof(struct String));
    vec_reserve(&self->log_colour, CHAT_HISTORY_MAX_LINES, sizeof(DWORD));
    vec_reserve(&self->log_extra, CHAT_HISTORY_MAX_LINES, sizeof(struct String));
    return self;
}

void ui_chat_his_box_destruct(struct UIChatHisBox *self)
{
    self->list.base.ops = &ui_chat_his_box_ops.base;
    string_vec_free(&self->log_extra);
    vec_free(&self->log_colour);
    string_vec_free(&self->log_text);
    string_vec_free(&self->line_extra);
    vec_free(&self->colours);
    list_free(&self->navi_links, destroy_windows);
    list_free(&self->item_tags, destroy_windows);
    ui_list_box_destruct(&self->list);
}

void ui_chat_his_box_destroy(struct UIWindow *window)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)window;
    ui_chat_his_box_destruct(self);
    mem_free(self);
}

void ui_chat_his_box_resize(struct UIWindow *window, int width, int height)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)window;
    ui_list_box_resize((struct UIWindow *)(&self->list), width, height);
    if (self->list.has_vscroll) {
        int top = bottom_top(self);

        self->list.top = top;
        self->list.vbar->pos = top;
        ui_window_move_base(&self->list.vbar->base, width - 13, 0);
        ui_window_invalidate_base(&self->list.vbar->base);
    } else {
        self->list.top = 0;
        self->list.vbar->pos = 0;
        ui_window_move_base(&self->list.vbar->base, width - 13, 0);
    }
}

BYTE ui_chat_his_box_is_click_through(struct UIWindow *self)
{
    return 1;
}

void ui_chat_his_box_set_show(struct UIWindow *window, int visible)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)window;
    self->list.base.visible = visible;
    if (visible)
        ui_window_invalidate_base(&self->list.base);
}

static struct UIScrollBar *add_bar(struct UIChatHisBox *self, const struct UIWindowOps *ops, int button_size,
                                   int orientation, int x, int y, int length)
{
    void *mem = mem_alloc(sizeof(struct UIScrollBar));
    struct UIScrollBar *bar = NULL;
    int thickness;

    if (mem) {
        bar = ui_scroll_bar_construct(mem);
        if (ops) {
            bar->base.ops = ops;
            bar->button_size = button_size;
        }
    }
    thickness = bar->thickness;
    bar->orientation = orientation;
    if (orientation == UI_SCROLL_VERT) {
        ui_window_resize_base(&bar->base, thickness, length);
        ui_window_move_base(&bar->base, x, y);
        ui_window_on_create_base(&bar->base, thickness, length);
    } else {
        ui_window_resize_base(&bar->base, length, thickness);
        ui_window_move_base(&bar->base, x, y);
        ui_window_on_create_base(&bar->base, length, thickness);
    }
    return bar;
}

void ui_chat_his_box_on_create(struct UIWindow *window, int width, int height)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)window;
    self->list.vbar = add_bar(self, &ui_scroll_bar2_ops, 10, UI_SCROLL_VERT, width - 13, 0, height);
    ui_window_add_child(&self->list.base, &self->list.vbar->base);
    self->list.hbar = add_bar(self, &ui_scroll_bar2_ops, 10, UI_SCROLL_HORZ, 0, height - 13, width - 13);
    ui_window_add_child(&self->list.base, &self->list.hbar->base);
}

static int right_aligned(void)
{
    return g_serviceType == SERVICE_UAE && g_windowMgr.arabic_text;
}

void ui_chat_his_box_on_draw(struct UIWindow *window)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)window;
    struct UIWindow *wnd = &self->list.base;
    int first, end, row, i, w;
    struct String *line;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (wnd->parent && wnd->x >= 0 && wnd->y >= 0)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_HISTORY_DRAW, 0, 0, 0, 0);
    first = self->list.top;
    end = view_end(self);
    for (row = 0, i = first; i < end; row++, i++) {
        line = LINE(self, i);
        if (right_aligned()) {
            w = ui_window_text_width(wnd, string_cstr(line), 0, 0, 12, 0, 0);
            ui_window_text_out(wnd, wnd->width - w - 0x12, self->list.line_height * row + 4,
                             string_cstr(line), 0, 0, 12, 0, 0, 0);
            ui_window_text_out(wnd, wnd->width - w - 0x13, self->list.line_height * row + 3,
                             string_cstr(line), 0, 0, 12, DWORDS(&self->colours)[i], 0, 0);
        } else {
            ui_window_text_out(wnd, 3, self->list.line_height * row + 4, string_cstr(line), 0, 0, 12, 0,
                             0, 0);
            ui_window_text_out(wnd, 2, self->list.line_height * row + 3, string_cstr(line), 0, 0, 12,
                             DWORDS(&self->colours)[i], 0, 0);
        }
        place_navi_links(self, i);
        place_item_tags(self, i);
    }
}

void ui_chat_his_box_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)window;
    self->list.selected = ui_list_box_hit_line(&self->list, x, y);
}

const char *ui_chat_his_box_get_line_extra(struct UIChatHisBox *self, unsigned i)
{
    struct String *extra;

    if (i >= STRING_COUNT(&self->line_extra))
        return NULL;
    extra = &STRINGS(&self->line_extra)[i];
    if (!extra->size)
        return NULL;
    return string_cstr(extra);
}

int ui_chat_his_box_save_to_file(struct UIChatHisBox *self, const char *path)
{
    static const char newline = '\n';
    FILE *file = fopen(path, "w");
    int count, i;

    if (!file)
        return 0;
    count = (int)STRING_COUNT(&self->list.items);
    for (i = 0; i < count; i++) {
        const struct String *line = &STRINGS(&self->list.items)[i];

        fwrite(string_cstr(line), line->size, 1, file);
        fwrite(&newline, 1, 1, file);
    }
    fclose(file);
    return 1;
}

void ui_chat_his_box_on_l_btn_dbl_clk(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)window;
    int i = self->list.selected;
    struct UIWindow *parent;

    if (i == -1)
        return;
    if ((unsigned)i >= STRING_COUNT(&self->line_extra) || !STRINGS(&self->line_extra)[i].size)
        return;
    if (!string_cstr(&STRINGS(&self->line_extra)[i]))
        return;
    parent = self->list.base.parent;
    ui_window_send_msg(parent, &self->list.base, UI_MSG_COMMAND, UI_ID_HISTORY_LINE,
             (int)ui_chat_his_box_get_line_extra(self, (unsigned)self->list.selected), 0, 0);
}

void ui_chat_his_box_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused)
{
}

int ui_chat_his_box_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)window;
    return ui_list_box_send_msg((struct UIWindow *)(&self->list), sender, msg, val1, val2, val3, val4);
}

static void ui_chat_his_box_create(struct UIListBox *base_list, int x, int y, int width, int height,
                                   BYTE transparent_unused)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)base_list;
    self->list.transparent = 1;
    ui_window_resize_base(&self->list.base, width, height);
    ui_window_move_base(&self->list.base, x, y);
    ui_window_on_create(&self->list.base, width, height);
}

void ui_chat_his_box_clear_items(struct UIListBox *base_list)
{
    struct UIChatHisBox *self = (struct UIChatHisBox *)base_list;
    ui_list_box_clear_items(&self->list);
    self->colours.last = self->colours.first;
    string_vec_clear(&self->line_extra);
}

void ui_chat_his_box_add_line(struct UIChatHisBox *self, const char *text, COLORREF colour, const char *extra)
{
    BYTE at_bottom = self->list.has_vscroll ? self->list.top == bottom_top(self) : 1;
    int break_mode = 4;

    if (LINE_COUNT(self) >= self->max_lines) {
        drop_navi_links_first_half(self);
        drop_item_tags_first_half(self);
        drop_lines_first_half(self);
    }
    push_string(&self->line_extra, extra ? extra : "");
    vec_push(&self->colours, &colour, sizeof colour);
    if (has_item_link(text)) {
        ui_chat_his_box_add_item_link_line(self, text, &colour, &break_mode);
    } else {
        if (navi_links_has_link(text_view_cstr(text)))
            ui_chat_his_box_add_navi_link_line(self, text, &colour, &break_mode);
        else
            ui_list_box_add_item(&self->list, text);
    }
    if (self->list.has_vscroll && at_bottom)
        scroll_to_bottom(self);
    ui_window_invalidate_base(&self->list.base);
}

void ui_chat_his_box_add_wrapped(struct UIChatHisBox *self, const char *text, COLORREF colour,
                                        const char *extra)
{
    struct TextArray lines = { 0 };
    int width = self->list.vbar ? self->list.base.width - 16 : self->list.base.width;
    unsigned i;

    if (navi_links_has_link(text_view_cstr(text))) {
        ui_chat_his_box_add_line(self, text, colour, extra);
    } else {
        ui_window_break_lines(&self->list.base, text, &lines, width);
        for (i = 0; i < lines.size; i++)
            ui_chat_his_box_add_line(self, text_array_at(&lines, i).data, colour, extra);
    }
    text_array_clear(&lines);
}

void ui_chat_his_box_log_message(struct UIChatHisBox *self, const char *text, COLORREF colour,
                                        const char *extra)
{
    if (STRING_COUNT(&self->log_text) >= self->max_lines)
        drop_log_first_half(self);
    push_string(&self->log_text, text);
    vec_push(&self->log_colour, &colour, sizeof colour);
    push_string(&self->log_extra, extra);
}

void ui_chat_his_box_rebuild(struct UIChatHisBox *self)
{
    unsigned i;

    ui_chat_his_box_clear_items(&self->list);
    remove_all_item_tags(self);
    for (i = 0; i < STRING_COUNT(&self->log_text); i++) {
        struct String *text = &STRINGS(&self->log_text)[i];
        const char *extra = string_cstr(&STRINGS(&self->log_extra)[i]);

        if (text_view_find(string_view(text), string_view(&text_tag_ITEML), 0) != STRING_NPOS)
            ui_chat_his_box_add_line(self, string_cstr(text), DWORDS(&self->log_colour)[i], extra);
        else
            ui_chat_his_box_add_wrapped(self, string_cstr(text), DWORDS(&self->log_colour)[i], extra);
    }
    ui_list_box_update_scroll_bar(&self->list);
}

void ui_chat_his_box_resize_to(struct UIChatHisBox *self, int width, int height, BYTE rebuild)
{
    int old_width = self->list.base.width;

    ui_window_resize(&self->list.base, width, height);
    if (old_width != self->list.base.width && rebuild)
        ui_chat_his_box_rebuild(self);
}

void ui_chat_his_box_copy_log(struct UIChatHisBox *self, struct UIChatHisBox *src)
{
    if (&self->log_text != &src->log_text)
        string_vec_assign(&self->log_text, src->log_text.first, src->log_text.last);
    if (&self->log_colour != &src->log_colour)
        vec_assign(&self->log_colour, src->log_colour.first, src->log_colour.last, sizeof(DWORD));
    if (&self->log_extra != &src->log_extra)
        string_vec_assign(&self->log_extra, src->log_extra.first, src->log_extra.last);
    ui_chat_his_box_rebuild(self);
}

struct UIChatHisBox2 *ui_chat_his_box2_construct(struct UIChatHisBox2 *self)
{
    ui_chat_his_box_construct(&self->box);
    self->box.list.base.ops = &ui_chat_his_box2_ops.base;
    self->back_r = 0xff;
    self->back_g = 0xff;
    self->back_b = 0xff;
    return self;
}

void ui_chat_his_box2_destruct(struct UIChatHisBox2 *self)
{
    self->box.list.base.ops = &ui_chat_his_box2_ops.base;
    ui_chat_his_box_destruct(&self->box);
}

void ui_chat_his_box2_destroy(struct UIWindow *window)
{
    struct UIChatHisBox2 *self = (struct UIChatHisBox2 *)window;
    ui_chat_his_box2_destruct(self);
    mem_free(self);
}

BYTE ui_chat_his_box2_is_click_through(struct UIWindow *self)
{
    return 0;
}

void ui_chat_his_box2_on_create(struct UIWindow *window, int width, int height)
{
    struct UIChatHisBox2 *self = (struct UIChatHisBox2 *)window;
    struct UIChatHisBox *box = &self->box;

    box->list.vbar = add_bar(box, NULL, 0, UI_SCROLL_VERT, width - 13, 0, height);
    ui_window_add_child(&box->list.base, &box->list.vbar->base);
    box->list.hbar = add_bar(box, &ui_scroll_bar2_ops, 10, UI_SCROLL_HORZ, 0, height - 13, width - 13);
    ui_window_add_child(&box->list.base, &box->list.hbar->base);
}

void ui_chat_his_box2_on_draw(struct UIWindow *window)
{
    struct UIChatHisBox2 *self = (struct UIChatHisBox2 *)window;
    struct UIChatHisBox *box = &self->box;
    struct UIWindow *wnd = &box->list.base;
    int i = box->list.top;
    int end = view_end(box);
    int row, x;
    struct String *line;

    ui_window_clear_canvas(wnd, RGB(self->back_r, self->back_g, self->back_b));
    for (row = 0; i < end; row++, i++) {
        line = LINE(box, i);
        x = 2;
        if (right_aligned())
            x = wnd->width - ui_window_text_width(wnd, string_cstr(line), 0, 0, 12, 0, 0) - 0x13;
        ui_window_text_out(wnd, x, box->list.line_height * row + 3, string_cstr(line), 0, 0, 12,
                         DWORDS(&box->colours)[i], 0, 0);
        place_navi_links(box, i);
        place_item_tags(box, i);
    }
}

void ui_chat_his_box2_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIChatHisBox2 *self = (struct UIChatHisBox2 *)window;
    struct UIWindow *wnd = &self->box.list.base;

    ui_window_on_r_btn_up(wnd->parent, wnd->x + x, wnd->y + y);
}

void ui_chat_his_box2_create(struct UIListBox *base_list, int x, int y, int width, int height,
                             BYTE transparent_unused)
{
    struct UIChatHisBox2 *self = (struct UIChatHisBox2 *)base_list;
    struct UIWindow *wnd = &self->box.list.base;

    self->box.list.transparent = 0;
    ui_window_resize_base(wnd, width, height);
    ui_window_move_base(wnd, x, y);
    ui_window_on_create(wnd, width, height);
}

void ui_chat_his_box3_destruct(struct UIChatHisBox3 *self)
{
    self->box2.box.list.base.ops = &ui_chat_his_box3_ops.base;
    self->box2.box.list.base.ops = &ui_chat_his_box2_ops.base;
    ui_chat_his_box_destruct(&self->box2.box);
}

void ui_chat_his_box3_destroy(struct UIWindow *window)
{
    struct UIChatHisBox3 *self = (struct UIChatHisBox3 *)window;
    ui_chat_his_box3_destruct(self);
    mem_free(self);
}

void ui_chat_his_box3_resize(struct UIWindow *window, int width, int height)
{
    struct UIChatHisBox3 *self = (struct UIChatHisBox3 *)window;
    struct UIChatHisBox *box = &self->box2.box;

    ui_window_resize_base(&box->list.base, width, height);
    ui_scroll_bar_resize(&box->list.vbar->base, 9, height);
    ui_window_move_base(&box->list.vbar->base, width - 10, 0);
    ui_scroll_bar_resize(&box->list.hbar->base, width - 10, 9);
    ui_window_move_base(&box->list.hbar->base, 0, height - 10);
    ui_list_box_update_scroll_bar(&box->list);
}

void ui_chat_his_box3_on_create(struct UIWindow *window, int width, int height)
{
    struct UIChatHisBox3 *self = (struct UIChatHisBox3 *)window;
    struct UIChatHisBox *box = &self->box2.box;

    box->list.vbar = add_bar(box, &ui_scroll_bar3_ops, 9, UI_SCROLL_VERT, width - 10, 0, height);
    ui_window_add_child(&box->list.base, &box->list.vbar->base);
    box->list.hbar = add_bar(box, &ui_scroll_bar3_ops, 9, UI_SCROLL_HORZ, 0, height - 10, width - 10);
    ui_window_add_child(&box->list.base, &box->list.hbar->base);
}

void ui_chat_his_box3_on_draw(struct UIWindow *window)
{
    struct UIChatHisBox3 *self = (struct UIChatHisBox3 *)window;
    struct UIChatHisBox *box = &self->box2.box;
    struct UIWindow *wnd = &box->list.base;
    int i = box->list.top;
    int end;
    int row;

    if (box->list.transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, RGB(box->list.r, box->list.g, box->list.b));
    }
    end = view_end(box);
    for (row = 0; i < end; row++, i++) {
        ui_window_text_out(wnd, 2, box->list.line_height * row + 3, string_cstr(LINE(box, i)), 0, 0,
                         self->font_height, DWORDS(&box->colours)[i], 0, 0);
        place_navi_links(box, i);
        place_item_tags(box, i);
    }
}
