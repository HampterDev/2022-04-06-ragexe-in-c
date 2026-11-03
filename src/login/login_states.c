#include "ui/window_types.h"
#include "contents/player_names.h"
#include "session/character_name.h"
#include "web/service_managers.h"
#include "session/snapshot_values.h"
#include "replay/replay.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"
#include "std/std.h"
#include "login/zone_addr.h"
#include "login/password_encode.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ui/character/select_server.h"
#include "ui/character/replay_play.h"
#include "render/render_api.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "session/session_achievements.h"
#include "session/session_windows.h"
#include "ui/character/character.h"
#include "ui/mail/rodex_box.h"
#include "ui/net/packet_crypt.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/character/skill_list_wnd.h"
#include "game/rodex_mail.h"

static const char warning2_bmp[] = "유저인터페이스\\login_interface\\warning2.bmp";


static int box(const char *text, int a, int b, int c, int d)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, a, b, c, d, "메시지", 0x118, 0x78, 0);
}

static void show_wallpaper(struct LoginMode *self)
{
    ui_window_mgr_set_backdrop(&g_windowMgr,
                            res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->wallpaper))));
}

static void default_wallpaper(struct LoginMode *self)
{
    string_assign_cstr(&self->wallpaper, g_loginBackgroundName);
    show_wallpaper(self);
}

static void *wait_window(int msg, int a)
{
    void *w = ui_window_open(6);

    ui_wait_wnd_set_text(w, msg_str(msg));
    return w;
}

static void connect_failed(struct LoginMode *self)
{
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    if (box(msg_str(1), 1, 1, 1, 0) == 0x105)
        return;
    g_mapServerAidPending = 0;
    self->base.next_sub_mode = 3;
}

static void reset_session_items(void)
{
    session_reset_mail_items(&g_session1);
    session_clear_mail_item_counts(&g_session1);
    session_clear_inventory(&g_session1);
    session_clear_worn_items(&g_session1);
    session_clear_worn_costumes(&g_session1);
    session_clear_cart(&g_session1);
    session_clear_switch_equips(&g_session1);
    session_clear_switch_costumes(&g_session1);
}

static void session_reset_achievements(struct Session *session)
{
    tree_clear(&session->achievements, NULL);
    session->achTotalPoints = 0;
    session->achRank = 0;
    session->achCurrentPoints = 0;
    session->achNextPoints = 0;
    session->titles.last = session->titles.first;
    session->newTitles.last = session->newTitles.first;
    session->ownTitleId = 0;
    tracked_achievements_clear(&session->trackedAchievements);
}

void login_mode_show_warning(struct LoginMode *self)
{
    void *w;

    if (g_serviceType == 0)
        string_assign(&self->wallpaper, warning2_bmp, 0x2b);
    else
        string_assign(&self->wallpaper, "유저인터페이스\\login_interface\\warning.bmp", 0x2a);
    show_wallpaper(self);
    w = ui_window_open(0x14);
    UIWND_SENDMSG(w, 0, 0x50, 0x2722, 0, 0, 0);
}

void login_mode_show_license(struct LoginMode *self)
{
    void *w;
    int x, y;

    show_wallpaper(self);
    string_assign_cstr(&self->wallpaper, g_loginBackgroundName);
    w = ui_window_open(0x10);
    ui_window_resize(w, 0x190, 0x12c);
    y = (g_renderer->height - 0x1e0) / 2 + 0x5a;
    x = (g_renderer->width - 0x280) / 2 + 0x78;
    ui_window_move(w, x, y);
    UIWND_SENDMSG(w, 0, 0x55, 0xc1, (int)"btn_disagree", 0, 0);
    UIWND_SENDMSG(w, 0, 0x55, 0xd2, (int)"btn_agree", 0, 0);
    UIWND_SENDMSG(w, 0, 0x1a, (int)get_licence_text_(), 0, g_serviceType == 6 ? 0x35 : 0x3a, 0);
    UIWND_SENDMSG(w, 0, 0x50, 0x2722, 0, 0, 0);
}

static void connection_list(struct LoginMode *self, struct Vec *list, int msg)
{
    struct LoginConnEntry *e;
    struct UIWindow *wnd;
    struct Vec *balloons;

    default_wallpaper(self);
    wnd = ui_window_open(2);
    if (!wnd)
        return;
    UIWND_SENDMSG(wnd, 0, 0x50, msg, (int)"NUMACCOUNT", 0, 0);
    for (e = (struct LoginConnEntry *)list->first; e != (struct LoginConnEntry *)list->last; e++) {
        ui_list_box_add_item(ui_select_server_list(wnd), string_cstr(&e->display));
        balloons = ui_server_list_balloons(ui_select_server_list(wnd));
        string_vec_push_copy(balloons, &e->balloon);
    }
    UIWND_SENDMSG(wnd, 0, 0x28, 0, 0, 0, 0);
    UIWND_SENDMSG(wnd, 0, 0x59, 0, 0, 0, 0);
}

static void login_acf(struct LoginMode *self)
{
    struct ServerAddress addr;
#pragma pack(push, 1)
    struct {
        WORD id;
        DWORD tag;
        char userId[0x19];
        char hash[0x20];
        char code[5];
    } pkt;
#pragma pack(pop)
    char password[0x21];

    memset(&addr, 0, sizeof addr);
    _snprintf_s(addr.host, 0x81, (size_t)-1, "%s", g_accountServerAddr);
    self->base.connected = (BYTE)connection_connect(rag_connection_transport(rag_connection_instance()), &addr);
    if (!self->base.connected) {
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        if (box(msg_str(1), 1, 1, 1, 0) == 0x105)
            return;
        self->base.next_sub_mode = 3;
        return;
    }
    memset(&pkt, 0, sizeof pkt);
    pkt.tag = 0x31313030;
    pkt.id = 0xacf;
    _snprintf_s(pkt.userId, 0x19, (size_t)-1, "%s", self->userId);
    memset(password, 0, sizeof password);
    _snprintf_s(password, 0x21, (size_t)-1, "%s", self->password);
    if (!login_password_encode(LOGIN_PASSWORD_ACF, password, sizeof password, pkt.hash, sizeof pkt.hash)) {
        connect_failed(self);
        return;
    }
    _snprintf_s(pkt.code, 5, (size_t)-1, "%s", (const char *)self->account_class);
    rag_connection_send_packet(rag_connection_instance(), 0x44, &pkt);
    self->stateTick = timeGetTime();
    self->waitWnd = wait_window(0x7a, 0x10);
}

static void login_screen(struct LoginMode *self)
{
    account_user_data_reset(g_accountUserDataMgr);
    char_user_data_reset(g_charUserDataMgr);
    mcstore_reset(&g_session1.mcStoreBackup);
    guild_mgr_clear_versions(g_guildMgr);
    emblem_mgr_build_urls(g_emblemMgr);
    default_wallpaper(self);
    if (g_session1.canBackToLogin == 1) {
        self->stateTick = timeGetTime();
        g_mapServerAidPending = 0;
        self->auth_answered = 0;
        self->auth_waiting = 0;
        memset(g_session1.ssoToken, 0, 0x1f4);
    }
    if (strcmp((const char *)g_session1.ssoToken, "") == 0) {
        g_session1.canBackToLogin = 1;
        if (!ui_window_find(3))
            ui_window_open(3);
        return;
    }
    if ((g_session1.launchArgsSet) == 1) {
        (g_session1.launchArgsSet) = 0;
        self->base.next_sub_mode = 4;
        return;
    }
    mode_mgr_quit(&g_modeMgr);
}

static void server_list(struct LoginMode *self)
{
    static const char *const colours[4] = { "^00B509", "^FF7800", "^FF0000", "^BA00FF" };
    static const int busy[4] = { 0xcbd, 0xcbe, 0xcbf, 0xcc0 };
    struct LoginServerEntry *e;
    struct String label, colour[4], text_s;
    char text[0x40];
    size_t used;
    struct UIWindow *wnd;
    int i, k;

    replay_recorder_set_mode(replay_recorder_instance(), 1);
    default_wallpaper(self);
    wnd = ui_window_open(2);
    if (!wnd)
        return;
    UIWND_SENDMSG(wnd, 0, 0x50, 0x2713, (int)"NUMSERVER", 0, 0);
    for (i = 0; i < self->numServer; i++) {
        e = &self->servers[i];
        memset(text, 0, sizeof text);
        if (e->isNew == 1)
            sprintf(text, msg_str(0x1e2));
        /* The name is login-server text in a 20-byte field that need not end in NUL. */
        used = strlen(text);
        _snprintf_s(text + used, sizeof text - used, _TRUNCATE, "%.*s", (int)sizeof e->name, e->name);
        string_init(&label);
        for (k = 0; k < 4; k++) {
            string_init(&colour[k]);
            string_assign(&colour[k], colours[k], 7);
        }
        if (e->usercount <= 3) {
            k = e->usercount;
            string_init(&text_s);
            string_format(&text_s, "%s%s  ^000000%s", string_cstr(&colour[k]), msg_str(busy[k]), text);
            string_assign(&label, string_cstr(&text_s), text_s.size);
            string_free(&text_s);
        }
        switch (e->state) {
        case 2: string_append_cstr(&label, msg_str(0xcba)); break;
        case 3: string_append_cstr(&label, msg_str(0xcbb)); break;
        case 4: string_append_cstr(&label, msg_str(0xcbc)); break;
        }
        if (label.size)
            strcpy_s(text, 0x40, string_cstr(&label));
        for (k = 3; k >= 0; k--)
            string_free(&colour[k]);
        string_free(&label);
        ui_list_box_add_item(ui_select_server_list(wnd), text);
    }
    UIWND_SENDMSG(wnd, 0, 0x28, 0, 0, 0, 0);
}

static void char_select(struct LoginMode *self)
{
    g_wobbleFramesLeft = 0;
    session_set_hide_other_players(&g_session1, 0);
    g_mapPropertyQuiet = 0;
    g_skillPointPending = 0;
    (g_session1.cartCount) = 0;
    (g_session1.cartWeight) = 0;
    (g_session1.petJob) = -1;
    session_set_arrow(&g_session1, 0);
    (g_session1.ownIllusion) = 0;
    renew_quest_clear_ongoing(g_questTable);
    session_reset_achievements(&g_session1);
    gameguard_on_connect(g_gameGuard);
    default_wallpaper(self);
    rodex_clear(g_rodex);
    ui_window_open(0x115);
    session_reset_store_search(&g_session1);
    session_clear_transform_jobs(&g_session1, 1, 1, 1);
    ui_window_mgr_post_quit1(&g_windowMgr, 0xc6);
    ui_window_mgr_post_quit1(&g_windowMgr, 0x10d);
    reset_session_items();
    switch ((g_session1.pinState)) {
    case 2:
        (g_session1.pinChangeOnly) = 0;
        show_wallpaper(self);
        ui_window_open(0xc7);
        break;
    case 3:
        (g_session1.pinChangeOnly) = 0;
        show_wallpaper(self);
        ui_window_open(0xc7);
        UIWND_SENDMSG(g_windowMgr.second_password_input_wnd, 0, 6, 0x11c, 0, 0, 0);
        break;
    case 4:
        (g_session1.pinChangeOnly) = 1;
        show_wallpaper(self);
        ui_window_open(0xc8);
        UIWND_SENDMSG(g_windowMgr.second_password_edit_wnd, 0, 6, 0x11c, 0, 0, 0);
        UIWND_SENDMSG(g_windowMgr.second_password_edit_wnd, 0, 6, 0xb0, 1, 0, 0);
        break;
    case 5:
        (g_session1.pinChangeOnly) = 1;
        show_wallpaper(self);
        ui_window_open(0xc8);
        break;
    }
}

static void char_server(struct LoginMode *self)
{
    struct ServerAddress addr;
#pragma pack(push, 1)
    struct {
        WORD id;
        DWORD version;
        char userId[24];
        char password[24];
        BYTE clientType;
    } login;
#pragma pack(pop)
    WORD hashed;
    int st;

    player_names_clear();
    default_wallpaper(self);
    strcpy(addr.ip, g_serverAddress);
    addr.port = atoi(g_serverPort);
    _snprintf_s(addr.host, 0x81, (size_t)-1, "%s", g_serverDomain);
    self->base.connected = (BYTE)connection_connect(rag_connection_transport(rag_connection_instance()), &addr);
    if (!self->base.connected) {
        connect_failed(self);
        return;
    }
    packet_keys_next(g_packetKeys, 2);
    login_mode_send_exe_digest(self);
    if (g_passwordEncrypt) {
        hashed = 0x1db;
        login_send_packet(0x1db, &hashed);
        self->waitWnd = wait_window(0x7a, 0x10);
        return;
    }
    st = g_serviceType;
    login.id = 0x64;
    login.version = g_clientVersion;
    memcpy(login.userId, self->userId, 24);
    if (st == 7) {
        if (!login_password_encode(LOGIN_PASSWORD_SERVICE7, self->password, sizeof self->password,
                                   login.password, sizeof login.password)) {
            connect_failed(self);
            return;
        }
    } else if (st == 4) {
        if (!login_password_encode(LOGIN_PASSWORD_SERVICE4, self->password, sizeof self->password,
                                   login.password, sizeof login.password)) {
            connect_failed(self);
            return;
        }
    } else {
        memcpy(login.password, self->password, 24);
    }
    login.clientType = login_mode_client_type();
    login_send_packet((short)login.id, &login);
    wait_window(0x7a, 0x10);
}

static void map_server(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct {
        WORD id;
        DWORD aid;
        DWORD authCode;
        DWORD userLevel;
        WORD client_type;
        BYTE sex;
    } pkt;
#pragma pack(pop)

    default_wallpaper(self);
    party_booking_reset(&g_session1.partyBooking);
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    self->base.connected = (BYTE)connection_connect(rag_connection_transport(rag_connection_instance()), &g_charServerAddr);
    if (!self->base.connected) {
        connect_failed(self);
        return;
    }
    packet_keys_next(g_packetKeys, 2);
    g_mapServerAidPending = 1;
    pkt.id = 0x65;
    pkt.client_type = 0;
    pkt.aid = g_session1.ownAid;
    pkt.authCode = (g_session1.authCode);
    pkt.userLevel = (g_session1.userLevel);
    pkt.sex = (BYTE)g_session1.accountSex;
    login_send_packet(0x65, &pkt);
    wait_window(0x7a, 0x10);
    account_user_data_load(g_accountUserDataMgr, g_session1.ownAid, string_cstr(&g_session1.worldName));
}

static void make_char(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct {
        WORD id;
        char name[24];
        BYTE slot;
        WORD hairColor;
        WORD hairStyle;
        int job;
        BYTE sex;
    } pkt;
#pragma pack(pop)
    void *w;

    default_wallpaper(self);
    pkt.id = 0xa39;
    pkt.hairColor = g_loginCharInfo.head_palette;
    pkt.hairStyle = g_loginCharInfo.head;
    pkt.slot = g_loginCharInfo.slot;
    pkt.job = (short)g_loginCharInfo.job;
    pkt.sex = g_loginCharInfo.sex;
    memcpy(pkt.name, g_loginCharInfo.name, 24);
    login_send_packet(0xa39, &pkt);
    w = ui_window_open(6);
    if (!w)
        return;
    ui_wait_wnd_set_text(w, msg_str(0x7b));
}

static void delete_char(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct { WORD id; DWORD gid; char email[6]; } pkt;
    struct { WORD id; DWORD gid; char email[0x32]; } pkt2;
#pragma pack(pop)
    struct CharInfo *c;

    default_wallpaper(self);
    pkt.id = 0x829;
    strncpy(pkt.email, self->email, 6);
    c = (struct CharInfo *)login_mode_send_msg(&self->base, 8, g_loginCharInfo.slot, 0, 0, 0);
    pkt.gid = c->gid;
    if (g_serviceType == 0) {
        if (self->email[5] == 0)
            pkt.email[0] = 0;
        login_send_packet((short)pkt.id, &pkt);
    } else if (g_serviceType == 0xa || g_serviceType == 0xc || g_serviceType == 1) {
        pkt2.gid = c->gid;
        pkt2.id = 0x1fb;
        strncpy(pkt2.email, self->email, 0x32);
        pkt2.email[0x31] = 0;
        login_send_packet((short)pkt2.id, &pkt2);
    } else {
        login_send_packet((short)pkt.id, &pkt);
    }
    wait_window(0x7c, 0x11);
}

static void select_char(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct { WORD id; BYTE slot; } pkt;
#pragma pack(pop)

    default_wallpaper(self);
    pkt.id = 0x66;
    pkt.slot = g_loginCharInfo.slot;
    login_send_packet(0x66, &pkt);
    wait_window(0x7a, 0x10);
}

static void enter_map(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct {
        WORD id;
        DWORD aid;
        DWORD gid;
        DWORD authCode;
        DWORD reserved;
        DWORD clientTime;
        BYTE sex;
    } pkt = {0};
#pragma pack(pop)
    char path[0x58];
    HKEY key;
    DWORD disposition, value;

    char_user_data_load(g_charUserDataMgr, g_session1.ownAid, g_session1.ownGid, string_cstr(&g_session1.worldName));
    (g_session1.mdWndShown) = 0;
    default_wallpaper(self);
    self->base.connected = (BYTE)connection_connect(rag_connection_transport(rag_connection_instance()), &g_mapServerIp);
    if (!self->base.connected) {
        connect_failed(self);
        return;
    }
    gameguard_on_connect(g_gameGuard);
    packet_keys_next(g_packetKeys, 1);
    strcpy(path, g_registryKeyPath);
    strcat(path, "\\NeoCash");
    disposition = 0;
    value = 0;
    if (RegCreateKeyExA((HKEY)0x80000001,
                        path, 0, NULL, 0, 0xf003f, NULL, &key, &disposition) == 0) {
        RegSetValueExA(key, "CASH_CATEGORY", 0, REG_DWORD, (const BYTE *)&value, 4);
        RegCloseKey(key);
    }
    pkt.id = 0x436;
    pkt.clientTime = timeGetTime();
    pkt.aid = g_session1.ownAid;
    pkt.gid = ((DWORD)g_session1.ownGid);
    pkt.authCode = (g_session1.authCode);
    pkt.sex = (BYTE)session_get_sex(&g_session1);
    login_send_packet((short)pkt.id, &pkt);
    session_set_pair(&g_session1, 0x11, 1);
    connection_set_hold_sends(rag_connection_transport(rag_connection_instance()), 1);
    wait_window(0x7a, 0x10);
}

static void char_select_userver(struct LoginMode *self)
{
    g_wobbleFramesLeft = 0;
    g_mapPropertyQuiet = 0;
    g_skillPointPending = 0;
    (g_session1.petJob) = -1;
    session_set_arrow(&g_session1, 0);
    default_wallpaper(self);
    ui_window_open(0x7f);
}

static void replay_entry(struct LoginMode *self)
{
    BYTE type;
    void *rec, *w;

    if (g_serviceType == 6 || g_serviceType == 0xc || g_serviceType == 0xa) {
        type = 0;
        (g_session1.showName) = 0;
    } else {
        type = (g_session1.showName);
    }
    session_set_text_type(type, type);
    rec = replay_recorder_instance();
    replay_file_list_set_recorder(replay_file_list_instance(), rec);
    string_assign(&self->wallpaper, warning2_bmp, 0x2b);
    show_wallpaper(self);
    w = ui_window_open(0x14);
    UIWND_SENDMSG(w, 0, 0x50, 0x272a, 0, 0, 0);
}

static void replay_open(struct LoginMode *self)
{
    if (replay_file_list_open_selected(replay_file_list_instance(), 0x20) == 1) {
        self->base.next_sub_mode = 0x1c;
        return;
    }
    box(msg_str(0xc4e), 4, 1, 1, 0);
    self->base.next_sub_mode = 0x1a;
}

static void replay_list(struct LoginMode *self)
{
    struct UIWindow *wnd;
    struct ReplayFileList *list;
    int n, i;

    string_assign_cstr(&self->wallpaper, g_loginBackgroundName);
    show_wallpaper(self);
    wnd = ui_window_open(0xba);
    replay_file_list_scan(replay_file_list_instance());
    list = replay_file_list_instance();
    n = (int)list->count;
    for (i = 0; wnd && i < n; i++) {
        ui_list_box_add_item(ui_select_replay_list(wnd), replay_file_list_get_name(replay_file_list_instance(), i));
    }
    UIWND_SENDMSG(wnd, 0, 0x28, 0, 0, 0, 0);
    replay_file_list_instance()->serverCount = self->numServer;
    if ((unsigned)self->numServer > 100) {
        self->numServer = 0;
        replay_file_list_instance()->serverCount = 0;
    }
    for (i = 0; i < self->numServer; i++) {
        list->servers[i] = self->servers[i];
    }
}

static void replay_back(struct LoginMode *self)
{
    int i;

    self->numServer = replay_file_list_instance()->serverCount;
    if ((unsigned)self->numServer > 100) {
        self->numServer = 0;
        replay_file_list_instance()->serverCount = 0;
    }
    for (i = 0; i < self->numServer; i++)
        self->servers[i] = replay_file_list_instance()->servers[i];
    login_mode_send_msg(&self->base, 0x272a, 0, 0, 0, 0);
}

static void replay_start(struct LoginMode *self)
{
    struct ReplaySessionInfo info;
    struct ReplayBuffer chunk = {0};
    char map[0x40];

    if (!replay_read_chunk(replay_recorder_instance(), 3, &chunk) ||
        session_values_validate(SESSION_VALUES_IDENTITY, chunk.data, chunk.size) < 0) {
        replay_buffer_destroy(&chunk);
        replay_recorder_close(replay_recorder_instance());
        self->base.next_sub_mode = 0x1b;
        return;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, 0x10d);
    reset_session_items();
    replay_recorder_set_mode(replay_recorder_instance(), 6);
    info = replay_recorder_instance()->info;
    session_set_character_name(&g_session1, text_view((const char *)info.name, sizeof info.name));
    memcpy(g_session1.map_name_tail, info.mapData + sizeof g_session1.mapName, sizeof g_session1.map_name_tail);
    g_session1.mapType = info.mapType;
    g_session1.playerPosX = info.playerX;
    g_session1.playerPosY = info.playerY;
    g_session1.playerDir = info.playerDirection;
    memcpy(g_session1.mapName, info.mapData, sizeof g_session1.mapName);
    ui_window_mgr_reset_shown_infos(&g_windowMgr, info.options);
    session_reset_for_replay(&g_session1);
    session_values_load(&g_session1, SESSION_VALUES_IDENTITY, chunk.data, chunk.size);
    replay_buffer_destroy(&chunk);
    sprintf(map, "%s.rsw", (const char *)g_session1.mapName);
    mode_mgr_switch(&g_modeMgr, 1, map);
}

static void zone_list(struct LoginMode *self)
{
    char name[0x10];
    struct UIWindow *wnd;
    unsigned i;

    wnd = ui_window_open(2);
    if (!wnd)
        return;
    UIWND_SENDMSG(wnd, 0, 0x50, 0x272c, (int)"NUMZONE", 0, 0);
    for (i = 0; i < (unsigned)(((BYTE *)self->zones.last - (BYTE *)self->zones.first) / 0x14); i++) {
        strncpy(name, session_get_rsw_name((const char *)((struct LoginZoneEntry *)self->zones.first)[i].data), 0x10);
        ui_list_box_add_item(ui_select_server_list(wnd), session_map_display_name(name));
    }
    UIWND_SENDMSG(wnd, 0, 0x28, 0, 0, 0, 0);
}

static void select_zone(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct { WORD id; BYTE slot; BYTE zone; } pkt;
#pragma pack(pop)

    pkt.id = 0x841;
    pkt.slot = g_loginCharInfo.slot;
    pkt.zone = *(BYTE *)self->zones.first;
    login_send_packet(0x841, &pkt);
}

void login_mode_on_change_state(struct Mode *mode, int state)
{
    struct LoginMode *self = (struct LoginMode *)mode;
    self->stateTick = timeGetTime();
    ui_window_mgr_close_all_windows(&g_windowMgr);
    self->base.help_balloon = NULL;
    (g_session1.battle_mode) = 0;
    self->base.connected = 1;
    switch (state) {
    case 0: login_mode_show_warning(self); return;
    case 1: login_mode_show_license(self); return;
    case 2:
        if (!login_mode_load_connections(self)) {
            login_mode_send_msg(&self->base, 0x1c, 0, 0, 0, 0);
            return;
        }
        connection_list(self, &self->connections, 0x2723);
        return;
    case 3: login_screen(self); return;
    case 4: char_server(self); return;
    case 5: map_server(self); return;
    case 6: server_list(self); return;
    case 7: char_select(self); return;
    case 8:
        default_wallpaper(self);
        ui_window_open(0x116);
        return;
    case 9: select_char(self); return;
    case 0xa: make_char(self); return;
    case 0xb: delete_char(self); return;
    case 0xc: enter_map(self); return;
    case 0xd:
        if (!login_mode_load_sub_connections(self, self->connectionIndex)) {
            login_mode_send_msg(&self->base, 0x1c, 0, 0, 0, 0);
            return;
        }
        connection_list(self, &self->subConnections, 0x2726);
        return;
    case 0x13: char_select_userver(self); return;
    case 0x18: ui_window_open(0xb4); return;
    case 0x19: replay_open(self); return;
    case 0x1a:
        replay_back(self);
    case 0x1b: replay_list(self); return;
    case 0x1c: replay_start(self); return;
    case 0x1d: replay_entry(self); return;
    case 0x1e: zone_list(self); return;
    case 0x1f: select_zone(self); return;
    case 0x26: login_acf(self); return;
    }
}
