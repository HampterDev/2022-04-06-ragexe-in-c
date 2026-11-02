#include "core/thread_text.h"
#include "world/mode_actions.h"
#include "contents/player_names.h"
#include "platform/code_page.h"
#include "ui/window_manager_lifecycle.h"
#include "render/device.h"
#include "render/sprite_atlas.h"
#include "contents/schedule.h"
#include "session/option_pairs.h"
#include "contents/mcstore_backup.h"
#include "ui/window_messages.h"
#include "session/character_name.h"
#include "contents/quest.h"
#include "session/map_names.h"
#include "game/client_state.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#ifndef LOGINMODE_H
#define LOGINMODE_H

#include "world/tooltip.h"
#include "fileio/resource_api.h"

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "render/renderer.h"

#include "base.h"
#include "std/std.h"
#include "platform/clientinfo.h"
#include "login/char_info.h"
#include "login/server_entry.h"

#include "world/mode_callbacks.h"
#include "world/mode_dispatch.h"

#define CMODE_SENDMSG(m, msg, a, b, c, d) \
    mode_send_message((struct Mode *)(m), (msg), (a), (b), (c), (d))

#pragma pack(push, 1)

struct LoginBillingInfo {
    BYTE begin_billing;
    DWORD value1;
    DWORD value5;
    DWORD value9;
    BYTE end_billing[7];
};
SIZE_ASSERT(LoginBillingInfo, 0x14);

struct LoginZoneEntry {
    int index;
    BYTE data[16];
};
SIZE_ASSERT(LoginZoneEntry, 0x14);

#pragma pack(pop)

#define LOGIN_MAX_SERVERS       100
#define LOGIN_MAX_CHARS         68

struct LoginMode {
    struct Mode base;
    struct NewCharLooks new_char;
    char new_char_name[0x40];
    char email[0x80];
    char password[0x40];
    char userId[0x40];
    BYTE otp_code[0xc];
    int numServer;
    int selectedServer;
    int numChar;
    BYTE account_class[8];
    int chars_received;
    int char_list_pages;
    BYTE char_deleted;
    DWORD stateTick;
    struct LoginServerEntry servers[LOGIN_MAX_SERVERS];
    struct CharInfo chars[LOGIN_MAX_CHARS];
    struct String wallpaper;
    struct LoginBillingInfo billing;
    DWORD nextKeepAlive;
    void *waitWnd;
    int pin_failures;
    struct Vec messageIds;
    int mobile_otp_id;
    struct Vec connections;
    struct Vec subConnections;
    BYTE savedFlag;
    int connectionIndex;
    int sub_connection_index;
    struct String exit_cleared_text;
    struct Vec zones;
    BYTE auth_answered;
    BYTE auth_waiting;
    BYTE auth_timed_out;
    DWORD refuse_seconds_left;
};

struct LoginConnEntry {
    struct String display;
    struct String desc;
    struct String balloon;
    struct String address;
    struct String port;
    DWORD version, service_type;
};

#define UIWND_SENDMSG(w, a, msg, b, c, d, e) \
    ui_window_send((struct UIWindow *)(w), (struct UIWindow *)(a), (msg), (b), (c), (d), (e))

extern int g_exchangeGiveBack;

#include "contents/guild_emblem_api.h"

#include "contents/party_booking.h"

void game_mode_ignore_packet(const unsigned char *pkt);

static __inline void login_send_packet(int id, const void *buf)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), id);
    rag_connection_send_packet(rag_connection_instance(), len, buf);
}

extern const char *g_loginBackgroundName;
extern int g_mapServerAidPending;
extern struct ServerAddress g_charServerAddr;
extern DWORD g_charServerIp;
extern const struct ModeOps g_CLoginMode_ops;

void login_mode_on_update(struct Mode *mode);
int login_mode_send_msg(struct Mode *mode, int msg, int a, int b, int c, int d);
void login_mode_on_change_state(struct Mode *mode, int subMode);
void login_mode_check_timeouts(struct LoginMode *self, int subMode);
void login_mode_send_keep_alive(struct LoginMode *self);

void login_mode_process_packets(struct LoginMode *self);


int login_mode_client_type(void);
void login_mode_on_hash_key(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_accept_login(struct LoginMode *self, const BYTE *pkt, int form);
void login_mode_send_exe_digest(struct LoginMode *self);

BYTE login_mode_load_connections(struct LoginMode *self);
BYTE login_mode_load_sub_connections(struct LoginMode *self, int index);

int login_mode_max_char_slot(struct LoginMode *self);
void login_mode_remove_deleted_char(struct LoginMode *self);
void login_mode_on_char_list_header(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_accept_enter(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_accept_make_char(struct LoginMode *self, const struct CharInfo *chara);
void login_mode_on_char_list_page(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_char_list(struct LoginMode *self, const BYTE *pkt);

void login_mode_on_select_wnd_result(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_select_wnd_result2(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_delete_result(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_delete_reserved(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_pin_state(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_pin_state2(struct LoginMode *self, const BYTE *pkt);

void login_mode_refuse_login(struct LoginMode *self, int code, const char *text);
void login_mode_on_refuse_byte(struct LoginMode *self, const BYTE *pkt);

void login_mode_on_play_time(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_disconnect_reason(struct LoginMode *self, const BYTE *pkt);

void login_mode_on_auth_result(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_auth_result2(struct LoginMode *self, const BYTE *pkt);

void login_mode_enter_zone(struct LoginMode *self, DWORD gid, struct String mapName, DWORD ip, DWORD port,
                          struct String host);
void login_mode_on_zone_accept(struct LoginMode *self, const BYTE *pkt);
void login_mode_on_zone_accept2(const BYTE *pkt);

BYTE login_mode_add_notice(struct LoginMode *self, int msg, BYTE append);

struct LoginConnEntry *login_conn_entry_construct(struct LoginConnEntry *self);
struct LoginConnEntry *login_conn_entry_copy(struct LoginConnEntry *self, const struct LoginConnEntry *src);
void login_conn_entry_destruct(struct LoginConnEntry *self);
void login_conn_entries_clear(struct Vec *v);
void login_conn_entries_free(struct Vec *v);

#endif
