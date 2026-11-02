#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"

#include <string.h>
#include "core/msg_str.h"
#include "core/text_util.h"
#include "platform/clock.h"

static void reopen_wait_window(void)
{
    if (ui_window_find(0xb4))
        ui_window_mgr_post_quit1(&g_windowMgr, 0xb4);
    ui_window_open(0xb4);
}

static void assign_msg(struct String *s, int id)
{
    const char *m = msg_str(id);
    string_assign_cstr(s, m);
}

#pragma pack(push, 1)
struct PACKET_AC_AUTH_RESULT {
    WORD packet_type;
    WORD packet_length;
    int code;
    char text[0x14];
    BYTE token[];
};
struct PACKET_AC_AUTH_RESULT2 {
    WORD packet_type;
    WORD packet_length;
    int code;
    BYTE token[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_AC_AUTH_RESULT, 0x1c);
SIZE_ASSERT(PACKET_AC_AUTH_RESULT2, 8);

static void keep_token(const BYTE *pkt, int header)
{
    const struct PACKET_AC_AUTH_RESULT2 *packet = (const struct PACKET_AC_AUTH_RESULT2 *)pkt;
    size_t len = (size_t)(packet->packet_length - header);
    int i;

    for (i = 0; i < 0x1f4; i++)
        g_session1.ssoToken[i] = 0;
    memcpy_s(g_session1.ssoToken, 0x1f4, pkt + header, len > 0x1f4 ? 0x1f4 : len);
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    CMODE_SENDMSG(client_current_mode(), 0x2710, 0, 0, 0, 0);
}

static void after_message(struct LoginMode *self, int code)
{
    if (self->auth_timed_out == 1)
        return;
    if (code == 500) {
        self->stateTick = platform_ticks_ms();
        reopen_wait_window();
        return;
    }
    if (ui_window_find(0xb4))
        ui_window_mgr_post_quit1(&g_windowMgr, 0xb4);
    self->auth_answered = 0;
    self->auth_waiting = 0;
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    self->stateTick = platform_ticks_ms();
    self->base.next_sub_mode = 3;
}

void login_mode_on_auth_result(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_AC_AUTH_RESULT *packet = (const struct PACKET_AC_AUTH_RESULT *)pkt;
    struct String s, tmp;
    int code;

    string_init(&s);
    self->auth_answered = 1;
    string_clear(&s);
    switch (packet->code) {
    case 500: assign_msg(&s, 0x6d8); break;
    case 400:
        reopen_wait_window();
        self->auth_waiting = 1;
        self->stateTick = platform_ticks_ms();
        break;
    case 0:   keep_token(pkt, offsetof(struct PACKET_AC_AUTH_RESULT, token)); break;
    case 600: assign_msg(&s, 7); break;
    case 700: assign_msg(&s, 0xcb4); break;
    case 900:
        error_msg(msg_str(0xd86), 0);
        after_message(self, packet->code);
        break;
    default:  assign_msg(&s, 0x718); break;
    }

    if (s.size) {
        code = packet->code;
        string_init(&tmp);
        string_format(&tmp, "(%d)", code);
        string_append(&s, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        string_format(&tmp, " [%s]", packet->text);
        string_append(&s, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        ui_window_mgr_message_box(&g_windowMgr, string_cstr(&s), 0, 1, 1, 0, "메시지", 0x118, 0x78, 0);
        after_message(self, code);
    }
    string_free(&s);
}

void login_mode_on_auth_result2(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_AC_AUTH_RESULT2 *packet = (const struct PACKET_AC_AUTH_RESULT2 *)pkt;
    struct String s, tmp;
    int code;

    string_init(&s);
    self->auth_answered = 1;
    string_clear(&s);
    switch (packet->code) {
    case 500: assign_msg(&s, 0x6d8); break;
    case 400:
        reopen_wait_window();
        self->auth_waiting = 1;
        self->stateTick = platform_ticks_ms();
        break;
    case 0:   keep_token(pkt, offsetof(struct PACKET_AC_AUTH_RESULT2, token)); break;
    case 600: assign_msg(&s, 7); break;
    case 700: assign_msg(&s, 0xcb4); break;
    case 800: assign_msg(&s, 0xd70); break;
    default:  assign_msg(&s, 0x718); break;
    }

    if (s.size) {
        code = packet->code;
        if (code == 800) {
            ui_window_mgr_message_box(&g_windowMgr, string_cstr(&s), 0, 1, 1, 0, "메세지", 0x118, 0x78, 1);
        } else {
            string_init(&tmp);
            string_format(&tmp, "(%d)", code);
            string_append(&s, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            ui_window_mgr_message_box(&g_windowMgr, string_cstr(&s), 0, 1, 1, 0, "메시지", 0x118, 0x78, 0);
        }
        after_message(self, code);
    }
    string_free(&s);
}
