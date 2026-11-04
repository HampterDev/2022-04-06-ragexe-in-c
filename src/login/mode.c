#include "contents/adventurer_agency.h"
#include "world/mode_actions.h"
#include "ui/window_types.h"
#include "world/tooltip.h"
#include "render/device.h"
#include "contents/sound.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"
#include "login/lifecycle.h"

#include <string.h>
#include <stdlib.h>
#include "render/device.h"
#include "core/app.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "session/session_items.h"
#include "session/session_options.h"
#include "session/session_state.h"
#include "ui/render/render.h"
#include "ui/window_mgr.h"
#include "ui/wndmgr/wndmgr.h"
#include "game/mode_initialize.h"
#include "platform/clock.h"

const char *g_loginBackgroundName = "";

static struct LoginMode *login_mode_construct(struct LoginMode *self)
{
    mode_construct(&self->base);
    self->base.ops = &g_CLoginMode_ops;
    string_init(&self->wallpaper);
    self->messageIds.first = self->messageIds.last = self->messageIds.end = NULL;
    self->connections.first = self->connections.last = self->connections.end = NULL;
    string_init(&self->exit_cleared_text);
    self->zones.first = self->zones.last = self->zones.end = NULL;
    self->base.fade_in_start = 1;
    self->nextKeepAlive = platform_ticks_ms() + 12000;
    self->connectionIndex = -1;
    self->waitWnd = NULL;
    memset(self->chars, 0, sizeof self->chars);
    self->chars_received = 0;
    self->char_list_pages = 0;
    self->char_deleted = 0;
    return self;
}

struct Mode *login_mode_create(void)
{
    struct LoginMode *mode = malloc(sizeof *mode);
    return mode ? &login_mode_construct(mode)->base : NULL;
}

static void login_mode_destruct(struct LoginMode *self)
{
    self->base.ops = &g_CLoginMode_ops;
    vec_free(&self->zones);
    string_free(&self->exit_cleared_text);
    login_conn_entries_free(&self->connections);
    vec_free(&self->messageIds);
    string_free(&self->wallpaper);
}

void login_mode_destroy(struct Mode *mode)
{
    struct LoginMode *self = (struct LoginMode *)mode;
    login_mode_destruct(self);
    free(self);
}

void login_mode_on_init(struct Mode *mode, const char *name)
{
    struct LoginMode *self = (struct LoginMode *)mode;
    int start;

    sound_stop_stream();
    self->new_char.stats[0] = 5;
    self->new_char.stats[1] = 5;
    self->new_char.stats[2] = 5;
    self->new_char.stats[3] = 5;
    self->new_char.stats[4] = 5;
    self->new_char.stats[5] = 5;
    self->new_char.head_palette = 0;
    self->new_char.head = 1;
    self->new_char_name[0] = 0;
    string_assign_cstr(&self->wallpaper, g_loginBackgroundName);
    self->base.loop_cond = 1;
    self->base.next_sub_mode = -1;
    memset(self->email, 0, sizeof self->email);

    start = g_loginStartMode;
    self->base.sub_mode = start;
    self->base.sub_mode_count = 0;
    self->base.next_sub_mode = -1;
    login_mode_on_change_state(&self->base, start);
    g_loginStartMode = 3;
    g_isStopByLogin = 1;

    session_clear_character_lists(&g_session1);
    guild_info_reset(&g_guildInfo);
    adventurer_agency_reset(g_adventurerAgency);
    session_pre_load_always_needed_res(&g_session1);
    if (g_sound_settings.bgm_on)
        sound_play_stream("bgm\\01.mp3", 0);

    session_clear_hotkey_tab0(&g_session1);
    session_clear_hotkey_tab1(&g_session1);
    (g_session1.ownDressUp) = 0;
    self->numServer = 0;
    self->numChar = 0;
    self->auth_answered = 0;
    self->refuse_seconds_left = 0;
}

void login_mode_on_exit(struct Mode *mode)
{
    struct LoginMode *self = (struct LoginMode *)mode;
    sound_end_samples();
    res_mgr_purge_unlocked(g_resMgr());
    sprite_atlas_clear(g_renderer);
    string_clear(&self->exit_cleared_text);
}

int login_mode_on_run(struct Mode *mode)
{
    struct LoginMode *self = (struct LoginMode *)mode;
    struct Mode *m = &self->base;
    int next;

    while (m->loop_cond) {
        if (g_quitRequested)
            break;
        if (!g_windowMgr.modal_wnd && (next = m->next_sub_mode) != -1) {
            m->sub_mode = next;
            m->sub_mode_count = 0;
            m->next_sub_mode = -1;
            login_mode_on_change_state(m, next);
        }

        login_mode_on_update(m);
        m->sub_mode_count++;
    }
    mode_run_fade_out(m, 0);
    return 1;
}

void login_mode_on_update(struct Mode *mode)
{
    struct LoginMode *self = (struct LoginMode *)mode;
    check_system_message();
    login_mode_process_packets(self);
    login_mode_send_keep_alive(self);
    mouse_read_state(&g_mouse);
    ui_window_mgr_process_input(&g_windowMgr);
    mode_nothing(&self->base);
    if (self->base.loop_cond) {
        login_mode_check_timeouts(self, self->base.sub_mode);
        mode_process_help_balloon(&self->base);
    }
    time_scheduler_tick(g_timeScheduler);

    if (g_activatedNow) {
        g_activatedNow = 0;
        c3d_device_restore_surfaces(&g_3dDevice);
        ui_window_mgr_invalidate_all(&g_windowMgr);
        ui_window_mgr_set_backdrop(&g_windowMgr,
                   res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->wallpaper))));
    }

    if (!g_windowActive && g_3dDevice.fullscreen)
        return;

    renderer_clear_frame(g_renderer, 1);
    ui_window_mgr_on_process(&g_windowMgr);
    ui_window_mgr_draw_login_screen(&g_windowMgr);
    ui_window_mgr_render_windows(&g_windowMgr);
    mode_draw_mouse_cursor(&self->base);
    mode_process_fade_in(&self->base);
    if ((char)renderer_draw_scene(g_renderer))
        ui_window_mgr_draw_serial_number(&g_windowMgr);
    renderer_flip(g_renderer, 1);
    mode_screen_shot(&self->base);
}

static int timeout_box(int msgId)
{
    return ui_window_mgr_message_box(&g_windowMgr, msg_str(msgId), 0, 1, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
}

void login_mode_check_timeouts(struct LoginMode *self, int subMode)
{
    switch (subMode) {
    case 4:
        if (platform_ticks_ms() > self->stateTick + 15000)
            login_mode_send_msg(&self->base, 0x2725, 0, 0, 0, 0);
        return;

    case 5:
        if (platform_ticks_ms() <= self->stateTick + 80000)
            return;
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        if (timeout_box(1) == 0x105)
            return;
        self->stateTick = platform_ticks_ms();
        g_mapServerAidPending = 0;
        self->base.next_sub_mode = 3;
        return;

    case 0xc:
        if (platform_ticks_ms() <= self->stateTick + 15000)
            return;
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        if (timeout_box(1) == 0x105)
            return;
        self->stateTick = platform_ticks_ms();
        g_mapServerAidPending = 0;
        self->base.next_sub_mode = 3;
        return;

    case 6:
        if (platform_ticks_ms() <= self->stateTick + 15000)
            return;
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        if (timeout_box(1) == 0x105)
            return;
        mode_mgr_quit(&g_modeMgr);
        return;

    case 0x26:
        if (platform_ticks_ms() <= self->stateTick + 80000)
            return;
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        if (self->auth_answered != 0)
            return;
        if (timeout_box(0xd70) != 0xb0)
            return;
        self->auth_answered = 0;
        self->stateTick = platform_ticks_ms();
        self->base.next_sub_mode = 3;
        return;
    }
}

void login_mode_send_keep_alive(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct { WORD id; DWORD aid; } pkt;
#pragma pack(pop)
    int m = self->base.sub_mode;

    if (m != 7 && m != 8 && m != 0xb && m != 0x13)
        return;
    if (platform_ticks_ms() <= self->nextKeepAlive)
        return;
    self->nextKeepAlive = platform_ticks_ms() + 12000;
    pkt.id = 0x187;
    pkt.aid = g_session1.ownAid;
    login_send_packet(0x187, &pkt);
}
