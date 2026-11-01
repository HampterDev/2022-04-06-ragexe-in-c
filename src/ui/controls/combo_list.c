#include "ui/controls/listbox.h"
#include "ui/controls/combo_box.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "diag/perf.h"

struct UIComboBox;
struct UIWindow;
const struct UIListBoxOps ui_combo_box_ops = {
    .base = {
        .destroy = ui_combo_box_destroy,
        .resize = ui_combo_box_resize,
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
        .on_draw = ui_combo_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_combo_box_ignore_click,
        .on_l_btn_dbl_clk = ui_combo_box_ignore_click,
        .on_mouse_move = ui_combo_box_on_mouse_move,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_combo_box_on_l_btn_up,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_combo_box_send_msg,
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
    .update_scroll_bar = ui_combo_box_update_scroll_bar,
};

struct UIComboBoxWnd;
const struct UIWindowOps ui_combo_box_wnd_ops = {
    .destroy = ui_combo_box_wnd_destroy,
    .resize = ui_combo_box_wnd_resize,
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
    .on_create = ui_combo_box_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_combo_box_wnd_on_draw,
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
    .send_msg = ui_combo_box_wnd_send_msg,
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

static void ui_combo_box_id_wnd_resize(struct UIWindow *window, int width, int height);
const struct UIWindowOps ui_combo_box_id_wnd_ops = {
    .destroy = ui_combo_box_id_wnd_destroy,
    .resize = ui_combo_box_id_wnd_resize,
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
    .on_create = ui_combo_box_id_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_combo_box_wnd_on_draw,
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
    .send_msg = ui_combo_box_wnd_send_msg,
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

#define LINE_COUNT(lb) VEC_COUNT(&(lb)->items, struct String)
#define LINE(lb, i) (&((struct String *)(lb)->items.first)[i])

struct UIComboBox *ui_combo_box_construct(struct UIComboBox *self)
{
    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_combo_box_ops.base;
    self->opener_id = COMBO_NO_OPENER;
    return self;
}

void ui_combo_box_destroy(struct UIWindow *window)
{
    struct UIComboBox *self = (struct UIComboBox *)window;
    self->list.base.ops = &ui_combo_box_ops.base;
    ui_list_box_destruct(&self->list);
    mem_free(self);
}

void ui_combo_box_resize(struct UIWindow *window, int width, int height)
{
    struct UIComboBox *self = (struct UIComboBox *)window;
    struct UIListBox *lb = &self->list;

    ui_list_box_resize((struct UIWindow *)(lb), width, height);
    lb->top = 0;
    lb->vbar->pos = lb->top;
    ui_window_move_base(&lb->vbar->base, width - 13, 0);
    if (lb->has_vscroll)
        ui_window_invalidate_base(&lb->vbar->base);
}

void ui_combo_box_on_draw(struct UIWindow *window)
{
    struct UIComboBox *self = (struct UIComboBox *)window;
    struct UIListBox *lb = &self->list;
    struct UIWindow *wnd = &lb->base;
    struct BitmapRes *corner;
    int line, end, row;

    ui_window_clear_canvas(wnd, ui_skin_colour(6, 6));
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, ui_skin_colour(2, 2));
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_lu.bmp"), 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_ld.bmp");
    ui_window_draw_bitmap(wnd, 0, wnd->height - corner->height, corner, 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_ru.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - corner->width, 0, corner, 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_rd.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - corner->width, wnd->height - corner->height, corner, 0);

    line = lb->top;
    end = wnd->height / lb->line_height - lb->extra_lines + line;
    if ((unsigned)end > LINE_COUNT(lb))
        end = (int)LINE_COUNT(lb);
    for (row = 0; line < end; line++, row++) {
        if (line == lb->selected)
            ui_window_fill_rect(wnd, 3, row * lb->line_height + 1, wnd->width - 6, lb->line_height - 2,
                              ui_skin_colour(6, 2));
        ui_window_text_out(wnd, 3 - lb->scroll_x, lb->line_height * row + 2, string_cstr(LINE(lb, line)),
                         0, 0, 12, 0, 0, 0);
    }
    if (lb->has_vscroll)
        ui_window_fill_rect(wnd, wnd->width - 16, 0, 16, wnd->height, ui_skin_colour(2, 2));
    ui_window_invalidate(wnd->parent);
}

void ui_combo_box_ignore_click(struct UIWindow *self, int x_unused, int y_unused)
{
}

void ui_combo_box_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIComboBox *self = (struct UIComboBox *)window;
    int old = self->list.selected;

    self->list.selected = ui_list_box_hit_line(&self->list, x, y);
    if (old != self->list.selected)
        ui_window_invalidate_base(&self->list.base);
}

static void report_choice(struct UIComboBox *self, struct UIWindow *opener)
{
    ui_window_send_msg(opener, NULL, COMBO_MSG_CHOSEN, self->list.selected, 0, 0, 0);
}

void ui_combo_box_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIComboBox *self = (struct UIComboBox *)window;
    struct UIWindow *opener;

    self->list.selected = ui_list_box_hit_line(&self->list, x, y);
    opener = ui_get_window(self->opener_id);
    if (opener)
        report_choice(self, opener);
    else if (self->list.base.parent)
        report_choice(self, self->list.base.parent);
    ui_close_window(UI_WND_COMBO_BOX);
}

int ui_combo_box_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UIComboBox *self = (struct UIComboBox *)window;
    struct UIListBox *lb = &self->list;
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
        if (!LINE_COUNT(lb))
            return 0;
        if (--lb->selected < 0)
            lb->selected = 0;
        if (lb->selected < lb->top)
            lb->top = lb->selected;
        break;
    case UI_MSG_BAR_PAGE_DOWN:
        if (!LINE_COUNT(lb))
            return 0;
        if ((unsigned)++lb->selected >= LINE_COUNT(lb))
            lb->selected = (int)LINE_COUNT(lb) - 1;
        if (lb->top <= lb->extra_lines - lb->base.height / lb->line_height + lb->selected)
            lb->top = lb->extra_lines - lb->base.height / lb->line_height + lb->selected + 1;
        break;
    case COMBO_MSG_ADD_LINE:
        ui_list_box_add_item(lb, (const char *)val1);
        ui_window_invalidate_base(&lb->base);
        return 0;
    case COMBO_MSG_SELECT:
        lb->selected = val1;
        if (lb->selected < 0)
            lb->selected = 0;
        if ((unsigned)lb->selected > LINE_COUNT(lb) - 1)
            lb->selected = (int)LINE_COUNT(lb) - 1;
        break;
    case COMBO_MSG_OPENER_ID:
        self->opener_id = val1;
        return 0;
    default:
        return ui_list_box_send_msg((struct UIWindow *)(lb), sender, msg, val1, val2, val3, val4);
    }
    ui_combo_box_update_scroll_bar(lb);
    ui_window_invalidate_base(&lb->base);
    return 0;
}

void ui_combo_box_update_scroll_bar(struct UIListBox *base_list)
{
    struct UIListBox *lb = base_list;
    struct UIScrollBar *bar = lb->vbar;
    int in_view = lb->base.height / lb->line_height;
    int beyond = (int)LINE_COUNT(lb) - in_view;

    if (beyond < 0)
        beyond = 0;
    lb->has_vscroll = beyond != 0;
    if (beyond) {
        bar->max_pos = lb->extra_lines + beyond;
        bar->page = in_view;
        bar->total = (int)LINE_COUNT(lb);
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
        bar->total = (int)LINE_COUNT(lb);
        bar->max_pos = 0;
        bar->page = in_view;
        ui_window_invalidate_base(&bar->base);
        if (bar->pos > bar->max_pos)
            bar->pos = bar->max_pos;
    }
    ui_window_invalidate_base(&lb->vbar->base);
}

struct UIComboBoxWnd *ui_combo_box_wnd_construct(struct UIComboBoxWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_combo_box_wnd_ops;
    self->list = NULL;
    self->chosen_to = NULL;
    return self;
}

struct UIWindow *ui_combo_box_wnd_new(void)
{
    return (struct UIWindow *)ui_combo_box_wnd_construct(mem_alloc(sizeof(struct UIComboBoxWnd)));
}

struct UIComboBoxWnd *ui_combo_box_id_wnd_construct(struct UIComboBoxWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_combo_box_id_wnd_ops;
    self->list = NULL;
    self->chosen_to = NULL;
    return self;
}

struct UIWindow *ui_combo_box_id_wnd_new(void)
{
    return (struct UIWindow *)ui_combo_box_id_wnd_construct(mem_alloc(sizeof(struct UIComboBoxWnd)));
}

void ui_combo_box_wnd_destroy(struct UIWindow *window)
{
    struct UIComboBoxWnd *self = (struct UIComboBoxWnd *)window;
    self->frame.base.ops = &ui_combo_box_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_combo_box_id_wnd_destroy(struct UIWindow *window)
{
    struct UIComboBoxWnd *self = (struct UIComboBoxWnd *)window;
    self->frame.base.ops = &ui_combo_box_id_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_combo_box_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct UIComboBoxWnd *self = (struct UIComboBoxWnd *)window;
    ui_window_resize_base(&self->frame.base, width, height);
    ui_window_resize(self->list, width, height);
    ui_list_box_refresh_scroll((struct UIListBox *)self->list);
    ui_window_invalidate(self->list);
}

static void ui_combo_box_id_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct UIComboBoxWnd *self = (struct UIComboBoxWnd *)window;
    ui_window_resize_base(&self->frame.base, width, height);
    ui_window_resize(self->list, width, height);
    ui_list_box_ex_refresh_scroll((struct UIListBoxEx *)self->list);
    ui_window_invalidate(self->list);
}

void ui_combo_box_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIComboBoxWnd *self = (struct UIComboBoxWnd *)window;
    void *mem = mem_alloc(sizeof(struct UIComboBox));

    self->list = mem ? &ui_combo_box_construct(mem)->list.base : NULL;
    ui_list_box_create_window((struct UIListBox *)self->list, 0, 0, width, height, 1);
    ui_window_add_child(&self->frame.base, self->list);
}

void ui_combo_box_id_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIComboBoxWnd *self = (struct UIComboBoxWnd *)window;
    void *mem = mem_alloc(UI_COMBO_BOX_ID_SIZE);

    self->list = mem ? &ui_combo_box_id_construct(mem)->list.list.base : NULL;
    ui_list_box_ex_create((struct UIListBoxEx *)self->list, 0, 0, width, height, 1);
    ui_window_add_child(&self->frame.base, self->list);
}

void ui_combo_box_wnd_on_draw(struct UIWindow *window)
{
    struct UIComboBoxWnd *self = (struct UIComboBoxWnd *)window;
    ui_window_clear_canvas(&self->frame.base, UI_TRANSPARENT);
}

int ui_combo_box_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIComboBoxWnd *self = (struct UIComboBoxWnd *)window;
    if (self->chosen_to && msg == COMBO_MSG_CHOSEN)
        return ui_window_send_msg(self->chosen_to, &self->frame.base, COMBO_MSG_CHOSEN, val1, val2, val3, val4);
    return ui_window_send_msg(self->list, sender, msg, val1, val2, val3, val4);
}

void ui_combo_box_wnd_set_chosen_to(struct UIComboBoxWnd *self, struct UIWindow *chosen_to)
{
    if (chosen_to)
        self->chosen_to = chosen_to;
}
