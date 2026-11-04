#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "core/markup_tags.h"
#include "ui/social/social.h"
#include <string.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"

#define MSG_PARTY_SETTING_TITLE 0x123
#define MSG_PARTY_CREATE_TITLE 0x806
#define MSG_PARTY_SETTING_TEXT 0x124
#define MSG_PARTY_CREATE_TEXT 0x809
#define MSG_PARTY_OPTION1_TEXT 0x125
#define MSG_PARTY_OPTION2_TEXT 0x2e2
#define MSG_PARTY_OPTION0_A 0x11f
#define MSG_PARTY_OPTION0_B 0x120
#define MSG_PARTY_OPTION1_A 0x121
#define MSG_PARTY_OPTION1_B 0x122
#define MSG_PARTY_OPTION2_A 0x2e3
#define MSG_PARTY_OPTION2_B 0x2e4

#define PARTY_SETTING_HEIGHT_FULL 0xf5
#define PARTY_SETTING_HEIGHT_SHORT 0xb4
#define PARTY_SETTING_HEIGHT_CREATE 0xdc
#define PARTY_LAYOUT_FULL 0x46
#define PARTY_LAYOUT_SHORT 5
#define PARTY_LAYOUT_CREATE 0x32

struct UIPartySettingWnd {
    struct UIFrameWnd frame;
    struct UIWindow *option0_radio[2];
    int option0;
    struct UIWindow *option1_radio[2];
    int option1;
    struct UIWindow *option2_radio[2];
    int option2;
    struct WindowInfo *info;
    struct UIWindow *name;
    int layout;
};
void ui_party_setting_wnd_destroy(struct UIWindow *window);
void ui_party_setting_wnd_store_info(struct UIWindow *self);
void ui_party_setting_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_party_setting_wnd_on_draw(struct UIWindow *self);
void ui_party_setting_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_party_setting_wnd_send_msg(struct UIWindow *frame, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_party_setting_wnd_ops = {
    .destroy = ui_party_setting_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_party_setting_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_party_setting_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_party_setting_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_party_setting_wnd_on_l_btn_down,
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
    .send_msg = ui_party_setting_wnd_send_msg,
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

struct UIPartySettingWnd *ui_party_setting_wnd_construct(struct UIPartySettingWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_party_setting_wnd_ops;
    self->layout = 0;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_party_setting_wnd_new(void)
{
    return (struct UIWindow *)ui_party_setting_wnd_construct(mem_alloc(sizeof(struct UIPartySettingWnd)));
}

void ui_party_setting_wnd_destruct(struct UIPartySettingWnd *self)
{
    self->frame.base.ops = &ui_party_setting_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_party_setting_wnd_destroy(struct UIWindow *window)
{
    struct UIPartySettingWnd *self = (struct UIPartySettingWnd *)window;
    ui_party_setting_wnd_destruct(self);
    mem_free(self);
}

static struct UIWindow *new_radio(void)
{
    struct UIWindow *radio = mem_alloc(sizeof(struct UIRadioBtn));

    if (radio)
        radio = (struct UIWindow *)ui_radio_btn_construct((struct UIRadioBtn *)radio);
    ui_window_create(radio, 0xa0, 0xc);
    return radio;
}

static void radio_set(struct UIPartySettingWnd *self, struct UIWindow *radio, int chosen)
{
    ui_window_send_msg(radio, &self->frame.base, UI_MSG_SET_STATE, chosen, 0, 0, 0);
}

static void add_lower_pair(struct UIPartySettingWnd *self, struct UIWindow **radios, int chosen, int label_a,
                           int label_b, int top, int label_first)
{
    const char *labels[2];
    int i;

    labels[0] = msg_str(label_a);
    labels[1] = msg_str(label_b);
    for (i = 0; i < 2; i++) {
        struct UIWindow *radio = new_radio();

        radios[i] = radio;
        if (label_first)
            ui_radio_btn_set_label(radio, labels[i]);
        ui_window_move(radios[i], 0x14, top + 0x14 * i);
        if (!label_first)
            ui_radio_btn_set_label(radios[i], labels[i]);
        radio_set(self, radios[i], chosen == i);
        ((struct UIRadioBtn *)(radios[i]))->enabled = session_get_party_member_count(&g_session1) == 0;
        if (g_session1.partyLeader)
            ((struct UIRadioBtn *)(radios[i]))->enabled = 1;
        ui_window_add_child(&self->frame.base, radios[i]);
    }
}

void ui_party_setting_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIPartySettingWnd *self = (struct UIPartySettingWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapButton *close;
    int close_x, i;

    self->option0 = g_session1.partyExpOption == 2 ? 0 : g_session1.partyExpOption;
    self->option1 = g_session1.partyItemPickupRule;
    self->option2 = g_session1.partyItemDivisionRule;
    if (height == PARTY_SETTING_HEIGHT_FULL)
        self->layout = PARTY_LAYOUT_FULL;
    else if (height == PARTY_SETTING_HEIGHT_SHORT)
        self->layout = PARTY_LAYOUT_SHORT;
    else if (height == PARTY_SETTING_HEIGHT_CREATE)
        self->layout = PARTY_LAYOUT_CREATE;

    if (self->layout == PARTY_LAYOUT_FULL) {
        const char *labels[2];

        labels[0] = msg_str(MSG_PARTY_OPTION0_A);
        labels[1] = msg_str(MSG_PARTY_OPTION0_B);
        for (i = 0; i < 2; i++) {
            struct UIWindow *radio = new_radio();

            self->option0_radio[i] = radio;
            ui_window_move_base(radio, 0x14, 0x2a + 0x14 * i);
            ui_radio_btn_set_label(self->option0_radio[i], labels[i]);
            radio_set(self, self->option0_radio[i], self->option0 == i);
            ui_window_add_child(wnd, self->option0_radio[i]);
            if (!g_session1.partyLeader)
                ((struct UIRadioBtn *)(self->option0_radio[i]))->enabled = 0;
        }
    } else if (self->layout == PARTY_LAYOUT_CREATE) {
        struct UIWindow *name = mem_alloc(sizeof(struct UIEditCtrl));

        if (name)
            name = ui_edit_ctrl_construct(name);
        self->name = name;
        ui_window_create(name, 0x64, 0x14);
        ui_window_move(self->name, 0x14, 0x26);
        UI_EDIT_MAX_CHARS(self->name) = 0x17;
        ui_edit_ctrl_set_back_colour(self->name, 0xe6, 0xe6, 0xe6);
        ui_window_add_child(wnd, self->name);
    }

    add_lower_pair(self, self->option1_radio, self->option1, MSG_PARTY_OPTION1_A, MSG_PARTY_OPTION1_B,
                   self->layout + 0x25, 1);
    add_lower_pair(self, self->option2_radio, self->option2, MSG_PARTY_OPTION2_A, MSG_PARTY_OPTION2_B,
                   self->layout + 0x66, 0);

    UI_FRAME_DEFAULT_ID(&self->frame) = UI_ID_OK;
    ui_add_bitmap_button(wnd, UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp",
                         wnd->width - 0x5c, wnd->height - 0x18, UI_ID_OK);
    ui_add_bitmap_button(wnd, UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp", UI_SKIN "btn_cancel_b.bmp",
                         wnd->width - 0x2e, wnd->height - 0x18, UI_ID_CANCEL);

    close_x = wnd->width - 0xe;
    close = mem_alloc(sizeof(struct UIBitmapButton));
    if (close)
        close = ui_bitmap_button_construct(close);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\sys_close_off.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\sys_close_on.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\sys_close_on.bmp", UI_BTN_DOWN, 0);
    ui_window_create(&close->button.base, close->bitmap_width, close->bitmap_height);
    ui_window_move_base(&close->button.base, close_x, 3);
    close->button.base.id = UI_ID_CLOSE;
    ui_bitmap_button_set_tool_tip(close, "Alt+P");
    ui_window_add_child(wnd, &close->button.base);

    if (self->layout == PARTY_LAYOUT_CREATE)
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->name);
}

void ui_party_setting_wnd_on_draw(struct UIWindow *window)
{
    struct UIPartySettingWnd *self = (struct UIPartySettingWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int title = self->layout == PARTY_LAYOUT_CREATE ? MSG_PARTY_CREATE_TITLE : MSG_PARTY_SETTING_TITLE;
    int top;

    top = ui_window_draw_title_bar(wnd, 1, msg_str(title), 0);
    ui_window_draw_body(wnd, top, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    if (self->layout == PARTY_LAYOUT_FULL)
        ui_window_text_out(wnd, 0xa, 0x16, msg_str(MSG_PARTY_SETTING_TEXT), 0, 0, 0xc, 0, 0, 0);
    else if (self->layout == PARTY_LAYOUT_CREATE)
        ui_window_text_out(wnd, 0xa, 0x16, msg_str(MSG_PARTY_CREATE_TEXT), 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 0xa, self->layout + 0x52, msg_str(MSG_PARTY_OPTION2_TEXT), 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 0xa, self->layout + 0x11, msg_str(MSG_PARTY_OPTION1_TEXT), 0, 0, 0xc, 0, 0, 0);
}

void ui_party_setting_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIPartySettingWnd *self = (struct UIPartySettingWnd *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

void ui_party_setting_wnd_store_info(struct UIWindow *window)
{
    struct UIPartySettingWnd *self = (struct UIPartySettingWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
}

#define MSG_PARTY_INVITE_TITLE 0x808
#define MSG_PARTY_INVITE_TEXT 0x80a

struct UIPartyInvitationToWnd {
    struct UIPartySettingWnd setting;
    struct UIWindow *name;
};
void ui_party_invitation_to_wnd_destroy(struct UIWindow *window);
void ui_party_invitation_to_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_party_invitation_to_wnd_on_draw(struct UIWindow *self);
intptr_t ui_party_invitation_to_wnd_send_msg(struct UIWindow *frame, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_party_invitation_to_wnd_ops = {
    .destroy = ui_party_invitation_to_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_party_setting_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_party_invitation_to_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_party_invitation_to_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_party_setting_wnd_on_l_btn_down,
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
    .send_msg = ui_party_invitation_to_wnd_send_msg,
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

struct UIPartyInvitationToWnd *ui_party_invitation_to_wnd_construct(struct UIPartyInvitationToWnd *self)
{
    ui_party_setting_wnd_construct(&self->setting);
    self->setting.frame.base.ops = &ui_party_invitation_to_wnd_ops;
    return self;
}

struct UIWindow *ui_party_invitation_to_wnd_new(void)
{
    return (struct UIWindow *)ui_party_invitation_to_wnd_construct(mem_alloc(sizeof(struct UIPartyInvitationToWnd)));
}

void ui_party_invitation_to_wnd_destroy(struct UIWindow *window)
{
    struct UIPartyInvitationToWnd *self = (struct UIPartyInvitationToWnd *)window;
    self->setting.frame.base.ops = &ui_party_invitation_to_wnd_ops;
    ui_party_setting_wnd_destruct(&self->setting);
    mem_free(self);
}

void ui_party_invitation_to_wnd_on_create(struct UIWindow *window, int width_unused,
                                                int height_unused)
{
    struct UIPartyInvitationToWnd *self = (struct UIPartyInvitationToWnd *)window;
    struct UIWindow *wnd = &self->setting.frame.base;
    struct UIWindow *name = mem_alloc(sizeof(struct UIEditCtrl));

    if (name)
        name = ui_edit_ctrl_construct(name);
    self->name = name;
    ui_window_create(name, 0x78, 0x14);
    ui_window_move(self->name, 0x14, 0x2a);
    UI_EDIT_MAX_CHARS(self->name) = 0x17;
    ui_edit_ctrl_set_back_colour(self->name, 0xe6, 0xe6, 0xe6);
    ui_window_add_child(wnd, self->name);

    ui_add_bitmap_button(wnd, UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp",
                         wnd->width - 0x5c, wnd->height - 0x18, UI_ID_OK);
    ui_add_bitmap_button(wnd, UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp", UI_SKIN "btn_cancel_b.bmp",
                         wnd->width - 0x2e, wnd->height - 0x18, UI_ID_CANCEL);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->name);
}

void ui_party_invitation_to_wnd_on_draw(struct UIWindow *window)
{
    struct UIPartyInvitationToWnd *self = (struct UIPartyInvitationToWnd *)window;
    struct UIWindow *wnd = &self->setting.frame.base;
    int top = ui_window_draw_title_bar(wnd, 1, msg_str(MSG_PARTY_INVITE_TITLE), 0);

    ui_window_draw_body(wnd, top, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_text_out(wnd, 0xa, 0x16, msg_str(MSG_PARTY_INVITE_TEXT), 0, 0, 0xc, 0, 0, 0);
}

#define PARTY_LAYOUT_SHORT 5
#define PARTY_LAYOUT_CREATE 0x32
#define PARTY_SETTING_WND 0x35
#define PARTY_RADIO_ID 0xcf
#define PARTY_INVITE_WND 0xd0
#define UI_MSG_SET_STATE 0xd
#define UI_MSG_REFRESH 0x3c
#define UI_MSG_BUTTON 6

#define MODE_MSG_PARTY_INVITE 0x3b
#define MODE_MSG_PARTY_OPTIONS 0xa8
#define MODE_MSG_PARTY_CHANGE 0x103
#define MODE_MSG_PARTY_CREATE 0x11d

#define MSG_PARTY_NAME_BAD_CHAR 0xb17
#define MSG_PARTY_NAME_FORBIDDEN 0xafc
#define MSG_PARTY_NAME_EMPTY 0x819
#define MSG_PARTY_INVITE_EMPTY 0x818
#define MSG_PARTY_NOT_LEADER 0x126

static void radio_pick(struct UIWindow *self, struct UIWindow *sender, struct UIWindow *pair[2], int *option)
{
    int i;

    if (sender != pair[0] && sender != pair[1])
        return;
    for (i = 0; i < 2; i++) {
        if (sender == pair[i]) {
            *option = i;
            ui_window_send_msg(pair[i], self, UI_MSG_SET_STATE, 1, 0, 0, 0);
        } else {
            ui_window_send_msg(pair[i], self, UI_MSG_SET_STATE, 0, 0, 0, 0);
        }
    }
}

static void party_setting_ok(struct UIPartySettingWnd *self)
{
    struct Mode *mode;

    if (self->layout != PARTY_LAYOUT_CREATE)
        ui_window_mgr_post_quit1(&g_windowMgr, PARTY_SETTING_WND);
    mode = game_current_base_mode();
    if (session_get_party_member_count(&g_session1)) {
        if (g_session1.partyLeader
            && (self->option0 != g_session1.partyExpOption || self->option1 != g_session1.partyItemPickupRule
                || self->option2 != g_session1.partyItemDivisionRule))
            mode_send_message(mode, MODE_MSG_PARTY_CHANGE, self->option0, self->option1, self->option2, 0);
        return;
    }
    if (self->layout == PARTY_LAYOUT_CREATE) {
        struct String name;

        string_init(&name);
        string_assign_cstr(&name, ui_edit_ctrl_get_text(self->name));
        if (string_find(&name, "#", 0) != STRING_NPOS) {
            ui_chat_line(msg_str(MSG_PARTY_NAME_BAD_CHAR), 0xff);
        } else if (text_view_find(string_view(&name), string_view(&text_tag_ITEML), 0) != STRING_NPOS || text_view_find(string_view(&name), string_view(&text_tag_ITEML_END), 0) != STRING_NPOS) {
            ui_chat_line(msg_str(MSG_PARTY_NAME_FORBIDDEN), 0xff);
        } else if (!strlen(string_cstr(&name))) {
            ui_chat_line(msg_str(MSG_PARTY_NAME_EMPTY), 0xff);
        } else {
            mode_send_message(mode, MODE_MSG_PARTY_CREATE, (intptr_t)string_cstr(&name), 0, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, PARTY_SETTING_WND);
            ui_window_mgr_post_quit1(&g_windowMgr, PARTY_RADIO_ID);
            mode_send_message(mode, MODE_MSG_PARTY_OPTIONS, 0, self->option1, self->option2, 0);
        }
        string_free(&name);
        return;
    }
    mode_send_message(mode, MODE_MSG_PARTY_OPTIONS, 0, self->option1, self->option2, 0);
}

intptr_t ui_party_setting_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                         intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIFrameWnd *frame = (struct UIFrameWnd *)window;
    struct UIPartySettingWnd *wnd = (struct UIPartySettingWnd *)frame;
    struct UIWindow *self = &frame->base;
    int i;

    if (msg == UI_MSG_REFRESH) {
        if (wnd->layout == PARTY_LAYOUT_SHORT) {
            ui_window_mgr_post_quit1(&g_windowMgr, PARTY_SETTING_WND);
            return 0;
        }
        ui_window_invalidate(self);
        wnd->option0 = g_session1.partyExpOption;
        wnd->option1 = g_session1.partyItemPickupRule;
        wnd->option2 = g_session1.partyItemDivisionRule;
        for (i = 0; i < 2; i++) {
            struct UIWindow *r1, *r2;

            ui_window_send_msg(wnd->option0_radio[i], self, UI_MSG_SET_STATE, wnd->option0 == i, 0, 0, 0);
            r1 = wnd->option1_radio[i];
            r2 = wnd->option2_radio[i];
            if (!r1 || !r2)
                continue;
            ui_window_send_msg(r1, self, UI_MSG_SET_STATE, wnd->option1 == i, 0, 0, 0);
            ui_window_send_msg(r2, self, UI_MSG_SET_STATE, wnd->option2 == i, 0, 0, 0);
        }
        return 0;
    }
    if (msg == UI_MSG_SET_INFO) {
        struct WindowInfo *info = (struct WindowInfo *)val1;

        wnd->info = info;
        if (!party_setting_wnd_info_is_valid(info))
            party_setting_wnd_info_make_default(wnd->info);
        info = wnd->info;
        ui_window_move(self, info->x, info->y);
        return 0;
    }
    if (msg != UI_MSG_BUTTON)
        return ui_frame_wnd_send_msg((struct UIWindow *)(frame), sender, msg, val1, val2, val3, val4);

    switch (val1) {
    case UI_ID_OK:
        party_setting_ok(wnd);
        break;
    case UI_ID_CANCEL:
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, PARTY_SETTING_WND);
        ui_window_mgr_post_quit1(&g_windowMgr, PARTY_RADIO_ID);
        break;
    case PARTY_RADIO_ID:
        if (session_get_party_member_count(&g_session1) && !g_session1.partyLeader) {
            ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(MSG_PARTY_NOT_LEADER), 0xffff64,
                                  9, 0);
            break;
        }
        radio_pick(self, sender, wnd->option0_radio, &wnd->option0);
        radio_pick(self, sender, wnd->option1_radio, &wnd->option1);
        radio_pick(self, sender, wnd->option2_radio, &wnd->option2);
        break;
    }
    return 0;
}

intptr_t ui_party_invitation_to_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                              int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIFrameWnd *frame = (struct UIFrameWnd *)window;
    struct UIPartyInvitationToWnd *wnd = (struct UIPartyInvitationToWnd *)frame;

    if (msg != UI_MSG_BUTTON)
        return ui_frame_wnd_send_msg((struct UIWindow *)(frame), sender, msg, val1, val2, val3, val4);
    if (val1 == UI_ID_OK) {
        struct String name;
        const char *text;

        string_init(&name);
        text = ui_edit_ctrl_get_text(wnd->name);
        string_assign_cstr(&name, text);
        ui_edit_ctrl_select_all(wnd->name);
        if (strlen(string_cstr(&name))) {
            game_send_message(MODE_MSG_PARTY_INVITE, (intptr_t)string_cstr(&name), 0, 0, 0);
        } else {
            ui_chat_line(msg_str(MSG_PARTY_INVITE_EMPTY), 0xff);
        }
        string_free(&name);
    } else if (val1 == UI_ID_CANCEL) {
        ui_window_mgr_post_quit1(&g_windowMgr, PARTY_INVITE_WND);
    }
    return 0;
}
