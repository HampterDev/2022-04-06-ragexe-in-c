#include "core/thread_text.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "core/app.h"
#include "contents/md5ctx.h"
#include "core/msg_str.h"
#include "session/session_talk.h"
#include "ui/character/character.h"

BYTE g_wrongPasswordCount = 0;

#define SUBMODE_SERVER_SELECT   6

int login_mode_client_type(void)
{
    static const BYTE byService1[20] = {
        0x0e, 0x09, 0x04, 0x05, 0x07, 0x0d, 0x12, 0x10, 0x11, 0x14,
        0x15, 0x16, 0x17, 0x19, 0x21, 0x02, 0x02, 0x02, 0x23, 0x24,
    };
    static const BYTE byServiceOther[20] = {
        0x01, 0x03, 0x04, 0x05, 0x07, 0x0c, 0x0f, 0x10, 0x11, 0x14,
        0x15, 0x16, 0x17, 0x19, 0x1a, 0x00, 0x1e, 0x1f, 0x23, 0x24,
    };
    unsigned st;

    if (g_serverType == 1) {
        st = (unsigned)g_serviceType - 1;
        return st > 0x13 ? 2 : byService1[st];
    }
    if (g_serverType == 5) {
        switch (g_serviceType) {
        case 2:  return 0x1c;
        case 5:  return 0x1d;
        case 6:  return 0x22;
        case 7:  return 0x1b;
        case 15: return 0x20;
        default: return 0;
        }
    }
    st = (unsigned)g_serviceType - 1;
    return st > 0x13 ? 0 : byServiceOther[st];
}

#pragma pack(push, 1)
struct PACKET_AC_ACK_HASH {
    WORD packet_type;
    short packet_length;
    BYTE key[];
};
struct PACKET_AC_ACCEPT_LOGIN_HEAD {
    WORD packet_type;
    WORD packet_length;
    DWORD auth_code;
    DWORD aid;
    DWORD user_level;
    DWORD last_login_ip;
    char last_login_time[26];
    BYTE sex;
};
struct PACKET_AC_ACCEPT_LOGIN {
    struct PACKET_AC_ACCEPT_LOGIN_HEAD head;
    BYTE servers[];
};
struct PACKET_AC_ACCEPT_LOGIN_KEYED {
    struct PACKET_AC_ACCEPT_LOGIN_HEAD head;
    BYTE session_key[16];
    BYTE session_flag;
    BYTE servers[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_AC_ACK_HASH, 4);
SIZE_ASSERT(PACKET_AC_ACCEPT_LOGIN_HEAD, 0x2f);
SIZE_ASSERT(PACKET_AC_ACCEPT_LOGIN, 0x2f);
SIZE_ASSERT(PACKET_AC_ACCEPT_LOGIN_KEYED, 0x40);

void login_mode_on_hash_key(struct LoginMode *self, const BYTE *pkt)
{
#pragma pack(push, 1)
    struct {
        WORD id;
        DWORD version;
        char userId[24];
        BYTE digest[16];
        BYTE clientType;
        BYTE clientInfo;
    } login;
    struct {
        WORD id;
        DWORD version;
        char userId[24];
        BYTE digest[16];
        BYTE clientType;
        BYTE unset[13];
    } login4;
#pragma pack(pop)
    const struct PACKET_AC_ACK_HASH *packet = (const struct PACKET_AC_ACK_HASH *)pkt;
    struct MD5Context md5;
    int keyLen = packet->packet_length - 4;
    const BYTE *key = packet->key;
    const char *password = self->password;
    void *wnd;

    md5ctx_init(&md5);
    if (g_passwordEncrypt2) {
        md5ctx_update(&md5, password, strlen(password));
        md5ctx_update(&md5, key, keyLen);
    } else {
        md5ctx_update(&md5, key, keyLen);
        md5ctx_update(&md5, password, strlen(password));
    }

    if (g_serviceType == 5) {
        login.id = 0x1fa;
        login.version = g_clientVersion;
        memcpy(login.userId, self->userId, 24);
        md5ctx_final(&md5, login.digest);
        login.clientType = (BYTE)login_mode_client_type();
        login.clientInfo = g_session1.pcBang ? g_session1.connectionIndex + 10 : g_session1.connectionIndex;
        login_send_packet(login.id, &login);
    } else if (g_serviceType == 2) {
        login4.id = 0x27c;
        login4.version = g_clientVersion;
        memcpy(login4.userId, self->userId, 24);
        md5ctx_final(&md5, login4.digest);
        login4.clientType = (BYTE)login_mode_client_type();
        login_send_packet(login4.id, &login4);
    } else {
        login.id = 0x1dd;
        login.version = g_clientVersion;
        memcpy(login.userId, self->userId, 24);
        md5ctx_final(&md5, login.digest);
        login.clientType = (BYTE)login_mode_client_type();
        login_send_packet(login.id, &login);
    }

    wnd = ui_window_open(6);
    ui_wait_wnd_set_text(wnd, msg_str(0x7a));
}

void login_mode_on_accept_login(struct LoginMode *self, const BYTE *pkt, int form)
{
    const struct PACKET_AC_ACCEPT_LOGIN *packet = (const struct PACKET_AC_ACCEPT_LOGIN *)pkt;
    const struct PACKET_AC_ACCEPT_LOGIN_KEYED *keyed = (const struct PACKET_AC_ACCEPT_LOGIN_KEYED *)pkt;
    const char *text;
    unsigned len, n;
    BYTE sex;
    BYTE textType;

    g_session1.loginTick = timeGetTime();
    text = msg_str(0x3ac);
    string_assign_cstr(&g_session1.autoMessage, text);
    g_session1.authCode = packet->head.auth_code;
    g_session1.userLevel = packet->head.user_level;
    g_session1.ownAid = packet->head.aid;

    sex = packet->head.sex;
    if (sex >= 10) {
        session_set_sex(&g_session1, sex - 10);
        g_session1.accountSex = packet->head.sex - 10;
    } else {
        session_set_sex(&g_session1, sex);
        g_session1.accountSex = packet->head.sex;
    }

    if (g_serviceType == 6 || g_serviceType == 0xc || g_serviceType == 0xa) {
        g_session1.showName = 0;
        textType = 0;
    } else {
        textType = g_session1.showName;
    }
    session_set_text_type(textType, textType);
    g_session1.lastLoginIp = packet->head.last_login_ip;
    memcpy_s(g_session1.lastLoginTime, 26, packet->head.last_login_time, 26);
    g_session1.lastLoginTime[26] = 0;

    len = packet->head.packet_length;
    switch (form) {
    case 4:
        n = (len - 0x2f) / 160;
        self->numServer = n;
        memcpy(self->servers, packet->servers, n * 160);
        break;
    case 5:
        n = (len - 0x40) / 160;
        self->numServer = n;
        memcpy(self->servers, keyed->servers, n * 160);
        memcpy(g_session1.webAuthToken, keyed->session_key, 16);
        g_session1.snsAuthorised = keyed->session_flag;
        break;
    case 6:
        break;
    }

    gameguard_set_account(g_gameGuard);
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    g_wrongPasswordCount = 0;
    self->base.next_sub_mode = SUBMODE_SERVER_SELECT;
}

void login_mode_send_exe_digest(struct LoginMode *self)
{
#pragma pack(push, 1)
    struct { WORD id; BYTE digest[16]; } pkt;
#pragma pack(pop)
    struct MD5Context md5;
    char path[MAX_PATH];
    BYTE chunk[0x400];
    FILE *f;

    pkt.id = 0x204;
    md5ctx_init(&md5);
    GetModuleFileNameA(g_hInstance, path, MAX_PATH);
    f = fopen(path, "rb");
    if (!f)
        return;
    while (!feof(f)) {
        size_t got = fread(chunk, 1, sizeof chunk, f);
        md5ctx_update(&md5, chunk, got);
    }
    md5ctx_final(&md5, pkt.digest);
    login_send_packet((short)pkt.id, &pkt);
}
