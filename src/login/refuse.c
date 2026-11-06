#include "core/thread_text.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <shellapi.h>
#include "core/app.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "ui/ui_std.h"

static void set_msg(struct String *s, int id)
{
    string_assign_cstr(s, msg_str(id));
}

static void set_formatted(struct String *s, int id, const char *text)
{
    struct String tmp;

    string_init(&tmp);
    string_format(&tmp, msg_str(id), text);
    string_assign(s, string_cstr(&tmp), tmp.size);
    string_free(&tmp);
}

static void open_page(const char *url)
{
    ShellExecuteA(g_hMainWnd, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

static int refuse_box(const struct String *s, int a, int c)
{
    return ui_window_mgr_message_box(&g_windowMgr, string_cstr(s), a, 1, c, 0, "Message", 0x118, 0x78, 0);
}

void login_mode_refuse_login(struct LoginMode *self, int code, const char *text)
{
    struct String s;
    struct String tmp;
    const char *m;

    string_init(&s);
    if (code != 0x12)
        connection_disconnect(rag_connection_transport(rag_connection_instance()));

    if (code > 0x1393) {
        if (code == 0x14b4) {
            m = msg_str(0xdce);
            string_assign_cstr(&s, m);
            goto show;
        }
        if (code > 0x14b4) {
            if (code != 0x14b5)
                goto unknown;
            m = msg_str(0xdd3);
            string_assign_cstr(&s, m);
            goto show;
        }
        switch (code) {
        case 0x1394: set_formatted(&s, 0x337, text); break;
        case 0x1395:
        case 0x13be: set_msg(&s, 0x6fd); break;
        case 0x1396: case 0x1398: case 0x139b: case 0x139d:
        case 0x13bc: case 0x13bd:
        case 0x13bf: case 0x13c0: case 0x13c1: case 0x13c2:
        case 0x13c3: case 0x13c4: case 0x13c5:
                     set_msg(&s, 0x72e); break;
        case 0x1397: set_msg(&s, 0x730); break;
        case 0x1399: set_msg(&s, 0x731); break;
        case 0x139a: set_msg(&s, 0x6fa); break;
        case 0x13ba: set_msg(&s, 0x726); break;
        case 0x13bb: set_msg(&s, 5); break;
        case 0x13c6: set_msg(&s, 0x10a); break;
        case 0x13c7: set_msg(&s, 0x701); break;
        case 0x13c8: set_msg(&s, 0x702); break;
        default:     goto unknown;
        }
        goto show;
    }
    if (code == 0x1393) {
        m = msg_str(0x72f);
        string_assign_cstr(&s, m);
        goto show;
    }

    switch (code) {
    case 0:   set_msg(&s, 6); break;
    case 1:
        if (g_serviceType != 0 || (signed char)++g_wrongPasswordCount < 3) {
            set_msg(&s, 0x10b);
            break;
        }
        string_assign(&s,
            "비밀번호를 3회 잘못입력하였습니다. "
            "비밀번호를 확인해주세요.              "
            "클라이언트 실행을 종료합니다.", 0x66);
        if (refuse_box(&s, 0, 1) == 0xb0)
            mode_mgr_quit(&g_modeMgr);
        string_clear(&s);
        break;
    case 2:   set_msg(&s, 8); break;
    case 4:   set_msg(&s, 0x10a); break;
    case 5:   set_msg(&s, 0x136); break;
    case 6:   set_formatted(&s, 0x1c1, text); break;
    case 7:   set_msg(&s, 0x1b7); break;
    case 8:   set_msg(&s, 0x2a9); break;
    case 9:  case 10: case 11: case 12: case 13: case 14: case 15:
              set_msg(&s, 0x2bf + (code - 9)); break;
    case 16:
        if (g_serviceType != 0)
            break;
        string_assign(&s,
            "비밀 번호 변경 후, 다시 접속 해주시기 바랍니다.", 0x2f);
        if (refuse_box(&s, 0, 1) == 0xb0) {
            open_page("http://www.ragnarok.co.kr/setNewPW/password_change_agreement.asp");
            mode_mgr_quit(&g_modeMgr);
        }
        string_clear(&s);
        break;
    case 17:
        set_formatted(&s, 0x564, text);
        if (refuse_box(&s, 2, 1) == 0xb3)
            mode_mgr_quit(&g_modeMgr);
        string_clear(&s);
        break;
    case 18:  set_msg(&s, 0x565); break;
    case 19:  set_msg(&s, 0x6d7); break;
    case 20:  set_msg(&s, 0x6d8); break;
    case 21:  set_msg(&s, 0x6fa); break;
    case 22: case 23: case 24: case 25: case 26: case 27: case 30:
              set_msg(&s, 0xd70); break;
    case 28:  set_msg(&s, 0xaab); break;
    case 29:  set_msg(&s, 0xaac); break;
    case 33:  set_msg(&s, 0xa7a); break;
    case 36: {
        int left = (int)self->refuse_seconds_left;
        int hours = left / 3600;
        int minutes = (int)(float)ceil((double)((float)(left % 3600) / 60.0f));

        string_init(&tmp);
        string_format(&tmp, msg_str(0xd7d), hours, minutes);
        string_assign(&s, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        break;
    }
    case 99:  set_msg(&s, 0x170); break;
    case 100: set_formatted(&s, 0x329, text); break;
    case 101: set_formatted(&s, 0x32a, text); break;
    case 102: set_msg(&s, 0x32b); break;
    case 103: set_msg(&s, 0x35b); break;
    case 104: set_msg(&s, 0x35c); break;
    case 105: set_msg(&s, 0x55c); break;
    case 106: set_msg(&s, 0x50d); break;
    case 108: set_msg(&s, 0x571); break;
    case 109: set_msg(&s, 0x572); break;
    case 110:
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(0x929), 0, 1, 1, 0, "Message", 0x118, 0x78, 0) == 0xb0) {
            open_page(msg_str(0x92a));
            mode_mgr_quit(&g_modeMgr);
        }
        string_clear(&s);
        break;
    case 112: set_msg(&s, 0x96a); break;
    case 242: set_msg(&s, 0xd6f); break;
    case 243: set_msg(&s, 0xe67); break;
    default:
    unknown:
        m = msg_str(9);
        string_assign_cstr(&s, m);
        string_init(&tmp);
        string_format(&tmp, "(%d)", code);
        string_append(&s, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        break;
    }

show:
    if (s.size) {
        refuse_box(&s, 0, 0);
        if (code == 5 || code == 0x63)
            login_mode_send_msg(&self->base, 2, 0, 0, 0, 0);
    }
    if (g_session1.canBackToLogin == 1)
        self->base.next_sub_mode = 3;
    else
        mode_mgr_quit(&g_modeMgr);
    string_free(&s);
}

#pragma pack(push, 1)
struct PACKET_AC_REFUSE_LOGIN_BYTE {
    WORD packet_type;
    BYTE code;
    char name[0x14];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_AC_REFUSE_LOGIN_BYTE, 23);

void login_mode_on_refuse_byte(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_AC_REFUSE_LOGIN_BYTE *packet = (const struct PACKET_AC_REFUSE_LOGIN_BYTE *)pkt;
    char text[0x104];
    char name[21];
    int id;

    memset(text, 0, sizeof text);
    _snprintf_s(text, sizeof text, _TRUNCATE, "%s", msg_str(0xc82));
    switch (packet->code) {
    case 1:   id = 0xc7c; break;
    case 2:   id = 0xc7d; break;
    case 3:   id = 0xc7e; break;
    case 4:   id = 0xc7f; break;
    case 5:   id = 0xc80; break;
    case 6:   id = 0xc81; break;
    case 7:   id = 0xc8e; break;
    case 8:
    case 100:
    case 102: id = 0xc82; break;
    case 9:   id = 0xf47; break;
    case 101:
        memset(name, 0, sizeof name);
        memcpy_s(name, 0x14, packet->name, 0x14);
        _snprintf_s(text, sizeof text, _TRUNCATE, msg_str(0x1c1), name);
        goto show;
    default:  goto show;
    }
    _snprintf_s(text, sizeof text, _TRUNCATE, "%s", msg_str(id));
show:
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    ui_window_mgr_message_box(&g_windowMgr, thread_text_format("%s(%d)", text, packet->code), 0, 1, 0, 0, "Message", 0x118, 0x78, 0);
    g_mapServerAidPending = 0;
    self->base.next_sub_mode = 3;
}
