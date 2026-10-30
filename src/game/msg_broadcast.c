#include "game/mode_msg.h"
#include "game/chat.h"
#include "ui/window_manager_state.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { BROADCAST_TEXT_LIMIT = 361, CZ_BROADCAST = 0x99, CZ_LOCALBROADCAST = 0x19c };

#pragma pack(push, 1)
struct BroadcastPacket {
    uint16_t id;
    uint16_t length;
    char text[BROADCAST_TEXT_LIMIT + 1];
};
#pragma pack(pop)

int game_mode_msg_broadcast(int kind)
{
    struct BroadcastPacket packet;
    const char *line, *prefix = "", *separator = "";

    if (kind < 0 || kind > 5)
        return 0;

    line = window_mgr_get_chat_line(&g_windowMgr);
    if (*line) {
        if (kind < 2) {
            prefix = session_character_name(&g_session1);
            separator = ": ";
        } else if (kind >= 4) {
            prefix = "blue";
        }
        snprintf(packet.text, sizeof packet.text, "%s%s%s", prefix, separator, line);
        packet.id = (kind & 1) ? CZ_LOCALBROADCAST : CZ_BROADCAST;
        packet.length = (uint16_t)(4 + strlen(packet.text) + 1);
        send_raw(packet.length, &packet);
    }
    window_mgr_clear_chat_line(&g_windowMgr);
    return 0;
}
