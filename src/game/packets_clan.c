#include "game/packet_handlers.h"
#include "contents/manner_filter.h"
#include "session/state.h"
#include "ui/window_access.h"
#include <stdio.h>
#include <string.h>
#include "game/packets_clan.h"
#include "actor/game_actor.h"
#include "contents/clan_info.h"

#pragma pack(push, 1)
struct PACKET_ZC_CLANINFO {
    unsigned short id;
    unsigned short length;
    unsigned clan_id;
    char name[24];
    char master[24];
    char map_name[16];
    unsigned char ally_count;
    unsigned char antagonist_count;
    char names[1][24];
};
struct PACKET_ZC_NOTIFY_CLAN_CONNECTINFO {
    unsigned short id;
    unsigned short online;
    unsigned short total;
};
struct PACKET_ZC_NOTIFY_CLAN_CHAT {
    unsigned short id;
    short length;
    char sender[24];
    char text[1];
};
struct PACKET_ZC_GUILD_POSITION {
    unsigned short id;
    unsigned short length;
    unsigned aid;
    char position[1];
};
#pragma pack(pop)
STATIC_ASSERT(claninfo_names, offsetof(struct PACKET_ZC_CLANINFO, names) == 0x4a);

#define CLAN_CHAT_MAX 0xdc
#define CLAN_CHAT_COLOR 0xb4b4ff
#define WID_DRESSROOM 0x103

static void set_own_title_to_clan_name(struct GameMode *self)
{
    struct String title;

    string_copy(&title, &g_clanInfo->name);
    name_card_holder_set_position(&self->actor_names, ((unsigned)g_session1.ownAid), title);
}

void game_mode_zc_ack_clan_leave(struct GameMode *self)
{
    struct Player *player;

    clan_info_clear(g_clanInfo);
    set_own_title_to_clan_name(self);
    player = self->world->player;
    if (player != 0)
        game_actor_set_clan((struct GameActor *)player, 0, 0);
    ui_window_mgr_refresh_clan_wnd(&g_windowMgr);
}

static void add_related_clan(struct ClanInfo *clan, int relation, const char *name24)
{
    char name[24];
    struct String entry_name;
    struct ClanRelation entry;

    memset(name, 0, sizeof name);
    _snprintf_s(name, sizeof name, (size_t)-1, "%s", name24);
    string_init(&entry_name);
    string_assign_cstr(&entry_name, name);
    entry.relation = relation;
    string_copy(&entry.name, &entry_name);
    list_push_back(&clan->relations, &entry, sizeof entry);
    string_free(&entry_name);
}

void game_mode_zc_clan_info(struct GameMode *self, const struct PACKET_ZC_CLANINFO *packet)
{
    struct ClanInfo *clan;
    char name[24], master[24], map_name[16];
    const char *related;
    int i;

    clan_info_clear(g_clanInfo);
    g_clanInfo->in_clan = 1;
    g_clanInfo->emblem = packet->clan_id;

    memset(name, 0, sizeof name);
    strcpy_s(name, sizeof name, packet->name);
    string_assign_cstr(&g_clanInfo->name, name);
    set_own_title_to_clan_name(self);

    memset(master, 0, sizeof master);
    strcpy_s(master, sizeof master, packet->master);
    string_assign_cstr(&g_clanInfo->master, master);

    memset(map_name, 0, sizeof map_name);
    strcpy_s(map_name, sizeof map_name, packet->map_name);
    string_assign_cstr(&g_clanInfo->map_name, map_name);

    related = packet->names[0];
    for (i = 0; i < packet->ally_count; i++, related += 24) {
        clan = g_clanInfo;
        add_related_clan(clan, CLAN_ALLY, related);
    }
    for (i = 0; i < packet->antagonist_count; i++, related += 24) {
        clan = g_clanInfo;
        add_related_clan(clan, CLAN_ANTAGONIST, related);
    }
    ui_window_mgr_refresh_clan_wnd(&g_windowMgr);
}

void game_mode_zc_dressroom_open(void)
{
    ui_window_open(WID_DRESSROOM);
}

void game_mode_zc_notify_clan_connect_info(const struct PACKET_ZC_NOTIFY_CLAN_CONNECTINFO *packet)
{
    g_clanInfo->online_count = packet->online;
    g_clanInfo->member_count = packet->total;
}

void game_mode_zc_notify_clan_chat(const struct PACKET_ZC_NOTIFY_CLAN_CHAT *packet)
{
    char text[CLAN_CHAT_MAX];

    if ((unsigned)(packet->length - 0x1c) > CLAN_CHAT_MAX)
        return;
    memset(text, 0, sizeof text);
    strcpy_s(text, sizeof text, packet->text);
    if (word_filter_contains(&g_nameFilter, text) == 1) {
        word_filter_censor(&g_nameFilter, text);
    }
    if (g_session1.hideChatAndGuild == 0)
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)text, CLAN_CHAT_COLOR, 0x14, 0);
}

void game_mode_zc_guild_position(struct GameMode *self, const struct PACKET_ZC_GUILD_POSITION *packet)
{
    struct String title;
    size_t length = packet->length - 8;

    string_init(&title);
    if (length != 0)
        string_assign(&title, packet->position, length);
    name_card_holder_set_position(&self->actor_names, packet->aid, title);
}
