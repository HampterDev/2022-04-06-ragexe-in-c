#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include "login/loginmode.h"
#include "game/connection.h"

#include <string.h>
#include "platform/language.h"
#include "core/msg_str.h"

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
    case 6:   id = 3; break;
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
        ui_window_mgr_message_box(&g_windowMgr, line, 0, 1, 1, 0, "메시지", 0x118, 0x78, 0);
    }
    if (g_session1.canBackToLogin == 1)
        self->base.next_sub_mode = 3;
    else
        CMODE_SENDMSG(client_current_mode(), 2, 0, 0, 0, 0);
}
