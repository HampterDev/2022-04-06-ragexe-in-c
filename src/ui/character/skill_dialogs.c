#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "skills/skill_info.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/character/character.h"
#include "ui/character/skill_dialogs.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct UISkillDescribeWnd;
struct UIWindow;
void ui_skill_describe_wnd_destroy(struct UIWindow *window);
void ui_skill_describe_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_skill_describe_wnd_on_draw(struct UIWindow *self);
int ui_skill_describe_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_skill_describe_wnd_query_screen_position(struct UIWindow *window);
const struct UIWindowOps ui_skill_describe_wnd_ops = {
    .destroy = ui_skill_describe_wnd_destroy,
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
    .on_create = ui_skill_describe_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_skill_describe_wnd_on_draw,
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
    .send_msg = ui_skill_describe_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 1,
    .draw_backdrop = ui_skill_describe_wnd_query_screen_position,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
struct UISkillNameChangeWnd;
struct UIWindow;
void ui_skill_name_change_wnd_destroy(struct UIWindow *window);
void ui_skill_name_change_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_skill_name_change_wnd_on_draw(struct UIWindow *self);
int ui_skill_name_change_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_skill_name_change_wnd_ops = {
    .destroy = ui_skill_name_change_wnd_destroy,
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
    .on_create = ui_skill_name_change_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_skill_name_change_wnd_on_draw,
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
    .send_msg = ui_skill_name_change_wnd_send_msg,
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

#define WND_ID_SKILL_DESCRIBE 0x2e
#define WND_ID_SKILL_NAME_CHANGE 0x37

#define SKILL_DESCRIBE_MSG_SHOW 0x3d
#define MSG_SKILL_DESCRIBE_TITLE 0x20e
#define SKILL_DESCRIBE_LINE_HEIGHT 18

struct UISkillDescribeWnd {
    struct UIFrameWnd frame;
    struct UIWindow *close_button;
    struct UIWindow *text;
    struct String icon_path;
    struct String name;
    struct String title;
    int skill_id;
};

struct UISkillDescribeWnd *ui_skill_describe_wnd_construct(struct UISkillDescribeWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_skill_describe_wnd_ops;
    string_init(&self->icon_path);
    string_init(&self->name);
    string_init(&self->title);
    return self;
}

struct UIWindow *ui_skill_describe_wnd_new(void)
{
    return (struct UIWindow *)ui_skill_describe_wnd_construct(mem_alloc(sizeof(struct UISkillDescribeWnd)));
}

void ui_skill_describe_wnd_destroy(struct UIWindow *window)
{
    struct UISkillDescribeWnd *self = (struct UISkillDescribeWnd *)window;
    self->frame.base.ops = &ui_skill_describe_wnd_ops;
    string_free(&self->title);
    string_free(&self->name);
    string_free(&self->icon_path);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_skill_describe_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISkillDescribeWnd *self = (struct UISkillDescribeWnd *)window;
    const char *title = msg_str(MSG_SKILL_DESCRIBE_TITLE);
    struct UIWindow *text;

    string_assign_cstr(&self->name, title);
    self->skill_id = -1;
    text = mem_alloc(sizeof(struct UITextViewer));
    if (text)
        text = ui_text_viewer_construct(text);
    self->text = text;
    ui_list_box_create_window((struct UIListBox *)text, 3, 3, width - 6, height - 6, 0);
    UI_TEXT_VIEWER_BACKGROUND(self->text) = ui_skin_argb(2, 2);
    ui_window_invalidate(self->text);
    ui_window_add_child(&self->frame.base, self->text);
    self->close_button = ui_window_add_close_button(&self->frame.base, "ESC", 0);
}

void ui_skill_describe_wnd_on_draw(struct UIWindow *window)
{
    struct UISkillDescribeWnd *self = (struct UISkillDescribeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_draw_round_frame(wnd, 0, 0, wnd->width, wnd->height, 0xc3c3c3);
    ui_window_draw_round_frame(wnd, 2, 2, wnd->width - 4, wnd->height - 4, 0xffffff);
}

void ui_skill_describe_wnd_query_screen_position(struct UIWindow *window)
{
    struct UISkillDescribeWnd *self = (struct UISkillDescribeWnd *)window;

    int x = 0, y = 0;

    ui_window_to_screen(&self->frame.base, &x, &y);
}

static int show_skill(struct UISkillDescribeWnd *self, int skill_id, int short_form)
{
    struct UIWindow *wnd = &self->frame.base;
    struct SkillInfo info, known;

    const char *name, *id_name, *result = NULL;
    char costs[0x40];
    int lines, height;

    skill_info_construct(&info);
    info.id = skill_id;
    if (self->skill_id == skill_id) {
        skill_info_destruct(&info);
        return -1;
    }
    name = skill_info_get_name(&info);
    string_assign_cstr(&self->name, name);
    string_assign(&self->title, string_cstr(&self->name), self->name.size);
    self->skill_id = info.id;

    session_get_skill_info(&known, skill_id);
    if (known.found && known.sp_cost && known.level) {
        if (skill_info_get_ap_amount(&info, known.level))
            sprintf(costs, " (Sp : %d, Ap : %d)", known.sp_cost, known.ap_amount);
        else
            sprintf(costs, " (Sp : %d)", known.sp_cost);
        string_append_cstr(&self->title, costs);
    }
    skill_info_destruct(&known);

    id_name = skill_info_get_id_name(&info);
    string_assign_cstr(&self->icon_path, UI_SKIN "item\\");
    string_append_cstr(&self->icon_path, id_name);
    string_append_cstr(&self->icon_path, ".bmp");

    ui_list_box_clear((struct UIListBox *)self->text);

    if (client_lua_call(g_session1.luaState, "GetSkillDescript", "ddd>s", g_session1.job, skill_id, short_form, &result)) {
        struct String text;

        string_init(&text);
        string_assign_cstr(&text, result);
        if (text.size) {
            char *line = mem_alloc(text.size + 4);

            sprintf(line, "%s%s", language_charset_tag(g_language, 1), string_cstr(&text));
            ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)line, 0, 0, 1);
            mem_free(line);
        }
        string_free(&text);
    } else {
        ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)"Unknown Skill", 0x21, 0, 0);
    }

    lines = (int)VEC_COUNT(&((struct UITextViewer *)self->text)->list.items, struct String);
    height = lines * SKILL_DESCRIBE_LINE_HEIGHT + 4;
    ui_window_resize(self->text, self->text->width, height);
    ui_window_resize_base(wnd, wnd->width, height + 6);
    ui_window_invalidate_base(wnd);
    ui_window_mgr_raise_window(&g_windowMgr, wnd);
    skill_info_destruct(&info);
    return 0;
}

int ui_skill_describe_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UISkillDescribeWnd *self = (struct UISkillDescribeWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_SKILL_DESCRIBE);
        return 0;
    }
    if (msg == SKILL_DESCRIBE_MSG_SHOW) {
        if (val4)
            ui_window_move_base(self->close_button, -100, -100);
        else
            ui_window_move_base(self->close_button, self->frame.base.width - 0xe, 3);
        return show_skill(self, val1, val2 == 0 && val3 == 0 && val4 == 1);
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define SKILL_NAME_MSG_OPEN 0x44
#define SKILL_NAME_ID_OK 0xb0
#define SKILL_NAME_MAX_CHARS 0x1d
#define MODE_REQ_SKILL_NAME 0x62

struct UISkillNameChangeWnd {
    struct UIFrameWnd frame;
    struct UIWindow *edit;
    int skill_id;
    struct String skill_name;
};

struct UISkillNameChangeWnd *ui_skill_name_change_wnd_construct(struct UISkillNameChangeWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_skill_name_change_wnd_ops;
    string_init(&self->skill_name);
    return self;
}

struct UIWindow *ui_skill_name_change_wnd_new(void)
{
    return (struct UIWindow *)ui_skill_name_change_wnd_construct(mem_alloc(sizeof(struct UISkillNameChangeWnd)));
}

void ui_skill_name_change_wnd_destroy(struct UIWindow *window)
{
    struct UISkillNameChangeWnd *self = (struct UISkillNameChangeWnd *)window;
    self->frame.base.ops = &ui_skill_name_change_wnd_ops;
    string_free(&self->skill_name);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_skill_name_change_wnd_on_create(struct UIWindow *window, int width_unused,
                                              int height_unused)
{
    struct UISkillNameChangeWnd *self = (struct UISkillNameChangeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));
    struct UIBitmapButton *ok;

    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->edit = edit;
    ui_window_create(edit, wnd->width - 0x4d, 0x10);
    ui_window_move(self->edit, 0xf, 0x16);
    UI_EDIT_MAX_CHARS(self->edit) = SKILL_NAME_MAX_CHARS;
    ui_edit_ctrl_set_back_colour(self->edit, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, "");
    ui_window_add_child(wnd, self->edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->edit);
    ui_edit_ctrl_select_all(self->edit);

    ok = ui_bitmap_button(UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp");
    ui_window_move_base(&ok->button.base, wnd->width - 0x37, wnd->height - 0x20);
    ok->button.base.id = SKILL_NAME_ID_OK;
    ui_window_add_child(wnd, &ok->button.base);
    self->skill_id = -1;
    UI_FRAME_DEFAULT_ID(self) = SKILL_NAME_ID_OK;
}

void ui_skill_name_change_wnd_on_draw(struct UIWindow *window)
{
    struct UISkillNameChangeWnd *self = (struct UISkillNameChangeWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
    ui_window_text_out(&self->frame.base, 0xf, 6, string_cstr(&self->skill_name), 0, 0, 12, 0, 0, 0);
}

int ui_skill_name_change_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UISkillNameChangeWnd *self = (struct UISkillNameChangeWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 != SKILL_NAME_ID_OK)
            return 0;
        if (self->skill_id != -1 && ui_edit_ctrl_get_text_size(self->edit) != 0) {
            struct Mode *mode = g_modeMgr.curMode;

            mode_send_message(mode, MODE_REQ_SKILL_NAME, self->skill_id, (int)ui_edit_ctrl_get_text(self->edit), 0, 0);
        }
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_SKILL_NAME_CHANGE);
        return 0;
    }
    if (msg == SKILL_NAME_MSG_OPEN) {
        const char *name;

        self->skill_id = val1;
        name = skill_name(val1);
        string_assign_cstr(&self->skill_name, name);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, string_cstr(&self->skill_name));
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->edit);
        ui_edit_ctrl_select_all(self->edit);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

int ui_skill_describe_skill(const struct UIWindow *describe)
{
    return ((const struct UISkillDescribeWnd *)describe)->skill_id;
}
