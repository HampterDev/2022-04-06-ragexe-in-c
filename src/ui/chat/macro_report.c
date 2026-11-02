#include "game/actor_names.h"
#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "ui/misc/snap_mgr.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "world/actor_lookup.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "platform/win_types.h"
#include "ui/chat/chat.h"
#include "ui/ui_std.h"
#include <time.h>
#include "ui/chat/reported_players.h"
#include "ui/chat/reporter_controls.h"
#include "ui/chat/macro_reporter.h"
#include "actor/job.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

struct Vec;
struct UIMacroReporterWnd;
struct UIWindow;
void ui_macro_reporter_wnd_destroy(struct UIWindow *window);
void ui_macro_reporter_wnd_store_info(struct UIWindow *self);
void ui_macro_reporter_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_macro_reporter_wnd_on_process(struct UIWindow *self);
void ui_macro_reporter_wnd_on_draw(struct UIWindow *self);
void ui_macro_reporter_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_macro_reporter_wnd_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused);
void ui_macro_reporter_wnd_on_wheel(struct UIWindow *self, int delta);
intptr_t ui_macro_reporter_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_macro_reporter_wnd_ops = {
    .destroy = ui_macro_reporter_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_macro_reporter_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_macro_reporter_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_macro_reporter_wnd_on_process,
    .on_draw = ui_macro_reporter_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_macro_reporter_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_macro_reporter_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_macro_reporter_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_macro_reporter_wnd_send_msg,
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
struct Vec;
struct UIMacroUserInfoWnd;
struct UIWindow;
void ui_macro_user_info_wnd_destroy(struct UIWindow *window);
BYTE ui_macro_user_info_wnd_is_click_through(struct UIWindow *self);
void ui_macro_user_info_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_macro_user_info_wnd_on_draw(struct UIWindow *self);
intptr_t ui_macro_user_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_macro_user_info_wnd_ops = {
    .destroy = ui_macro_user_info_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_macro_user_info_wnd_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_macro_user_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_macro_user_info_wnd_on_draw,
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
    .send_msg = ui_macro_user_info_wnd_send_msg,
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

#define WND_ID_MACRO_REPORTER 0x11e
#define WND_ID_MACRO_USER_INFO 0x11f
#define REPORTER_ID_REMOVE 0xcb
#define REPORTER_ID_REPORT 0x151
#define MSG_REPORTER_RADIO1 0xb47
#define MSG_REPORTER_RADIO2 0xb48
#define MSG_REPORTER_RADIO1_TIP 0xb49
#define MSG_REPORTER_RADIO2_TIP 0xb4a
#define MSG_REPORTER_COUNT_TIP 0xb4b
#define MSG_REPORTER_CONFIRM 0xb3c
#define MSGBOX_ID_NO 0xb4
#define PACKET_MACRO_REPORT 0xa56
#define REPORTER_ROW_HEIGHT 0x16
#define REPORTER_NAME_WIDTH 0x8e

struct UIMacroUserInfoWnd {
    struct UIFrameWnd frame;
    struct String name;
    struct String job;
};

struct UIMacroReporterWnd {
    struct UIFrameWnd frame;
    struct Vec rows;
    struct UIScrollBar *scroll;
    BYTE scrolling;
    int font_height;
    int top_row;
    int rows_shown;
    struct UIWindow *pick_check;
    struct UIBitmapButton *report_button;
    struct UIWindow *radios[2];
    int radio_choice;
    struct UIWindow *count_edit;
    int count;
    int count_max;
    struct MacroReporterInfo *info;
    struct Vec players;
    BYTE pick_mode;
    struct Vec removed;
};

int ui_macro_reporter_picking(const struct UIWindow *wnd)
{
    return ((const struct UIMacroReporterWnd *)wnd)->pick_mode == 1;
}

int ui_macro_reporter_area(const struct UIWindow *wnd)
{
    return ((const struct UIMacroReporterWnd *)wnd)->radio_choice;
}

int ui_macro_reporter_radius(const struct UIWindow *wnd)
{
    return ((const struct UIMacroReporterWnd *)wnd)->count;
}

static int reporter_info_on_screen(const struct MacroReporterInfo *info)
{
    return info->x > 0 && g_renderer->width - info->width >= info->x && info->y > 0
        && g_renderer->height - info->height >= info->y;
}

static void reporter_info_default(struct MacroReporterInfo *info)
{
    info->width = 0xd1;
    info->height = 0x134;
    info->x = g_renderer->width - 0x103;
    info->y = (g_renderer->height - 0x134) / 2;
}

struct UIMacroReporterWnd *ui_macro_reporter_wnd_construct(struct UIMacroReporterWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_macro_reporter_wnd_ops;
    memset(&self->rows, 0, sizeof self->rows);
    self->scroll = NULL;
    self->scrolling = 0;
    self->font_height = 0xd;
    self->top_row = 0;
    self->rows_shown = 10;
    self->pick_check = NULL;
    self->report_button = NULL;
    self->radio_choice = 0;
    self->count_edit = NULL;
    self->count = 1;
    self->count_max = 5;
    self->info = NULL;
    memset(&self->players, 0, sizeof self->players);
    self->pick_mode = 0;
    memset(&self->removed, 0, sizeof self->removed);
    self->radios[0] = NULL;
    self->radios[1] = NULL;
    return self;
}

struct UIWindow *ui_macro_reporter_wnd_new(void)
{
    return (struct UIWindow *)ui_macro_reporter_wnd_construct(mem_alloc(sizeof(struct UIMacroReporterWnd)));
}

void ui_macro_reporter_wnd_destroy(struct UIWindow *window)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    struct ReporterRow *row;
    struct UIWindow **wnd;

    self->frame.base.ops = &ui_macro_reporter_wnd_ops;
    for (row = self->rows.first; row != self->rows.last; row++) {
        ui_window_remove_child(&self->frame.base, &row->remove->button.base, 1);
        ui_window_remove_child(&self->frame.base, row->name, 1);
    }
    self->rows.last = self->rows.first;
    for (wnd = self->removed.first; wnd != self->removed.last; wnd++)
        ui_window_remove_child(&self->frame.base, *wnd, 1);
    self->removed.last = self->removed.first;
    reported_players_clear(&self->players);
    vec_free(&self->removed);
    reported_players_free(&self->players);
    vec_free(&self->rows);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_macro_reporter_wnd_layout(struct UIMacroReporterWnd *self)
{
    int count = (int)VEC_COUNT(&self->rows, struct ReporterRow);
    int hidden;

    if (count == 0 || self->scroll == NULL)
        return;
    hidden = count - self->rows_shown;
    if (hidden < 0)
        hidden = 0;
    self->scrolling = hidden > 0;
    if (hidden > 0) {
        ui_scroll_bar_set_range(self->scroll, hidden, self->rows_shown,
                             (int)VEC_COUNT(&self->rows, struct ReporterRow));
        if (self->top_row < 0)
            self->top_row = 0;
        if (self->top_row > hidden)
            self->top_row = hidden;
        ui_scroll_bar_set_pos(self->scroll, self->top_row);
    } else {
        self->top_row = 0;
        ui_scroll_bar_set_range(self->scroll, 0, self->rows_shown,
                             (int)VEC_COUNT(&self->rows, struct ReporterRow));
    }
    ui_window_invalidate(&self->scroll->base);
    ui_window_invalidate_base(&self->frame.base);
}

void ui_macro_reporter_wnd_add_row(struct UIMacroReporterWnd *self, int aid, struct String name)
{
    static const char *const remove_bitmaps[3] = {
        UI_SKIN "basic_interface\\que_close_on.bmp",
        UI_SKIN "basic_interface\\que_close_off.bmp",
        UI_SKIN "basic_interface\\que_close_off.bmp",
    };
    struct ReporterRow *row;
    struct ReporterRow added;
    struct UIBitmapButton *remove;
    struct UIWindow *label;
    struct String shown;
    int state;

    for (row = self->rows.first; row != self->rows.last; row++)
        if (row->name->id == aid)
            break;
    if (row != self->rows.last) {
        string_free(&name);
        return;
    }

    remove = mem_alloc(sizeof(struct UIBitmapButton));
    if (remove)
        remove = ui_bitmap_button_construct(remove);
    for (state = 0; state < 3; state++)
        ui_bitmap_button_set_bitmap(remove, remove_bitmaps[state], state, 0);
    ui_window_create(&remove->button.base, remove->bitmap_width, remove->bitmap_height);
    remove->button.base.id = REPORTER_ID_REMOVE;
    remove->button.cmd_value = aid;
    ui_button_show((struct UIButton *)(&remove->button.base), 0);
    ui_window_add_child(&self->frame.base, &remove->button.base);

    ui_window_ellipsize_text(&self->frame.base, &shown, REPORTER_NAME_WIDTH, string_cstr(&name), 0, 0xd,
                           0);
    label = (struct UIWindow *)ui_text_button_create();
    ui_text_button_set_text(label, "", 0, 0, 0xd, 0, 1);
    ((struct UITextButton *)(label))->hover_colour = 0x2d2d2d;
    string_assign(&((struct UITextButton *)label)->button.text, string_cstr(&shown), shown.size);
    ui_text_button_fit_to_caption((struct UITextButton *)label);
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)label, string_cstr(&name));
    label->id = aid;
    ui_button_set_show((struct UIButton *)(label), 0);
    ui_window_add_child(&self->frame.base, label);

    added.remove = remove;
    added.name = label;
    vec_push(&self->rows, &added, sizeof(struct ReporterRow));
    string_free(&shown);
    string_free(&name);
}

void ui_macro_reporter_wnd_add_player(struct UIMacroReporterWnd *self, int aid)
{
    struct GameMode *mode = game_current_mode();
    void *actor = game_find_actor(aid);
    struct String name, copy;
    struct ReportedPlayer player, *p;

    if (!mode || !actor || !session_is_player_job(&g_session1, game_actor_get_job(actor)))
        return;
    game_mode_get_actor_name(mode, &name, aid);
    for (p = self->players.first; p != self->players.last; p++)
        if (p->aid == aid)
            break;
    if (p == self->players.last) {
        reported_player_init(&player, &name, aid, (WORD)game_actor_get_job(actor));
        vec_push(&self->players, &player, sizeof player);
    }
    string_copy(&copy, &name);
    ui_macro_reporter_wnd_add_row(self, aid, copy);
    ui_macro_reporter_wnd_layout(self);
    string_free(&name);
}

void ui_macro_reporter_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    static const char *const report_bitmaps[4] = {
        UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp", UI_SKIN "btn_ok_c.bmp",
    };
    struct UIWindow *wnd = &self->frame.base;
    struct String labels[2], tips[2], text;
    struct UIWindow *control;
    struct UIBitmapButton *button;
    const char *tip;
    int i, x;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);

    control = mem_alloc(sizeof(struct UIScrollBar));
    if (control)
        control = (struct UIWindow *)ui_scroll_bar_construct((struct UIScrollBar *)control);
    self->scroll = (struct UIScrollBar *)control;
    ui_scroll_bar_create((struct UIScrollBar *)control, 0xba, 0x38, 1, 0xd8);
    ui_window_add_child(wnd, (struct UIWindow *)self->scroll);

    control = mem_alloc(sizeof(struct UICheckBox));
    if (control)
        control = (struct UIWindow *)ui_check_box_construct(control);
    self->pick_check = control;
    ui_window_create(control, 0x2a, 0x14);
    ui_check_box_set_bitmaps_fit((struct UICheckBox *)self->pick_check, UI_SKIN "btn_q_active_a.bmp",
                             UI_SKIN "btn_q_active.bmp");
    ui_window_move(self->pick_check, 0x9d, 0x1b);
    ui_check_box_set_check(self->pick_check, 0);
    ui_window_add_child(wnd, self->pick_check);

    button = mem_alloc(sizeof(struct UIBitmapButton));
    if (button)
        button = ui_bitmap_button_construct(button);
    self->report_button = button;
    for (i = 0; i < 4; i++)
        ui_bitmap_button_set_bitmap(self->report_button, report_bitmaps[i], i, 0);
    ui_window_create(&self->report_button->button.base, self->report_button->bitmap_width,
                     self->report_button->bitmap_height);
    ui_window_move_base(&self->report_button->button.base, 0x9d, 0x11d);
    self->report_button->button.base.id = REPORTER_ID_REPORT;
    ui_window_add_child(wnd, &self->report_button->button.base);

    string_init(&labels[0]);
    string_assign_cstr(&labels[0], msg_str(MSG_REPORTER_RADIO1));
    string_init(&labels[1]);
    string_assign_cstr(&labels[1], msg_str(MSG_REPORTER_RADIO2));
    string_init(&tips[0]);
    string_assign_cstr(&tips[0], msg_str(MSG_REPORTER_RADIO1_TIP));
    string_init(&tips[1]);
    string_assign_cstr(&tips[1], msg_str(MSG_REPORTER_RADIO2_TIP));
    for (i = 0, x = 0xe; i < 2; i++, x += 0x37) {
        control = mem_alloc(sizeof(struct UIRadioBtn));
        if (control)
            control = (struct UIWindow *)ui_radio_btn_construct((struct UIRadioBtn *)control);
        self->radios[i] = control;
        ui_window_create(control, 0x32, 0xc);
        ui_window_move(self->radios[i], x, 0x1f);
        ui_radio_btn_set_label(self->radios[i], string_cstr(&labels[i]));
        self->radios[i]->id = UI_ID_RADIO;
        ((struct UIRadioBtn *)self->radios[i])->value = i;
        ui_window_send_msg(self->radios[i], wnd, UI_MSG_SET_STATE, i == self->radio_choice, 0, 0, 0);
        tip = string_cstr(&tips[i]);
        if (tip)
            string_assign_cstr(&((struct UIRadioBtn *)self->radios[i])->tooltip, tip);
        ui_window_add_child(wnd, self->radios[i]);
    }

    control = mem_alloc(sizeof(struct UIEditCtrl));
    if (control)
        control = ui_edit_ctrl_construct(control);
    self->count_edit = control;
    ui_window_create(control, 0xf, 0xe);
    ui_window_move(self->count_edit,
             ui_window_text_width(wnd, string_cstr(&labels[1]), 0, 0, 0xc, 0, 0) + 0x53, 0x1e);
    ((struct UIEditCtrl *)self->count_edit)->no_border = 0;
    ui_edit_ctrl_set_border_colour(self->count_edit, 0x7d, 0x7d, 0x7d);
    UI_EDIT_MAX_CHARS(self->count_edit) = 1;
    ((struct UIEditCtrl *)self->count_edit)->keep_ime_on_enter = 1;
    ((struct UIEditCtrl *)self->count_edit)->multiline = 1;
    string_init(&text);
    string_format(&text, "%d", self->count);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->count_edit, string_cstr(&text));
    string_free(&text);
    tip = msg_str(MSG_REPORTER_COUNT_TIP);
    if (tip)
        string_assign_cstr(&((struct UIEditCtrl *)self->count_edit)->tooltip, tip);
    ui_window_add_child(wnd, self->count_edit);
    if (self->radio_choice != 1) {
        ((struct UIEditCtrl *)self->count_edit)->disabled = 1;
        ui_window_invalidate(self->count_edit);
    }

    reported_players_clear(&self->players);
    for (i = 2; i-- > 0;)
        string_free(&tips[i]);
    for (i = 2; i-- > 0;)
        string_free(&labels[i]);
}

void ui_macro_reporter_wnd_on_process(struct UIWindow *window)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    struct UIWindow **wnd = self->removed.first;

    while (wnd != (struct UIWindow **)self->removed.last) {
        if (((struct UIButton *)*wnd)->disabled == 0) {
            wnd++;
            continue;
        }
        ui_window_remove_child(&self->frame.base, *wnd, 1);
        memmove(wnd, wnd + 1, (BYTE *)self->removed.last - (BYTE *)(wnd + 1));
        self->removed.last = (struct UIWindow **)self->removed.last - 1;
    }
    ui_window_invalidate_base(&self->frame.base);
}

void ui_macro_reporter_wnd_on_draw(struct UIWindow *window)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ReporterRow *rows;
    unsigned i;
    int row, y;

    ((struct UIButton *)self->report_button)->disabled = VEC_COUNT(&self->players, struct ReportedPlayer) == 0;
    ui_window_invalidate_base(&self->report_button->button.base);
    ui_window_fill_round_rect(wnd, 0, 0, wnd->width, wnd->height, 0xffffffff);
    ui_window_draw_title_bar(wnd, 1, "", 0);
    ui_window_draw_button_bar(wnd, 1, 0, 1);
    for (row = 0, y = 0x38; row < self->rows_shown; row++, y += REPORTER_ROW_HEIGHT)
        ui_window_fill_rect(wnd, 0xe, y, 0xa9, 0x12, 0xe6e6e6);
    for (i = 0; i < VEC_COUNT(&self->rows, struct ReporterRow); i++) {
        rows = self->rows.first;
        ui_button_show((struct UIButton *)(&rows[i].remove->button.base), 0);
        ui_button_show((struct UIButton *)(rows[i].name), 0);
    }
    for (row = self->top_row, y = 0x39; row < self->top_row + self->rows_shown; row++, y += REPORTER_ROW_HEIGHT) {
        rows = self->rows.first;
        if (VEC_COUNT(&self->rows, struct ReporterRow) <= (unsigned)row)
            break;
        ui_button_show((struct UIButton *)(&rows[row].remove->button.base), 1);
        ui_window_move_base(&rows[row].remove->button.base, 0xf, y + 2);
        ui_button_show((struct UIButton *)(rows[row].name), 1);
        ui_window_move(rows[row].name, 0x24, y);
    }
}

void ui_macro_reporter_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int sx = x, sy = y;

    if (self->pick_mode == 1 && y > 0x11)
        return;
    if (wnd->id != g_windowMgr.last_id) {
        ui_window_mgr_touch_window_id(&g_windowMgr, wnd->id);
        g_windowMgr.last_id = wnd->id;
    }
    ui_window_mgr_set_capture(&g_windowMgr, wnd);
    ui_window_to_screen(wnd, &sx, &sy);
    self->frame.press_x = sx;
    self->frame.press_y = sy;
    self->frame.saved_x = wnd->x;
    self->frame.saved_y = wnd->y;
    snap_mgr_save_group_positions(&g_snapMgr, wnd);
}

void ui_macro_reporter_wnd_on_l_btn_up(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    if (&self->frame.base == ui_window_mgr_get_capture(&g_windowMgr)) {
        g_windowMgr.last_id = -1;
        ui_window_mgr_release_capture(&g_windowMgr);
        snap_mgr_relink_all(&g_snapMgr, &self->frame.base);
    }
}

void ui_macro_reporter_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    if (self->scrolling == 1)
        ui_macro_reporter_wnd_send_msg(&self->frame.base, (struct UIWindow *)self->scroll, UI_MSG_SCROLL_V, delta < 0 ? 3 : -3, 0, 0, 0);
}

void ui_macro_reporter_wnd_store_info(struct UIWindow *window)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    struct MacroReporterInfo *info = self->info;
    struct ReportedPlayer *from, *to;
    unsigned i;

    if (info == NULL)
        return;
    info->width = self->frame.base.width;
    info->height = self->frame.base.height;
    info->x = self->frame.base.x;
    info->y = self->frame.base.y;
    info->pick_mode = self->pick_mode;
    reported_players_clear(&info->players);
    vec_resize(&info->players, VEC_COUNT(&self->players, struct ReportedPlayer), sizeof(struct ReportedPlayer));
    for (i = 0; i < VEC_COUNT(&self->players, struct ReportedPlayer); i++) {
        from = (struct ReportedPlayer *)self->players.first + i;
        to = (struct ReportedPlayer *)info->players.first + i;
        if (to != from)
            string_assign(&to->name, string_cstr(&from->name), from->name.size);
        to->aid = from->aid;
        to->job = from->job;
    }
}

static void reporter_set_info(struct UIMacroReporterWnd *self, struct MacroReporterInfo *info)
{
    unsigned i;

    self->info = info;
    if (!reporter_info_on_screen(info))
        reporter_info_default(info);
    ui_window_resize_base(&self->frame.base, info->width, info->height);
    ui_frame_wnd_move(&self->frame.base, info->x, info->y);
    self->pick_mode = info->pick_mode;
    ui_check_box_set_check(self->pick_check, self->pick_mode);
    reported_players_clear(&self->players);
    for (i = 0; i < VEC_COUNT(&info->players, struct ReportedPlayer); i++) {
        struct ReportedPlayer *saved = (struct ReportedPlayer *)info->players.first + i;
        struct ReportedPlayer player;
        struct String name;

        string_copy(&name, &saved->name);
        string_copy(&player.name, &name);
        player.aid = saved->aid;
        player.job = saved->job;
        string_free(&name);
        vec_push(&self->players, &player, sizeof player);
        string_copy(&name, &saved->name);
        ui_macro_reporter_wnd_add_row(self, saved->aid, name);
    }
    ui_macro_reporter_wnd_layout(self);
}

static void reporter_remove(struct UIMacroReporterWnd *self, struct UIWindow *sender, int aid)
{
    struct ReportedPlayer *player, *last = self->players.last;
    struct ReporterRow *row, *rows_last = self->rows.last;
    struct UIWindow *wnd;

    for (player = self->players.first; player != last; player++)
        if (player->aid == aid)
            break;
    if (player != last) {
        string_free(&player->name);
        vec_erase(&self->players, player, player + 1);
    }
    for (row = self->rows.first; row != rows_last; row++)
        if ((struct UIWindow *)row->remove == sender)
            break;
    if (row != rows_last) {
        ((struct UIButton *)row->remove)->disabled = 1;
        ui_window_invalidate_base(&row->remove->button.base);
        wnd = &row->remove->button.base;
        vec_push(&self->removed, &wnd, sizeof wnd);
        ((struct UIButton *)row->name)->disabled = 1;
        ui_window_invalidate(row->name);
        wnd = row->name;
        vec_push(&self->removed, &wnd, sizeof wnd);
        vec_erase(&self->rows, row, row + 1);
    }
    ui_macro_reporter_wnd_layout(self);
}

static void reporter_report(struct UIMacroReporterWnd *self)
{
#pragma pack(push, 1)
    struct {
        WORD id;
        int aid;
    } packet;
#pragma pack(pop)
    struct Vec aids = { NULL, NULL, NULL };
    struct ReportedPlayer *player;
    struct ReporterRow *row;
    struct String question;
    int *aid, answer, len;
    unsigned count, i;

    for (player = self->players.first; player != self->players.last; player++) {
        for (aid = aids.first; aid != aids.last; aid++)
            if (*aid == player->aid)
                break;
        if (aid != aids.last)
            continue;
        vec_push(&aids, &player->aid, sizeof(int));
    }
    count = (unsigned)VEC_COUNT(&aids, int);
    string_init(&question);
    string_format(&question, msg_str(MSG_REPORTER_CONFIRM), count);
    answer = ui_window_mgr_message_box(&g_windowMgr, string_cstr(&question), 2, 0, 0, 0, "메시지",
                                    0x118, 0x78, 0);
    string_free(&question);
    if (answer != MSGBOX_ID_NO) {
        for (i = 0; i < count; i++) {
            packet.id = PACKET_MACRO_REPORT;
            packet.aid = ((int *)aids.first)[i];
            len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_MACRO_REPORT);
            rag_connection_send_packet(rag_connection_instance(), len, &packet);
        }
        reported_players_clear(&self->players);
        for (row = self->rows.first; row != self->rows.last; row++) {
            struct UIWindow *wnd = &row->remove->button.base;

            ((struct UIButton *)row->remove)->disabled = 1;
            ui_window_invalidate_base(&row->remove->button.base);
            vec_push(&self->removed, &wnd, sizeof wnd);
            ((struct UIButton *)row->name)->disabled = 1;
            ui_window_invalidate(row->name);
            wnd = row->name;
            vec_push(&self->removed, &wnd, sizeof wnd);
        }
        self->rows.last = self->rows.first;
    }
    vec_free(&aids);
}

static void reporter_count_edited(struct UIMacroReporterWnd *self)
{
    struct String text, shown;
    const char *digits;

    string_init(&text);
    string_assign_cstr(&text, ui_edit_ctrl_get_text(self->count_edit));
    digits = string_cstr(&text);
    if (text.size > 1 || digits[0] < '0' || digits[0] > '9') {
        string_init(&shown);
        string_format(&shown, "%d", self->count);
    } else {
        self->count = atoi(digits);
        if (self->count <= self->count_max) {
            string_free(&text);
            return;
        }
        self->count = self->count_max;
        string_init(&shown);
        string_format(&shown, "%d", self->count_max);
    }
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->count_edit, string_cstr(&shown));
    string_free(&shown);
    string_free(&text);
}

static void reporter_show_user(struct UIMacroReporterWnd *self, struct UIWindow *sender, int aid)
{
    struct ReportedPlayer *player;
    struct ReporterRow *rows = self->rows.first;
    struct UIWindow *info;
    struct String name;
    const char *job;
    unsigned index, count;
    WORD job_id;

    for (player = self->players.first; player != self->players.last; player++)
        if (player->aid == aid)
            break;
    if (player == self->players.last)
        return;
    count = (unsigned)VEC_COUNT(&self->rows, struct ReporterRow);
    for (index = 0; index < count; index++)
        if (rows[index].name == sender)
            break;
    if (ui_close_window(WND_ID_MACRO_USER_INFO))
        return;
    info = ui_window_open(WND_ID_MACRO_USER_INFO);
    ui_window_move(info, self->frame.base.x + 0xf, self->frame.base.y + 0x4e + index * REPORTER_ROW_HEIGHT);
    job_id = player->job;
    string_copy(&name, &player->name);
    if (&((struct UIMacroUserInfoWnd *)info)->name != &name)
        string_assign(&((struct UIMacroUserInfoWnd *)info)->name, string_cstr(&name), name.size);
    job = session_get_job_name(&g_session1, job_id, -1);
    string_assign_cstr(&((struct UIMacroUserInfoWnd *)info)->job, job);
    ui_window_invalidate(info);
    string_free(&name);
}

intptr_t ui_macro_reporter_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMacroReporterWnd *self = (struct UIMacroReporterWnd *)window;
    int old;

    switch (msg) {
    case 0:
        if (sender == self->count_edit)
            reporter_count_edited(self);
        break;
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_MACRO_REPORTER);
            break;
        case REPORTER_ID_REMOVE:
            reporter_remove(self, sender, val2);
            break;
        case UI_ID_CHECKBOX:
            if (sender == self->pick_check) {
                self->pick_mode = val2 != 0;
                self->frame.base.alpha_target = val2 ? 0x80 : 0xff;
                self->frame.base.alpha_time = platform_ticks_ms();
            }
            break;
        case UI_ID_RADIO:
            self->radio_choice = val2;
            ui_window_send_msg(self->radios[0], &self->frame.base, UI_MSG_SET_STATE, 0, 0, 0, 0);
            ui_window_send_msg(self->radios[1], &self->frame.base, UI_MSG_SET_STATE, 0, 0, 0, 0);
            ui_window_send_msg(sender, &self->frame.base, UI_MSG_SET_STATE, 1, 0, 0, 0);
            ((struct UIEditCtrl *)self->count_edit)->disabled = sender != self->radios[1];
            ui_window_invalidate(self->count_edit);
            break;
        case REPORTER_ID_REPORT:
            reporter_report(self);
            break;
        }
        break;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old = self->top_row;
        if (msg == UI_MSG_PAGE_UP)
            self->top_row = old - self->rows_shown + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            self->top_row = old + self->rows_shown - 1;
        else
            self->top_row = old + val1;
        if (old != self->top_row)
            ui_macro_reporter_wnd_layout(self);
        return self->top_row - old;
    case UI_MSG_SET_INFO:
        reporter_set_info(self, (struct MacroReporterInfo *)val1);
        break;
    case UI_MSG_TEXT_BUTTON:
        reporter_show_user(self, sender, val1);
        break;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIMacroUserInfoWnd *ui_macro_user_info_wnd_construct(struct UIMacroUserInfoWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_macro_user_info_wnd_ops;
    string_init(&self->name);
    string_init(&self->job);
    return self;
}

struct UIWindow *ui_macro_user_info_wnd_new(void)
{
    return (struct UIWindow *)ui_macro_user_info_wnd_construct(mem_alloc(sizeof(struct UIMacroUserInfoWnd)));
}

void ui_macro_user_info_wnd_destroy(struct UIWindow *window)
{
    struct UIMacroUserInfoWnd *self = (struct UIMacroUserInfoWnd *)window;
    self->frame.base.ops = &ui_macro_user_info_wnd_ops;
    string_free(&self->job);
    string_free(&self->name);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

BYTE ui_macro_user_info_wnd_is_click_through(struct UIWindow *self)
{
    return 1;
}

void ui_macro_user_info_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMacroUserInfoWnd *self = (struct UIMacroUserInfoWnd *)window;
    ui_window_add_close_button(&self->frame.base, "", 0);
}

void ui_macro_user_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIMacroUserInfoWnd *self = (struct UIMacroUserInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_clear_canvas(wnd, 0xfafafa);
    ui_window_text_out(wnd, 5, 5, string_cstr(&self->name), 0, 0, 0xc, 0, 1, 0);
    ui_window_text_out(wnd, 5, 0x14, string_cstr(&self->job), 0, 0, 0xc, 0, 0, 0);
}

intptr_t ui_macro_user_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMacroUserInfoWnd *self = (struct UIMacroUserInfoWnd *)window;
    if (msg == UI_MSG_COMMAND && val1 == UI_ID_CLOSE)
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_MACRO_USER_INFO);
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define PACKET_REQ_PLAYER_AID_IN_RANGE 0xa6c
#pragma pack(push, 1)
struct PacketReqPlayerAidInRange {
    WORD id;
    WORD x, y;
    BYTE radius;
};
#pragma pack(pop)
SIZE_ASSERT(PacketReqPlayerAidInRange, 7);

void macro_reporter_request_players_in_range(WORD x, WORD y, BYTE radius)
{
    struct PacketReqPlayerAidInRange packet;
    int len;

    packet.id = PACKET_REQ_PLAYER_AID_IN_RANGE;
    packet.x = x;
    packet.y = y;
    packet.radius = radius;
    len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_REQ_PLAYER_AID_IN_RANGE);
    rag_connection_send_packet(rag_connection_instance(), len, &packet);
}
