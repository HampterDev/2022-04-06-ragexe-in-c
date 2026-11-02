#include "ui/controls/edit.h"
#include "ui/controls/popups.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <stdlib.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/ui_items.h"
#include "diag/perf.h"

#define CNT_MSG_REQUEST 0x50
#define CNT_MSG_SET_COUNT 0x51
#define CNT_EDIT_MAX_CHARS 6

struct UICntWnd;
struct UIWindow;
const struct UIWindowOps ui_cnt_wnd_ops = {
    .destroy = ui_cnt_wnd_destroy,
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
    .on_create = ui_cnt_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_cnt_wnd_on_draw,
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
    .send_msg = ui_cnt_wnd_send_msg,
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

struct UICntWnd {
    struct UIFrameWnd frame;
    int request;
    struct UIWindow *edit;
    struct UIWindow *text;
};

static struct UIWindow *ui_cnt_wnd_construct(struct UICntWnd *self)
{
    ui_window_construct(&self->frame.base, 0);
    self->frame.base.ops = &ui_frame_wnd_ops;
    self->frame.press_x = 0;
    self->frame.press_y = 0;
    self->frame.default_id = UI_ID_NONE;
    self->frame.cancel_id = UI_ID_NONE;
    self->frame.base.ops = &ui_cnt_wnd_ops;
    return &self->frame.base;
}

struct UIWindow *ui_cnt_wnd_new(void)
{
    struct UICntWnd *self = mem_alloc(sizeof *self);

    return self ? ui_cnt_wnd_construct(self) : NULL;
}

void ui_cnt_wnd_destruct(struct UICntWnd *self)
{
    self->frame.base.ops = &ui_cnt_wnd_ops;
    self->frame.base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->frame.base);
}

void ui_cnt_wnd_destroy(struct UIWindow *window)
{
    struct UICntWnd *self = (struct UICntWnd *)window;
    ui_cnt_wnd_destruct(self);
    mem_free(self);
}

void ui_cnt_wnd_on_create(struct UIWindow *window, int cx, int cy)
{
    struct UICntWnd *self = (struct UICntWnd *)window;
    static const char *const ok_names[3] = { "btn_ok", "btn_ok_a", "btn_ok_b" };
    struct String dir;
    void *mem;

    mem = mem_alloc(sizeof(struct UIEditCtrl));
    self->edit = mem ? ui_edit_ctrl_construct(mem) : NULL;
    ui_window_create(self->edit, 0x50, 0x10);
    ui_window_move(self->edit, 0xf, 0x16);
    UI_EDIT_MAX_CHARS(self->edit) = CNT_EDIT_MAX_CHARS;
    ui_edit_ctrl_set_back_colour(self->edit, 0xe8, 0xe8, 0xe8);
    ui_window_add_child(&self->frame.base, self->edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->edit);
    ui_edit_ctrl_select_all(self->edit);

    mem = mem_alloc(sizeof(struct UIStaticText));
    self->text = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(self->text, self->frame.base.width - 0x1e, 0xf);
    ui_window_move(self->text, 0xf, 6);
    ui_window_add_child(&self->frame.base, self->text);

    ui_skin_dir(&dir);
    ui_add_named_skin_button(&self->frame.base, &dir, ok_names, self->frame.base.width - 0x37,
                       self->frame.base.height - 0x20, UI_ID_OK);
    string_free(&dir);
    self->frame.default_id = UI_ID_OK;
}

void ui_cnt_wnd_on_draw(struct UIWindow *window)
{
    struct UICntWnd *self = (struct UICntWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
}

intptr_t ui_cnt_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UICntWnd *self = (struct UICntWnd *)window;
    char digits[16];
    struct Mode *mode;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_OK) {
            mode = client_current_mode();
            mode_send_message(mode, self->request, atoi(ui_edit_ctrl_get_text(self->edit)), 0, 0, 0);
        }
        ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
        return 0;
    case UI_MSG_ADD_TEXT:
        ui_static_text_set_text(self->text, (const char *)val1, 0);
        return 0;
    case CNT_MSG_REQUEST:
        self->request = val1;
        return 0;
    case CNT_MSG_SET_COUNT:
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, _itoa(val1, digits, 10));
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
