#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "core/text_array.h"
#include "render/rp_pool.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"

struct UIColoringTextViewer;
struct UIWindow;
void ui_coloring_text_viewer_destroy(struct UIWindow *window);
void ui_coloring_text_viewer_on_draw(struct UIWindow *self);
int ui_coloring_text_viewer_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIListBoxOps ui_coloring_text_viewer_ops = {
    .base = {
        .destroy = ui_coloring_text_viewer_destroy,
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
        .on_draw = ui_coloring_text_viewer_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_coloring_text_viewer_send_msg,
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
    .create = ui_list_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_list_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};
struct UIRoundBoxTooltipWnd;
void ui_round_box_tooltip_wnd_destroy(struct UIWindow *window);
void ui_round_box_tooltip_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_round_box_tooltip_wnd_on_draw(struct UIWindow *self);
int ui_round_box_tooltip_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_round_box_tooltip_wnd_ops = {
    .destroy = ui_round_box_tooltip_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_round_box_tooltip_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_round_box_tooltip_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
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
    .send_msg = ui_round_box_tooltip_wnd_send_msg,
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
};

#define MOVE_INFO_WND 0x114
#define LINE_BYTES 0x23

struct UIColoringTextViewer {
    struct UIListBox list;
    COLORREF colour;
    int max_lines;
    DWORD background;
};

static struct UIColoringTextViewer *coloring_text_viewer_construct(struct UIColoringTextViewer *self)
{
    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_coloring_text_viewer_ops.base;
    self->colour = 0;
    self->max_lines = 0;
    return self;
}

void ui_coloring_text_viewer_destroy(struct UIWindow *window)
{
    struct UIColoringTextViewer *self = (struct UIColoringTextViewer *)window;
    self->list.base.ops = &ui_coloring_text_viewer_ops.base;
    ui_list_box_destruct(&self->list);
    mem_free(self);
}

void ui_coloring_text_viewer_on_draw(struct UIWindow *window)
{
    struct UIColoringTextViewer *self = (struct UIColoringTextViewer *)window;
    struct UIWindow *wnd = &self->list.base;
    const struct String *lines = (const struct String *)self->list.items.first;
    unsigned int end, count;
    int step, limit, i, row;

    if (self->list.transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent != NULL)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, self->background);
    }

    step = self->list.line_height - self->list.extra_lines;
    if (step == 0)
        step = 1;
    end = (unsigned int)(wnd->height / step + self->list.top);
    count = (unsigned int)VEC_COUNT(&self->list.items, struct String);
    if (end > count)
        end = count;
    limit = self->max_lines != 0 ? self->max_lines : (int)end;
    for (i = self->list.top, row = 0; i < limit; i++, row++)
        ui_window_text_out_runs(wnd, 3, self->list.line_height * row + 4, string_cstr(&lines[i]), lines[i].size,
                   &self->colour, 0, 0xc, 0);
}

int ui_coloring_text_viewer_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UIColoringTextViewer *self = (struct UIColoringTextViewer *)window;
    struct TextArray lines = {0};
    unsigned int i;

    if (msg != 0x1d)
        return ui_list_box_send_msg((struct UIWindow *)(&self->list), sender, msg, val1, val2, val3, val4);
    text_wrap_bytes((const char *)val1, &lines, LINE_BYTES, val4 != 0);
    for (i = 0; i < lines.size; i++) {
        ui_list_box_add_item(&self->list, text_array_at(&lines, i).data);
    }
    ui_window_invalidate_base(&self->list.base);
    text_array_clear(&lines);
    return 0;
}

struct UIRoundBoxTooltipWnd {
    struct UIFrameWnd frame;
    struct UIColoringTextViewer *viewer;
};

struct UIRoundBoxTooltipWnd *ui_round_box_tooltip_wnd_construct(struct UIRoundBoxTooltipWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_round_box_tooltip_wnd_ops;
    self->viewer = NULL;
    return self;
}

struct UIWindow *ui_round_box_tooltip_wnd_new(void)
{
    return (struct UIWindow *)ui_round_box_tooltip_wnd_construct(mem_alloc(sizeof(struct UIRoundBoxTooltipWnd)));
}

void ui_round_box_tooltip_wnd_destroy(struct UIWindow *window)
{
    struct UIRoundBoxTooltipWnd *self = (struct UIRoundBoxTooltipWnd *)window;
    self->frame.base.ops = &ui_round_box_tooltip_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_round_box_tooltip_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIRoundBoxTooltipWnd *self = (struct UIRoundBoxTooltipWnd *)window;
    struct UIColoringTextViewer *viewer;

    if (self->viewer != NULL)
        return;
    viewer = mem_alloc(sizeof *viewer);
    if (viewer != NULL)
        viewer = coloring_text_viewer_construct(viewer);
    self->viewer = viewer;
    ui_list_box_create(&viewer->list, 4, 4, width - 6, height - 6, 0);
    self->viewer->background = UI_TRANSPARENT;
    ui_window_invalidate_base(&self->viewer->list.base);
    self->viewer->colour = 0x803b1a;
    ui_window_add_child(&self->frame.base, &self->viewer->list.base);
}

void ui_round_box_tooltip_wnd_on_draw(struct UIWindow *window)
{
    struct UIRoundBoxTooltipWnd *self = (struct UIRoundBoxTooltipWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_fill_round_rect(wnd, 0, 0, wnd->width, wnd->height, 0xefb8b8);
    ui_window_fill_round_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, 0xefe7e7);
}

static const struct {
    int flag, msg;
} move_restrictions[] = {
    { 0, 0xae4 }, { 2, 0xae5 }, { 3, 0xae6 }, { 5, 0xae7 }, { 1, 0xae8 }, { 7, 0xaea }, { 4, 0xaeb },
};

static int move_info_show(struct UIRoundBoxTooltipWnd *self, int item_id)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIColoringTextViewer *viewer;
    struct String text, widest;
    struct ItemMoveInfo info;
    const char *name;
    float height;
    int count, width, line_height;
    size_t i;

    string_init(&text);
    string_init(&widest);
    ui_list_box_clear_items(&self->viewer->list);
    info = *item_move_info_get(item_id);

    count = 0;
    for (i = 0; i < sizeof move_restrictions / sizeof move_restrictions[0]; i++) {
        if (!info.flags[move_restrictions[i].flag])
            continue;
        if (text.size != 0)
            string_append_cstr(&text, "\n");
        name = msg_str(move_restrictions[i].msg);
        string_append_cstr(&text, name);
        if (strlen(name) > strlen(string_cstr(&widest)))
            string_assign_cstr(&widest, name);
        count++;
    }

    if (text.size == 0) {
        ui_window_mgr_show_option_window(&g_windowMgr, MOVE_INFO_WND, 0);
        string_free(&widest);
        string_free(&text);
        return 0;
    }

    viewer = self->viewer;
    width = ui_window_text_width(wnd, string_cstr(&widest), 0, 0, 0xc, 0, 0);
    viewer->max_lines = count;
    line_height = ui_window_text_height(wnd, string_cstr(&text), 0, 0, 0xc, 0, 0);
    height = (float)(line_height * count + 10);
    height += (float)(count - 1) * 4.8f;
    height += 1.0f;
    ui_list_box_resize(&viewer->list.base, width + 0xf, (int)height);
    ui_window_resize_base(wnd, viewer->list.base.width + 2, viewer->list.base.height + 2);
    ui_frame_wnd_move(wnd, g_mouse.x + 0x1a, g_mouse.y);
    ui_coloring_text_viewer_send_msg(&viewer->list.base, NULL, 0x1d, (int)string_cstr(&text), 0, 0, 0);
    string_free(&widest);
    string_free(&text);
    ui_window_invalidate_base(wnd);
    ui_window_mgr_raise_window(&g_windowMgr, wnd);
    return 0;
}

int ui_round_box_tooltip_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UIRoundBoxTooltipWnd *self = (struct UIRoundBoxTooltipWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (msg == 0x93) {
        if (val1 != 0)
            return move_info_show(self, val1);
    } else if (msg == 6) {
        if (val1 == 0xc1)
            ui_window_mgr_post_quit1(&g_windowMgr, MOVE_INFO_WND);
    } else {
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
    ui_window_invalidate_base(wnd);
    ui_window_mgr_raise_window(&g_windowMgr, wnd);
    return 0;
}
