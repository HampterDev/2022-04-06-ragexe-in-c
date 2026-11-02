#include "ui/controls/scroll.h"
#include "ui/controls/listbox.h"
#include "ui/controls/combo_box.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include <string.h>
#include "diag/perf.h"

struct UIComboBoxEx;
struct UIWindow;
const struct UIListBoxExOps ui_combo_box_ex_ops = {
    .base = {
        .destroy = ui_combo_box_ex_destroy,
        .resize = ui_combo_box_ex_resize,
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
        .on_create = ui_list_box_ex_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_combo_box_ex_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_combo_box_ignore_click,
        .on_l_btn_dbl_clk = ui_combo_box_ignore_click,
        .on_mouse_move = ui_combo_box_ex_on_mouse_move,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_combo_box_ex_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_ex_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_combo_box_ex_send_msg,
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
    .clear_items = ui_list_box_ex_clear_items,
    .update_scroll_bar = ui_combo_box_ex_update_scroll_bar,
};

struct UIComboBoxEx2;
void ui_combo_box_ex2_on_draw(struct UIWindow *self);
const struct UIListBoxExOps ui_combo_box_ex2_ops = {
    .base = {
        .destroy = ui_combo_box_ex2_destroy,
        .resize = ui_combo_box_ex2_resize,
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
        .on_create = ui_combo_box_ex2_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_combo_box_ex2_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_combo_box_ignore_click,
        .on_l_btn_dbl_clk = ui_combo_box_ignore_click,
        .on_mouse_move = ui_combo_box_ex_on_mouse_move,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_combo_box_ex_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_ex_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_combo_box_ex_send_msg,
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
    .clear_items = ui_list_box_ex_clear_items,
    .update_scroll_bar = ui_combo_box_ex_update_scroll_bar,
};

struct UIComboBoxID;
const struct UIListBoxExOps ui_combo_box_id_ops = {
    .base = {
        .destroy = ui_combo_box_id_destroy,
        .resize = ui_combo_box_ex_resize,
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
        .on_create = ui_list_box_ex_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_combo_box_id_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_combo_box_ignore_click,
        .on_l_btn_dbl_clk = ui_combo_box_ignore_click,
        .on_mouse_move = ui_combo_box_ex_on_mouse_move,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_combo_box_id_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_ex_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_combo_box_id_send_msg,
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
    .clear_items = ui_list_box_id_clear_items,
    .update_scroll_bar = ui_combo_box_ex_update_scroll_bar,
};

struct UIComboBoxDefWnd;
const struct UIWindowOps ui_combo_box_def_wnd_ops = {
    .destroy = ui_combo_box_def_wnd_destroy,
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
    .on_create = ui_combo_box_def_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_combo_box_def_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_combo_box_def_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_combo_box_def_wnd_send_msg,
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

struct UIComboBoxDef2Wnd;
const struct UIWindowOps ui_combo_box_def2_wnd_ops = {
    .destroy = ui_combo_box_def2_wnd_destroy,
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
    .on_create = ui_combo_box_def2_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_combo_box_def2_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_combo_box_def_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_combo_box_def_wnd_send_msg,
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

struct UIComboBoxBmpWnd;
const struct UIWindowOps ui_combo_box_bmp_wnd_ops = {
    .destroy = ui_combo_box_bmp_wnd_destroy,
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
    .on_create = ui_combo_box_def_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_combo_box_bmp_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_combo_box_def_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_combo_box_def_wnd_send_msg,
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

struct UIComboBoxV2Wnd;
const struct UIWindowOps ui_combo_box_v2_wnd_ops = {
    .destroy = ui_combo_box_v2_wnd_destroy,
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
    .on_draw = ui_combo_box_v2_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_combo_box_def_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_combo_box_v2_wnd_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_combo_box_v2_wnd_send_msg,
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

#define ROW_COUNT(lb) ((lb)->rows.size)

static void draw_list_frame(struct UIWindow *wnd)
{
    struct BitmapRes *corner;

    ui_window_clear_canvas(wnd, ui_skin_colour(6, 6));
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, ui_skin_colour(2, 2));
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_lu.bmp"), 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_ld.bmp");
    ui_window_draw_bitmap(wnd, 0, wnd->height - corner->height, corner, 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_ru.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - corner->width, 0, corner, 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_rd.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - corner->width, wnd->height - corner->height, corner, 0);
}

struct UIComboBoxEx *ui_combo_box_ex_construct(struct UIComboBoxEx *self)
{
    ui_list_box_ex_construct(&self->list);
    self->list.base.ops = &ui_combo_box_ex_ops.base;
    self->opener_id = COMBO_NO_OPENER;
    return self;
}

void ui_combo_box_ex_destroy(struct UIWindow *window)
{
    struct UIComboBoxEx *self = (struct UIComboBoxEx *)window;
    self->list.base.ops = &ui_combo_box_ex_ops.base;
    ui_list_box_ex_destruct(&self->list);
    mem_free(self);
}

void ui_combo_box_ex_resize(struct UIWindow *window, int width, int height)
{
    struct UIComboBoxEx *self = (struct UIComboBoxEx *)window;
    struct UIListBoxEx *lb = &self->list;

    ui_list_box_ex_resize((struct UIWindow *)(lb), width, height);
    lb->top = 0;
    lb->vbar->pos = lb->top;
    ui_window_move_base(&lb->vbar->base, width - 13, 0);
    if (lb->has_vscroll)
        ui_window_invalidate_base(&lb->vbar->base);
}

void ui_combo_box_ex_on_draw(struct UIWindow *window)
{
    struct UIComboBoxEx *self = (struct UIComboBoxEx *)window;
    struct UIListBoxEx *lb = &self->list;
    struct UIWindow *wnd = &lb->base;
    int first, end, line;

    draw_list_frame(wnd);
    first = lb->top;
    end = wnd->height / lb->line_height - lb->extra_lines + first;
    if ((unsigned)end >= ROW_COUNT(lb))
        end = (int)ROW_COUNT(lb);
    for (line = first; line < end; line++) {
        COLORREF colour;
        const char *text;

        if (line == lb->selected)
            ui_window_fill_rect(wnd, 3, (line - first) * lb->line_height + 1, wnd->width - 6,
                              lb->line_height - 2, ui_skin_colour(6, 2));
        colour = list_box_ex_entry_at(&lb->rows, &line)->colour;
        text = string_cstr(&list_box_ex_entry_at(&lb->rows, &line)->text);
        ui_window_text_out(wnd, 3 - lb->scroll_x, (line - first) * lb->line_height + 2, text, 0, 0, 12, colour,
                         0, 0);
    }
    if (lb->has_vscroll)
        ui_window_fill_rect(wnd, wnd->width - 16, 0, 16, wnd->height, ui_skin_colour(2, 2));
    ui_window_invalidate(wnd->parent);
}

static int hit_row(const struct UIListBoxEx *lb, int x, int y)
{
    int row;

    if (x < 0 || y < 0 || x >= lb->base.width || y >= lb->base.height)
        return -1;
    row = y / lb->line_height + lb->top;
    if (row < 0 || (unsigned)row >= ROW_COUNT(lb))
        return -1;
    return row;
}

void ui_combo_box_ex_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIComboBoxEx *self = (struct UIComboBoxEx *)window;
    int old = self->list.selected;

    self->list.selected = hit_row(&self->list, x, y);
    if (old != self->list.selected)
        ui_window_invalidate_base(&self->list.base);
}

static void report_choice(struct UIComboBoxEx *self, struct UIWindow *to)
{
    ui_window_send_msg(to, NULL, COMBO_MSG_CHOSEN, self->list.selected, 0, 0, 0);
}

void ui_combo_box_ex_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIComboBoxEx *self = (struct UIComboBoxEx *)window;
    struct UIWindow *opener;

    self->list.selected = hit_row(&self->list, x, y);
    opener = ui_get_window(self->opener_id);
    if (opener)
        report_choice(self, opener);
    else if (self->list.base.parent)
        report_choice(self, self->list.base.parent);
    ui_close_window(UI_WND_COMBO_BOX);
}

int ui_combo_box_ex_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIComboBoxEx *self = (struct UIComboBoxEx *)window;
    struct UIListBoxEx *lb = &self->list;
    struct UIWindow *opener;

    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        opener = ui_get_window(self->opener_id);
        if (opener)
            report_choice(self, opener);
        if (g_windowMgr.combo_box_wnd)
            ui_close_window(UI_WND_COMBO_BOX);
        return 0;
    case UI_MSG_BAR_PAGE_UP:
        if (!ROW_COUNT(lb))
            return 0;
        if (--lb->selected < 0)
            lb->selected = 0;
        if (lb->selected < lb->top)
            lb->top = lb->selected;
        break;
    case UI_MSG_BAR_PAGE_DOWN:
        if (!ROW_COUNT(lb))
            return 0;
        if ((unsigned)++lb->selected >= ROW_COUNT(lb))
            lb->selected = (int)ROW_COUNT(lb) - 1;
        if (lb->top <= lb->extra_lines - lb->base.height / lb->line_height + lb->selected)
            lb->top = lb->extra_lines - lb->base.height / lb->line_height + lb->selected + 1;
        break;
    case COMBO_MSG_ADD_LINE:
        ui_list_box_ex_add_item(lb, (const char *)val1, (COLORREF)val2);
        ui_window_invalidate_base(&lb->base);
        return 0;
    case COMBO_MSG_SELECT:
        lb->selected = val1;
        if (lb->selected < 0)
            lb->selected = 0;
        if ((unsigned)lb->selected > ROW_COUNT(lb) - 1)
            lb->selected = (int)ROW_COUNT(lb) - 1;
        break;
    case COMBO_MSG_OPENER_ID:
        self->opener_id = val1;
        return 0;
    default:
        return ui_list_box_ex_send_msg((struct UIWindow *)(lb), sender, msg, val1, val2, val3, val4);
    }
    ui_combo_box_ex_update_scroll_bar(lb);
    ui_window_invalidate_base(&lb->base);
    return 0;
}

void ui_combo_box_ex_update_scroll_bar(struct UIListBoxEx *base_list)
{
    struct UIListBoxEx *lb = base_list;
    struct UIScrollBar *bar = lb->vbar;
    int in_view = lb->base.height / lb->line_height;
    int beyond = (int)ROW_COUNT(lb) - in_view;

    if (beyond < 0)
        beyond = 0;
    lb->has_vscroll = beyond != 0;
    if (beyond) {
        bar->page = in_view;
        bar->max_pos = lb->extra_lines + beyond;
        bar->total = (int)ROW_COUNT(lb);
        ui_window_invalidate_base(&bar->base);
        if (bar->pos > bar->max_pos)
            bar->pos = bar->max_pos;
        if (lb->selected < lb->top)
            lb->top = lb->selected;
        else if (lb->top + in_view <= lb->selected)
            lb->top = lb->selected - in_view + 1;
        if (lb->top < 0)
            lb->top = 0;
        if (lb->top > beyond)
            lb->top = beyond;
        lb->vbar->pos = lb->top;
    } else {
        bar->page = in_view;
        bar->max_pos = 0;
        bar->total = (int)ROW_COUNT(lb);
        ui_window_invalidate_base(&bar->base);
        if (bar->pos > bar->max_pos)
            bar->pos = bar->max_pos;
    }
    ui_window_invalidate_base(&lb->vbar->base);
    if (ui_window_get_ancestor(&lb->base))
        ui_window_invalidate(ui_window_get_ancestor(&lb->base));
}

struct UIComboBoxID *ui_combo_box_id_construct(struct UIComboBoxID *self)
{
    ui_list_box_id_construct(&self->list);
    self->list.list.base.ops = &ui_combo_box_id_ops.base;
    tree_init(&self->values);
    self->opener_id = COMBO_NO_OPENER;
    return self;
}

void ui_combo_box_id_destroy(struct UIWindow *window)
{
    struct UIComboBoxID *self = (struct UIComboBoxID *)window;
    self->list.list.base.ops = &ui_combo_box_id_ops.base;
    tree_free(&self->values, NULL);
    ui_list_box_id_destruct(&self->list);
    mem_free(self);
}

void ui_combo_box_id_on_draw(struct UIWindow *window)
{
    struct UIComboBoxID *self = (struct UIComboBoxID *)window;
    struct UIListBoxEx *lb = &self->list.list;
    struct UIWindow *wnd = &lb->base;
    int first, end, line;

    draw_list_frame(wnd);
    first = lb->top;
    end = wnd->height / lb->line_height - lb->extra_lines + first;
    if ((unsigned)end >= ROW_COUNT(lb))
        end = (int)ROW_COUNT(lb);
    for (line = first; line < end; line++) {
        if (line == lb->selected)
            ui_window_fill_rect(wnd, 3, (line - first) * lb->line_height + 1, wnd->width - 6,
                              lb->line_height - 2, ui_skin_colour(6, 2));
        ui_window_text_out(wnd, 3 - lb->scroll_x, (line - first) * lb->line_height + 2,
                         string_cstr(&ui_list_box_id_row_at(&lb->rows, &line)->text), 0, 0, 12, 0, 0, 0);
    }
    if (lb->has_vscroll)
        ui_window_fill_rect(wnd, wnd->width - 16, 0, 16, wnd->height, ui_skin_colour(2, 2));
    ui_window_invalidate(wnd->parent);
}

static void report_id_choice(struct UIComboBoxID *self, struct UIWindow *to)
{
    struct UIListBoxEx *lb = &self->list.list;
    struct IntIntPair *pair = tree_find(&self->values, &lb->selected, tree_less_int);
    int value = pair ? pair->value : 0;

    ui_window_send_msg(to, NULL, COMBO_MSG_CHOSEN, ui_list_box_id_row_at(&lb->rows, &lb->selected)->id, value, 0,
             0);
}

void ui_combo_box_id_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIComboBoxID *self = (struct UIComboBoxID *)window;
    struct UIListBoxEx *lb = &self->list.list;
    struct UIWindow *opener;

    lb->selected = hit_row(lb, x, y);
    opener = ui_get_window(self->opener_id);
    if (opener)
        report_id_choice(self, opener);
    else if (lb->base.parent)
        report_id_choice(self, lb->base.parent);
    ui_close_window(UI_WND_COMBO_BOX);
}

int ui_combo_box_id_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIComboBoxID *self = (struct UIComboBoxID *)window;
    struct UIListBoxEx *lb = &self->list.list;
    struct UIWindow *opener;
    struct IntIntPair pair;

    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        opener = ui_get_window(self->opener_id);
        if (opener)
            report_id_choice(self, opener);
        if (g_windowMgr.combo_box_wnd)
            ui_close_window(UI_WND_COMBO_BOX);
        return 0;
    case UI_MSG_BAR_PAGE_UP:
        if (!ROW_COUNT(lb))
            return 0;
        if (--lb->selected < 0)
            lb->selected = 0;
        if (lb->selected < lb->top)
            lb->top = lb->selected;
        break;
    case UI_MSG_BAR_PAGE_DOWN:
        if (!ROW_COUNT(lb))
            return 0;
        if ((unsigned)++lb->selected >= ROW_COUNT(lb))
            lb->selected = (int)ROW_COUNT(lb) - 1;
        if (lb->top <= lb->extra_lines - lb->base.height / lb->line_height + lb->selected)
            lb->top = lb->extra_lines - lb->base.height / lb->line_height + lb->selected + 1;
        break;
    case COMBO_MSG_ADD_LINE:
        pair.key = (int)ROW_COUNT(lb);
        ui_list_box_id_add_item(&self->list, (const char *)val1, val2);
        if (val3) {
            pair.value = val3;
            tree_add(&self->values, &pair, sizeof pair, tree_less_int, NULL);
        }
        ui_window_invalidate_base(&lb->base);
        return 0;
    case COMBO_MSG_SELECT:
        lb->selected = val1;
        if (lb->selected < 0)
            lb->selected = 0;
        if ((unsigned)lb->selected > ROW_COUNT(lb) - 1)
            lb->selected = (int)ROW_COUNT(lb) - 1;
        break;
    case COMBO_MSG_OPENER_ID:
        self->opener_id = val1;
        return 0;
    default:
        return ui_list_box_ex_send_msg((struct UIWindow *)(lb), sender, msg, val1, val2, val3, val4);
    }
    ui_combo_box_ex_update_scroll_bar(lb);
    ui_window_invalidate_base(&lb->base);
    return 0;
}

struct UIComboBoxEx2 *ui_combo_box_ex2_construct(struct UIComboBoxEx2 *self)
{
    ui_combo_box_ex_construct(&self->ex);
    self->ex.list.base.ops = &ui_combo_box_ex2_ops.base;
    self->style = 0;
    return self;
}

void ui_combo_box_ex2_destroy(struct UIWindow *window)
{
    struct UIComboBoxEx2 *self = (struct UIComboBoxEx2 *)window;
    self->ex.list.base.ops = &ui_combo_box_ex2_ops.base;
    self->ex.list.base.ops = &ui_combo_box_ex_ops.base;
    ui_list_box_ex_destruct(&self->ex.list);
    mem_free(self);
}

void ui_combo_box_ex2_resize(struct UIWindow *window, int width, int height)
{
    struct UIComboBoxEx2 *self = (struct UIComboBoxEx2 *)window;
    struct UIListBoxEx *lb = &self->ex.list;

    ui_window_resize_base(&lb->base, width, height);
    ui_combo_box_ex_update_scroll_bar(lb);
    ui_window_move_base(&lb->vbar->base, width - 7, 0);
    if (lb->has_vscroll) {
        ui_window_invalidate_base(&lb->vbar->base);
    } else {
        ui_scroll_bar_resize(&lb->vbar->base, 7, height);
        ui_window_move_base(&lb->vbar->base, width - 6, 0);
    }
}

void ui_combo_box_ex2_on_create(struct UIWindow *window, int width, int height)
{
    struct UIComboBoxEx2 *self = (struct UIComboBoxEx2 *)window;
    struct UIListBoxEx *lb = &self->ex.list;
    void *mem = mem_alloc(sizeof(struct UIScrollBar));
    struct UIScrollBar *bar = NULL;
    int thickness;

    if (mem) {
        bar = ui_scroll_bar_construct(mem);
        bar->base.ops = &ui_scroll_bar_navi_ops;
        bar->button_size = 7;
        bar->thickness = 7;
    }
    lb->vbar = bar;
    thickness = bar->thickness;
    bar->orientation = UI_SCROLL_VERT;
    ui_window_resize_base(&bar->base, thickness, height);
    ui_window_move_base(&bar->base, width - 7, 0);
    ui_window_on_create_base(&bar->base, thickness, height);
    ui_window_add_child(&lb->base, &lb->vbar->base);

    mem = mem_alloc(sizeof(struct UIScrollBar));
    bar = mem ? ui_scroll_bar_construct(mem) : NULL;
    lb->hbar = bar;
    thickness = bar->thickness;
    bar->orientation = UI_SCROLL_HORZ;
    ui_window_resize_base(&bar->base, width - 7, thickness);
    ui_window_move_base(&bar->base, 0, height - 7);
    ui_window_on_create_base(&bar->base, width - 7, thickness);
    ui_window_add_child(&lb->base, &lb->hbar->base);
    lb->line_height = 0x12;
}

struct UIWindow *ui_combo_box_def_wnd_construct(struct UIWindow *wnd)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)wnd;

    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_combo_box_def_wnd_ops;
    self->button = NULL;
    self->list = NULL;
    self->label = NULL;
    self->no_border = 0;
    self->open = 0;
    self->back_r = 0xff;
    self->back_g = 0xff;
    self->back_b = 0xff;
    self->border_r = 0;
    self->border_g = 0;
    self->border_b = 0;
    self->list_height = 0x28;
    return wnd;
}

void ui_combo_box_def_wnd_destroy(struct UIWindow *window)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)window;
    self->base.ops = &ui_combo_box_def_wnd_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

static void set_button_bitmap(struct UIBitmapButton *button, const struct String *dir, const char *name,
                              int state)
{
    struct String path;

    string_copy(&path, dir);
    string_append_cstr(&path, name);
    string_append_cstr(&path, ".bmp");
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

static void make_button(struct UIComboBoxDefWnd *self, const struct String *dir, const char *out,
                        const char *over, const char *press)
{
    void *mem = mem_alloc(sizeof(struct UIBitmapButton));
    struct UIBitmapButton *button = mem ? ui_bitmap_button_construct(mem) : NULL;

    self->button = button;
    set_button_bitmap(button, dir, out, 0);
    set_button_bitmap(self->button, dir, over, 1);
    set_button_bitmap(self->button, dir, press, 2);
    ui_window_create(&self->button->button.base, self->button->bitmap_width, self->button->bitmap_height);
    ui_window_move_base(&self->button->button.base, self->base.width - self->button->bitmap_width, 0);
    self->button->button.base.id = UI_ID_COMBO_BUTTON;
    ui_window_add_child(&self->base, &self->button->button.base);
}

void ui_combo_box_def_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)window;
    struct String dir;
    void *mem;

    string_init(&dir);
    string_assign_cstr(&dir, UI_SKIN "basic_interface\\");
    make_button(self, &dir, "txtbox_btn_a", "txtbox_btn_b", "txtbox_btn_c");

    mem = mem_alloc(sizeof(struct UIComboBoxEx));
    self->list = mem ? &ui_combo_box_ex_construct(mem)->list.base : NULL;
    ui_window_create(self->list, self->base.width, self->list_height);
    ui_window_move(self->list, 0, self->button->bitmap_height);
    ui_window_add_child(&self->base, self->list);

    mem = mem_alloc(sizeof(struct UIStaticText));
    self->label = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(self->label, self->base.width - self->button->bitmap_width, self->button->bitmap_height);
    ui_static_text_set_text(self->label, "(NO SELECT)", 0);
    ui_window_move(self->label, 2, 3);
    ((struct UIStaticText *)self->label)->white_shadow = 1;
    ui_window_add_child(&self->base, self->label);
    string_free(&dir);
}

void ui_combo_box_def_wnd_on_draw(struct UIWindow *window)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)window;
    RECT rect;

    if (self->no_border) {
        ui_window_clear_canvas(&self->base, RGB(self->back_r, self->back_g, self->back_b));
        return;
    }
    rect.left = 0;
    rect.top = 0;
    rect.right = self->base.width;
    rect.bottom = self->base.height;
    ui_window_fill_canvas(&self->base, RGB(self->border_r, self->border_g, self->border_b), &rect);
    SetRect(&rect, 1, 1, self->base.width - 1, self->base.height - 1);
    ui_window_fill_canvas(&self->base, RGB(self->back_r, self->back_g, self->back_b), &rect);
}

void ui_combo_box_def_wnd_on_l_btn_up(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)window;
    ui_window_send_msg(&self->base, NULL, UI_MSG_COMMAND, UI_ID_COMBO_BUTTON, 0, 0, 0);
}

static void show_row(struct UIComboBoxDefWnd *self, int row)
{
    struct UIListBoxEx *lb = (struct UIListBoxEx *)self->list;

    ui_static_text_set_text(self->label, string_cstr(&list_box_ex_entry_at(&lb->rows, &row)->text), 0);
}

int ui_combo_box_def_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)window;
    struct UIWindow *wnd = &self->base;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 != UI_ID_COMBO_BUTTON)
            return 0;
        self->open = !self->open;
        if (self->open)
            ui_window_resize_base(&self->base, wnd->width, self->list->height + wnd->height);
        else
            ui_window_resize_base(&self->base, wnd->width, wnd->height - self->list->height);
        ui_window_invalidate(wnd->parent);
        ui_window_invalidate_base(&self->base);
        ui_window_invalidate(self->list);
        if (ui_window_get_ancestor(wnd))
            ui_window_invalidate(ui_window_get_ancestor(wnd));
        if (wnd->parent)
            ui_window_send_msg(wnd->parent, NULL, COMBO_MSG_TOGGLED, 0, 0, 0, 0);
        return 0;
    case COMBO_MSG_CHOSEN:
        if (val1 < 0)
            return 0;
        show_row(self, val1);
        ui_window_send_msg(&self->base, NULL, UI_MSG_COMMAND, UI_ID_COMBO_BUTTON, 0, 0, 0);
        ui_window_send_msg(wnd->parent, wnd, COMBO_MSG_CHOSEN, val1, 0, 0, 0);
        return 0;
    case COMBO_MSG_SET_LABEL:
        if (val1 < 0)
            return 0;
        show_row(self, val1);
        ui_window_invalidate_base(&self->base);
        return 0;
    default:
        return ui_window_send_msg_base(wnd, sender, msg, val1, val2, val3, val4);
    }
}

void ui_combo_box_def_wnd_clear_list(struct UIWindow *wnd)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)wnd;

    ui_list_box_ex_clear((struct UIListBoxEx *)self->list);
}

void ui_combo_box_def_wnd_select(struct UIWindow *wnd, int index)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)wnd;

    ui_window_send_msg(self->list, NULL, COMBO_MSG_SELECT, index, 0, 0, 0);
    show_row(self, index);
}

void ui_combo_box_def2_wnd_add_item(struct UIWindow *wnd, struct String text, int value)
{
    struct UIComboBoxDefWnd *self = (struct UIComboBoxDefWnd *)wnd;

    ui_window_send_msg(self->list, NULL, COMBO_MSG_ADD_LINE, (int)string_cstr(&text), value, 0, 0);
    string_free(&text);
}

struct UIWindow *ui_combo_box_def2_wnd_construct(struct UIWindow *wnd)
{
    struct UIComboBoxDef2Wnd *self = (struct UIComboBoxDef2Wnd *)wnd;

    ui_combo_box_def_wnd_construct(wnd);
    self->def.base.ops = &ui_combo_box_def2_wnd_ops;
    self->login_skin = 0;
    self->style = 0;
    self->label_y = 0;
    self->label_x = 0;
    return wnd;
}

void ui_combo_box_def2_wnd_destroy(struct UIWindow *window)
{
    struct UIComboBoxDef2Wnd *self = (struct UIComboBoxDef2Wnd *)window;
    self->def.base.ops = &ui_combo_box_def2_wnd_ops;
    self->def.base.ops = &ui_combo_box_def_wnd_ops;
    ui_window_destruct(&self->def.base);
    mem_free(self);
}

static int centred_label_x(struct UIComboBoxDef2Wnd *self)
{
    struct String text;
    float half = (float)self->def.base.width * 0.5f;
    int x;

    string_copy(&text, &((struct UIStaticText *)self->def.label)->text);
    x = (int)(half - (float)ui_window_text_width(&self->def.base, string_cstr(&text), 0, 0, 12, 0, 0) * 0.5f);
    string_free(&text);
    return x;
}

void ui_combo_box_def2_wnd_on_create(struct UIWindow *window, int width, int height_unused)
{
    struct UIComboBoxDef2Wnd *self = (struct UIComboBoxDef2Wnd *)window;
    struct UIComboBoxDefWnd *def = &self->def;
    struct String dir;
    struct UIComboBoxEx2 *list;
    void *mem;

    string_init(&dir);
    string_assign_cstr(&dir, UI_SKIN "UIadventurerAgency\\");
    if (self->login_skin == 1)
        string_assign_cstr(&dir, UI_SKIN "login_interface\\");
    make_button(def, &dir, "btn_dropdown_out", "btn_dropdown_over", "btn_dropdown_press");

    mem = mem_alloc(sizeof(struct UIComboBoxEx2));
    list = mem ? ui_combo_box_ex2_construct(mem) : NULL;
    def->list = &list->ex.list.base;
    list->style = self->style;
    ui_window_create(def->list, width, def->list_height + 2);
    ui_window_move(def->list, 0, def->button->bitmap_height);
    ui_window_add_child(&def->base, def->list);

    mem = mem_alloc(sizeof(struct UIStaticText));
    def->label = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(def->label, def->base.width - def->button->bitmap_width, def->button->bitmap_height - 6);
    ui_static_text_set_text(def->label, "", 0);
    if (self->style == 0)
        ui_window_move(def->label, 2, self->label_y + 3);
    else if (self->style == 1)
        ui_window_move(def->label, centred_label_x(self), self->label_y + 3);
    ((struct UIStaticText *)def->label)->white_shadow = 1;
    ui_window_add_child(&def->base, def->label);
    string_free(&dir);
}

void ui_combo_box_def2_wnd_on_draw(struct UIWindow *window)
{
    struct UIComboBoxDef2Wnd *self = (struct UIComboBoxDef2Wnd *)window;
    int x;

    ui_window_clear_canvas(&self->def.base, UI_TRANSPARENT);
    if (self->style != 1)
        return;
    x = centred_label_x(self);
    if (x == self->label_x)
        return;
    self->label_x = x;
    ui_window_move(self->def.label, centred_label_x(self), self->label_y + 3);
    ui_window_invalidate(self->def.label);
}

struct UIComboBoxBmpWnd *ui_combo_box_bmp_wnd_construct(struct UIComboBoxBmpWnd *self)
{
    ui_combo_box_def_wnd_construct(&self->def.base);
    self->def.base.ops = &ui_combo_box_bmp_wnd_ops;
    string_init(&self->bitmap);
    return self;
}

void ui_combo_box_bmp_wnd_destroy(struct UIWindow *window)
{
    struct UIComboBoxBmpWnd *self = (struct UIComboBoxBmpWnd *)window;
    self->def.base.ops = &ui_combo_box_bmp_wnd_ops;
    string_free(&self->bitmap);
    self->def.base.ops = &ui_combo_box_def_wnd_ops;
    ui_window_destruct(&self->def.base);
    mem_free(self);
}

void ui_combo_box_bmp_wnd_on_draw(struct UIWindow *window)
{
    struct UIComboBoxBmpWnd *self = (struct UIComboBoxBmpWnd *)window;
    struct UIComboBoxDefWnd *def = &self->def;

    ui_window_clear_canvas(&def->base, RGB(def->back_r, def->back_g, def->back_b));
    ui_window_draw_bitmap(&def->base, 0, 0, ui_skin_bitmap(string_cstr(&self->bitmap)), 0);
}

static void set_bmp_button_bitmap(struct UIBitmapButton *button, const struct String *name, const char *suffix,
                                  size_t suffix_len, int state)
{
    struct String path;

    string_init(&path);
    string_reserve(&path, name->size + suffix_len);
    string_append(&path, string_cstr(name), name->size);
    string_append(&path, suffix, suffix_len);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

void ui_combo_box_bmp_wnd_create(struct UIComboBoxBmpWnd *self, int width, int height, const char *name,
                                        const char *bitmap)
{
    struct UIComboBoxDefWnd *def = &self->def;
    struct String base;
    char text[0xc] = { 0 };
    void *mem;

    ui_window_resize_base(&def->base, width, height);
    string_assign_cstr(&self->bitmap, bitmap);
    string_init(&base);
    string_assign_cstr(&base, name);

    mem = mem_alloc(sizeof(struct UIBitmapButton));
    def->button = mem ? ui_bitmap_button_construct(mem) : NULL;
    set_bmp_button_bitmap(def->button, &base, "_out.bmp", 8, 0);
    set_bmp_button_bitmap(def->button, &base, "_over.bmp", 9, 1);
    set_bmp_button_bitmap(def->button, &base, "_press.bmp", 10, 2);
    ui_window_create(&def->button->button.base, def->button->bitmap_width, def->button->bitmap_height);
    ui_window_move_base(&def->button->button.base, def->base.width - def->button->bitmap_width, 0);
    def->button->button.base.id = UI_ID_COMBO_BUTTON;
    ui_window_add_child(&def->base, &def->button->button.base);

    mem = mem_alloc(sizeof(struct UIComboBoxEx));
    def->list = mem ? &ui_combo_box_ex_construct(mem)->list.base : NULL;
    ui_window_create(def->list, def->base.width, def->list_height);
    ui_window_move(def->list, 0, def->button->bitmap_height);
    ui_window_add_child(&def->base, def->list);

    sprintf(text, "(NO SELECT)");
    mem = mem_alloc(sizeof(struct UIStaticText));
    def->label = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(def->label, def->base.width - def->button->bitmap_width, def->button->bitmap_height);
    ui_static_text_set_text(def->label, text, 0);
    ui_window_move(def->label, 2, 3);
    ((struct UIStaticText *)def->label)->white_shadow = 1;
    ui_window_add_child(&def->base, def->label);
    string_free(&base);
}

void ui_combo_box_v2_wnd_destroy(struct UIWindow *window)
{
    struct UIComboBoxV2Wnd *self = (struct UIComboBoxV2Wnd *)window;
    self->base.ops = &ui_combo_box_v2_wnd_ops;
    string_free(&self->bitmap);
    ui_window_destruct(&self->base);
    mem_free(self);
}

struct String *ui_combo_box_v2_wnd_get_selected_text(struct UIComboBoxV2Wnd *self, struct String *out)
{
    int row = self->selected;

    if (row < 0 || (int)self->list->rows.size <= row) {
        string_init(out);
        return out;
    }
    string_copy(out, &list_box_ex_entry_at(&self->list->rows, &row)->text);
    return out;
}

void ui_combo_box_v2_wnd_on_draw(struct UIWindow *window)
{
    struct UIComboBoxV2Wnd *self = (struct UIComboBoxV2Wnd *)window;
    struct String text;

    ui_window_clear_canvas(&self->base, UI_TRANSPARENT);
    if (self->bitmap.size)
        ui_window_draw_bitmap(&self->base, 0, 0, ui_skin_bitmap(string_cstr(&self->bitmap)), 0);
    ui_combo_box_v2_wnd_get_selected_text(self, &text);
    ui_window_text_out(&self->base, 2, 3, string_cstr(&text), 0, 0, 12, 0, 0, 0);
    string_free(&text);
}

void ui_combo_box_v2_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIComboBoxV2Wnd *self = (struct UIComboBoxV2Wnd *)window;
    self->selected += delta < 0 ? 1 : -1;
    if ((int)self->list->rows.size <= self->selected)
        self->selected = (int)self->list->rows.size - 1;
    if (self->selected < 0)
        self->selected = 0;
    if (self->base.parent)
        ui_window_send_msg(self->base.parent, &self->base, COMBO_MSG_CHOSEN, self->selected, 0, 0, 0);
    ui_window_invalidate_base(&self->base);
}

int ui_combo_box_v2_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIComboBoxV2Wnd *self = (struct UIComboBoxV2Wnd *)window;
    struct UIWindow *wnd = &self->base;
    int height;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 != UI_ID_COMBO_BUTTON)
            return 0;
        height = self->head->height;
        if (wnd->height <= height)
            height += self->list->base.height;
        ui_window_resize(&self->base, wnd->width, height);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
        ui_window_invalidate_base(&self->base);
        return 0;
    case COMBO_MSG_CHOSEN:
        ui_window_send_msg(&self->base, NULL, UI_MSG_COMMAND, UI_ID_COMBO_BUTTON, 0, 0, 0);
        self->selected = val1;
        if (wnd->parent)
            ui_window_send_msg(wnd->parent, wnd, COMBO_MSG_CHOSEN, val1, 0, 0, 0);
        ui_window_invalidate_base(&self->base);
        return 0;
    default:
        return ui_window_send_msg_base(wnd, sender, msg, val1, val2, val3, val4);
    }
}

#define EX2_FRAME 0xc59582
#define EX2_FACE 0xfcf4f0
#define EX2_SELECTION 0xf8d5c4
#define EX2_FONT 0xc
void ui_combo_box_ex2_on_draw(struct UIWindow *window)
{
    struct UIComboBoxEx2 *self = (struct UIComboBoxEx2 *)window;
    struct UIListBoxEx *lb = &self->ex.list;
    unsigned end;
    int top = lb->top, i, idx;

    ui_window_clear_canvas(&lb->base, EX2_FRAME);
    ui_window_fill_rect(&lb->base, 1, 1, lb->base.width - 2, lb->base.height - 2, EX2_FACE);
    end = (unsigned)(lb->base.height / lb->line_height - lb->extra_lines + top);
    if (end >= lb->rows.size)
        end = lb->rows.size;
    for (i = 0, idx = top; idx < (int)end; idx++, i++)
        if (idx == lb->selected)
            ui_window_fill_rect(&lb->base, 3, lb->line_height * i + 1, lb->base.width - 6, lb->line_height - 2,
                              EX2_SELECTION);
    for (idx = top; idx < (int)end; idx++) {
        struct UIListBoxExRow *row;
        int key = idx, x;

        if (self->style == 0) {
            row = list_box_ex_entry_at(&lb->rows, &key);
            x = 3 - lb->scroll_x;
        } else if (self->style == 1) {
            float half = (float)lb->base.width * 0.5f;
            int w;

            row = list_box_ex_entry_at(&lb->rows, &key);
            w = ui_window_text_width(&lb->base, string_cstr(&row->text), 0, 0, EX2_FONT, 0, 0);
            x = (int)(half - (float)w * 0.5f);
        } else {
            continue;
        }
        ui_window_text_out(&lb->base, x, (idx - top) * lb->line_height + 2, string_cstr(&row->text), 0, 0,
                         EX2_FONT, row->colour, 0, 0);
    }
    if (lb->base.parent)
        ui_window_invalidate(lb->base.parent);
}
