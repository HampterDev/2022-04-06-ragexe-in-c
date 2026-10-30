#include "ui/controls/listbox.h"
#include "ui/chat/chat_history.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/social/social.h"
#include "diag/perf.h"
#include "ui/chat/chat_room_wnd.h"

#define MEMBER_LIST_SELECTED_COLOUR 0xc8c832
#define UI_MSG_MEMBER_LIST_MENU 0x31

SIZE_ASSERT(UIMemberListBox, 0x12c);
struct UIWindow;
void ui_member_list_box_destroy(struct UIWindow *window);
void ui_member_list_box_on_draw(struct UIWindow *self);
void ui_member_list_box_choose(struct UIWindow *self, int x, int y);
void ui_member_list_box_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_member_list_box_on_r_btn_up(struct UIWindow *self, int x, int y);
const struct UIListBoxOps ui_member_list_box_ops = {
    .base = {
        .destroy = ui_member_list_box_destroy,
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
        .on_draw = ui_member_list_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_member_list_box_choose,
        .on_l_btn_dbl_clk = ui_chat_his_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_member_list_box_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_chat_his_box_on_l_btn_up,
        .on_r_btn_down = ui_member_list_box_choose,
        .on_r_btn_up = ui_member_list_box_on_r_btn_up,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_chat_his_box_send_msg,
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
    .create = ui_chat_his_box2_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_chat_his_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

struct UIMemberListBox *ui_member_list_box_construct(struct UIMemberListBox *self)
{
    ui_chat_his_box2_construct(&self->base);
    self->base.box.list.base.ops = &ui_member_list_box_ops.base;
    return self;
}

static void ui_member_list_box_destruct(struct UIMemberListBox *self)
{
    self->base.box.list.base.ops = &ui_member_list_box_ops.base;
    self->base.box.list.base.ops = &ui_chat_his_box2_ops.base;
    ui_chat_his_box_destruct(&self->base.box);
}

void ui_member_list_box_destroy(struct UIWindow *window)
{
    struct UIMemberListBox *self = (struct UIMemberListBox *)window;
    ui_member_list_box_destruct(self);
    mem_free(self);
}

static int visible_end(struct UIMemberListBox *self)
{
    struct UIWindow *wnd = &self->base.box.list.base;
    unsigned end = (unsigned)(wnd->height / self->base.box.list.line_height - self->base.box.list.extra_lines +
                              self->base.box.list.top);
    unsigned count = VEC_COUNT(&self->base.box.list.items, struct String);

    return (int)(end < count ? end : count);
}

void ui_member_list_box_on_draw(struct UIWindow *window)
{
    struct UIMemberListBox *self = (struct UIMemberListBox *)window;
    struct UIWindow *wnd = &self->base.box.list.base;
    const struct String *lines = self->base.box.list.items.first;
    const DWORD *colours = self->base.box.colours.first;
    int line_height = self->base.box.list.line_height;
    int end, i, row;

    ui_window_clear_canvas(wnd, RGB((BYTE)self->base.back_r, (BYTE)self->base.back_g, (BYTE)self->base.back_b));
    end = visible_end(self);
    for (i = self->base.box.list.top, row = 0; i < end; i++, row++) {
        if (i == self->base.box.list.selected)
            ui_window_fill_rect(wnd, 0, row * line_height + 1, wnd->width, line_height,
                              MEMBER_LIST_SELECTED_COLOUR);
        ui_window_text_out(wnd, 2, row * line_height + 3, string_cstr(&lines[i]), 0, 0, 0xc, colours[i], 0,
                         0);
    }
}

void ui_member_list_box_choose(struct UIWindow *window, int x, int y)
{
    struct UIMemberListBox *self = (struct UIMemberListBox *)window;
    self->base.box.list.selected = ui_list_box_hit_line(&self->base.box.list, x, y);
    ui_window_invalidate_base(&self->base.box.list.base);
}

void ui_member_list_box_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMemberListBox *self = (struct UIMemberListBox *)window;
    struct UIWindow *wnd = &self->base.box.list.base;
    int line_height = self->base.box.list.line_height;
    int top = self->base.box.list.top;
    int end = visible_end(self);
    int line = y / line_height + top;
    const char *text;
    int sx, sy;

    if (line < top || line >= end)
        return;
    text = string_cstr((struct String *)self->base.box.list.items.first + line);
    if (ui_window_text_width(wnd, text, 0, 0, 0xc, 0, 0) <= wnd->width - 15 * self->base.box.list.has_vscroll)
        return;
    sx = -4;
    sy = y - y % line_height;
    ui_window_to_screen(wnd, &sx, &sy);
    mode_show_tooltip(client_current_mode(), text, sx, sy, -1, 0);
}

void ui_member_list_box_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIMemberListBox *self = (struct UIMemberListBox *)window;
    struct UIWindow *parent = self->base.box.list.base.parent;

    if (parent)
        ui_window_send_msg(parent, NULL, UI_MSG_MEMBER_LIST_MENU, 0, 0, 0, 0);
}
