#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"
#include "game/connection.h"

#include <string.h>
#include <shellapi.h>
#include "core/app.h"
#include "core/msg_str.h"

#pragma pack(push, 1)
struct PACKET_AC_PLAY_TIME {
    WORD packet_type;
    DWORD first;
    DWORD second;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_AC_PLAY_TIME, 10);

void login_mode_on_play_time(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_AC_PLAY_TIME *packet = (const struct PACKET_AC_PLAY_TIME *)pkt;
    char text[0x100];
    DWORD a = packet->first;
    DWORD b;
    size_t len;

    if (a == 0xfffffffe) {
        if (packet->second == a) {
            sprintf(text, "%s\n", msg_str(0x2a8));
            goto show;
        }
    } else if (a == 0xfffffffd) {
        if (packet->second == a) {
            sprintf(text, "%s\n", msg_str(0x41c));
            goto show;
        }
    } else if (a == 0xffffffff) {
        if (packet->second == a) {
            sprintf(text, "%s\n", msg_str(0x2d5));
            goto show;
        }
    } else if (a == 0) {
        b = packet->second;
        if (b > 0) {
            sprintf(text, "%s\n", msg_str(0x21d));
            len = strlen(text);
            sprintf(text + len, msg_str(0x2a7), b / 60, b % 60);
        } else {
            sprintf(text, "\n");
        }
        goto show;
    }

    b = packet->second;
    if (b != 0) {
        sprintf(text, msg_str(0x2d6), a / 1440, a / 60 % 24, a % 60, b / 60, b % 60);
    } else {
        sprintf(text, "%s\n", msg_str(0x215));
        len = strlen(text);
        a = packet->first;
        sprintf(text + len, msg_str(0x2a5), a / 1440, a / 60 % 24, a % 60);
    }

show:
    self->stateTick = timeGetTime() + 90000;
    ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 1, 0, "Message", 0x118, 0x78, 0);
}

static void open_account_page(void)
{
    switch (g_serviceType) {
    case 0:
        ShellExecuteA(g_hMainWnd, "open", msg_str(g_session1.channelCopId == 1 ? 0x51c : 0x210), NULL, NULL, SW_SHOWNORMAL);
        return;
    case 1:
    case 2:
    case 3:
        return;
    default:
        ShellExecuteA(g_hMainWnd, "open", msg_str(0x210), NULL, NULL, SW_SHOWNORMAL);
        return;
    }
}

void login_mode_on_disconnect_reason(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_SC_NOTIFY_BAN *packet = (const struct PACKET_SC_NOTIFY_BAN *)pkt;
    char text[0x100];
    char line[0x100];
    char tmp[200];
    int id;

    text[0] = 0;
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    switch (packet->code) {
    case 0:   id = 3; break;
    case 1:   id = 4; break;
    case 2:   id = 5; break;
    case 3:   id = 0xf1; break;
    case 4:   id = 0x108; break;
    case 5:   id = 0x131; break;
    case 6:
        strcpy(text, msg_str(0x1b6));
        if (ui_window_mgr_message_box(&g_windowMgr, text, 2, 1, 1, 0, "Message", 0x118, 0x78, 0) == 0xb3) {
            mode_mgr_quit(&g_modeMgr);
            open_account_page();
        }
        text[0] = 0;
        goto show;
    case 7:   id = 0x1b7; break;
    case 8:   id = 0x1b8; break;
    case 9:   id = 0x211; break;
    case 10:  id = 0x212; break;
    case 11:  id = 0x23f; break;
    case 12:  id = 0x240; break;
    case 13:  id = 0x241; break;
    case 14:  id = 0x242; break;
    case 16:  id = 0x25e; break;
    case 17:  id = 0x25f; break;
    case 18:  id = 0x2a6; break;
    case 100: id = 0x463; break;
    case 101: id = 0x49a; break;
    case 102: id = 0x49b; break;
    case 103: id = 0x51d; break;
    case 104: id = 0x51e; break;
    case 105: id = 0x51f; break;
    case 107:
        if (g_serviceType == 0) {
            sprintf(tmp, msg_str(0x595), 9);
            strcpy(text, tmp);
        }
        goto show;
    case 108: id = 0x62e; break;
    case 109: id = 0x62f; break;
    case 110: id = 0x635; break;
    case 113: id = 0x7e5; break;
    case 114: id = 0x928; break;
    case 116: id = 0xe33; break;
    default:  id = 3; break;
    }
    strcpy(text, msg_str(id));

show:
    if (strlen(text) != 0) {
        sprintf(line, "%s(%d)", text, packet->code);
        ui_window_mgr_message_box(&g_windowMgr, line, 0, 1, 1, 0, "Message", 0x118, 0x78, 0);
    }
    if (g_session1.canBackToLogin == 1)
        self->base.next_sub_mode = 3;
    else
        CMODE_SENDMSG(client_current_mode(), 2, 0, 0, 0, 0);
}
