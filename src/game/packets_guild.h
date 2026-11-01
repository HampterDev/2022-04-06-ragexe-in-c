#ifndef GAMEMODE_PACKETS_GUILD_H
#define GAMEMODE_PACKETS_GUILD_H

#include "game/packet_handlers.h"
#include "actor/actor_ops.h"
#include "contents/player_names.h"
#include "ui/guild_windows.h"
#include "contents/guild_info.h"
#include "game/objects.h"

struct PACKET_ZC_REQ_GUILD_INVITE;
struct PACKET_ZC_RESULT_MAKE_GUILD;
struct PACKET_ZC_UPDATE_CHARSTAT;
struct PACKET_ZC_UPDATE_CHARSTAT2;
struct PACKET_ZC_UPDATE_GDID;

struct PacketMemberList;
struct PacketMemberFull;
struct PacketMemberMemo;
struct PacketMemberNamed;

struct PacketGuildInfo;
struct PacketGuildInfoNew;
struct PacketGuildNotice;
struct PacketGuildRelations;
struct PacketPositionNames;
struct PacketPositions;
struct PacketBanList;

struct GuildInfo;

static __inline void guild_member_record_destroy(struct GuildMember *record)
{
    string_free(&record->memo);
    string_free(&record->position_name);
    string_free(&record->name);
}

struct String *guild_position_title(int position_id);

#endif
