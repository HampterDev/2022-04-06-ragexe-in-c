#include "login/otp.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"
#include "platform/language.h"

#include <string.h>
#include <stdio.h>
#include "core/msg_str.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "session/session_windows.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

#define LMSG_SERVICE            0x00
#define LMSG_DISCONNECT_BOX     0x01
#define LMSG_CHAR_BY_SLOT       0x08
#define LMSG_SEND_MOBILE_OTP           0x17
#define LMSG_MESSAGE_WND        0x18
#define LMSG_TO_LOGIN           0x1c
#define LMSG_SEND_OTP           0x27
#define LMSG_LAST_CHAR          0x102
#define LMSG_TO_SUBMODE_4       0x2710

struct ServerAddress g_charServerAddr;
DWORD g_charServerIp;

static int back_to_login(struct LoginMode *self)
{
    g_mapServerAidPending = 0;
    self->base.next_sub_mode = 3;
    return 0;
}

static int disconnect_to_login(struct LoginMode *self)
{
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    return back_to_login(self);
}

static int message_wnd(struct LoginMode *self, int index)
{
    void *wnd;
    int text;

    switch (((int *)self->messageIds.first)[index]) {
    case 0x2d: text = 0x1a8; break;
    case 0x2e: text = 0x1a9; break;
    case 0x2f: text = 0x157; break;
    default: return 0;
    }
    wnd = ui_window_find(0x115);
    if (!wnd)
        return 0;
    UIWND_SENDMSG(wnd, 0, 6, text, 0, 0, 0);
    return 0;
}

static int select_server(struct LoginMode *self, int server)
{
    struct {
        char ip[16];
        int port;
        char name[0x84];
    } info;
    struct LoginServerEntry *e;
    DWORD ip;

    self->selectedServer = server;
    if (server == -1)
        return 0;
    if (self->servers[server].state == 3) {
        ui_window_mgr_message_box(&g_windowMgr, msg_str(0x2fc), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
        self->base.next_sub_mode = 6;
        return 0;
    }
    e = &self->servers[server];
    string_assign_cstr(&g_session1.worldName, e->name);
    ip = self->servers[self->selectedServer].ip;
    wsprintfA(info.ip, "%d.%d.%d.%d", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, ip >> 24);
    info.port = (short)self->servers[self->selectedServer].port;
    snprintf(info.name, 0x81, "%s", (const char *)self->servers[self->selectedServer].host);
    memcpy(&g_charServerAddr, &info, sizeof info);
    g_charServerIp = self->servers[self->selectedServer].ip;
    string_assign_cstr(&g_session1.serverName,
                           self->servers[self->selectedServer].name);
    g_exchangeGiveBack = 0;
    self->base.next_sub_mode = 5;
    return 0;
}

static int select_connection(struct LoginMode *self, int connection)
{
    self->connectionIndex = connection;
    (g_session1.connectionIndex) = connection;
    select_client_info2(connection);
    if (g_serviceType == 0x13) {
        if (!language_set_folder(g_language, connection == 1 ? "uae\\" : "english\\",
                                 connection == 1 ? (WORD)connection : 9))
            return 0;
        g_windowMgr.arabic_text = connection == 1;
        session_reload_texts(&g_session1);
    }
    if (g_serviceType == 5 || g_serviceType == 7) {
        self->base.next_sub_mode = 0xd;
        return 0;
    }
    self->base.next_sub_mode = 3;
    return 0;
}

static int select_sub_connection(struct LoginMode *self, int sub_connection)
{
    struct Vec *subs = &self->subConnections;

    self->sub_connection_index = sub_connection;
    login_conn_entries_clear(subs);
    select_client_info_(self->connectionIndex, self->sub_connection_index);
    self->base.next_sub_mode = 3;
    return 0;
}

static int enter_char(struct LoginMode *self)
{
    struct CharInfo *c;
    WORD job;
    int i, n;

    (g_session1.shortcutCharSlot) = g_loginCharInfo.slot;
    ui_window_mgr_init_shortcut_item_info_from_reg(&g_windowMgr);
    n = self->numChar;
    if (n <= 0)
        return 0;
    for (i = 0; self->chars[i].slot != g_loginCharInfo.slot; i++)
        if (i + 1 >= n)
            return 0;
    c = &self->chars[i];
    (g_session1.charSlot) = c->slot;
    session_set_job(&g_session1, c->job);
    (g_session1.ownGid) = (int)c->gid;
    (g_session1.walkSpeed) = c->walk_speed;
    session_invalidate_windows(&g_session1);
    (g_session1.charBaseLevel) = c->level;
    session_invalidate_windows(&g_session1);
    (g_session1.skillPoints) = c->skill_points;
    session_invalidate_windows(&g_session1);
    (g_session1.statusPoints) = c->status_points;
    session_invalidate_windows(&g_session1);
    (g_session1.stats[0]) = c->stats[0];
    session_invalidate_windows(&g_session1);
    (g_session1.stats[1]) = c->stats[1];
    session_invalidate_windows(&g_session1);
    (g_session1.stats[2]) = c->stats[2];
    session_invalidate_windows(&g_session1);
    (g_session1.stats[3]) = c->stats[3];
    session_invalidate_windows(&g_session1);
    (g_session1.stats[4]) = c->stats[4];
    session_invalidate_windows(&g_session1);
    (g_session1.stats[5]) = c->stats[5];
    session_invalidate_windows(&g_session1);
    memcpy(&g_session1.baseExp, &c->exp, sizeof g_session1.baseExp);
    session_invalidate_windows(&g_session1);
    if ((g_session1.hp) != (int)c->hp) {
        (g_session1.hp) = (int)c->hp;
        session_refresh_basic_info(&g_session1);
    }
    if ((g_session1.maxHp) != (int)c->max_hp) {
        (g_session1.maxHp) = (int)c->max_hp;
        session_refresh_basic_info(&g_session1);
    }
    if ((g_session1.sp) != (int)c->sp) {
        (g_session1.sp) = (int)c->sp;
        session_refresh_basic_info(&g_session1);
    }
    if ((g_session1.maxSp) != (int)c->max_sp) {
        (g_session1.maxSp) = (int)c->max_sp;
        session_refresh_basic_info(&g_session1);
    }
    (g_session1.zeny) = c->zeny;
    session_refresh_basic_info(&g_session1);
    session_invalidate_windows(&g_session1);
    memcpy(&g_session1.jobExp, &c->job_exp, sizeof g_session1.jobExp);
    session_invalidate_windows(&g_session1);
    (g_session1.karma) = c->karma;
    session_invalidate_windows(&g_session1);
    (g_session1.manner) = c->manner;
    session_invalidate_windows(&g_session1);
    (g_session1.jobLevel) = c->job_level;
    session_invalidate_windows(&g_session1);
    (g_session1.char_info_hair_color) = c->hair_color;
    (g_session1.hairStyle) = c->head;
    g_session1.weapon = 0;
    (g_session1.headBottom) = c->accessory;
    (g_session1.shield) = c->shield;
    job = (WORD)c->job;
    if ((WORD)(job - 0xfa1) <= 6 && c->body_palette < 1)
        (g_session1.clothesColor) = 1;
    else
        (g_session1.clothesColor) = c->body_palette;
    (g_session1.hairColor) = c->head_palette;
    (g_session1.headTop) = c->accessory2;
    (g_session1.headMid) = c->accessory3;
    (g_session1.robe) = c->robe;
    (g_session1.body) = c->body;
    (g_session1.bodyState) = c->body_state;
    (g_session1.healthState) = c->health_state;
    (g_session1.effectState) = c->effect_state;
    g_session1.favouritesSorted = 0;
    return 0;
}

static int billing_text(struct LoginMode *self, char *out)
{
    DWORD code = self->billing.value1, t;

    switch (code) {
    case 100:
        sprintf(out, msg_str(0x215), self->billing.value5 / 60);
        return 0;
    case 200:
        t = self->billing.value5;
        sprintf(out, msg_str(0x216), t / 3600, t / 60 % 60, t % 60);
        return 0;
    case 300:
        sprintf(out, msg_str(0x217));
        return 0;
    case 400:
        t = self->billing.value9 - self->billing.value5;
        sprintf(out, msg_str(0x218), t / 3600, t / 60 % 60, t % 60);
        return 0;
    case 500:
        t = self->billing.value5;
        if (t) {
            sprintf(out, msg_str(0x219), t / 3600, t / 60 % 60, t % 60);
            return 0;
        }
        t = self->billing.value9;
        sprintf(out, msg_str(0x21a), t / 3600, t / 60 % 60, t % 60);
        return 0;
    case 600:
        sprintf(out, msg_str(0x21b));
        return 0;
    case 700:
        sprintf(out, msg_str(0x21c));
        return 0;
    case 800:
        sprintf(out, msg_str(0x21d));
        return 0;
    }
    sprintf(out, msg_str(0x64));
    return 0;
}

static int select_zone(struct LoginMode *self, int zone)
{
    struct LoginZoneEntry entry;
    struct LoginZoneEntry *zones;

    if (zone == -1)
        return 0;
    zones = (struct LoginZoneEntry *)self->zones.first;
    entry.index = zones[zone].index;
    self->zones.last = zones;
    vec_push(&self->zones, &entry, sizeof entry);
    self->base.next_sub_mode = 0x1f;
    return 0;
}

int login_mode_send_msg(struct Mode *mode, int msg, int a, int b, int c, int d)
{
    struct LoginMode *self = (struct LoginMode *)mode;
    int i;

    switch (msg) {
    case LMSG_SERVICE:
        switch (a) {
        case 0x2711: self->base.next_sub_mode = 8; return 0;
        case 0x2712: self->base.next_sub_mode = 9; return 0;
        case 0x2714: self->base.next_sub_mode = 7; return 0;
        case 0x2716:
            if (self->base.sub_mode != 7)
                return 0;
            ui_window_mgr_close_all_windows(&g_windowMgr);
            connection_disconnect(rag_connection_transport(rag_connection_instance()));
            self->base.next_sub_mode = 6;
            return 0;
        case 0x271a: self->base.next_sub_mode = 0xb; return 0;
        }
        return 0;
    case LMSG_DISCONNECT_BOX:
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(2), 0, 1, 1, 0, "메시지", 0x118, 0x78, 0) == 0x105)
            return 0;
        return back_to_login(self);
    case LMSG_CHAR_BY_SLOT:
        for (i = 0; i < self->numChar; i++)
            if (self->chars[i].slot == a)
                return (int)&self->chars[i];
        return 0;
    case LMSG_SEND_MOBILE_OTP:
        return login_send_mobile_otp(self);
    case LMSG_MESSAGE_WND:
        return message_wnd(self, a);
    case LMSG_TO_LOGIN:
        return back_to_login(self);
    case LMSG_SEND_OTP:
        return login_send_otp(self);
    case LMSG_LAST_CHAR:
        if (!self->numChar)
            return 0;
        return (int)&self->chars[self->numChar - 1];
    case LMSG_TO_SUBMODE_4:
        self->base.next_sub_mode = 4;
        return 0;

    case 0x2713: return select_server(self, a);
    case 0x2715: self->base.next_sub_mode = 0xa; return 0;
    case 0x2716:
        if (self->base.sub_mode == 6 || self->base.sub_mode == 0x1b)
            return disconnect_to_login(self);
        if (self->base.sub_mode == 0x1e)
            self->base.next_sub_mode = 7;
        return 0;
    case 0x2717: strcpy(self->password, (const char *)a); return 0;
    case 0x2718:
        strcpy(self->userId, (const char *)a);
        strcpy(g_session1.userId, (const char *)a);
        return 0;
    case 0x2719: return enter_char(self);
    case 0x271b: return disconnect_to_login(self);
    case 0x271c:
        ui_window_mgr_invalidate_all(&g_windowMgr);
        strcpy(self->email, (const char *)a);
        return 0;
    case 0x271d: return (int)self->email;
    case 0x271e: return (int)self->new_char_name;
    case 0x271f: strcpy(self->new_char_name, (const char *)a); return 0;
    case 0x2720: return (int)&self->new_char;
    case 0x2721: self->new_char = *(const struct NewCharLooks *)a; return 0;
    case 0x2722:
        self->base.next_sub_mode = 2;
        return 0;
    case 0x2723: return select_connection(self, a);
    case 0x2724: return billing_text(self, (char *)a);
    case 0x2725:
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(1), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0) == 0x105)
            return 0;
        self->stateTick = platform_ticks_ms();
        return back_to_login(self);
    case 0x2726: return select_sub_connection(self, a);
    case 0x2728: login_set_otp_code(self, (const char *)a); return 0;
    case 0x2729: return 0;

    case 0x272c: return select_zone(self, a);
    case 0x2735:
        self->base.next_sub_mode = a;
        return (int)self->email;
    case 0x2736: strcpy((char *)self->account_class, (const char *)a); return 0;
    }
    return mode_send_msg_base(&self->base, msg, a, b, c, 0);
}
