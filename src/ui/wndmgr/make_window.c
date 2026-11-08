#include "ui/character/select_char_new.h"
#include "render/renderer.h"
#include "ui/chat/captcha.h"
#include "contents/client_lua_calls.h"
#include "replay/replay.h"
#include "session/state.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/wndmgr/make_window_internal.h"
#include "core/object_type_tables.h"
#include "session/session_options.h"
#include "ui/admin/admin.h"
#include "ui/character/char_preview.h"
#include "ui/character/character.h"
#include "ui/character/replay_play.h"
#include "ui/character/sel_char_userver.h"
#include "ui/character/select_server.h"
#include "ui/chat/chat.h"
#include "ui/hotkeys/hotkey_bars.h"
#include "ui/quests/quests.h"
#include "ui/shops/shops.h"
#include "ui/social/entry_queue_standby_internal.h"
#include "ui/social/entry_queue_windows.h"

void wm_move(struct UIWindow *wnd, int x, int y)
{
    ui_window_move(wnd, x, y);
}

void wm_set_info(struct UIWindow *wnd, void *info)
{
    ui_window_send_msg(wnd, NULL, UI_MSG_SET_INFO, (int)(INT_PTR)info, 0, 0, 0);
}

void wm_show(struct UIWindow *wnd)
{
    ui_window_set_show(wnd, 1);
}

int wm_center_x(int width)
{
    return (g_renderer->width - width) / 2;
}
int wm_center_y(int height)
{
    return (g_renderer->height - height) / 2;
}
int wm_half_x(int width)
{
    return (int)((double)(unsigned)(g_renderer->width - width) * 0.5);
}
int wm_half_y(int height)
{
    return (int)((double)(unsigned)(g_renderer->height - height) * 0.5);
}

static void wm_move_login(struct UIWindow *wnd)
{
    int y = scale_y_from480(0x12c);

    wm_move(wnd, ui_center_x640(0xb9), y);
}
void wm_move_640(struct UIWindow *wnd, int x, int y480)
{
    int y = scale_y_from480(y480);

    wm_move(wnd, ui_center_x640(x), y);
}
void wm_move_640_up(struct UIWindow *wnd, int x, int y480, int up)
{
    int y = scale_y_from480(y480) - up;

    wm_move(wnd, ui_center_x640(x), y);
}
void wm_move_center(struct UIWindow *wnd, int x, int y480)
{
    int y = ui_center_y480(y480);

    wm_move(wnd, ui_center_x640(x), y);
}

struct UIWindow *wm_register(struct UIWindowMgr *self, int window_id, struct UIWindow *wnd)
{
    struct WmWindowEntry *entry;
    struct UIWindow *top;

    if (wnd == NULL)
        return NULL;
    wnd->id = window_id;
    entry = tree_get(&self->windows_by_id, &window_id, sizeof window_id, sizeof *entry, tree_less_int);
    entry->wnd = wnd;
    for (top = wnd; top->parent; top = top->parent)
        ;
    self->active_top_wnd = top;
    return wnd;
}

static struct UIWindow *make_select_char(struct UIWindowMgr *self)
{
    static const char bg[] = UI_SKIN "select_character_ver3\\bg_back1.tga";
    struct UIWindow *wnd, *backdrop;
    struct BitmapRes *res;

    wnd = ui_new_select_char_wnd_create();
    self->new_select_char_wnd = wnd;
    backdrop = (struct UIWindow *)uirp_image_wnd_new(bg, 5);
    res = ui_skin_bitmap(bg);
    if (backdrop && res) {
        ui_window_create(backdrop, res->width, res->height);
        wm_move(backdrop, wm_center_x(backdrop->width), wm_center_y(backdrop->height));
        ui_window_mgr_add_window(self, backdrop);
        ui_window_create(wnd, backdrop->width, backdrop->height);
        wm_move(wnd, wm_center_x(backdrop->width), wm_center_y(backdrop->height));
        ui_window_mgr_add_window(self, wnd);
    }
    own_states_clear();
    skill_cooldowns_clear(&g_session1.skillDelays);
    if (g_session1.pinState == 1) {
        ui_new_select_char_wnd_show_button((struct UINewSelectCharWnd *)self->new_select_char_wnd, 0x19a);
    } else {
        ui_new_select_char_wnd_hide_button((struct UINewSelectCharWnd *)self->new_select_char_wnd, 0x19a);
        ui_new_select_char_wnd_hide_button((struct UINewSelectCharWnd *)self->new_select_char_wnd, 0x198);
        ui_new_select_char_wnd_hide_button((struct UINewSelectCharWnd *)self->new_select_char_wnd, 0x199);
    }
    return wnd;
}

static struct UIWindow *make_make_char(struct UIWindowMgr *self)
{
    struct UIWindow *wnd, *backdrop;

    wnd = ui_new_make_char_wnd_new();
    backdrop = (struct UIWindow *)uirp_image_wnd_new(UI_SKIN "make_character_ver2\\bg_back2.tga", 1);
    ui_window_create(backdrop, 0x404, 0x300);
    wm_move(backdrop, wm_center_x(0x404) - 2, wm_center_y(0x300));
    ui_window_mgr_add_window(self, backdrop);
    ui_window_create(wnd, 0x31a, 0x1a6);
    wm_move(wnd, wm_center_x(0x31a), wm_center_y(0x1a6) - 0x46);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

static void wm_place(struct UIWindow *wnd, int at, int x, int y)
{
    switch (at) {
    case WM_AT_XY:
        wm_move(wnd, x, y);
        break;
    case WM_AT_640:
        wm_move_640(wnd, x, y);
        break;
    case WM_AT_CENTER:
        wm_move_center(wnd, x, y);
        break;
    case WM_AT_640_UP:
        wm_move_640_up(wnd, x, y, 0x3c);
        break;
    }
}

struct UIWindow *wm_open(struct UIWindowMgr *self, struct UIWindow **slot, WmNew new_window, WmNew1 new_window1,
                         int arg, int width, int height, int at, int x, int y, void *info)
{
    struct UIWindow *wnd = *slot;

    if (wnd == NULL) {
        wnd = new_window ? new_window() : new_window1(arg);
        *slot = wnd;
        ui_window_create(wnd, width, height);
        wm_place(wnd, at, x, y);
        ui_window_mgr_add_window(self, wnd);
        if (info)
            wm_set_info(wnd, info);
    }
    wm_show(wnd);
    return wnd;
}
int wm_times_half(int v)
{
    return (int)((double)v * 0.5);
}

static struct UIWindow *make_second_password(struct UIWindowMgr *self, struct UIWindow **slot,
                                             const struct UIWindowOps *ops)
{
    int room_x = g_renderer->width - 0x21c, room_y = g_renderer->height - 0x16d;
    int half_x = room_x / 2, half_y = room_y / 2;
    struct UIWindow *wnd = *slot;

    if (wnd == NULL) {
        int x, y;

        wnd = ui_second_password_input_wnd_new();
        if (wnd && ops)
            wnd->ops = ops;
        *slot = wnd;
        ui_window_create(wnd, 0x21c, 0x16d);
        for (;;) {
            x = rand() % room_x;
            y = rand() % room_y;
            if (x > half_x && x < room_x && y > half_y && y < room_y)
                break;
            rand();
            rand();
        }
        wm_move(wnd, x, y);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    ui_second_password_wnd_setup((struct UISecondPasswordInputWnd *)*slot, g_session1.pinResultValue);
    return wnd;
}
void wm_move_on_centre(struct UIWindow *wnd)
{
    wm_move(wnd, g_renderer->centreX - wnd->width / 2, g_renderer->centreY - wnd->height / 2);
}

static struct UIWindow *make_lua_window(struct UIWindowMgr *self, int window_id)
{
    int width = 0, height = 0, x = 0, y = 0;
    int at_x, at_y;
    struct WindowInfo *closed;
    struct String name_x, name_y;
    struct UIWindow *wnd;

    if (!client_lua_call(g_session1.luaState, "GetWindowInfo", "d>dddd", window_id, &width, &height, &x, &y))
        return NULL;
    x = g_renderer->width * x / 1024;
    y = g_renderer->height * y / 768;
    wnd = ui_window_mgr_get_window(self, window_id);
    string_init(&name_y);
    string_init(&name_x);
    if (wnd)
        goto done;
    switch (window_id) {
    case 0x9c:
        wnd = ui_hot_key_wnd_new();
        self->hot_key_wnd = wnd;
        break;
    case 0x9d:
        wnd = ui_entry_queue_wnd_new();
        break;
    case 0x9f:
        wnd = ui_quik_slot_wnd_new();
        break;
    case 0xb5:
        wnd = ui_serach_store_info_wnd_new();
        g_session1.storeSearchOpenOnEnter = 1;
        break;
    case 0xb6:
        wnd = ui_buttons_of_basic_wnd_new();
        break;
    case 0xd1:
        wnd = ui_entry_queue_stand_by_wnd_new();
        break;
    }
    closed = ui_window_mgr_get_closed_rect(self, window_id);
    if (closed) {
        at_x = closed->x;
        at_y = closed->y;
    } else {
        at_x = session_get_option_int(&g_session1, string_cstr(&name_x), x);
        at_y = session_get_option_int(&g_session1, string_cstr(&name_y), y);
    }
    wnd->id = window_id;
    ui_window_create(wnd, width, height);
    if (at_x >= 0 && at_x <= g_renderer->width - wnd->width && at_y >= 0 &&
        at_y <= g_renderer->height - wnd->height) {
        x = at_x;
        y = at_y;
    }
    wm_move(wnd, x, y);
    ui_window_mgr_add_window(self, wnd);
    if (window_id == 0x9f)
        wm_set_info(wnd, &self->quick_slot_info);
    else if (window_id == 0x9d)
        wm_set_info(wnd, &self->battlefield_info);
    else if (window_id == 0xd4)
        wm_set_info(wnd, &self->guild_helper_info);
done:
    string_free(&name_x);
    string_free(&name_y);
    return wnd;
}

static struct UIWindow *make_window_by_id(struct UIWindowMgr *self, int window_id);

struct UIWindow *ui_window_mgr_make_window(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd;
    int tracked;

    if (ui_window_mgr_is_dragging_member_plate(self) && ui_window_mgr_is_hot_key_of_window(self))
        ui_window_mgr_close_drag_plate(self);
    tracked = !ui_window_mgr_is_untracked_id(self, window_id);
    if (tracked) {
        ui_window_mgr_forget_window_id(self, window_id);
        list_insert(&self->id_list, window_id == 0xcb ? self->id_list.head : self->id_list.head->next, &window_id,
                    sizeof window_id);
    }
    wnd = make_window_by_id(self, window_id);
    if (wnd == NULL && tracked && ui_window_mgr_get_window(self, window_id) == NULL)
        ui_window_mgr_forget_window_id(self, window_id);
    return wnd;
}

static struct UIWindow *make_window_by_id(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    switch (window_id) {
    case 0x2:
        wnd = ui_select_server_wnd_new();
        ui_window_create(wnd, 0x118, 0xc8);
        {
            int y = scale_y_from480(0x12c) - 0x50;

            wm_move(wnd, ui_center_x640(0xb9), y);
        }
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x145:
        if (ui_window_mgr_get_window(self, 0x145)) {
            wnd = ui_window_mgr_get_window(self, 0x145);
            break;
        }
        wnd = uirp_image_wnd2_new();
        self->rpimage_wnd2 = wnd;
        ui_window_create(wnd, 0x12d, 0x84);
        wm_move_login(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x3:
        wnd = ui_login_wnd_new();
        self->login_wnd = wnd;
        ui_window_create(wnd, 0x12d, 0x84);
        wm_move_login(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0xb4:
        wnd = ui_login_otp_wnd_new();
        self->login_otpwnd = wnd;
        ui_window_create(wnd, 0x118, 0x8e);
        wm_move_login(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x5d:
        wnd = ui_notice_wnd_new();
        ui_window_create(wnd, 0x258, 0x190);
        {
            int y = ui_center_y480(0x28);

            wm_move(wnd, ui_center_x640(0x21), y);
        }
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x4:
        wnd = ui_make_char_wnd_new();
        ui_window_create(wnd, 0x96, 0x11e);
        wm_move(wnd, wm_center_x(0x96), wm_center_y(0x11e));
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x115:
        wnd = make_select_char(self);
        break;
    case 0x116:
        wnd = make_make_char(self);
        break;
    case 0x6:
        wnd = ui_wait_wnd_new();
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_login(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x7:
        wnd = self->loading_wnd;
        if (wnd == NULL) {
            wnd = ui_loading_wnd_new();
            self->loading_wnd = wnd;
            ui_window_create(wnd, 0xf0, 0xf);
            wm_move(wnd, g_renderer->width / 2 - 0x64, g_renderer->height / 2 + 0xa0);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x9:
        wnd = ui_tool_tip_wnd_new();
        ui_window_create(wnd, 0xc8, 0x78);
        wm_move_login(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x13:
        wnd = WM_OPEN(restart_wnd, ui_restart_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c);
        break;
    case 0x30:
        wnd = WM_OPEN(quit_wnd, ui_quit_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c);
        break;
    case 0x14:
        wnd = ui_notice_confirm_wnd_new();
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_login(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x14c:
        wnd = ui_disconnected_server_msg_wnd_new();
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_login(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0xba:
        wnd = ui_select_replay_data_wnd_new();
        ui_window_create(wnd, 0x118, 0x137);
        {
            int y = ui_center_y480(0xf0) - 0xa0;

            wm_move(wnd, ui_center_x640(0x140) - 0x8c, y);
        }
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0xbb:
        if (self->replay_control_wnd)
            return NULL;
        wnd = ui_replay_control_wnd_new();
        self->replay_control_wnd = wnd;
        ui_window_create(wnd, 0xc5, 0x8a);
        {
            int y = scale_y_from480(0x1e0) - 0x8a;

            wm_move(wnd, ui_scale_x640(0x280) - 0xc5, y);
        }
        wnd->list_rank = 0;
        wm_window_list_sort(&self->windows);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0xc6:
        if (self->replay_reccontrol_wnd)
            return NULL;
        wnd = ui_replay_rec_control_wnd_new(replay_recorder_instance()->state == WM_REPLAY_RECORDING);
        self->replay_reccontrol_wnd = wnd;
        ui_window_create(wnd, 0xc2, 0x22);
        wm_move_center(wnd, 0x103, 0xe6);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->replay_reccontrol_wnd, &self->replay_rec_control_info);
        break;
    case 0x7f:
    {
        int width = g_session1.charCount > 0xc ? 0x30f : 0x21c;

        wnd = ui_sel_char_for_u_server_wnd_new();
        self->sel_char_for_userver_wnd = wnd;
        ui_window_create(wnd, width, 0x190);
        wm_move(wnd, wm_center_x(width), wm_center_y(0x190));
        ui_window_mgr_add_window(self, wnd);
        break;
    }
    case 0x80:
        wnd = WM_OPEN(change_name_wnd, ui_change_name_wnd_new, 0xb8, 0x69, WM_AT_CENTER, 0x104, 0xb4);
        break;
    case 0x81:
        wnd = WM_OPEN(rebirth_wnd, ui_rebirth_wnd_new, 0x118, 0x8f, WM_AT_640, 0xb9, 0x12c);
        break;
    case 0x9c: case 0x9e: case 0x9f: case 0xa5: case 0xa6: case 0xa7: case 0xb5: case 0xb6: case 0xb7:
    case 0xbf:
        wnd = make_lua_window(self, window_id);
        if (wnd == NULL)
            return NULL;
        wm_show(wnd);
        break;
    case 0xc7:
        wnd = make_second_password(self, &self->second_password_input_wnd, NULL);
        ui_second_password_wnd_set_mode((struct UISecondPasswordInputWnd *)self->second_password_input_wnd, 0);
        break;
    case 0xc8:
    case 0x167:
        wnd = make_second_password(self, &self->second_password_edit_wnd, &ui_second_password_edit_wnd_ops);
        ui_second_password_wnd_set_mode((struct UISecondPasswordInputWnd *)self->second_password_edit_wnd, 1);
        break;
    case 0x11c:
        wnd = self->captcha_register_wnd;
        if (wnd == NULL) {
            wnd = captcha_register_create();
            if (!wnd) break;
            self->captcha_register_wnd = wnd;
            wm_move(wnd, wm_center_x(0x122), wm_center_y(0xd3));
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x11d:
        wnd = self->macro_detector_wnd;
        if (wnd == NULL) {
            wnd = captcha_detector_create();
            if (!wnd) break;
            self->macro_detector_wnd = wnd;
            wm_move(wnd, wm_center_x(0x154), wm_center_y(0x138));
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        self->macro_detector_edit = captcha_detector_edit(self->macro_detector_wnd);
        break;
    case 0x11e:
        wnd = self->macro_reporter_wnd;
        if (wnd == NULL) {
            wnd = ui_macro_reporter_wnd_new();
            self->macro_reporter_wnd = wnd;
            ui_window_create(wnd, 0xd1, 0x134);
            wm_move(wnd, g_renderer->width - 0x103, wm_center_y(0x134));
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        wm_set_info(wnd, &self->macro_reporter_info);
        self->macro_reporter_info.open = 1;
        break;
    case 0x11f:
        wnd = self->macro_user_info_wnd;
        if (wnd == NULL) {
            wnd = ui_macro_user_info_wnd_new();
            self->macro_user_info_wnd = wnd;
            ui_window_create(wnd, 0xc8, 0x25);
            wm_move(wnd, wm_center_x(0xc8), wm_center_y(0x25));
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x120:
        wnd = self->captcha_preview_wnd;
        if (wnd == NULL) {
            wnd = captcha_preview_create();
            if (!wnd) break;
            self->captcha_preview_wnd = wnd;
            wm_move(wnd, wm_center_x(0xdc), wm_center_y(0x5a));
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x154:
        wnd = ui_window_mgr_get_window(self, 0x154);
        if (wnd)
            break;
        wnd = ui_account_limited_tool_wnd_new();
        ui_window_create(wnd, 0x186, 0x15e);
        wm_move_on_centre(wnd);
        wm_show(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    default:
        wnd = wm_make_character(self, window_id);
        if (wnd == WM_NOT_MINE)
            wnd = wm_make_hud(self, window_id);
        if (wnd == WM_NOT_MINE)
            wnd = wm_make_trade(self, window_id);
        if (wnd == WM_NOT_MINE)
            wnd = wm_make_social(self, window_id);
        return wnd == WM_NOT_MINE ? NULL : wnd;
    }
    return wm_register(self, window_id, wnd);
}

int scale_y_from480(int n)
{
    return n * g_renderer->height / 480;
}
