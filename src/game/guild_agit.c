#include "game/packet_handlers.h"
#include "contents/client_lua_calls.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "core/msg_str.h"
#include "session/session_jobs.h"

#pragma pack(push, 1)
struct PacketGuildAgitInfo {
    unsigned short packet_type;
    short packet_length;
    unsigned char agit_ids[1];
};
#pragma pack(pop)

void game_mode_on_guild_agit_info(struct GameMode *self, const struct PacketGuildAgitInfo *packet)
{
    const unsigned char *id = packet->agit_ids;
    int left = packet->packet_length - 4;

    (void)self;
    guild_info_clear_castles(&g_guildInfo);
    for (; left > 0; left--, id++)
        guild_info_add_castle(&g_guildInfo, *id);
}

#define CHAT_COLOR_AGIT 0x000000ff

void game_mode_on_req_ack_agit_investment(struct GameMode *self, unsigned char agit_id, int economy, int defense)
{
    const char *name;
    char text[256];

    (void)self;
    if (g_windowMgr.guild_info_manage_wnd == 0)
        return;
    name = 0;

    client_lua_call(g_session1.luaState, "get_agit_name", "d>s", (int)agit_id, &name);
    memset(text, 0, sizeof text);
    sprintf_s(text, 0x100, msg_str(0xe30), name, defense, 100, economy, 100);
    ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 0, 0, "Message", 0xfa, 0x82, 0);
}

void game_mode_on_req_ack_move_guild_agit(struct GameMode *self, short result)
{
    int msg;

    (void)self;
    if (result == 1)
        msg = 0x442;
    else if (result == 2)
        msg = 0xe31;
    else
        return;
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)msg_str(msg), CHAT_COLOR_AGIT, 0, 0);
}

void game_mode_on_change_agit_owner(struct GameMode *self, unsigned char agit_id, int old_guild_id,
                                           int new_guild_id)
{
    (void)self;
    if (session_is_own_guild_id(&g_session1, old_guild_id) == 1)
        guild_info_remove_castle(&g_guildInfo, agit_id);
    else if (session_is_own_guild_id(&g_session1, new_guild_id) == 0)
        guild_info_add_castle(&g_guildInfo, agit_id);
    if (g_windowMgr.guild_info_manage_wnd != 0)
        ui_window_send(g_windowMgr.guild_info_manage_wnd, NULL, 0x3c, 0, 0, 0, 0);
}
