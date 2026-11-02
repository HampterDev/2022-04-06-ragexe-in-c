#include "render/rp_pool.h"
#include "ui/social/guild_requests.h"
#include "contents/guild_info.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <stdio.h>
#include <string.h>
#include "ui/social/social.h"
#include "ui/social/guild_wnd.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct UIGuildWnd;
struct UIWindow;
const struct UIGuildWndOps ui_guild_wnd_ops = {
    .base = {
        .destroy = ui_guild_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 1,
        .refreshes_children = 0,
        .move = ui_frame_wnd_move,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_guild_wnd_store_info,
        .save_pos = ui_frame_wnd_save_pos,
        .move_delta = ui_frame_wnd_move_delta,
        .set_show = ui_window_set_show_base,
        .on_create = ui_window_abstract_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_guild_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_guild_wnd_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_frame_wnd_on_mouse_move,
        .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_frame_wnd_snap,
        .send_msg = ui_guild_wnd_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_FRAME,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .tab = -1,
};

#define GUILD_NAME_HIDDEN (-200)
#define GUILD_LIST_TOP 0x2e
#define GUILD_LIST_TEXT_TOP 0x3f

#define MSG_EXPELLED_NAME 0x197
#define MSG_EXPELLED_REASON 0x1ce
#define MSG_TOTAL_INFO_COL0 0x205

#define EXPELLED_ROWS 0xb
#define TOTAL_INFO_ROWS 0xc

struct UIGuildBanishedMemberWnd {
    struct UIGuildWnd base;
    struct UIWindow *names[EXPELLED_ROWS];
    struct UIWindow *reasons[EXPELLED_ROWS];
    struct GuildListView list;
};
void ui_guild_banished_member_wnd_destroy(struct UIWindow *window);
void ui_guild_banished_member_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_guild_banished_member_wnd_on_draw(struct UIWindow *self);
void ui_guild_banished_member_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_banished_member_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_guild_banished_member_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIGuildWndOps ui_guild_banished_member_wnd_ops = {
    .base = {
        .destroy = ui_guild_banished_member_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 1,
        .refreshes_children = 0,
        .move = ui_frame_wnd_move,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_guild_wnd_store_info,
        .save_pos = ui_frame_wnd_save_pos,
        .move_delta = ui_frame_wnd_move_delta,
        .set_show = ui_window_set_show_base,
        .on_create = ui_guild_banished_member_wnd_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_guild_banished_member_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_guild_banished_member_wnd_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_frame_wnd_on_mouse_move,
        .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_guild_banished_member_wnd_on_wheel,
        .snap = ui_frame_wnd_snap,
        .send_msg = ui_guild_banished_member_wnd_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_FRAME,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .tab = GUILD_TAB_EXPELLED,
};

struct UIGuildTotalInfoWnd {
    struct UIGuildWnd base;
    struct UIWindow *names[TOTAL_INFO_ROWS];
    struct GuildListView list;
};
void ui_guild_total_info_wnd_destroy(struct UIWindow *window);
void ui_guild_total_info_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_guild_total_info_wnd_on_draw(struct UIWindow *self);
void ui_guild_total_info_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_total_info_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_guild_total_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIGuildWndOps ui_guild_total_info_wnd_ops = {
    .base = {
        .destroy = ui_guild_total_info_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 1,
        .refreshes_children = 0,
        .move = ui_frame_wnd_move,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_guild_wnd_store_info,
        .save_pos = ui_frame_wnd_save_pos,
        .move_delta = ui_frame_wnd_move_delta,
        .set_show = ui_window_set_show_base,
        .on_create = ui_guild_total_info_wnd_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_guild_total_info_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_guild_total_info_wnd_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_frame_wnd_on_mouse_move,
        .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_guild_total_info_wnd_on_wheel,
        .snap = ui_frame_wnd_snap,
        .send_msg = ui_guild_total_info_wnd_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_FRAME,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .tab = GUILD_TAB_TOTAL_INFO,
};

#define WND(self) (&(self)->base.frame.base)

static struct UIScrollBar *guild_add_scroll_bar(struct UIWindow *wnd)
{
    struct UIScrollBar *bar = mem_alloc(sizeof(struct UIScrollBar));

    if (bar)
        bar = ui_scroll_bar_construct(bar);
    ui_scroll_bar_create(bar, wnd->width - 0xf, GUILD_LIST_TEXT_TOP, UI_SCROLL_VERT, wnd->height - 0x57);
    ui_window_add_child(wnd, &bar->base);
    return bar;
}

static struct UIWindow *guild_add_name_text(struct UIWindow *wnd, int width, int x, int y)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    if (text)
        text = ui_static_text_construct(text);
    ui_window_create(text, width, 0xf);
    ui_static_text_set_text_ellipsis(text, "");
    ui_window_move(text, x, y);
    ui_window_add_child(wnd, text);
    return text;
}

static void guild_list_refresh(struct GuildListView *list, unsigned count)
{
    int lines, excess;

    if (!list->bar)
        return;
    lines = count / list->per_row + (count % list->per_row != 0);
    excess = lines - list->rows;
    if (excess < 0)
        excess = 0;
    list->scrollable = excess != 0;
    if (excess) {
        ui_scroll_bar_set_range(list->bar, excess, list->rows, lines);
        if (list->top < 0)
            list->top = 0;
        if (list->top > lines - list->rows)
            list->top = lines - list->rows;
        ui_scroll_bar_set_pos(list->bar, list->top);
    } else {
        list->top = 0;
        ui_scroll_bar_set_range(list->bar, 0, list->rows, lines);
    }
    ui_window_invalidate_base(&list->bar->base);
}

int guild_list_view_hit_test(const struct GuildListView *list, const struct UIWindow *wnd,
                             const struct ListNode *head, int x, int y, int row_height)
{
    const struct ListNode *node;
    int row, index, shown;

    if (x > wnd->width - 0xe || x < 4)
        return -1;
    if (y < row_height + GUILD_LIST_TOP || y >= (list->rows + 1) * row_height + GUILD_LIST_TOP)
        return -1;
    row = (y - row_height - GUILD_LIST_TOP) / row_height;
    shown = 0;
    index = 0;
    for (node = head->next; node != head; node = node->next, index++) {
        if (index < list->top * list->per_row)
            continue;
        if (shown == list->per_row * row)
            return index;
        shown++;
    }
    return -1;
}

static void guild_list_wheel(struct UIWindow *wnd, const struct GuildListView *list, int delta)
{
    int step;

    if (!list->scrollable)
        return;
    step = list->rows <= 3 ? list->rows - 1 : 3;
    if (delta >= 0)
        step = -step;
    ui_window_send_msg(wnd, &list->bar->base, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

static void guild_draw_panel(struct UIWindow *wnd, int height_less)
{
    ui_window_fill_rect(wnd, 4, 0x2d, wnd->width - 0x16, wnd->height - height_less, ui_skin_colour(0xe, 6));
}

static void guild_draw_cells(struct UIWindow *wnd, const int *widths, int cols, int row_height)
{
    COLORREF colour = ui_skin_colour(2, 2);
    int row, col, x;

    for (row = 0; row < 12; row++) {
        x = 0;
        for (col = 0; col < cols; col++) {
            ui_window_fill_rect(wnd, x + 5, row_height * row + GUILD_LIST_TOP, widths[col], row_height - 1,
                              colour);
            x += widths[col] + 1;
        }
    }
}

static void guild_draw_headers(struct UIWindow *wnd, const int *widths, char *const *titles, int cols)
{
    int col, x = 0;

    for (col = 0; col < cols; col++) {
        ui_window_text_out(wnd, x + 8, 0x31, titles[col], 0, 0, 0xc, 0, 0, 0);
        x += widths[col] + 1;
    }
}

static void guild_draw_selection(struct UIWindow *wnd, int y)
{
    ui_window_fill_rect(wnd, 5, y - 3, wnd->width - 0x18, 0x12, ui_skin_colour(6, 2));
}

static void guild_hide_texts(struct UIWindow **texts, int count)
{
    int i;

    for (i = 0; i < count; i++)
        ui_window_move(texts[i], GUILD_NAME_HIDDEN, GUILD_NAME_HIDDEN);
}

struct UIGuildBanishedMemberWnd *ui_guild_banished_member_wnd_construct(struct UIGuildBanishedMemberWnd *self)
{
    ui_guild_wnd_construct(&self->base);
    WND(self)->ops = &ui_guild_banished_member_wnd_ops.base;
    return self;
}

struct UIWindow *ui_guild_banished_member_wnd_new(void)
{
    return (struct UIWindow *)ui_guild_banished_member_wnd_construct(mem_alloc(sizeof(struct UIGuildBanishedMemberWnd)));
}

void ui_guild_banished_member_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildBanishedMemberWnd *self = (struct UIGuildBanishedMemberWnd *)window;
    WND(self)->ops = &ui_guild_banished_member_wnd_ops.base;
    WND(self)->ops = &ui_guild_wnd_ops.base;
    ui_frame_wnd_destruct(&self->base.frame);
    mem_free(self);
}

void ui_guild_banished_member_wnd_refresh(struct UIGuildBanishedMemberWnd *self)
{
    guild_list_refresh(&self->list, g_guildInfo.expelled.size);
}

void ui_guild_banished_member_wnd_on_create(struct UIWindow *window, int width_unused,
                                                  int height_unused)
{
    struct UIGuildBanishedMemberWnd *self = (struct UIGuildBanishedMemberWnd *)window;
    struct UIWindow *wnd = WND(self);
    int row;

    ui_guild_wnd_init_tab_text(&self->base);
    guild_request_page(GUILD_PAGE_EXPELLED);
    self->list.top = 0;
    self->list.selected = 0;
    self->list.per_row = 1;
    self->list.rows = EXPELLED_ROWS;
    self->list.scrollable = 0;
    self->list.bar = guild_add_scroll_bar(wnd);
    for (row = 0; row < EXPELLED_ROWS; row++) {
        int y = g_guildLayout.expelled_row_height * row + GUILD_LIST_TEXT_TOP;

        self->names[row] = guild_add_name_text(wnd, g_guildLayout.expelled_cols[0] - 4, 8, y);
        self->reasons[row] = guild_add_name_text(wnd, g_guildLayout.expelled_cols[1] - 4,
                                                 g_guildLayout.member_position_cols[0] + 0x3a, y);
    }
    ui_guild_banished_member_wnd_refresh(self);
}

void ui_guild_banished_member_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildBanishedMemberWnd *self = (struct UIGuildBanishedMemberWnd *)window;
    struct UIWindow *wnd = WND(self);
    const int row_height = g_guildLayout.expelled_row_height;
    int widths[2];
    char *titles[2];
    struct ListNode *node;
    int index, drawn;

    ui_guild_wnd_on_draw((struct UIWindow *)(&self->base));
    guild_hide_texts(self->names, EXPELLED_ROWS);
    guild_hide_texts(self->reasons, EXPELLED_ROWS);
    guild_draw_panel(wnd, row_height + 0x45);
    widths[0] = g_guildLayout.expelled_cols[0];
    widths[1] = g_guildLayout.expelled_cols[1];
    guild_draw_cells(wnd, widths, 2, row_height);
    titles[0] = msg_str(MSG_EXPELLED_NAME);
    titles[1] = msg_str(MSG_EXPELLED_REASON);
    guild_draw_headers(wnd, widths, titles, 2);

    drawn = 0;
    index = 0;
    for (node = g_guildInfo.expelled.head->next; node != g_guildInfo.expelled.head; node = node->next, index++) {
        struct String name;
        char reason[0x104];
        int y;

        if (index < self->list.top * self->list.per_row)
            continue;
        y = drawn / self->list.per_row * row_height + 0x44;
        if (self->list.selected == index)
            guild_draw_selection(wnd, y);
        guild_name_cut_nameless(&name, &LIST_VALUE(node, struct GuildExpelled)->name);
        ui_static_text_set_text_ellipsis(self->names[drawn], string_cstr(&name));
        ui_window_move(self->names[drawn], 8, y);
        memset(reason, 0, sizeof reason);
        strncpy_s(reason, sizeof reason, string_cstr(&LIST_VALUE(node, struct GuildExpelled)->reason), LIST_VALUE(node, struct GuildExpelled)->reason.size);
        ui_static_text_set_text_ellipsis(self->reasons[drawn], reason);
        ui_window_move(self->reasons[drawn], g_guildLayout.expelled_cols[0] + 9, y);
        string_free(&name);
        if (++drawn == self->list.rows * self->list.per_row)
            break;
    }
}

void ui_guild_banished_member_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildBanishedMemberWnd *self = (struct UIGuildBanishedMemberWnd *)window;
    int old;

    ui_guild_wnd_on_l_btn_down((struct UIWindow *)(&self->base), x, y);
    old = self->list.selected;
    self->list.selected = guild_list_view_hit_test(&self->list, WND(self), g_guildInfo.expelled.head, x, y,
                                                  g_guildLayout.expelled_row_height);
    if (old != self->list.selected)
        ui_window_invalidate_base(&self->base.frame.base);
}

void ui_guild_banished_member_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIGuildBanishedMemberWnd *self = (struct UIGuildBanishedMemberWnd *)window;
    guild_list_wheel(WND(self), &self->list, delta);
}

struct UIGuildTotalInfoWnd *ui_guild_total_info_wnd_construct(struct UIGuildTotalInfoWnd *self)
{
    ui_guild_wnd_construct(&self->base);
    WND(self)->ops = &ui_guild_total_info_wnd_ops.base;
    return self;
}

struct UIWindow *ui_guild_total_info_wnd_new(void)
{
    return (struct UIWindow *)ui_guild_total_info_wnd_construct(mem_alloc(sizeof(struct UIGuildTotalInfoWnd)));
}

void ui_guild_total_info_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildTotalInfoWnd *self = (struct UIGuildTotalInfoWnd *)window;
    WND(self)->ops = &ui_guild_total_info_wnd_ops.base;
    WND(self)->ops = &ui_guild_wnd_ops.base;
    ui_frame_wnd_destruct(&self->base.frame);
    mem_free(self);
}

void ui_guild_total_info_wnd_refresh(struct UIGuildTotalInfoWnd *self)
{
    guild_list_refresh(&self->list, g_guildInfo.total_info.size);
}

void ui_guild_total_info_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIGuildTotalInfoWnd *self = (struct UIGuildTotalInfoWnd *)window;
    struct UIWindow *wnd = WND(self);
    int row;

    ui_guild_wnd_init_tab_text(&self->base);
    guild_request_page(GUILD_PAGE_TOTAL_INFO);
    self->list.top = 0;
    self->list.selected = 0;
    self->list.per_row = 1;
    self->list.rows = TOTAL_INFO_ROWS;
    self->list.scrollable = 0;
    self->list.bar = guild_add_scroll_bar(wnd);
    for (row = 0; row < TOTAL_INFO_ROWS; row++)
        self->names[row] = guild_add_name_text(wnd, g_guildLayout.expelled_cols[0] - 4, 8,
                                               g_guildLayout.total_row_height * row + GUILD_LIST_TEXT_TOP);
    ui_guild_total_info_wnd_refresh(self);
}

void ui_guild_total_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildTotalInfoWnd *self = (struct UIGuildTotalInfoWnd *)window;
    struct UIWindow *wnd = WND(self);
    const int *cols = g_guildLayout.total_cols;
    int widths[4];
    char *titles[4];
    char buf[0x200];
    struct ListNode *node;
    int i, index, drawn;

    ui_guild_wnd_on_draw((struct UIWindow *)(&self->base));
    guild_hide_texts(self->names, TOTAL_INFO_ROWS);
    guild_draw_panel(wnd, 0x45);
    for (i = 0; i < 4; i++)
        widths[i] = cols[i];
    guild_draw_cells(wnd, widths, 4, g_guildLayout.total_row_height);
    for (i = 0; i < 4; i++)
        titles[i] = msg_str(MSG_TOTAL_INFO_COL0 + i);
    guild_draw_headers(wnd, widths, titles, 4);

    drawn = 0;
    index = 0;
    for (node = g_guildInfo.total_info.head->next; node != g_guildInfo.total_info.head; node = node->next, index++) {
        int y;

        if (index < self->list.top * self->list.per_row)
            continue;
        y = drawn / self->list.per_row * g_guildLayout.expelled_row_height + 0x44;
        if (self->list.selected == index)
            guild_draw_selection(wnd, y);
        ui_static_text_set_text_ellipsis(self->names[drawn], string_cstr(&LIST_VALUE(node, struct GuildTotalInfoRow)->name));
        ui_window_move(self->names[drawn], 8, y);
        sprintf(buf, "%d", LIST_VALUE(node, struct GuildTotalInfoRow)->values[0]);
        ui_window_text_out(wnd, cols[0] + 9, y, buf, 0, 0, 0xc, 0, 0, 0);
        sprintf(buf, "%d/%d", LIST_VALUE(node, struct GuildTotalInfoRow)->values[1], LIST_VALUE(node, struct GuildTotalInfoRow)->values[2]);
        ui_window_text_out(wnd, cols[0] + 0xa + cols[1], y, buf, 0, 0, 0xc, 0, 0, 0);
        sprintf(buf, "%d", LIST_VALUE(node, struct GuildTotalInfoRow)->values[3]);
        ui_window_text_out(wnd, cols[0] + 0xb + cols[1] + cols[2], y, buf, 0, 0, 0xc, 0, 0, 0);
        if (++drawn == self->list.rows * self->list.per_row)
            break;
    }
}

void ui_guild_total_info_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildTotalInfoWnd *self = (struct UIGuildTotalInfoWnd *)window;
    int old;

    ui_guild_wnd_on_l_btn_down((struct UIWindow *)(&self->base), x, y);
    old = self->list.selected;
    self->list.selected = guild_list_view_hit_test(&self->list, WND(self), g_guildInfo.total_info.head, x, y,
                                                  g_guildLayout.total_row_height);
    if (old != self->list.selected)
        ui_window_invalidate_base(&self->base.frame.base);
}

void ui_guild_total_info_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIGuildTotalInfoWnd *self = (struct UIGuildTotalInfoWnd *)window;
    guild_list_wheel(WND(self), &self->list, delta);
}

struct GuildExpelledEntry {
    int gid;
    struct String name;
    struct String reason;
};

struct GuildTotalInfoEntry {
    struct String name;
};

static int guild_list_move_top(struct GuildListView *list, int msg, int val1)
{
    int old = list->top;

    if (msg == UI_MSG_PAGE_UP)
        list->top = old - list->rows + 1;
    else if (msg == UI_MSG_PAGE_DOWN)
        list->top = list->rows - 1 + old;
    else
        list->top = val1 + old;
    return old;
}

int ui_guild_banished_member_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                int msg, int val1, int val2, int val3, int val4)
{
    struct UIGuildBanishedMemberWnd *self = (struct UIGuildBanishedMemberWnd *)window;
    int old;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE)
            ui_close_window(GUILD_WND_ID);
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old = guild_list_move_top(&self->list, msg, val1);
        if (old != self->list.top) {
            ui_guild_banished_member_wnd_refresh(self);
            ui_window_invalidate_base(WND(self));
        }
        return self->list.top - old;
    case 0x3c:
        ui_window_invalidate_children(WND(self));
        ui_guild_banished_member_wnd_refresh(self);
        return 0;
    }
    return ui_guild_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

int ui_guild_total_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIGuildTotalInfoWnd *self = (struct UIGuildTotalInfoWnd *)window;
    int old;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE)
            ui_close_window(GUILD_WND_ID);
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old = guild_list_move_top(&self->list, msg, val1);
        if (old != self->list.top) {
            ui_guild_total_info_wnd_refresh(self);
            ui_window_invalidate_base(WND(self));
        }
        return self->list.top - old;
    }
    return ui_guild_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
