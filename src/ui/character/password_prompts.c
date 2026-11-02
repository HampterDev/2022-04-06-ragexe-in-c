#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/items/items.h"

struct UIPasswordWnd;
struct UIWindow;
void ui_password_wnd_destroy(struct UIWindow *window);
void ui_password_wnd_set_show(struct UIWindow *self, int visible);
void ui_password_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_password_wnd_on_draw(struct UIWindow *self);
intptr_t ui_password_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_password_wnd_ops = {
    .destroy = ui_password_wnd_destroy,
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
    .set_show = ui_password_wnd_set_show,
    .on_create = ui_password_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_password_wnd_on_draw,
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
    .send_msg = ui_password_wnd_send_msg,
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
struct UIEmailAddressWnd;
void ui_email_address_wnd_destroy(struct UIWindow *window);
void ui_email_address_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_email_address_wnd_on_draw(struct UIWindow *self);
intptr_t ui_email_address_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_email_address_wnd_ops = {
    .destroy = ui_email_address_wnd_destroy,
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
    .on_create = ui_email_address_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_email_address_wnd_on_draw,
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
    .send_msg = ui_email_address_wnd_send_msg,
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

#define PROMPT_ID_OK 0xb0
#define PROMPT_MASK_CHAR '*'

static void add_ok_button(struct UIFrameWnd *frame, int x, int y)
{
    struct UIBitmapButton *ok =
        ui_bitmap_button(UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp");

    ui_window_move_base(&ok->button.base, x, y);
    ok->button.base.id = PROMPT_ID_OK;
    ui_window_add_child(&frame->base, &ok->button.base);
}

#define MSG_PASSWORD_SET_ARG 0x2f
#define MODE_REQ_PASSWORD 0x2d
#define WND_ID_PASSWORD 0x1d
#define MSG_PASSWORD_PROMPT 0xf
#define PASSWORD_MAX_CHARS 8

struct UIPasswordWnd {
    struct UIItemDropCntWnd base;
    int arg;
};

struct UIPasswordWnd *ui_password_wnd_construct(struct UIPasswordWnd *self)
{
    ui_item_drop_cnt_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_password_wnd_ops;
    return self;
}

struct UIWindow *ui_password_wnd_new(void)
{
    return (struct UIWindow *)ui_password_wnd_construct(mem_alloc(sizeof(struct UIPasswordWnd)));
}

void ui_password_wnd_destroy(struct UIWindow *window)
{
    struct UIPasswordWnd *self = (struct UIPasswordWnd *)window;
    self->base.frame.base.ops = &ui_password_wnd_ops;
    ui_item_drop_cnt_wnd_destruct(&self->base);
    mem_free(self);
}

void ui_password_wnd_set_show(struct UIWindow *window, int visible)
{
    struct UIPasswordWnd *self = (struct UIPasswordWnd *)window;
    self->base.frame.base.visible = visible;
}

void ui_password_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIPasswordWnd *self = (struct UIPasswordWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));
    struct UIWindow *text;

    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->base.edit = edit;
    ui_window_create(edit, 0x5a, 0x10);
    ui_window_move(self->base.edit, 0xf, 0x16);
    UI_EDIT_MAX_CHARS(self->base.edit) = PASSWORD_MAX_CHARS;
    ui_edit_ctrl_set_back_colour(self->base.edit, 0xe8, 0xe8, 0xe8);
    UI_EDIT_MASK_CHAR(self->base.edit) = PROMPT_MASK_CHAR;
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->base.edit, "");
    ui_window_add_child(wnd, self->base.edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->base.edit);

    text = mem_alloc(sizeof(struct UIStaticText));
    if (text)
        text = ui_static_text_construct(text);
    self->base.name = (struct UIStaticText *)text;
    ui_window_create(text, wnd->width - 0x1e, 0xf);
    ui_window_move_base(&self->base.name->base, 0xf, 6);
    ui_static_text_set_text(&self->base.name->base, msg_str(MSG_PASSWORD_PROMPT), 0);
    ui_window_add_child(wnd, &self->base.name->base);

    add_ok_button(&self->base.frame, wnd->width - 0x37, wnd->height - 0x20);
    UI_FRAME_DEFAULT_ID(self) = PROMPT_ID_OK;
    wnd->state = 0;
    self->arg = 0;
}

void ui_password_wnd_on_draw(struct UIWindow *window)
{
    struct UIPasswordWnd *self = (struct UIPasswordWnd *)window;
    ui_window_draw_sys_box(&self->base.frame.base, 0, 0, 0, 0);
}

intptr_t ui_password_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                     intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIPasswordWnd *self = (struct UIPasswordWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == PROMPT_ID_OK) {
            struct Mode *mode = g_modeMgr.curMode;

            mode_send_message(mode, MODE_REQ_PASSWORD, self->arg,
                          (intptr_t)ui_edit_ctrl_get_text(self->base.edit), 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_PASSWORD);
        }
        return 0;
    }
    if (msg == MSG_PASSWORD_SET_ARG) {
        self->arg = val1;
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->base.frame), sender, msg, val1, val2, val3, val4);
}

#define MODE_REQ_EMAIL_OPENED 0x271d
#define MODE_REQ_EMAIL 0x271c
#define EMAIL_MAX_CHARS 0x31

struct UIEmailAddressWnd {
    struct UIFrameWnd frame;
    struct UIWindow *edit;
};

static struct UIEmailAddressWnd *ui_email_address_wnd_construct(struct UIEmailAddressWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_email_address_wnd_ops;
    return self;
}

struct UIEmailAddressWnd *ui_email_address_wnd_new(void)
{
    struct UIEmailAddressWnd *self = mem_alloc(sizeof *self);

    return self ? ui_email_address_wnd_construct(self) : NULL;
}

void ui_email_address_wnd_destroy(struct UIWindow *window)
{
    struct UIEmailAddressWnd *self = (struct UIEmailAddressWnd *)window;
    self->frame.base.ops = &ui_email_address_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_email_address_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIEmailAddressWnd *self = (struct UIEmailAddressWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *edit;

    mode_send_message(g_modeMgr.curMode, MODE_REQ_EMAIL_OPENED, 0, 0, 0, 0);
    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->edit = edit;
    ui_window_create(edit, wnd->width - 0x4d, 0x10);
    ui_window_move(self->edit, 0xf, 0x16);
    UI_EDIT_MAX_CHARS(self->edit) = EMAIL_MAX_CHARS;
    ui_edit_ctrl_set_back_colour(self->edit, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, "");
    UI_EDIT_MASK_CHAR(self->edit) = PROMPT_MASK_CHAR;
    ui_window_add_child(wnd, self->edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->edit);
    ui_edit_ctrl_select_all(self->edit);

    add_ok_button(&self->frame, wnd->width - 0x37, wnd->height - 0x1c);
    UI_FRAME_DEFAULT_ID(self) = PROMPT_ID_OK;
}

intptr_t ui_email_address_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                         intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIEmailAddressWnd *self = (struct UIEmailAddressWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    g_windowMgr.modal_result = val1;
    if (val1 == PROMPT_ID_OK)
        mode_send_message(g_modeMgr.curMode, MODE_REQ_EMAIL, (intptr_t)ui_edit_ctrl_get_text(self->edit), 0, 0, 0);
    return 0;
}

void ui_email_address_wnd_on_draw(struct UIWindow *self)
{
    ui_window_draw_sys_box(self, 0, 0, 0, 0);
    ui_window_text_out(self, 0xf, 6, msg_str(g_serviceType == 0 ? 0x717 : 0x12b), 0, 0, 0xc, 0, 0, 0);
}
