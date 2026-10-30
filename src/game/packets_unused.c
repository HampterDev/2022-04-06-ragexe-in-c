#include "session/skill_commands.h"
#include "actor/actor_ops.h"
#include "game/actor_packets.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <windows.h>
#include "game/packets.h"
#include "contents/clan_info.h"
#include "session/session_jobs.h"
#include "core/app.h"

void game_mode_player_progress(struct GameMode *self, const struct PacketActorProgress *pkt)
{
    struct GameActor *player = (struct GameActor *)self->world->player;

    if (!player)
        return;
    actor_send(player, 0x52, pkt->seconds * 1000u, 0);
    g_session1.progressBarPending = 1;
}

void game_mode_ignore_packet(const unsigned char *pkt)
{
    (void)pkt;
}
