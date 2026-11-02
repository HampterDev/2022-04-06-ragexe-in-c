#include "ui/controls/listbox.h"
#include "ui/controls/popups.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "diag/perf.h"

struct UIToolTipWnd {
    struct UIFrameWnd frame;
    struct UIListBox *list;
};
struct UIWindow;
const struct UIWindowOps ui_tool_tip_wnd_ops = {
    .destroy = ui_tool_tip_wnd_destroy,
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
    .on_create = ui_tool_tip_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_tool_tip_wnd_on_draw,
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
    .send_msg = ui_tool_tip_wnd_send_msg,
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

struct UIToolTipWnd *ui_tool_tip_wnd_construct(struct UIToolTipWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_tool_tip_wnd_ops;
    return self;
}

struct UIWindow *ui_tool_tip_wnd_new(void)
{
    return (struct UIWindow *)ui_tool_tip_wnd_construct(mem_alloc(sizeof(struct UIToolTipWnd)));
}

void ui_tool_tip_wnd_destroy(struct UIWindow *window)
{
    struct UIToolTipWnd *self = (struct UIToolTipWnd *)window;
    self->frame.base.ops = &ui_tool_tip_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
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

void ui_tool_tip_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIToolTipWnd *self = (struct UIToolTipWnd *)window;
    static const char *const names[2][3] = {
        { "btn_ok", "btn_ok_a", "btn_ok_b" },
        { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
    };
    static const int ids[2] = { UI_ID_OK, UI_ID_CANCEL };
    struct UIWindow *wnd = &self->frame.base;
    void *mem = mem_alloc(sizeof(struct UIListBox));
    struct UIListBox *list = mem ? ui_list_box_construct(mem) : NULL;
    int list_width = width - 24, list_height = height - 60;
    int pos[2][2];
    struct String dir;
    int i;

    self->list = list;
    if (list_width < 20)
        list_width = 20;
    if (list_height < 30)
        list_height = 30;
    ui_list_box_create_window(list, 12, 22, list_width, list_height, 1);
    self->list->base.id = UI_ID_OK;
    ui_list_box_append_text(self->list, "__________");
    ui_list_box_append_text(self->list, "**********");
    ui_window_add_child(wnd, &self->list->base);

    string_init(&dir);
    string_assign(&dir, UI_SKIN, 15);
    pos[0][0] = wnd->width - 91;
    pos[0][1] = wnd->height - 24;
    pos[1][0] = wnd->width - 46;
    pos[1][1] = wnd->height - 24;
    for (i = 0; i < 2; i++) {
        struct UIBitmapButton *button;

        mem = mem_alloc(sizeof(struct UIBitmapButton));
        button = mem ? ui_bitmap_button_construct(mem) : NULL;
        set_button_bitmap(button, &dir, names[i][0], 0);
        set_button_bitmap(button, &dir, names[i][1], 1);
        set_button_bitmap(button, &dir, names[i][2], 2);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, pos[i][0], pos[i][1]);
        button->button.base.id = ids[i];
        ui_window_add_child(wnd, &button->button.base);
    }
    string_free(&dir);
}

void ui_tool_tip_wnd_on_draw(struct UIWindow *window)
{
    struct UIToolTipWnd *self = (struct UIToolTipWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
}

intptr_t ui_tool_tip_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                    intptr_t val3, intptr_t val4)
{
    struct UIToolTipWnd *self = (struct UIToolTipWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    if (val1 == UI_ID_OK || val1 == UI_ID_CANCEL)
        ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
    return 0;
}
