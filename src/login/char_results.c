#include "contents/connection_api.h"
#include "ui/window_manager_state.h"
#include "login/loginmode.h"

#include <string.h>
#include <stdio.h>
#include "core/msg_str.h"
#include "core/text_util.h"
#include "platform/clock.h"

#define SUBMODE_LOGIN           3
#define SUBMODE_SELECT_CHAR     7

/* HC_SECOND_PASSWD_LOGIN states that need no PIN code: accepted/disabled and unused. */
#define PIN_STATE_OK            0
#define PIN_STATE_UNUSED        7
#define PIN_STATE_LAST_KNOWN    0xb

#pragma pack(push, 1)
struct PACKET_HC_SELECT_WND_RESULT {
    WORD packet_type;
    DWORD result;
};
struct PACKET_HC_SELECT_WND_RESULT2 {
    WORD packet_type;
    short result;
};
struct PACKET_HC_DELETE_CHAR3 {
    WORD packet_type;
    DWORD gid;
    DWORD result;
};
struct PACKET_HC_DELETE_CHAR3_RESERVED {
    WORD packet_type;
    DWORD gid;
    DWORD result;
    DWORD delete_delay;
};
struct PACKET_HC_SECOND_PASSWD_LOGIN {
    WORD packet_type;
    DWORD seed;
    DWORD aid;
    WORD state;
    BYTE tries_left;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_HC_SELECT_WND_RESULT, 6);
SIZE_ASSERT(PACKET_HC_SELECT_WND_RESULT2, 4);
SIZE_ASSERT(PACKET_HC_DELETE_CHAR3, 10);
SIZE_ASSERT(PACKET_HC_DELETE_CHAR3_RESERVED, 14);
SIZE_ASSERT(PACKET_HC_SECOND_PASSWD_LOGIN, 13);

static int result_box(const char *text)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
}

static int confirm_box(const char *text)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 1, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
}

static void select_wnd_msg(void *wnd, int msg)
{
    if (wnd)
        UIWND_SENDMSG(wnd, 0, 6, msg, 0, 0, 0);
}

void login_mode_on_select_wnd_result(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_SELECT_WND_RESULT *packet = (const struct PACKET_HC_SELECT_WND_RESULT *)pkt;
    void *wnd = g_windowMgr.new_select_char_wnd;

    switch (packet->result) {
    case 0:
        result_box(msg_str(0x4be));
        select_wnd_msg(wnd, 0x158);
        return;
    case 1:  if (confirm_box(msg_str(0x4c2)) == 0xb0) goto close_change_name_wnd; return;
    case 3:  if (confirm_box(msg_str(0x4bf)) == 0xb0) goto close_change_name_wnd; return;
    case 5:  if (confirm_box(msg_str(0x7e6)) == 0xb0) goto close_change_name_wnd; return;
    case 6:  if (confirm_box(msg_str(0x7e7)) == 0xb0) goto close_change_name_wnd; return;
    case 10: if (confirm_box(msg_str(0x7e8)) == 0xb0) goto close_change_name_wnd; return;
    case 2:
        if (confirm_box(msg_str(0x4c3)) == 0xb0)
            select_wnd_msg(wnd, 0xb1);
        return;
    case 4:  if (confirm_box(msg_str(0x4c4)) == 0xb0) select_wnd_msg(wnd, 0x157); return;
    case 7:  if (confirm_box(msg_str(0x7eb)) == 0xb0) select_wnd_msg(wnd, 0x157); return;
    case 8:  if (confirm_box(msg_str(0x7ec)) == 0xb0) select_wnd_msg(wnd, 0x157); return;
    case 9:  if (confirm_box(msg_str(0x7ed)) == 0xb0) select_wnd_msg(wnd, 0x157); return;
    default:
        return;
    }
close_change_name_wnd:
    if (g_windowMgr.change_name_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, 0x80);
}

void login_mode_on_select_wnd_result2(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_SELECT_WND_RESULT2 *packet = (const struct PACKET_HC_SELECT_WND_RESULT2 *)pkt;
    void *wnd = g_windowMgr.new_select_char_wnd;
    int msg;

    switch (packet->result) {
    case 0:
        result_box(msg_str(0x4be));
        select_wnd_msg(wnd, 0x158);
        return;
    case 3: msg = 0x4bf; goto confirm_change_name;
    case 4: msg = 0x4c4; goto confirm_change_name;
    case 1: msg = 0x4c2; break;
    case 2: msg = 0x4c3; break;
    case 5: msg = 0x7e6; break;
    case 6: msg = 0x7e7; break;
    case 10: msg = 0x7e8; break;
    default:
        return;
    }
    if (confirm_box(msg_str(msg)) == 0xb0)
        select_wnd_msg(wnd, 0xb1);
    return;
confirm_change_name:
    if (confirm_box(msg_str(msg)) == 0xb0)
        select_wnd_msg(wnd, 0x157);
}

static void show_result_string(struct String *s)
{
    if (s->size)
        result_box(string_cstr(s));
}

static void assign_msg(struct String *s, int id)
{
    const char *text = msg_str(id);
    string_assign_cstr(s, text);
}

void login_mode_on_delete_result(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_DELETE_CHAR3 *packet = (const struct PACKET_HC_DELETE_CHAR3 *)pkt;
    struct String text;

    string_init(&text);
    switch (packet->result) {
    case 0: assign_msg(&text, 0x718); break;
    case 1: self->char_deleted = 1; break;
    case 2: assign_msg(&text, 0x71c); break;
    case 3: assign_msg(&text, 0x719); break;
    case 4: assign_msg(&text, 0x71d); break;
    case 5: assign_msg(&text, 0x71e); break;
    case 6: assign_msg(&text, 0x941); break;
    case 7: assign_msg(&text, 0x12d); break;
    case 8: assign_msg(&text, 0xbe4); break;
    default: assign_msg(&text, 0x718); break;
    }
    show_result_string(&text);
    if (self->char_deleted != 1)
        self->base.next_sub_mode = SUBMODE_SELECT_CHAR;
    string_free(&text);
}

void login_mode_on_delete_reserved(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_DELETE_CHAR3_RESERVED *packet = (const struct PACKET_HC_DELETE_CHAR3_RESERVED *)pkt;
    struct String text;
    int32_t now;
    int i;

    string_init(&text);
    switch (packet->result) {
    case 0:
        assign_msg(&text, 0x718);
        break;
    case 1:
        for (i = 0; i < self->numChar; i++) {
            if (self->chars[i].gid == packet->gid) {
                now = platform_time_now();
                self->chars[i].delete_date = packet->delete_delay + now;
                break;
            }
        }
        self->base.next_sub_mode = SUBMODE_SELECT_CHAR;
        break;
    case 2:
        break;
    case 3: assign_msg(&text, 0x719); break;
    case 4: assign_msg(&text, 0x71a); break;
    case 5: assign_msg(&text, 0x71b); break;
    case 6: assign_msg(&text, 0xba5); break;
    case 7: assign_msg(&text, 0xc14); break;
    default:
        assign_msg(&text, 0x718);
        break;
    }
    show_result_string(&text);
    string_free(&text);
}

/* PIN codes are unsupported: continue only when the server requires none, otherwise leave. */
void login_mode_on_pin_state(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_SECOND_PASSWD_LOGIN *packet = (const struct PACKET_HC_SECOND_PASSWD_LOGIN *)pkt;
    WORD state = packet->state;

    if (state > PIN_STATE_LAST_KNOWN)
        return;
    if (state == PIN_STATE_OK || state == PIN_STATE_UNUSED) {
        self->base.next_sub_mode = SUBMODE_SELECT_CHAR;
        return;
    }
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    result_box("PIN codes are not supported by this client. "
               "Please ask the server administrator to disable PIN codes.");
    g_mapServerAidPending = 0;
    self->base.next_sub_mode = SUBMODE_LOGIN;
}
