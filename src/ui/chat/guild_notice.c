#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "contents/guild_info.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include <string.h>
#include "ui/chat/chat.h"
#include "ui/social/guild_wnd.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/ui_std.h"

struct UIGuildNoticeWnd;
struct UIWindow;
void ui_guild_notice_wnd_destroy(struct UIWindow *window);
void ui_guild_notice_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_guild_notice_wnd_on_draw(struct UIWindow *self);
void ui_guild_notice_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_guild_notice_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIGuildWndOps ui_guild_notice_wnd_ops = {
    .base = {
        .destroy = ui_guild_notice_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 1,
        .refreshes_children = 0,
        .move = ui_frame_wnd_move,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_guild_wnd_store_info,
        .save_pos = ui_frame_wnd_save_pos,
        .move_delta = ui_frame_wnd_move_delta,
        .set_show = ui_window_set_show_base,
        .on_create = ui_guild_notice_wnd_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_guild_notice_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_guild_notice_wnd_on_l_btn_down,
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
        .send_msg = ui_guild_notice_wnd_send_msg,
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
    },
    .tab = GUILD_TAB_NOTICE,
};

#define WND_ID_GUILD 0x3b
#define GUILD_NOTICE_ID_SUBJECT 0x111
#define GUILD_NOTICE_ID_SEND 0x10d
#define GUILD_SUBJECT_MAX 0x3c
#define GUILD_BODY_MAX 0x77
#define MSG_GUILD_NOTICE_SUBJECT 0x203
#define MSG_GUILD_NOTICE_BODY 0x204
#define UI_MSG_GUILD_REFRESH 0x3c
#define MODE_REQ_GUILD_INFO 0x6b
#define MODE_REQ_GUILD_NOTICE 0x76

struct UIGuildNoticeWnd {
    struct UIGuildWnd base;
    struct UIWindow *subject_edit;
    struct UIWindow *body_edit;
    struct UIBitmapButton *send_button;
};
SIZE_ASSERT(UIGuildNoticeWnd, 0xbc);
#define GUILD_WND(self) (&(self)->base.frame.base)

struct UIGuildNoticeWnd *ui_guild_notice_wnd_construct(struct UIGuildNoticeWnd *self)
{
    ui_guild_wnd_construct(&self->base);
    GUILD_WND(self)->ops = &ui_guild_notice_wnd_ops.base;
    return self;
}

struct UIWindow *ui_guild_notice_wnd_new(void)
{
    return (struct UIWindow *)ui_guild_notice_wnd_construct(mem_alloc(sizeof(struct UIGuildNoticeWnd)));
}

void ui_guild_notice_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildNoticeWnd *self = (struct UIGuildNoticeWnd *)window;
    GUILD_WND(self)->ops = &ui_guild_notice_wnd_ops.base;
    GUILD_WND(self)->ops = &ui_guild_wnd_ops.base;
    ui_frame_wnd_destruct(&self->base.frame);
    mem_free(self);
}

void ui_guild_notice_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIGuildNoticeWnd *self = (struct UIGuildNoticeWnd *)window;
    static const char *const send_bitmaps[3] = { "btn_send", "btn_send_a", "btn_send_b" };
    struct UIWindow *wnd = GUILD_WND(self);
    struct UIWindow *edit;
    struct UIBitmapButton *button;
    struct String skin, path;
    int state;

    ui_guild_wnd_init_tab_text(&self->base);
    if (strcmp(string_cstr(&g_guildInfo.master_name), "") == 0)
        mode_send_message(g_modeMgr.curMode, MODE_REQ_GUILD_INFO, 0, 0, 0, 0);

    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit) {
        edit = ui_edit_ctrl_construct(edit);
        edit->ops = &ui_edit_ctrl_var_id_ops.base;
    }
    self->subject_edit = edit;
    ui_window_create(edit, wnd->width - 0x3c, 0x10);
    ui_window_move(self->subject_edit, 0x32, 0x42);
    UI_EDIT_MAX_CHARS(self->subject_edit) = GUILD_SUBJECT_MAX;
    ui_edit_ctrl_set_back_colour(self->subject_edit, 0xf2, 0xf2, 0xf2);
    self->subject_edit->id = GUILD_NOTICE_ID_SUBJECT;
    ui_window_add_child(wnd, self->subject_edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->subject_edit);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->subject_edit, string_cstr(&g_guildInfo.notice_subject));

    edit = mem_alloc(sizeof(struct UIEditCtrl3));
    if (edit)
        edit = ui_edit_ctrl3_construct(edit);
    self->body_edit = edit;
    ui_window_create(edit, wnd->width - 0xa, 0x9c);
    ((struct UIEditCtrl *)self->body_edit)->multiline = 1;
    ui_window_move(self->body_edit, 5, 0x68);
    UI_EDIT_MAX_CHARS(self->body_edit) = GUILD_BODY_MAX;
    ui_edit_ctrl_set_back_colour(self->body_edit, 0xf2, 0xf2, 0xf2);
    ui_window_add_child(wnd, self->body_edit);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->body_edit, string_cstr(&g_guildInfo.notice_body));

    string_init(&skin);
    string_assign(&skin, UI_SKIN, sizeof UI_SKIN - 1);
    button = mem_alloc(sizeof(struct UIBitmapButton));
    if (button)
        button = ui_bitmap_button_construct(button);
    self->send_button = button;
    for (state = 0; state < 3; state++) {
        string_copy(&path, &skin);
        string_append_cstr(&path, send_bitmaps[state]);
        string_append_cstr(&path, ".bmp");
        ui_bitmap_button_set_bitmap(self->send_button, string_cstr(&path), state, 0);
        string_free(&path);
    }
    ui_window_create(&self->send_button->button.base, self->send_button->bitmap_width,
                     self->send_button->bitmap_height);
    if (g_guildInfo.is_master != 0)
        ui_window_move_base(&self->send_button->button.base, wnd->width - 0x2e, wnd->height - 0x18);
    self->send_button->button.base.id = GUILD_NOTICE_ID_SEND;
    ui_window_add_child(wnd, &self->send_button->button.base);
    string_free(&skin);
}

void ui_guild_notice_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildNoticeWnd *self = (struct UIGuildNoticeWnd *)window;
    struct UIWindow *wnd = GUILD_WND(self);
    char label[0x40];
    struct String count;

    ui_guild_wnd_on_draw((struct UIWindow *)(&self->base));
    sprintf(label, "%s:", msg_str(MSG_GUILD_NOTICE_SUBJECT));
    ui_window_text_out(wnd, 5, 0x43, label, 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 5, 0x56, msg_str(MSG_GUILD_NOTICE_BODY), 0, 0, 0xc, 0, 0, 0);
    string_init(&count);
    string_format(&count, "[%d/%d]", ui_edit_ctrl_get_text_size(self->body_edit), GUILD_BODY_MAX);
    ui_window_text_out_right(wnd, wnd->width - 7, 0x10a, string_cstr(&count), 0, 0, 0xc, 0, 0);
    string_free(&count);
}

void ui_guild_notice_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildNoticeWnd *self = (struct UIGuildNoticeWnd *)window;
    ui_guild_wnd_on_l_btn_down((struct UIWindow *)(&self->base), x, y);
}

static void guild_notice_send(struct UIGuildNoticeWnd *self)
{
    struct String body;
    const char *text;

    string_init(&body);
    text = ui_edit_ctrl_get_text(self->body_edit);
    string_assign_cstr(&body, text);
    if (g_guildInfo.is_master != 0)
        mode_send_message(g_modeMgr.curMode, MODE_REQ_GUILD_NOTICE, g_guildInfo.guild_id,
                      (int)ui_edit_ctrl_get_text(self->subject_edit), (int)string_cstr(&body), 0);
    ui_window_invalidate_children(GUILD_WND(self));
    string_free(&body);
}

int ui_guild_notice_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UIGuildNoticeWnd *self = (struct UIGuildNoticeWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_GUILD);
        else if (val1 == GUILD_NOTICE_ID_SEND)
            guild_notice_send(self);
        return 0;
    }
    if (msg == UI_MSG_GUILD_REFRESH) {
        ui_window_invalidate_children(GUILD_WND(self));
        return 0;
    }
    return ui_guild_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
