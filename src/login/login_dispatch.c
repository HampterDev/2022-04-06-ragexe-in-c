#include "core/thread_text.h"
#include "contents/shutdown_notice.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"
#include "diag/diag.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <winsock2.h>
#include "core/msg_str.h"
#include "session/session_names.h"
#include "ui/character/select_char_new.h"
#include "core/unknown_packet.h"
#include "core/packet_table.h"

int g_mapServerAidPending = 0;

#pragma pack(push, 1)
struct PACKET_AC_REFUSE_LOGIN {
    WORD packet_type;
    BYTE error_code;
    char block_date[20];
};
struct PACKET_AC_REFUSE_LOGIN2 {
    WORD packet_type;
    DWORD error_code;
    char block_date[20];
};
struct PACKET_HC_NOTIFY_ZONESVR {
    WORD packet_type;
    DWORD gid;
    char map_name[16];
    DWORD ip;
    short port;
};
struct PACKET_HC_NOTIFY_ZONESVR2 {
    struct PACKET_HC_NOTIFY_ZONESVR zone;
    char host[0x80];
};
struct PACKET_HC_REFUSE_ENTER {
    WORD packet_type;
    BYTE error_code;
};
struct PACKET_HC_REFUSE_MAKECHAR {
    WORD packet_type;
    BYTE error_code;
};
struct PACKET_HC_REFUSE_DELETECHAR {
    WORD packet_type;
    BYTE error_code;
};
struct PACKET_HC_REFUSE_SELECTCHAR {
    WORD packet_type;
    BYTE error_code;
};
struct PACKET_AC_LOGIN_TAREN_REFUSE {
    WORD packet_type;
    BYTE error_code;
};
struct PACKET_AC_SERVER_TEXT {
    WORD packet_type;
    WORD packet_length;
    char text[];
};
struct PACKET_HC_ACCEPT_ENTER2 {
    WORD packet_type;
    WORD packet_length;
    struct LoginBillingInfo billing;
    BYTE chars[];
};
struct PACKET_HC_ACK_RENAME_CHECK {
    WORD packet_type;
    WORD result;
};
struct PACKET_HC_DELETE_CHAR3_CANCEL {
    WORD packet_type;
    DWORD gid;
    DWORD result;
};
struct PACKET_MAP_STATE_ENTRY {
    DWORD flag;
    BYTE data[16];
};
struct PACKET_HC_MAP_STATES {
    WORD packet_type;
    WORD packet_length;
    struct PACKET_MAP_STATE_ENTRY entries[];
};
struct PACKET_HC_SECOND_PASSWD_ACK {
    WORD packet_type;
    WORD result;
    DWORD value;
};
struct PACKET_HC_RESULT_SHORT {
    WORD packet_type;
    WORD packet_length;
    short result;
};
struct PACKET_HC_CHARLIST_NOTIFY {
    WORD packet_type;
    DWORD count;
};
struct PACKET_AC_REQ_MOBILE_OTP {
    WORD packet_type;
    DWORD value;
};
struct PACKET_AC_REFUSE_LOGIN3 {
    WORD packet_type;
    BYTE error_code;
    DWORD value;
};
struct PACKET_ZC_OVERWEIGHT_PERCENT {
    WORD packet_type;
    DWORD percent;
};
struct PACKET_ZC_INVENTORY_EXPANSION_INFO {
    WORD packet_type;
    short size;
};
struct PACKET_HC_ACCEPT_MAKECHAR_NEO {
    WORD packet_type;
    struct CharInfo chara;
};
struct PACKET_HC_UPDATE_CHARINFO {
    WORD packet_type;
    struct CharInfo chara;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_HEADER, 2);
SIZE_ASSERT(PACKET_AC_REFUSE_LOGIN, 23);
SIZE_ASSERT(PACKET_AC_REFUSE_LOGIN2, 26);
SIZE_ASSERT(PACKET_HC_NOTIFY_ZONESVR, 28);
SIZE_ASSERT(PACKET_HC_NOTIFY_ZONESVR2, 28 + 0x80);
SIZE_ASSERT(PACKET_HC_ACCEPT_ENTER2, 0x18);
SIZE_ASSERT(PACKET_MAP_STATE_ENTRY, 0x14);
SIZE_ASSERT(PACKET_HC_MAP_STATES, 4);
SIZE_ASSERT(PACKET_HC_SECOND_PASSWD_ACK, 8);
SIZE_ASSERT(PACKET_HC_RESULT_SHORT, 6);
SIZE_ASSERT(PACKET_AC_REFUSE_LOGIN3, 7);
SIZE_ASSERT(PACKET_HC_UPDATE_CHARINFO, 0xb1);
SIZE_ASSERT(PACKET_HC_ACCEPT_MAKECHAR_NEO, 0xb1);

static int box(const char *text, int a, int b, int c, int d)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, a, b, c, d, "Message", 0x118, 0x78, 0);
}

static void notice(int msg)
{
    box(msg_str(msg), 0, 1, 0, 0);
}

static void back_to_login(struct LoginMode *self)
{
    g_mapServerAidPending = 0;
    self->base.next_sub_mode = 3;
}

static void disconnect(void)
{
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
}

static void read_billing(struct LoginMode *self, const struct LoginBillingInfo *src)
{
    self->billing = *src;
    self->billing.value1 = ntohl(self->billing.value1);
    self->billing.value5 = ntohl(self->billing.value5);
    self->billing.value9 = ntohl(self->billing.value9);
}

static void enter_zone(struct LoginMode *self, struct String host)
{
    const struct PACKET_HC_NOTIFY_ZONESVR *packet = (const struct PACKET_HC_NOTIFY_ZONESVR *)g_packetBuffer;
    struct String name, map;

    string_init(&name);
    string_assign(&name, packet->map_name, 16);
    map = string_from_cstr(string_cstr(&name));
    login_mode_enter_zone(self, packet->gid, map, packet->ip, (DWORD)(int)packet->port, host);
    string_free(&name);
}

static void char_map_display_name(char *mapName)
{
    _snprintf_s(mapName, 0x10, (size_t)-1, "%s", session_get_rsw_name(mapName));
}

static void on_refuse_char_server(struct LoginMode *self)
{
    const struct PACKET_HC_REFUSE_ENTER *packet = (const struct PACKET_HC_REFUSE_ENTER *)g_packetBuffer;

    disconnect();
    switch (packet->error_code) {
    case 0x01:
        box(msg_str(0xd0), 0, 1, 1, 0);
        break;
    case 0x6f:
        notice(0x749);
        break;
    case 0x76:
        notice(0xeea);
        self->base.next_sub_mode = 6;
        return;
    case 0x77:
        notice(0xb71);
        self->base.next_sub_mode = 6;
        return;
    default:
        notice(9);
        break;
    }
    back_to_login(self);
}

static void on_make_char_refused(struct LoginMode *self)
{
    static const int texts[] = { 0xa, 0x12a, 0x4f8, 0x54b };
    const struct PACKET_HC_REFUSE_MAKECHAR *packet = (const struct PACKET_HC_REFUSE_MAKECHAR *)g_packetBuffer;
    int code = packet->error_code, msg;

    if (code <= 3)
        msg = texts[code];
    else if (code == 0xb)
        msg = 0x553;
    else if (code == 0xc)
        msg = 0x7e2;
    else
        msg = 0xb;
    notice(msg);
    self->base.next_sub_mode = 7;
}

static void on_delete_char_refused(struct LoginMode *self)
{
    const struct PACKET_HC_REFUSE_DELETECHAR *packet = (const struct PACKET_HC_REFUSE_DELETECHAR *)g_packetBuffer;
    int msg;

    switch (packet->error_code) {
    case 0: msg = g_serviceType == 0 ? 0x12f : 0x12d; break;
    case 1: msg = 0x54c; break;
    case 2: msg = 0x788; break;
    default: msg = 0xc; break;
    }
    notice(msg);
    self->base.next_sub_mode = 7;
}

static void on_ask_hash(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct { WORD id; BYTE flag; } pkt;
#pragma pack(pop)

    if (box(msg_str(0x2b3), 2, 1, 0, 0) != 0xb3) {
        disconnect();
        back_to_login(self);
        return;
    }
    pkt.flag = 1;
    pkt.id = 0x1bf;
    login_send_packet(0x1bf, &pkt);
}

static void on_server_text(struct LoginMode *self)
{
    const struct PACKET_AC_SERVER_TEXT *packet = (const struct PACKET_AC_SERVER_TEXT *)g_packetBuffer;
    struct String text;

    string_init(&text);
    string_replace(&text, 0, text.size, packet->text, (size_t)packet->packet_length - 4);
    box(string_cstr(&text), 0, 1, 1, 0);
    back_to_login(self);
    string_free(&text);
}

static void on_char_list_userver(struct LoginMode *self)
{
    const struct PACKET_HC_ACCEPT_ENTER2 *packet = (const struct PACKET_HC_ACCEPT_ENTER2 *)g_packetBuffer;
    unsigned len;

    read_billing(self, &packet->billing);
    len = (unsigned)packet->packet_length - 0x18;
    self->numChar = (int)(len / 0xaf);
    (g_session1.charCount) = self->numChar;
    memcpy_s(self->chars, 0x2e7c, packet->chars + len % 0xaf, self->numChar * 0xaf);
    self->base.next_sub_mode = 0x13;
}

static void on_rename_check(struct LoginMode *self)
{
    const struct PACKET_HC_ACK_RENAME_CHECK *packet = (const struct PACKET_HC_ACK_RENAME_CHECK *)g_packetBuffer;
    char text[300];
#pragma pack(push, 1)
    struct { WORD id; DWORD gid; } pkt;
#pragma pack(pop)

    if (packet->result != 0) {
        sprintf(text, msg_str(0x4bd), g_session1.renameNewName);
        if (box(text, 2, 0, 0, 0) == 0xb3) {
            pkt.id = 0x28f;
            pkt.gid = ((DWORD)g_session1.renameCharId);
            login_send_packet(0x28f, &pkt);
            return;
        }
    } else if (box(msg_str(0xa), 0, 1, 1, 0) != 0xb0) {
        return;
    }
    if (g_windowMgr.new_select_char_wnd)
        UIWND_SENDMSG(g_windowMgr.new_select_char_wnd, 0, 6, 0x157, 0, 0, 0);
}

static void on_select_char_refused(struct LoginMode *self)
{
    static const int texts[] = { 0x546, 0x547, 0x548, 0x549, 0x54a, 0x554 };
    const struct PACKET_HC_REFUSE_SELECTCHAR *packet = (const struct PACKET_HC_REFUSE_SELECTCHAR *)g_packetBuffer;
    int code = packet->error_code;

    if (code <= 5)
        notice(texts[code]);
    self->base.next_sub_mode = connection_is_open(rag_connection_transport(rag_connection_instance())) ? 7 : 3;
}

static void on_delete_cancel(struct LoginMode *self)
{
    const struct PACKET_HC_DELETE_CHAR3_CANCEL *packet = (const struct PACKET_HC_DELETE_CHAR3_CANCEL *)g_packetBuffer;
    struct String text;
    const char *msg = NULL;
    int i;

    string_init(&text);
    switch (packet->result) {
    case 0:
        msg = msg_str(0x718);
        break;
    case 1:
        for (i = 0; i < self->numChar; i++) {
            if (self->chars[i].gid == packet->gid) {
                self->chars[i].delete_date = 0;
                break;
            }
        }
        self->base.next_sub_mode = 7;
        break;
    case 2:
        msg = msg_str(0x719);
        break;
    default:
        msg = msg_str(0x718);
        break;
    }
    if (msg)
        string_assign_cstr(&text, msg);
    if (text.size)
        box(string_cstr(&text), 0, 1, 0, 0);
    string_free(&text);
}

static void on_map_states(struct LoginMode *self)
{
    char text[0x100];
    struct Vec *zones = &self->zones;
    struct LoginZoneEntry entry;
    const struct PACKET_HC_MAP_STATES *packet = (const struct PACKET_HC_MAP_STATES *)g_packetBuffer;
    unsigned len = (unsigned)packet->packet_length - 4;
    int n = (int)(len / 0x14), i;
    const struct PACKET_MAP_STATE_ENTRY *rec;

    if (len % 0x14) {
        sprintf(text, "mismatch MapState size(size = %d)", 0x14);
        box(text, 0, 1, 0, 0);
        self->base.next_sub_mode = 7;
        return;
    }
    zones->last = zones->first;
    entry.index = -1;
    for (i = 0, rec = packet->entries; i < n; i++, rec++) {
        if (rec->flag != 0)
            continue;
        entry.index = i;
        memcpy(entry.data, rec->data, sizeof entry.data);
        vec_push(zones, &entry, sizeof(struct LoginZoneEntry));
    }
    if (((BYTE *)zones->last - (BYTE *)zones->first) / 0x14 == 0) {
        notice(0x713);
        self->base.next_sub_mode = 7;
        return;
    }
    self->base.next_sub_mode = 0x1e;
}

static void pin_box(struct LoginMode *self, int wnd, int msg, int toSelect)
{
    ui_window_mgr_post_quit1(&g_windowMgr, wnd);
    notice(msg);
    if (toSelect)
        self->base.next_sub_mode = 7;
}

static void on_pin_make_result(struct LoginMode *self)
{
    const struct PACKET_HC_SECOND_PASSWD_ACK *packet = (const struct PACKET_HC_SECOND_PASSWD_ACK *)g_packetBuffer;

    (g_session1.pinResultValue) = packet->value;
    switch (packet->result) {
    case 0: pin_box(self, 0xc8, 0x761, 0); break;
    case 1: pin_box(self, 0xc8, 0x76a, 1); break;
    case 2: pin_box(self, 0xc8, 0x768, 1); break;
    case 3: pin_box(self, 0xc8, 0x769, 1); break;
    case 4: pin_box(self, 0xc8, 0x765, 1); break;
    }
}

static void on_pin_delete_result(struct LoginMode *self)
{
    const struct PACKET_HC_SECOND_PASSWD_ACK *packet = (const struct PACKET_HC_SECOND_PASSWD_ACK *)g_packetBuffer;

    (g_session1.pinResultValue) = packet->value;
    switch (packet->result) {
    case 0: pin_box(self, 0xc8, 0x762, 0); break;
    case 1: pin_box(self, 0xc8, 0x766, 1); break;
    }
}

static void on_pin_edit_result(struct LoginMode *self)
{
    const struct PACKET_HC_SECOND_PASSWD_ACK *packet = (const struct PACKET_HC_SECOND_PASSWD_ACK *)g_packetBuffer;

    (g_session1.pinResultValue) = packet->value;
    switch (packet->result) {
    case 0: pin_box(self, 0xc7, 0x763, 0); break;
    case 1: pin_box(self, 0xc7, 0x767, 1); break;
    case 2: pin_box(self, 0xc7, 0x768, 1); break;
    case 3: pin_box(self, 0xc7, 0x769, 1); break;
    }
}

static void on_pin_disable_result(struct LoginMode *self)
{
    const struct PACKET_HC_SECOND_PASSWD_ACK *packet = (const struct PACKET_HC_SECOND_PASSWD_ACK *)g_packetBuffer;

    (g_session1.pinResultValue) = packet->value;
    switch (packet->result) {
    case 0:
        pin_box(self, 0xc7, 0x76d, 0);
        ui_new_select_char_wnd_show_button((struct UINewSelectCharWnd *)g_windowMgr.new_select_char_wnd, 0x19a);
        (g_session1.pinState) = 1;
        break;
    case 1:
        pin_box(self, 0xc7, 0x76e, 1);
        break;
    }
}

static void on_pin_enable_result(struct LoginMode *self)
{
    const struct PACKET_HC_SECOND_PASSWD_ACK *packet = (const struct PACKET_HC_SECOND_PASSWD_ACK *)g_packetBuffer;

    switch (packet->result) {
    case 0:
        pin_box(self, 0xc7, 0x76f, 0);
        ui_new_select_char_wnd_hide_button((struct UINewSelectCharWnd *)g_windowMgr.new_select_char_wnd, 0x19a);
        break;
    case 1:
        pin_box(self, 0xc7, 0x770, 1);
        break;
    }
}

static void on_char_list_notify(void)
{
    const struct PACKET_HC_CHARLIST_NOTIFY *packet = (const struct PACKET_HC_CHARLIST_NOTIFY *)g_packetBuffer;
    WORD pkt;
    int i;

    for (i = 1; i <= (int)packet->count; i++) {
        pkt = 0x9a1;
        rag_connection_send_packet(rag_connection_instance(), 2, &pkt);
    }
}

static void on_refuse_login3(struct LoginMode *self)
{
    const struct PACKET_AC_REFUSE_LOGIN3 *packet = (const struct PACKET_AC_REFUSE_LOGIN3 *)g_packetBuffer;

    if (packet->error_code != 0x12)
        disconnect();
    self->refuse_seconds_left = packet->value;
    login_mode_refuse_login(self, packet->error_code, "");
}

static void on_refuse_taren(struct LoginMode *self)
{
    static const int texts[] = { 0xc7c, 0xc7d, 0xc7e, 0xc7f, 0xc80, 0xc81, 0xc8e, 0xc82, 0xf47 };
    const struct PACKET_AC_LOGIN_TAREN_REFUSE *packet = (const struct PACKET_AC_LOGIN_TAREN_REFUSE *)g_packetBuffer;
    int code = packet->error_code, msg = 0xc82;

    if ((unsigned)(code - 1) <= 8)
        msg = texts[code - 1];
    disconnect();
    box(thread_text_format("%s(%d)", msg_str(msg), code), 0, 1, 0, 0);
    back_to_login(self);
}

static void on_zone_addr_domain(struct LoginMode *self)
{
    const struct PACKET_HC_NOTIFY_ZONESVR2 *packet = (const struct PACKET_HC_NOTIFY_ZONESVR2 *)g_packetBuffer;
    struct String host;

    string_init(&host);
    string_assign(&host, packet->host, 0x80);
    enter_zone(self, string_from_cstr(string_cstr(&host)));
    string_free(&host);
}

static void on_refuse_login_ex(struct LoginMode *self)
{
    const struct PACKET_AC_REFUSE_LOGIN2 *packet = (const struct PACKET_AC_REFUSE_LOGIN2 *)g_packetBuffer;
    struct String text;

    string_init(&text);
    string_assign(&text, packet->block_date, 0x14);
    login_mode_refuse_login(self, (int)packet->error_code, string_cstr(&text));
    string_free(&text);
}

static void on_char_updated(struct LoginMode *self)
{
    const struct PACKET_HC_UPDATE_CHARINFO *packet = (const struct PACKET_HC_UPDATE_CHARINFO *)g_packetBuffer;
    struct CharInfo *c = (struct CharInfo *)CMODE_SENDMSG(client_current_mode(), 8, packet->chara.slot, 0, 0, 0);

    if (!c || c->gid != ((DWORD)g_session1.renameCharId))
        return;
    memcpy_s(c, 0xaf, &packet->chara, 0xaf);
    char_map_display_name(c->map_name);
    self->base.next_sub_mode = 7;
}

void login_mode_process_packets(struct LoginMode *self)
{
    BYTE sink[4];
    unsigned short packet_length;
    void *conn;

    if (g_replayLaunch)
        return;
    memset(g_packetBuffer, 0, 0x10000);
    if (!connection_poll(rag_connection_transport(rag_connection_instance())))
        CMODE_SENDMSG(client_current_mode(), 1, 0, 0, 0, 0);
    if (g_mapServerAidPending == 1) {
        if (connection_recv_bytes(rag_connection_transport(rag_connection_instance()), sink, 4, 1))
            g_mapServerAidPending = 0;
        return;
    }
    while (rag_connection_get_packet(rag_connection_instance(), g_packetBuffer, &packet_length)) {
        switch ((short)rag_connection_get_packet_type(g_packetBuffer)) {
        case 0x69:
        case 0x6d:
        case 0x187:
        case 0x283:
        case 0x8e3:
        case 0x8e4:
        case 0x99d:
        case 0x9c2:
        case 0xa4d:
        case 0xa73:
        case 0xa79:
        case 0xa7b:
        case 0xae0:
            break;
        case 0x6a: {
            struct PACKET_AC_REFUSE_LOGIN *packet = (struct PACKET_AC_REFUSE_LOGIN *)g_packetBuffer;

            packet->block_date[19] = 0;
            login_mode_refuse_login(self, packet->error_code, packet->block_date);
            break;
        }
        case 0x6b: login_mode_on_accept_enter(self, g_packetBuffer); break;
        case 0x6c: on_refuse_char_server(self); break;
        case 0x6e: on_make_char_refused(self); break;
        case 0x6f: login_mode_remove_deleted_char(self); break;
        case 0x70: on_delete_char_refused(self); break;
        case 0x71: enter_zone(self, string_from_cstr("")); break;
        case 0x73:
            login_mode_on_zone_accept2(g_packetBuffer);
            return;
        case 0x74:
            notice(9);
            disconnect();
            back_to_login(self);
            break;
        case 0x81: login_mode_on_disconnect_reason(self, g_packetBuffer); break;
        case 0x1b5: login_mode_on_play_time(self, g_packetBuffer); break;
        case 0x1be: on_ask_hash(self); break;
        case 0x1c7: error_msg("No Packet Encryption !!!", 0); break;
        case 0x1dc: login_mode_on_hash_key(self, g_packetBuffer); break;
        case 0x1f1: on_server_text(self); break;
        case 0x20d: session_set_blocked_characters(&g_session1, g_packetBuffer, packet_length); break;
        case 0x23d: notice(0x3fc); break;
        case 0x28b: on_char_list_userver(self); break;
        case 0x28e: on_rename_check(self); break;
        case 0x290: login_mode_on_select_wnd_result2(self, g_packetBuffer); break;
        case 0x2ca: on_select_char_refused(self); break;
        case 0x2eb:
            login_mode_on_zone_accept(self, g_packetBuffer);
            return;
        case 0x448: login_mode_on_char_list(self, g_packetBuffer); break;
        case 0x828: login_mode_on_delete_reserved(self, g_packetBuffer); break;
        case 0x82a: login_mode_on_delete_result(self, g_packetBuffer); break;
        case 0x82c: on_delete_cancel(self); break;
        case 0x82d: login_mode_on_char_list_header(self, g_packetBuffer); break;
        case 0x83e: {
            struct PACKET_AC_REFUSE_LOGIN2 *packet = (struct PACKET_AC_REFUSE_LOGIN2 *)g_packetBuffer;

            packet->block_date[19] = 0;
            login_mode_refuse_login(self, (int)packet->error_code, packet->block_date);
            break;
        }
        case 0x840: on_map_states(self); break;
        case 0x851: game_mode_ignore_packet(g_packetBuffer); break;
        case 0x8b9: login_mode_on_pin_state2(self, g_packetBuffer); break;
        case 0x8bb: on_pin_make_result(self); break;
        case 0x8bd: on_pin_delete_result(self); break;
        case 0x8bf: on_pin_edit_result(self); break;
        case 0x8c4: on_pin_disable_result(self); break;
        case 0x8c6: on_pin_enable_result(self); break;
        case 0x8d5:
        case 0xb70:
            if (((const struct PACKET_HC_RESULT_SHORT *)g_packetBuffer)->result == 1)
                box(msg_str(0x7e1), 0, 1, 0, 0);
            break;
        case 0x8fd: login_mode_on_select_wnd_result(self, g_packetBuffer); break;
        case 0x986: shutdown_notice_receive(g_packetBuffer, packet_length); break;
        case 0x9a0: on_char_list_notify(); break;
        case 0x9a2:
            self->mobile_otp_id = (int)((const struct PACKET_AC_REQ_MOBILE_OTP *)g_packetBuffer)->value;
            self->base.next_sub_mode = 0x18;
            break;
        case 0x9a5: on_refuse_login3(self); break;
        case 0xac4: login_mode_on_accept_login(self, g_packetBuffer, 5); break;
        case 0xac5: on_zone_addr_domain(self); break;
        case 0xac9: login_mode_on_accept_login(self, g_packetBuffer, 4); break;
        case 0xaca: on_refuse_taren(self); break;
        case 0xacd: login_mode_on_refuse_byte(self, g_packetBuffer); break;
        case 0xad1: login_mode_on_auth_result2(self, g_packetBuffer); break;
        case 0xade:
        {
            const struct PACKET_ZC_OVERWEIGHT_PERCENT *packet = (const struct PACKET_ZC_OVERWEIGHT_PERCENT *)g_packetBuffer;

            if ((int)packet->percent > 0)
                g_session1.weightPenaltyPercent = (int)packet->percent;
            break;
        }
        case 0xae3: login_mode_on_auth_result(self, g_packetBuffer); break;
        case 0xae9: login_mode_on_pin_state(self, g_packetBuffer); break;
        case 0xb02: on_refuse_login_ex(self); break;
        case 0xb07: login_mode_on_accept_login(self, g_packetBuffer, 6); break;
        case 0xb18:
        {
            const struct PACKET_ZC_INVENTORY_EXPANSION_INFO *packet = (const struct PACKET_ZC_INVENTORY_EXPANSION_INFO *)g_packetBuffer;

            if (packet->size <= 100)
                g_session1.inventoryExtension = packet->size;
            break;
        }
        case 0xb6f: login_mode_on_accept_make_char(self, &((const struct PACKET_HC_ACCEPT_MAKECHAR_NEO *)g_packetBuffer)->chara); break;
        case 0xb71: on_char_updated(self); break;
        case 0xb72: login_mode_on_char_list_page(self, g_packetBuffer); break;
        default:
            conn = rag_connection_instance();
            connection_reset_queues(rag_connection_transport(conn));
            break;
        }
    }
}
