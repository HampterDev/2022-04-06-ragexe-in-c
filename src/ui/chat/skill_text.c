#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <string.h>
#include "ui/chat/chat.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/ui_std.h"

struct UIFrameWnd;
struct UISkillTextWnd;
struct UIWindow;
void ui_graffi_strbox_wnd_destroy(struct UIWindow *window);
void ui_graffi_strbox_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_graffi_strbox_wnd_on_draw(struct UIWindow *self);
int ui_graffi_strbox_wnd_send_msg(struct UIWindow *frame, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_graffi_strbox_wnd_ops = {
    .destroy = ui_graffi_strbox_wnd_destroy,
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
    .on_create = ui_graffi_strbox_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_graffi_strbox_wnd_on_draw,
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
    .send_msg = ui_graffi_strbox_wnd_send_msg,
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
void ui_talkbox_trap_input_wnd_destroy(struct UIWindow *window);
void ui_talkbox_trap_input_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_talkbox_trap_input_wnd_on_draw(struct UIWindow *self);
int ui_talkbox_trap_input_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_talkbox_trap_input_wnd_ops = {
    .destroy = ui_talkbox_trap_input_wnd_destroy,
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
    .on_create = ui_talkbox_trap_input_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_talkbox_trap_input_wnd_on_draw,
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
    .send_msg = ui_talkbox_trap_input_wnd_send_msg,
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

#define MSG_SKILL_TEXT_TITLE 0x1b3
#define MSG_SKILL_TEXT_PROMPT 0x1b4
#define GRAFFITI_MAX_CHARS 0x14
#define TALKBOX_MAX_CHARS 0x4e

struct UISkillTextWnd {
    struct UIFrameWnd frame;
    struct GroundSkillUse use;
    struct UIWindow *edit;
};

int ui_graffi_strbox_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                              int val1, int val2, int val3, int val4)
{
    return ui_talkbox_trap_input_wnd_send_msg(window, sender, msg, val1, val2, val3, val4);
}

static void skill_text_create(struct UISkillTextWnd *self, int max_chars)
{
    static const char *const bitmaps[2][3] = {
        { "btn_ok", "btn_ok_a", "btn_ok_b" },
        { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
    };
    static const int places[2][2] = { { 0xbd, 0x60 }, { 0xea, 0x60 } };
    static const int ids[2] = { UI_ID_OK, UI_ID_CANCEL };
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *edit;
    struct UIBitmapButton *button;
    struct String skin, path;
    int i, state;

    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->edit = edit;
    ui_window_create(edit, 0xf0, 0x10);
    ui_window_move(self->edit, 0x16, 0x32);
    UI_EDIT_MAX_CHARS(self->edit) = max_chars;
    ui_edit_ctrl_set_back_colour(self->edit, 0xf2, 0xf2, 0xf2);
    ui_window_add_child(wnd, self->edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->edit);

    string_init(&skin);
    string_assign(&skin, UI_SKIN, sizeof UI_SKIN - 1);
    for (i = 0; i < 2; i++) {
        button = mem_alloc(sizeof(struct UIBitmapButton));
        if (button)
            button = ui_bitmap_button_construct(button);
        for (state = 0; state < 3; state++) {
            string_copy(&path, &skin);
            string_append_cstr(&path, bitmaps[i][state]);
            string_append_cstr(&path, ".bmp");
            ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
            string_free(&path);
        }
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, places[i][0], places[i][1]);
        button->button.base.id = ids[i];
        ui_window_add_child(wnd, &button->button.base);
    }
    UI_FRAME_DEFAULT_ID(self) = UI_ID_OK;
    string_free(&skin);
}

static void skill_text_draw(struct UISkillTextWnd *self, int title)
{
    struct UIWindow *wnd = &self->frame.base;

    ui_window_draw_body(wnd, ui_window_draw_title_bar(wnd, 0, "", 0), 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    if (title) {
        ui_window_text_out(wnd, 0x11, 3, msg_str(MSG_SKILL_TEXT_TITLE), 0, 0, 0xc, 0xffffff, 0, 0);
        ui_window_text_out(wnd, 0x10, 2, msg_str(MSG_SKILL_TEXT_TITLE), 0, 0, 0xc, 0, 0, 0);
    }
    ui_window_text_out(wnd, 0x11, 0x17, msg_str(MSG_SKILL_TEXT_PROMPT), 0, 0, 0xc, 0xffffff, 0, 0);
    ui_window_text_out(wnd, 0x10, 0x16, msg_str(MSG_SKILL_TEXT_PROMPT), 0, 0, 0xc, 0, 0, 0);
}

struct UISkillTextWnd *ui_graffi_strbox_wnd_construct(struct UISkillTextWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_graffi_strbox_wnd_ops;
    return self;
}

struct UIWindow *ui_graffi_strbox_wnd_new(void)
{
    return (struct UIWindow *)ui_graffi_strbox_wnd_construct(mem_alloc(sizeof(struct UISkillTextWnd)));
}

void ui_graffi_strbox_wnd_destroy(struct UIWindow *window)
{
    struct UISkillTextWnd *self = (struct UISkillTextWnd *)window;
    self->frame.base.ops = &ui_graffi_strbox_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_graffi_strbox_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UISkillTextWnd *self = (struct UISkillTextWnd *)window;
    skill_text_create(self, GRAFFITI_MAX_CHARS);
}

void ui_graffi_strbox_wnd_on_draw(struct UIWindow *window)
{
    struct UISkillTextWnd *self = (struct UISkillTextWnd *)window;
    skill_text_draw(self, 0);
}

struct UISkillTextWnd *ui_talkbox_trap_input_wnd_construct(struct UISkillTextWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_talkbox_trap_input_wnd_ops;
    return self;
}

struct UIWindow *ui_talkbox_trap_input_wnd_new(void)
{
    return (struct UIWindow *)ui_talkbox_trap_input_wnd_construct(mem_alloc(sizeof(struct UISkillTextWnd)));
}

void ui_talkbox_trap_input_wnd_destroy(struct UIWindow *window)
{
    struct UISkillTextWnd *self = (struct UISkillTextWnd *)window;
    self->frame.base.ops = &ui_talkbox_trap_input_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_talkbox_trap_input_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UISkillTextWnd *self = (struct UISkillTextWnd *)window;
    skill_text_create(self, TALKBOX_MAX_CHARS);
}

void ui_talkbox_trap_input_wnd_on_draw(struct UIWindow *window)
{
    struct UISkillTextWnd *self = (struct UISkillTextWnd *)window;
    skill_text_draw(self, 1);
}

#define SKILL_TEXT_MSG_SET_CELL 0x44
#define MODE_MSG_TALKBOX_TEXT 0x83

int ui_talkbox_trap_input_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UISkillTextWnd *self = (struct UISkillTextWnd *)window;
    struct GroundSkillTalkbox request;

    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_OK) {
            request.use = self->use;
            request.text = ui_edit_ctrl_get_text(self->edit);
            game_send_message(MODE_MSG_TALKBOX_TEXT, (int)&request, 0, 0, 0);
        } else if (val1 != UI_ID_CANCEL) {
            return 0;
        }
        ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
        return 0;
    }
    if (msg == SKILL_TEXT_MSG_SET_CELL) {
        self->use = *(const struct GroundSkillUse *)val1;
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
