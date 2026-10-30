#ifndef GAMEMODE_PACKETS_CLAN_H
#define GAMEMODE_PACKETS_CLAN_H

#include "game/packet_handlers.h"
#include "session/state.h"
#include "game/objects.h"
#include "game/packets_guild.h"
#include "contents/clan_info.h"

struct PACKET_ZC_CLANINFO;
struct PACKET_ZC_NOTIFY_CLAN_CONNECTINFO;
struct PACKET_ZC_NOTIFY_CLAN_CHAT;
struct PACKET_ZC_GUILD_POSITION;

enum { CLAN_ALLY = 0, CLAN_ANTAGONIST = 1 };

void ui_window_mgr_refresh_clan_wnd(struct UIWindowMgr *mgr);

#endif
