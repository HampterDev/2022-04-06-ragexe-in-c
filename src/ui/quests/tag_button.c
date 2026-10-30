#include "ui/controls/buttons.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/quests/tag_list_box.h"
#include "diag/perf.h"

#define UI_MSG_TAG_BUTTON_CLICK 0x62
#define UI_MSG_EDIT_INSERT_ITEM 0x94

struct UITagButton;
struct UIWindow;
void ui_tag_button_destroy(struct UIWindow *window);
void ui_tag_button_on_l_btn_up(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_tag_button_ops = {
    .base = {
        .destroy = ui_tag_button_destroy,
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
        .on_draw = ui_text_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_text_button_on_mouse_move,
        .on_mouse_hover = ui_text_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_tag_button_on_l_btn_up,
        .on_r_btn_down = ui_text_button_on_r_btn_down,
        .on_r_btn_up = ui_text_button_on_r_btn_up,
        .on_wheel = ui_text_button_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_window_send_msg_base,
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
    .set_show = ui_button_set_show,
};

struct UITagButton *ui_tag_button_construct(struct UITagButton *self)
{
    ui_text_button_construct(&self->button);
    self->button.button.base.ops = &ui_tag_button_ops.base;
    item_info_construct(&self->item);
    return self;
}

static void ui_tag_button_destruct(struct UITagButton *self)
{
    self->button.button.base.ops = &ui_tag_button_ops.base;
    string_free(&self->item.name);
    string_free(&self->item.id_text);
    ui_text_button_destruct(&self->button);
}

void ui_tag_button_destroy(struct UIWindow *window)
{
    struct UITagButton *self = (struct UITagButton *)window;
    ui_tag_button_destruct(self);
    free(self);
}

void ui_tag_button_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UITagButton *self = (struct UITagButton *)window;
    struct UIWindow *wnd = &self->button.button.base;

    if (self->button.button.disabled)
        return;
    if (GetAsyncKeyState(VK_SHIFT) & 0xff00) {
        struct UIWindow *edit;

        if (!ui_window_mgr_get_focus_edit(&g_windowMgr) || !self->item.identified)
            return;
        edit = ui_window_mgr_get_focus_edit(&g_windowMgr);
        ui_window_send_msg(edit, NULL, UI_MSG_EDIT_INSERT_ITEM, (int)&self->item, 0, 0, 0);
        return;
    }
    if (ui_window_mgr_get_capture(&g_windowMgr) != wnd)
        return;
    if (x >= 0 && x < wnd->width && y >= 0 && y < wnd->height) {
        wnd->state = 1;
        if (wnd->parent)
            ui_window_send_msg(wnd->parent, wnd, UI_MSG_TAG_BUTTON_CLICK, wnd->id, self->button.value, 0, 0);
    } else {
        wnd->state = 0;
    }
    ui_window_invalidate_base(wnd);
    ui_window_mgr_release_capture(&g_windowMgr);
}

struct UITagButton *ui_tag_button_create(void)
{
    struct UITagButton *self = malloc(sizeof *self);
    if (!self) abort();
    return ui_tag_button_construct(self);
}
