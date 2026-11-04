#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/quests/navigation.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/ui_std.h"
#include "core/skin_path.h"
#include "diag/perf.h"

struct UIListBox_NaviSearch;
struct UIWindow;
void ui_list_box_navi_search_destroy(struct UIWindow *window);
void ui_list_box_navi_search_resize(struct UIWindow *self, int width, int height);
void ui_list_box_navi_search_on_create(struct UIWindow *self, int width, int height);
void ui_list_box_navi_search_on_draw(struct UIWindow *self);
void ui_list_box_navi_search_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_list_box_navi_search_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_list_box_navi_search_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
void ui_list_box_navi_search_clear_items(struct UIListBox *base_list);
static void ui_list_box_navi_search_update_scroll_bar(struct UIListBox *base_list);
const struct UIListBoxOps ui_list_box_navi_search_ops = {
    .base = {
        .destroy = ui_list_box_navi_search_destroy,
        .resize = ui_list_box_navi_search_resize,
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
        .on_create = ui_list_box_navi_search_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_list_box_navi_search_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_navi_search_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_navi_search_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_navi_search_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_navi_search_send_msg,
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
    .clear_items = ui_list_box_navi_search_clear_items,
    .update_scroll_bar = ui_list_box_navi_search_update_scroll_bar,
};

#define NAVI_ICON_WIDTH 11
#define NAVI_LIST_EDGE_COLOUR 0xebdede
#define NAVI_WHEEL_STEP 6
#define NAVI_SCROLL_BAR_SIZE sizeof(struct UIScrollBar)

void ui_list_box_navi_search_destroy(struct UIWindow *window)
{
    struct UIListBox_NaviSearch *self = (struct UIListBox_NaviSearch *)window;
    self->list.base.ops = &ui_list_box_navi_search_ops.base;
    vec_free(&self->kinds);
    ui_list_box_destruct(&self->list);
    mem_free(self);
}

void ui_list_box_navi_search_clear_items(struct UIListBox *base_list)
{
    struct UIListBox_NaviSearch *self = (struct UIListBox_NaviSearch *)base_list;
    ui_list_box_clear_items(&self->list);
    self->kinds.last = self->kinds.first;
}

void ui_list_box_navi_search_on_create(struct UIWindow *window, int width, int height)
{
    struct UIListBox_NaviSearch *self = (struct UIListBox_NaviSearch *)window;
    struct UIScrollBar *bar = (struct UIScrollBar *)mem_alloc(NAVI_SCROLL_BAR_SIZE);

    if (bar)
        bar = ui_scroll_bar_navi_construct(bar);
    self->list.vbar = bar;
    ui_scroll_bar_create(bar, width - 7, 0, UI_SCROLL_VERT, height);
    ui_window_add_child(&self->list.base, &self->list.vbar->base);
    bar = (struct UIScrollBar *)mem_alloc(NAVI_SCROLL_BAR_SIZE);
    if (bar)
        bar = ui_scroll_bar_construct(bar);
    self->list.hbar = bar;
    ui_scroll_bar_create(bar, 0, height - 13, UI_SCROLL_HORZ, width - 13);
    ui_window_add_child(&self->list.base, &self->list.hbar->base);
}

static const char *kind_icon(int kind)
{
    switch (kind) {
    case NAVI_LINE_MAP: return NAVI_DIR "icon_list_map.bmp";
    case NAVI_LINE_NPC: return NAVI_DIR "icon_list_npc.bmp";
    case NAVI_LINE_MOB: return NAVI_DIR "icon_list_mob.bmp";
    case NAVI_LINE_START: return NAVI_DIR "icon_list_Start.bmp";
    case NAVI_LINE_STEP: return NAVI_DIR "icon_list_Riute.bmp";
    case NAVI_LINE_TARGET: return NAVI_DIR "icon_list_Target.bmp";
    default: return NULL;
    }
}

void ui_list_box_navi_search_on_draw(struct UIWindow *window)
{
    struct UIListBox_NaviSearch *self = (struct UIListBox_NaviSearch *)window;
    struct UIListBox *lb = &self->list;
    struct UIWindow *wnd = &lb->base;
    COLORREF colour = 0;
    unsigned count, end, last;
    int i, row;

    if (lb->transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, (BYTE)lb->r | (BYTE)lb->g << 8 | (BYTE)lb->b << 16);
    }
    ui_window_fill_rect(wnd, wnd->width - 1, 0, 1, wnd->height, NAVI_LIST_EDGE_COLOUR);
    end = wnd->height / lb->line_height - lb->extra_lines + lb->top;
    count = VEC_COUNT(&lb->items, struct String);
    last = end < count ? end : count;
    for (i = lb->top, row = 0; i < (int)last; i++, row++) {
        struct String *item = &VEC_AT(&lb->items, struct String, i);
        int x = 3 - lb->scroll_x;
        int y = lb->line_height * row;
        int kind;

        if (i == lb->selected)
            ui_window_fill_rect(wnd, 0, y, wnd->width, lb->line_height - 1, lb->selection_colour);
        kind = VEC_AT(&self->kinds, int, i);
        if (kind > 0) {
            const char *icon = kind_icon(kind);

            if (icon)
                ui_window_draw_bitmap(wnd, x, y, res_mgr_get(g_resMgr(), ui_skin_path(icon)), 0);
            x += NAVI_ICON_WIDTH;
        }
        ui_window_text_out_runs(wnd, x, y, string_cstr(item), item->size, &colour, 0, 0xc, 0);
    }
}

void ui_list_box_navi_search_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIListBox_NaviSearch *self = (struct UIListBox_NaviSearch *)window;
    struct UIWindow *wnd = &self->list.base;
    int line = ui_list_box_hit_line(&self->list, x, y);

    self->list.selected = line;
    if (line != -1)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, line, 0, 0);
    ui_window_invalidate_base(wnd);
}

void ui_list_box_navi_search_on_wheel(struct UIWindow *window, int delta)
{
    struct UIListBox_NaviSearch *self = (struct UIListBox_NaviSearch *)window;
    struct UIWindow *wnd = &self->list.base;

    if (!self->list.has_vscroll)
        return;
    ui_list_box_navi_search_send_msg(wnd, &self->list.vbar->base, 7, delta < 0 ? NAVI_WHEEL_STEP : -NAVI_WHEEL_STEP, 0, 0, 0);
}

static void ui_list_box_navi_search_update_scroll_bar(struct UIListBox *base_list)
{
    struct UIListBox *lb = base_list;
    int visible = lb->base.height / lb->line_height;
    int count = (int)VEC_COUNT(&lb->items, struct String);
    int over = count - visible;
    int max_pos;

    if (over < 0)
        over = 0;
    if (over) {
        lb->has_vscroll = 1;
        max_pos = lb->extra_lines + over;
    } else {
        lb->has_vscroll = 0;
        max_pos = 0;
    }
    ui_scroll_bar_set_range(lb->vbar, max_pos, visible,
                         (int)VEC_COUNT(&lb->items, struct String));
    ui_window_invalidate_base(&lb->vbar->base);
}

void ui_list_box_navi_search_resize(struct UIWindow *window, int width, int height)
{
    struct UIListBox_NaviSearch *self = (struct UIListBox_NaviSearch *)window;
    struct UIListBox *lb = &self->list;

    ui_window_resize_base(&lb->base, width, height);
    ui_scroll_bar_resize(&lb->vbar->base, 12, height);
    ui_window_move_base(&lb->vbar->base, width - 13, 0);
    ui_scroll_bar_resize(&lb->hbar->base, width - 13, 12);
    ui_window_move_base(&lb->hbar->base, 0, height - 13);
    ui_list_box_navi_search_update_scroll_bar(lb);
}

intptr_t ui_list_box_navi_search_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIListBox_NaviSearch *self = (struct UIListBox_NaviSearch *)window;
    struct UIListBox *lb = &self->list;
    int line_height, height, old, top, max_top;

    if (msg != 7 && msg != 9 && msg != 10)
        return ui_list_box_send_msg((struct UIWindow *)(lb), sender, msg, val1, val2, val3, val4);
    line_height = lb->line_height;
    height = lb->base.height;
    old = lb->top;
    if (msg == 9)
        top = old - height / line_height + 1;
    else if (msg == 10)
        top = old - 1 + height / line_height;
    else
        top = old + val1;
    lb->top = top;
    if (top < 0)
        lb->top = top = 0;
    max_top = (int)VEC_COUNT(&lb->items, struct String) - height / line_height + lb->extra_lines;
    if ((unsigned)top > (unsigned)max_top)
        lb->top = top = max_top;
    ui_scroll_bar_set_pos(lb->vbar, top);
    ui_window_invalidate_base(&lb->vbar->base);
    ui_window_invalidate_base(&lb->base);
    if (lb->transparent && lb->base.parent)
        ui_window_invalidate(lb->base.parent);
    return lb->top - old;
}
