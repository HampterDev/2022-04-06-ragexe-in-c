#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "contents/manner_filter.h"
#include "ui/character/gauge.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "core/markup_tags.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <time.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/character/character.h"
#include "ui/character/character_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "ui/character/gauge_internal.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

struct UIWindow;
const struct UIWindowOps ui_frame_wnd_ops = {
    .destroy = ui_frame_wnd_destroy,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_window_on_draw_base,
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
    .send_msg = ui_frame_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define WND_HOMUN_INFO 0x71
#define WND_HOMUN_SKILL 0x72

#define HOMUN_ID_RENAME UI_ID_OK
#define HOMUN_ID_SKILL 0x119
#define HOMUN_ID_FEED 0x11a
#define HOMUN_ID_DELETE 0x11b
#define UI_MSG_NORMAL_MB_ANSWER 0x1e0

#define MODE_REQ_HOMUN_COMMAND 0xb8
#define MODE_REQ_HOMUN_RENAME 0xb9
#define HOMUN_COMMAND_FEED 1
#define HOMUN_COMMAND_DELETE 2

#define UI_MSGBOX_ANSWER_NO 0xb4
#define HOMUN_NAME_MAX 0x18

#define MSG_HOMUN_TITLE 0x3a8
#define MSG_HOMUN_NAME 0x197
#define MSG_HOMUN_LEVEL 0x198
#define MSG_HOMUN_HUNGER 0x3fb
#define MSG_HOMUN_EXP 0x249
#define MSG_HOMUN_INTIMACY_CAPTION 0x24a
#define MSG_HOMUN_AUTO_FEED 0xccf
#define MSG_HOMUN_INTIMACY_HEAD 0xcd4
#define MSG_HOMUN_NAME_TOO_LONG 0x3aa
#define MSG_HOMUN_NAME_HAS_TAG 0xafc
#define MSG_HOMUN_RENAME_QUESTION 0xba1
#define MSG_HOMUN_NAME_REFUSED 0xb84
#define MSG_HOMUN_DELETE_QUESTION 0x3bb
#define MSG_HOMUN_FEED_QUESTION 0x259
#define MSG_HOMUN_FEED_BLOCKED 0xddf

struct UIHomunInfoWnd;
void ui_homun_info_wnd_destroy(struct UIWindow *window);
void ui_homun_info_wnd_store_info(struct UIWindow *self);
void ui_homun_info_wnd_on_create(struct UIWindow *self, int cx, int cy);
void ui_homun_info_wnd_on_draw(struct UIWindow *self);
int ui_homun_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_homun_info_wnd_ops = {
    .destroy = ui_homun_info_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_homun_info_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_homun_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_homun_info_wnd_on_draw,
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
    .send_msg = ui_homun_info_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIHomunInfoWnd {
    struct UIFrameWnd frame;
    struct UIWindow *name_edit;
    struct UIWindow *button;
    struct UIWindow *hp_gauge;
    struct UIWindow *sp_gauge;
    struct UIWindow *exp_bar;
    struct UIWindow *hunger_bar;
    struct WindowInfo *info;
    struct UIWindow *question;
    struct UIWindow *auto_feed;
};
SIZE_ASSERT(UIHomunInfoWnd, 0xb8);

struct UIHomunInfoWnd *ui_homun_info_wnd_construct(struct UIHomunInfoWnd *self)
{
    ui_window_construct(&self->frame.base, 0);
    self->frame.base.ops = &ui_frame_wnd_ops;
    self->frame.press_x = 0;
    self->frame.press_y = 0;
    self->frame.default_id = UI_ID_NONE;
    self->frame.cancel_id = UI_ID_NONE;
    self->frame.base.ops = &ui_homun_info_wnd_ops;
    self->auto_feed = NULL;
    self->question = NULL;
    return self;
}

struct UIWindow *ui_homun_info_wnd_new(void)
{
    return (struct UIWindow *)ui_homun_info_wnd_construct(mem_alloc(sizeof(struct UIHomunInfoWnd)));
}

void ui_homun_info_wnd_destruct(struct UIHomunInfoWnd *self)
{
    self->frame.base.ops = &ui_homun_info_wnd_ops;
    if (self->question)
        ui_window_mgr_post_quit2(&g_windowMgr, self->question);
    self->frame.base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->frame.base);
}

void ui_homun_info_wnd_destroy(struct UIWindow *window)
{
    struct UIHomunInfoWnd *self = (struct UIHomunInfoWnd *)window;
    ui_homun_info_wnd_destruct(self);
    mem_free(self);
}

static void set_state_bitmap(struct UIBitmapButton *button, const struct String *base, const char *suffix,
                             int state)
{
    struct String path;

    string_copy(&path, base);
    string_append_cstr(&path, suffix);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

static struct UIWindow *add_button(struct UIWindow *parent, struct UIWindow **slot, const char *path, int x, int y,
                                   int id)
{
    struct String base;
    struct UIBitmapButton *button;
    void *mem;

    string_init(&base);
    string_assign_cstr(&base, path);
    mem = mem_alloc(sizeof(struct UIBitmapButton));
    button = mem ? ui_bitmap_button_construct(mem) : NULL;
    *slot = &button->button.base;
    set_state_bitmap(button, &base, ".bmp", 0);
    set_state_bitmap(button, &base, "_a.bmp", 1);
    set_state_bitmap(button, &base, "_b.bmp", 2);
    ui_window_create(*slot, button->bitmap_width, button->bitmap_height);
    ui_window_move(*slot, x, y);
    (*slot)->id = id;
    ui_window_add_child(parent, *slot);
    string_free(&base);
    return *slot;
}

void ui_homun_info_wnd_on_create(struct UIWindow *window, int cx, int cy)
{
    struct UIHomunInfoWnd *self = (struct UIHomunInfoWnd *)window;
    void *mem;

    ui_window_add_close_button(&self->frame.base, "", 0);

    mem = mem_alloc(sizeof(struct UIEditCtrl));
    self->name_edit = mem ? ui_edit_ctrl2_construct(mem) : NULL;
    ui_window_create(self->name_edit, 0x50, 0x10);
    if (strcmp(g_session1.homunName, "") == 0)
        _snprintf_s(g_session1.homunName, sizeof g_session1.homunName, _TRUNCATE, "%s",
                    session_get_job_name(&g_session1, g_session1.homunJob, -1));
    ui_window_move(self->name_edit, 0x87, 0x16);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->name_edit, g_session1.homunName);
    UI_EDIT_MAX_CHARS(self->name_edit) = 0x32;
    ui_edit_ctrl_set_back_colour(self->name_edit, 0xe8, 0xe8, 0xe8);
    ui_window_add_child(&self->frame.base, self->name_edit);

    add_button(&self->frame.base, &self->button, UI_SKIN "btn_del", 0xbb, 0x2c, HOMUN_ID_DELETE);
    add_button(&self->frame.base, &self->button, UI_SKIN "btn_skill", 0xe8, 0x2c, HOMUN_ID_SKILL);
    add_button(&self->frame.base, &self->button, UI_SKIN "btn_feed", 0xd2, 0x9b, HOMUN_ID_FEED);
    ui_window_move(self->button, 0xd2, 0x69);
    add_button(&self->frame.base, &self->button, UI_SKIN "btn_rewrite", 0xe8, 0x15, HOMUN_ID_RENAME);

    self->hp_gauge = ui_value_gauge_create(VALUE_GAUGE_HOMUN);
    ui_value_gauge_set(self->hp_gauge, gauge_value_from_int(&g_session1.homunHp), gauge_value_from_int(&g_session1.homunMaxHp));
    ui_window_create(self->hp_gauge, 0x9d, 9);
    ui_window_move_base(self->hp_gauge, 0x75, 0x49);
    ui_window_add_child(&self->frame.base, self->hp_gauge);

    self->sp_gauge = ui_value_gauge_create(VALUE_GAUGE_HOMUN);
    ui_value_gauge_set(self->sp_gauge, gauge_value_from_int(&g_session1.homunSp), gauge_value_from_int(&g_session1.homunMaxSp));
    ui_window_create(self->sp_gauge, 0x9d, 9);
    ui_window_move_base(self->sp_gauge, 0x75, 0x58);
    ui_window_add_child(&self->frame.base, self->sp_gauge);

    self->exp_bar = (struct UIWindow *)uiint64_bar_graph_new();
    uiint64_bar_graph_set_values((struct UIINT64BarGraph *)self->exp_bar, g_session1.homunExp, g_session1.homunMaxExp);
    uiint64_bar_graph_set_no_warning((struct UIINT64BarGraph *)self->exp_bar, 1);
    ui_window_create(self->exp_bar, 0x66, 6);
    ui_window_move_base(self->exp_bar, 0x65, 0x77);
    ui_window_add_child(&self->frame.base, self->exp_bar);

    mem = mem_alloc(sizeof(struct UIBarGraph4));
    if (mem) {
        ui_bar_graph3_construct(mem);
        ((struct UIWindow *)mem)->ops = &ui_bar_graph4_ops;
    }
    self->hunger_bar = mem;
    ((struct UIBarGraph4 *)self->hunger_bar)->no_warning = 0;
    ui_window_create(self->hunger_bar, 0x66, 6);
    ui_window_move(self->hunger_bar, 0x65, 0x91);
    ui_bar_graph3_set_values((struct UIBarGraph3 *)self->hunger_bar, &g_session1.homunFullness, &g_session1.homunMaxFullness);
    ui_window_add_child(&self->frame.base, self->hunger_bar);

    self->frame.default_id = UI_ID_CLOSE;
    mem = mem_alloc(sizeof(struct UICheckBox));
    self->auto_feed = mem ? (struct UIWindow *)ui_check_box_construct(mem) : NULL;
    ui_window_create(self->auto_feed, 0xc, 0xc);
    ui_window_move_base(self->auto_feed, 0xd2, 0x8e);
    ui_check_box_set_bitmaps_fit((struct UICheckBox *)self->auto_feed, UI_SKIN "checkbox_1.bmp",
                             UI_SKIN "checkbox_0.bmp");
    ui_check_box_set_check(self->auto_feed, g_session1.homunAutoFeed);
    ui_window_add_child(&self->frame.base, self->auto_feed);
}

static void caption(struct UIWindow *wnd, int x, int y, const char *text)
{
    ui_window_text_out(wnd, x, y, text, 0, 0, 0xc, 0, 0, 0);
}

static void stat_cell(struct UIWindow *wnd, int y, int value)
{
    char buf[16];
    int len = sprintf(buf, "%d", value);

    ui_window_text_out_right(wnd, 0x4f, y, buf, len, 1, 0xe, 0, 0);
}

static int intimacy_msg(int v)
{
    if (0 <= v && v <= 3)
        return 0x3fe;
    if (3 < v && v <= 10)
        return 0x3fd;
    if (10 < v && v <= 100)
        return 0x2a0;
    if (100 < v && v <= 250)
        return 0x2a1;
    if (250 < v && v <= 750)
        return 0x29d;
    if (750 < v && v <= 910)
        return 0x2a2;
    if (910 < v && v <= 1000)
        return 0x2a3;
    return 0x2a4;
}

void ui_homun_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIHomunInfoWnd *self = (struct UIHomunInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char title[0x20], cell[12], text[0x18];
    const char *msg;
    struct String line;
    int len;

    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\homuninfo_bg.bmp"), 0);
    ui_window_fill_round_rect(wnd, 0, wnd->height - 0x28, wnd->width, 0x28, 0xffffff);
    sprintf(title, msg_str(MSG_HOMUN_TITLE));
    caption(wnd, 5, 2, title);
    caption(wnd, 0x64, 0x18, msg_str(MSG_HOMUN_NAME));
    caption(wnd, 0x64, 0x2f, msg_str(MSG_HOMUN_LEVEL));
    caption(wnd, 0x64, 0x69, msg_str(MSG_HOMUN_HUNGER));
    caption(wnd, 0x64, 0x83, msg_str(MSG_HOMUN_EXP));
    caption(wnd, 0x64, 0x9f, msg_str(MSG_HOMUN_INTIMACY_CAPTION));
    caption(wnd, 0x8c, 0x9f, ":");
    stat_cell(wnd, 0x18, g_session1.homunStats[0]);
    stat_cell(wnd, 0x2a, g_session1.homunStats[1]);
    stat_cell(wnd, 0x3c, g_session1.homunStats[2]);
    stat_cell(wnd, 0x4e, g_session1.homunStats[3]);
    stat_cell(wnd, 0x60, g_session1.homunStats[4]);
    stat_cell(wnd, 0x72, g_session1.homunStats[5]);
    stat_cell(wnd, 0x84, g_session1.homunStats[6]);
    len = sprintf(cell, "%d", (2000 - g_session1.homunStats[7]) / 10);
    ui_window_text_out_right(wnd, 0x4f, 0x96, cell, len, 1, 0xe, 0, 0);
    _itoa(g_session1.homunLevel, cell, 10);
    caption(wnd, 0x87, 0x2f, cell);
    if (g_session1.homunJob == -1)
        return;

    len = sprintf(text, "%lld", g_session1.homunMaxExp);
    ui_window_text_out_right(wnd, 0xcd, 0x68, text, len, 1, 0xe, 0, 0);
    len = sprintf(text, "%d / %d", g_session1.homunFullness, 100);
    ui_window_text_out_right(wnd, 0xc8, 0x82, text, len, 1, 0xe, 0, 0);
    strcpy(text, msg_str(intimacy_msg(g_session1.homunIntimacy)));
    caption(wnd, 0x96, 0x9f, text);

    string_init(&line);
    msg = msg_str(MSG_HOMUN_AUTO_FEED);
    string_assign_cstr(&line, msg);
    ui_window_text_out_wrapped(wnd, 0xe1, 0x88, &line, 0, 0, 0, 0, 0xc, 0, 0);
    string_free(&line);
    string_init(&line);
    msg = msg_str(MSG_HOMUN_INTIMACY_HEAD);
    string_assign_cstr(&line, msg);
    ui_window_text_out_wrapped(wnd, 8, 0xb4, &line, 0, 0, 0, 0, 0xc, 0xff, 0);
    string_free(&line);
}

static int ask(const char *text)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_QUESTION, 0, 0, 0, "메시지", 0x118,
                                  0x78, 0);
}

static void homun_rename(struct UIHomunInfoWnd *self)
{
    char question[0x200];
    struct Mode *mode;

    if (strlen(ui_edit_ctrl_get_text(self->name_edit)) >= HOMUN_NAME_MAX) {
        ui_message_box(msg_str(MSG_HOMUN_NAME_TOO_LONG));
        return;
    }
    if (_mbsstr((const unsigned char *)ui_edit_ctrl_get_text(self->name_edit),
                (const unsigned char *)string_cstr(&text_tag_ITEML)) ||
        _mbsstr((const unsigned char *)ui_edit_ctrl_get_text(self->name_edit),
                (const unsigned char *)string_cstr(&text_tag_ITEML_END))) {
        ui_message_box(msg_str(MSG_HOMUN_NAME_HAS_TAG));
        return;
    }
    sprintf(question, msg_str(MSG_HOMUN_RENAME_QUESTION), ui_edit_ctrl_get_text(self->name_edit));
    if (ask(question) != UI_MSGBOX_ANSWER_NO) {
        if (word_filter_contains(&g_nameFilter, ui_edit_ctrl_get_text(self->name_edit)) != 1) {
            mode = game_current_base_mode();
            mode_send_message(mode, MODE_REQ_HOMUN_RENAME, (int)ui_edit_ctrl_get_text(self->name_edit), 0, 0, 0);
            _snprintf_s(g_session1.homunName, sizeof g_session1.homunName, _TRUNCATE, "%s",
                        ui_edit_ctrl_get_text(self->name_edit));
            ui_window_invalidate_base(&self->frame.base);
            return;
        }
        ui_message_box(msg_str(MSG_HOMUN_NAME_REFUSED));
    }
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->name_edit, g_session1.homunName);
    ui_window_invalidate(self->name_edit);
}

int ui_homun_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                      int msg, int val1, int val2, int val3, int val4)
{
    struct UIHomunInfoWnd *self = (struct UIHomunInfoWnd *)window;
    struct Mode *mode;

    if (self->question && sender != self->question)
        return 0;
    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!homun_info_wnd_info_is_on_screen(self->info))
            homun_info_wnd_info_set_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    case UI_MSG_COMMAND:
        break;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }

    ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
    switch (val1) {
    case UI_ID_CHECKBOX:
        if (sender == self->auto_feed)
            session_send_homun_auto_feed(&g_session1, val2 > 0 ? 1 : 0);
        break;
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_HOMUN_INFO);
        break;
    case HOMUN_ID_RENAME:
        homun_rename(self);
        break;
    case HOMUN_ID_SKILL:
        if (!ui_window_mgr_post_quit1(&g_windowMgr, WND_HOMUN_SKILL))
            ui_window_open(WND_HOMUN_SKILL);
        break;
    case HOMUN_ID_FEED:
        self->question = ui_normal_mb_new();
        ui_normal_mb_create(self->question, &self->frame.base, msg_str(MSG_HOMUN_FEED_QUESTION), UI_MSGBOX_QUESTION,
                          1, 0, 0x118, 0x78, 0);
        ui_window_mgr_add_window(&g_windowMgr, self->question);
        break;
    case HOMUN_ID_DELETE:
        mode = game_current_base_mode();
        if (ask(msg_str(MSG_HOMUN_DELETE_QUESTION)) != UI_MSGBOX_ANSWER_YES)
            break;
        mode_send_message(mode, MODE_REQ_HOMUN_COMMAND, HOMUN_COMMAND_DELETE, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_HOMUN_INFO);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_HOMUN_SKILL);
        session_clear_homun_commands(&g_session1);
        g_session1.homun_ai_flags[0] = 0;
        break;
    case UI_MSG_NORMAL_MB_ANSWER:
        mode = game_current_base_mode();
        if (sender == self->question && val2 == UI_MSGBOX_ANSWER_YES) {
            if (g_windowMgr.exchange_wnd != 0)
                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(MSG_HOMUN_FEED_BLOCKED),
                                      0xff, 0, 0);
            else
                mode_send_message(mode, MODE_REQ_HOMUN_COMMAND, HOMUN_COMMAND_FEED, 0, 0, 0);
        }
        ui_window_mgr_post_quit2(&g_windowMgr, self->question);
        self->question = NULL;
        break;
    }
    return 0;
}

#define WND_MER_INFO 0x7d
#define WND_MER_SKILL 0x7e

#define MER_ID_SKILL 0x155
#define MER_ID_FIRE 0x156
#define MODE_REQ_MER_COMMAND 0xba
#define MER_COMMAND_FIRE 2

#define MSG_MER_EXPIRE 0x4a0
#define MSG_MER_LOYALTY 0x4a1
#define MSG_MER_SUMMONS 0x4a2
#define MSG_MER_KILLS 0x4a3
#define MSG_MER_EXPIRE_FORMAT 0x4ec
#define MSG_MER_FIRE_QUESTION 0x55a

struct UIMerInfoWnd;
void ui_mer_info_wnd_destroy(struct UIWindow *window);
void ui_mer_info_wnd_store_info(struct UIWindow *self);
void ui_mer_info_wnd_on_create(struct UIWindow *self, int cx, int cy);
void ui_mer_info_wnd_on_draw(struct UIWindow *self);
int ui_mer_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_mer_info_wnd_ops = {
    .destroy = ui_mer_info_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_mer_info_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_mer_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_mer_info_wnd_on_draw,
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
    .send_msg = ui_mer_info_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIMerInfoWnd {
    struct UIFrameWnd frame;
    struct UIWindow *button;
    struct UIWindow *hp_gauge;
    struct UIWindow *sp_gauge;
    struct WindowInfo *info;
};

struct UIMerInfoWnd *ui_mer_info_wnd_construct(struct UIMerInfoWnd *self)
{
    ui_window_construct(&self->frame.base, 0);
    self->frame.base.ops = &ui_frame_wnd_ops;
    self->frame.press_x = 0;
    self->frame.press_y = 0;
    self->frame.default_id = UI_ID_NONE;
    self->frame.cancel_id = UI_ID_NONE;
    self->frame.base.ops = &ui_mer_info_wnd_ops;
    return self;
}

struct UIWindow *ui_mer_info_wnd_new(void)
{
    return (struct UIWindow *)ui_mer_info_wnd_construct(mem_alloc(sizeof(struct UIMerInfoWnd)));
}

void ui_mer_info_wnd_destruct(struct UIMerInfoWnd *self)
{
    self->frame.base.ops = &ui_mer_info_wnd_ops;
    self->frame.base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->frame.base);
}

void ui_mer_info_wnd_destroy(struct UIWindow *window)
{
    struct UIMerInfoWnd *self = (struct UIMerInfoWnd *)window;
    ui_mer_info_wnd_destruct(self);
    mem_free(self);
}

void ui_mer_info_wnd_on_create(struct UIWindow *window, int cx, int cy)
{
    struct UIMerInfoWnd *self = (struct UIMerInfoWnd *)window;
    ui_window_add_close_button(&self->frame.base, "", 0);
    add_button(&self->frame.base, &self->button, UI_SKIN "btn_fired", 0xbb, 0x2c, MER_ID_FIRE);
    add_button(&self->frame.base, &self->button, UI_SKIN "btn_skill", 0xe8, 0x2c, MER_ID_SKILL);

    self->hp_gauge = ui_value_gauge_create(VALUE_GAUGE_PLAYER);
    ui_value_gauge_set(self->hp_gauge, gauge_value_from_int(&g_session1.mercHp), gauge_value_from_int(&g_session1.mercMaxHp));
    ui_window_create(self->hp_gauge, 0x55, 9);
    ui_window_move_base(self->hp_gauge, 0x75, 0x49);
    ui_window_add_child(&self->frame.base, self->hp_gauge);

    self->sp_gauge = ui_value_gauge_create(VALUE_GAUGE_PLAYER);
    ui_value_gauge_set(self->sp_gauge, gauge_value_from_int(&g_session1.mercSp), gauge_value_from_int(&g_session1.mercMaxSp));
    ui_window_create(self->sp_gauge, 0x55, 9);
    ui_window_move_base(self->sp_gauge, 0x75, 0x58);
    ui_window_add_child(&self->frame.base, self->sp_gauge);

    self->frame.default_id = UI_ID_CLOSE;
}

static int mer_title_msg(int job)
{
    if ((unsigned)(job - 0x1781) <= 9)
        return 0x49d;
    if ((unsigned)(job - 0x1795) <= 9)
        return 0x49e;
    if ((unsigned)(job - 0x178b) <= 9)
        return 0x49f;
    return 0x557;
}

static void mer_number(struct UIWindow *wnd, int x, int y, const char *text, int len)
{
    ui_window_text_out(wnd, x, y, text, len, 1, 0xe, 0, 0, 0);
}

void ui_mer_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIMerInfoWnd *self = (struct UIMerInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char title[0x32];
    char text[0x20] = { 0 };
    char cell[0x18] = { 0 };
    char level[12] = { 0 };
    int len;
    __time32_t expires;

    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\homuninfo_bg.bmp"), 0);
    memset(title, 0, sizeof title);
    sprintf(title, msg_str(mer_title_msg(g_session1.mercJob)));
    caption(wnd, 5, 2, title);
    caption(wnd, 0x64, 0x18, msg_str(MSG_HOMUN_NAME));
    caption(wnd, 0x64, 0x2f, msg_str(MSG_HOMUN_LEVEL));
    caption(wnd, 0x64, 0x6f, msg_str(MSG_MER_EXPIRE));
    caption(wnd, 0x64, 0x87, msg_str(MSG_MER_LOYALTY));
    caption(wnd, 0x64, 0x9f, msg_str(MSG_MER_SUMMONS));
    caption(wnd, 0xe6, 0x85, msg_str(MSG_MER_KILLS));
    stat_cell(wnd, 0x18, g_session1.mercStats[0]);
    stat_cell(wnd, 0x2a, g_session1.mercStats[1]);
    stat_cell(wnd, 0x3c, g_session1.mercStats[2]);
    stat_cell(wnd, 0x4e, g_session1.mercStats[3]);
    stat_cell(wnd, 0x60, g_session1.mercStats[4]);
    stat_cell(wnd, 0x72, g_session1.mercStats[5]);
    stat_cell(wnd, 0x84, g_session1.mercStats[6]);
    len = sprintf(cell, "%d", (2000 - g_session1.mercStats[7]) / 10);
    ui_window_text_out_right(wnd, 0x4f, 0x96, cell, len, 1, 0xe, 0, 0);
    ui_window_text_out(wnd, 0x88, 0x18, g_session1.mercName, 0, 0, 0xc, 0x646464, 0, 0);
    caption(wnd, 0x87, 0x18, g_session1.mercName);
    _itoa(g_session1.mercLevel, level, 10);
    caption(wnd, 0x87, 0x2f, level);
    if (g_session1.mercJob == -1)
        return;

    len = sprintf(text, "%d / %d", g_session1.mercHp, g_session1.mercMaxHp);
    mer_number(wnd, 0xda, 0x46, text, len);
    len = sprintf(text, "%d / %d", g_session1.mercSp, g_session1.mercMaxSp);
    mer_number(wnd, 0xda, 0x55, text, len);
    expires = g_session1.merc_contract_stats[4];
    strftime(text, 0x80, msg_str(MSG_MER_EXPIRE_FORMAT), _localtime32(&expires));
    ui_window_text_out(wnd, 0x9b, 0x6f, text, strlen(text), 0, 0xc, 0xff, 0, 0);
    len = sprintf(text, "%d", g_session1.merc_contract_stats[0]);
    mer_number(wnd, 0xaa, 0x85, text, len);
    len = sprintf(text, "%d", g_session1.merc_contract_stats[1]);
    mer_number(wnd, 0xaa, 0x9d, text, len);
    len = sprintf(text, "%d", g_session1.merc_contract_stats[2]);
    mer_number(wnd, 0xf5, 0x9d, text, len);
}

int ui_mer_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                    int val1, int val2, int val3, int val4)
{
    struct UIMerInfoWnd *self = (struct UIMerInfoWnd *)window;
    struct Mode *mode;

    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!mer_info_wnd_info_is_on_screen(self->info))
            mer_info_wnd_info_set_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    case UI_MSG_COMMAND:
        break;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }

    ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
    switch (val1) {
    case UI_ID_OK:
        ui_window_invalidate_base(&self->frame.base);
        break;
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_MER_INFO);
        break;
    case MER_ID_SKILL:
        if (!ui_window_mgr_post_quit1(&g_windowMgr, WND_MER_SKILL))
            ui_window_open(WND_MER_SKILL);
        break;
    case MER_ID_FIRE:
        mode = game_current_base_mode();
        if (ask(msg_str(MSG_MER_FIRE_QUESTION)) != UI_MSGBOX_ANSWER_YES)
            break;
        mode_send_message(mode, MODE_REQ_MER_COMMAND, MER_COMMAND_FIRE, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_MER_INFO);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_MER_SKILL);
        session_clear_merc_commands(&g_session1);
        g_session1.mercPresent = 0;
        break;
    }
    return 0;
}

static void store_info(struct UIWindow *wnd, struct WindowInfo *info)
{
    if (!info)
        return;
    info->x = wnd->x;
    info->y = wnd->y;
    info->width = wnd->width;
    info->height = wnd->height;
}

void ui_homun_info_wnd_store_info(struct UIWindow *self)
{
    store_info(self, ((struct UIHomunInfoWnd *)self)->info);
}

void ui_mer_info_wnd_store_info(struct UIWindow *self)
{
    store_info(self, ((struct UIMerInfoWnd *)self)->info);
}
