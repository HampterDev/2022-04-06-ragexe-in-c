#include "render/rp_pool.h"
#include "session/state.h"
#include "session/session_talk.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <stdio.h>
#include <string.h>
#include "ui/chat/chat.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "platform/clientinfo.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

struct UIProhibitListWnd;
struct UIWindow;
void ui_prohibit_list_wnd_destroy(struct UIWindow *window);
void ui_prohibit_list_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_prohibit_list_wnd_on_process(struct UIWindow *self);
void ui_prohibit_list_wnd_on_draw(struct UIWindow *self);
intptr_t ui_prohibit_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_prohibit_list_wnd_ops = {
    .destroy = ui_prohibit_list_wnd_destroy,
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
    .on_create = ui_prohibit_list_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_prohibit_list_wnd_on_process,
    .on_draw = ui_prohibit_list_wnd_on_draw,
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
    .send_msg = ui_prohibit_list_wnd_send_msg,
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

#define WND_ID_PROHIBIT_LIST 0x67
#define PROHIBIT_ID_COMMAND 0x104
#define MSG_PROHIBIT_TITLE 0x2ca
#define MSG_PROHIBIT_COMMAND1 0x2cf
#define MSG_PROHIBIT_COMMAND2 0x2d0
#define MSG_PROHIBIT_COMMAND3 0x2d1
#define MSG_PROHIBIT_COUNT_A 0x186
#define MSG_PROHIBIT_COUNT_B 0x185
#define MODE_REQ_PROHIBIT_A 0x98
#define MODE_REQ_PROHIBIT_B 0x97

#define CNT_MSG_REQUEST 0x50
#define CNT_MSG_SET_COUNT 0x51

struct UIProhibitListWnd {
    struct UIFrameWnd frame;
    int shown_count;
    int aid;
};

struct UIProhibitListWnd *ui_prohibit_list_wnd_construct(struct UIProhibitListWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_prohibit_list_wnd_ops;
    self->shown_count = 0;
    return self;
}

struct UIWindow *ui_prohibit_list_wnd_new(void)
{
    return (struct UIWindow *)ui_prohibit_list_wnd_construct(mem_alloc(sizeof(struct UIProhibitListWnd)));
}

void ui_prohibit_list_wnd_destroy(struct UIWindow *window)
{
    struct UIProhibitListWnd *self = (struct UIProhibitListWnd *)window;
    self->frame.base.ops = &ui_prohibit_list_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_prohibit_list_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIProhibitListWnd *self = (struct UIProhibitListWnd *)window;
    static const char *const bitmaps[3] = { "txtbox_btn_a", "txtbox_btn_b", "txtbox_btn_c" };
    struct UIWindow *wnd = &self->frame.base;
    char tooltip[8];
    struct UIWindow *hidden;
    struct UIBitmapButton *button;
    struct String skin, path;
    int state;

    ui_window_add_close_button(wnd, "", 0);
    memcpy(tooltip, "Command", 8);
    hidden = mem_alloc(sizeof(struct UINoneVisibleButton));
    if (hidden)
        hidden = (struct UIWindow *)ui_none_visible_button_construct((struct UINoneVisibleButton *)hidden);
    ui_window_create(hidden, 0x78, 0x12);
    ui_window_move_base(hidden, 0x15e, 0x3b);
    hidden->id = PROHIBIT_ID_COMMAND;
    ui_window_add_child(wnd, hidden);

    string_init(&skin);
    string_assign(&skin, UI_SKIN "basic_interface\\", sizeof UI_SKIN "basic_interface\\" - 1);
    button = mem_alloc(sizeof(struct UIBitmapButton));
    if (button)
        button = ui_bitmap_button_construct(button);
    for (state = 0; state < 3; state++) {
        string_copy(&path, &skin);
        string_append_cstr(&path, bitmaps[state]);
        string_append_cstr(&path, ".bmp");
        ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
        string_free(&path);
    }
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, 0x10e - button->bitmap_width, 0x2c);
    ui_bitmap_button_set_tool_tip(button, tooltip);
    button->button.base.id = PROHIBIT_ID_COMMAND;
    ui_window_add_child(wnd, &button->button.base);
    string_free(&skin);
}

void ui_prohibit_list_wnd_on_draw(struct UIWindow *window)
{
    struct UIProhibitListWnd *self = (struct UIProhibitListWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char line[0x100];
    struct BitmapRes *res;
    int count, i, y;

    ui_window_draw_body(wnd, ui_window_draw_title_bar(wnd, 1, msg_str(MSG_PROHIBIT_TITLE), 0), 0, "");
    res = ui_skin_bitmap(UI_SKIN "sysboxs_ld.bmp");
    ui_window_draw_bitmap(wnd, 0, wnd->height - res->height, res, 0);
    res = ui_skin_bitmap(UI_SKIN "sysboxs_rd.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - res->width, wnd->height - res->height, res,
                        0);

    y = 0;
    count = (int)((BYTE *)g_session1.prohibitionLog.last - (BYTE *)g_session1.prohibitionLog.first) / (int)sizeof(struct ProhibitionRecord);
    for (i = 0; i < count; i++) {
        const struct ProhibitionRecord *entry = (const struct ProhibitionRecord *)g_session1.prohibitionLog.first + i;

        if (strcmp(entry->giver, string_cstr(&g_session1.serverName)) != 0 || entry->aid != self->aid)
            continue;
        if (y == 0) {
            sprintf(line, "AID : %d        CharacterName : %s", entry->aid, entry->name);
            ui_window_text_out(wnd, 0xa, 0x19, line, 0, 0, 0xc, 0, 0, 0);
            sprintf(line, "ServerName : %s", entry->giver);
            ui_window_text_out(wnd, 0xa, 0x29, line, 0, 0, 0xc, 0, 0, 0);
        }
        sprintf(line,
                    "ProhibitTime : %d분   LogTime : %d년 %d월 %d일   "
                    "%d시 %d분 %d초",
                    entry->minutes, (WORD)(entry->date >> 16), (BYTE)(entry->date >> 8),
                    (BYTE)entry->date, (BYTE)(entry->time >> 16), (BYTE)(entry->time >> 8),
                    (BYTE)entry->time);
        ui_window_text_out(wnd, 0xa, 0x41 + y, line, 0, 0, 0xc, 0, 0, 0);
        y += 0x12;
    }
}

void ui_prohibit_list_wnd_on_process(struct UIWindow *window)
{
    struct UIProhibitListWnd *self = (struct UIProhibitListWnd *)window;
    int count = (int)((BYTE *)g_session1.prohibitionLog.last - (BYTE *)g_session1.prohibitionLog.first) / (int)sizeof(struct ProhibitionRecord);

    if (self->shown_count != count) {
        self->shown_count = count;
        ui_window_invalidate_base(&self->frame.base);
    }
}

static void prohibit_open_commands(struct UIProhibitListWnd *self)
{
    struct UIWindow *combo = ui_window_open(WND_ID_COMBO_BOX);
    int x, y;

    ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE, (intptr_t)msg_str(MSG_PROHIBIT_COMMAND1), 0, 0, 0);
    ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE, (intptr_t)msg_str(MSG_PROHIBIT_COMMAND2), 0, 0, 0);
    ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE, (intptr_t)msg_str(MSG_PROHIBIT_COMMAND3), 0, 0, 0);
    ui_window_send_msg(combo, NULL, COMBO_MSG_SELECT, 0, 0, 0, 0);
    ui_window_resize(combo, 0x82, 0x30);
    ui_window_send_msg(combo, NULL, COMBO_MSG_OPENER_ID, WND_ID_PROHIBIT_LIST, 0, 0, 0);
    x = 0xa0;
    y = 0x2c;
    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(combo, x, y);
}

static void prohibit_open_count(int request, int msg)
{
    struct UIWindow *cnt = ui_cnt_wnd_new();
    int y;

    ui_window_create(cnt, 0xb6, 0x2e);
    y = scale_y_from480(0xc8);
    ui_frame_wnd_move(cnt, (g_renderer->width - 0x280) / 2 + 0xc8, y);
    ui_window_mgr_add_window(&g_windowMgr, cnt);
    ui_window_send_msg(cnt, NULL, CNT_MSG_REQUEST, request, 0, 0, 0);
    ui_window_send_msg(cnt, NULL, UI_MSG_ADD_TEXT, (intptr_t)msg_str(msg), 0, 0, 0);
    ui_window_send_msg(cnt, NULL, CNT_MSG_SET_COUNT, 1, 0, 0, 0);
}

intptr_t ui_prohibit_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                         intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIProhibitListWnd *self = (struct UIProhibitListWnd *)window;
    struct Mode *mode;

    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_PROHIBIT_LIST);
        else if (val1 == PROHIBIT_ID_COMMAND)
            prohibit_open_commands(self);
    } else if (msg == UI_MSG_SET_INFO) {
        self->aid = val1;
    } else if (msg == COMBO_MSG_CHOSEN) {
        mode = game_current_base_mode();
        if (val1 == 0) {
            if (clientinfo_is_yellow_aid(g_session1.ownAid))
                prohibit_open_count(MODE_REQ_PROHIBIT_A, MSG_PROHIBIT_COUNT_A);
        } else if (val1 == 1) {
            if (clientinfo_is_admin_aid(g_session1.ownAid))
                mode_send_message(mode, MODE_REQ_PROHIBIT_B, 30000, 0, 0, 0);
        } else if (val1 == 2) {
            if (clientinfo_is_admin_aid(g_session1.ownAid))
                prohibit_open_count(MODE_REQ_PROHIBIT_B, MSG_PROHIBIT_COUNT_B);
        }
    }
    return 0;
}
