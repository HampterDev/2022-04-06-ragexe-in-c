#include "game/packet_handlers.h"
#include "actor/actor_ops.h"
#include "contents/player_names.h"
#include "std/string.h"
#include "session/character_name.h"
#include "contents/guild_emblem_api.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <string.h>
#include "game/packets_guild.h"
#include "actor/game_actor.h"
#include "core/msg_str.h"
#include "session/session_options.h"
#include "session/session_state.h"

#pragma pack(push, 1)
struct PACKET_ZC_REQ_GUILD_INVITE {
    unsigned short id;
    unsigned inviter;
    char guild_name[24];
};
struct PACKET_ZC_RESULT_MAKE_GUILD {
    unsigned short id;
    unsigned char result;
};
struct PACKET_ZC_UPDATE_CHARSTAT {
    unsigned short id;
    unsigned aid;
    unsigned gid;
    int status;
};
struct PACKET_ZC_UPDATE_CHARSTAT2 {
    unsigned short id;
    unsigned aid;
    unsigned gid;
    int status;
    unsigned short sex;
    unsigned short head;
    unsigned short head_palette;
};
struct PACKET_ZC_UPDATE_GDID {
    unsigned short id;
    unsigned gdid;
    unsigned emblem_version;
    unsigned rights;
    unsigned char is_master;
    unsigned inter_sid;
    char guild_name[24];
    unsigned extra;
};
#pragma pack(pop)
STATIC_ASSERT(gdid_extra, offsetof(struct PACKET_ZC_UPDATE_GDID, extra) == 0x2b);
STATIC_ASSERT(charstat2_palette, offsetof(struct PACKET_ZC_UPDATE_CHARSTAT2, head_palette) == 0x12);

enum {
    GUILD_RIGHT_INVITE = 0x01,
    GUILD_RIGHT_EXPEL = 0x10,
};

enum {
    WID_GUILD_JOIN_ACCEPT = 0x46,
    WID_GUILD_ALLY_ACCEPT = 0x48,
    UM_SET_GUILD_JOIN = 0x49,
    UM_SET_GUILD_ALLY = 0x4a,
};

enum {
    MM_GUILD_MEMBER_STATUS = 0x8c,
    MM_GUILD_MEMBER_LEFT = 0xab,
    MM_REQ_GUILD_INFO = 0x6b,
};

static void show_guild_invite(const struct PACKET_ZC_REQ_GUILD_INVITE *packet, int window_id, int message)
{
    char guild_name[25];

    struct UIWindow *window;

    memcpy(guild_name, packet->guild_name, 24);
    guild_name[24] = 0;
    window = ui_window_open(window_id);  ui_window_send(window, NULL, message, (int)packet->inviter, (intptr_t)guild_name, 0, 0);
}

void game_mode_zc_req_ally_guild(const struct PACKET_ZC_REQ_GUILD_INVITE *packet)
{
    show_guild_invite(packet, WID_GUILD_ALLY_ACCEPT, UM_SET_GUILD_ALLY);
}

void game_mode_zc_req_join_guild(const struct PACKET_ZC_REQ_GUILD_INVITE *packet)
{
    show_guild_invite(packet, WID_GUILD_JOIN_ACCEPT, UM_SET_GUILD_JOIN);
}

void game_mode_zc_result_make_guild(const struct PACKET_ZC_RESULT_MAKE_GUILD *packet)
{
    int msg;

    switch (packet->result) {
    case 0:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(0x176), CHAT_COLOR_CYAN, 4, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, 0xd6);
        ui_window_mgr_post_quit1(&g_windowMgr, 0xd5);
        ui_window_mgr_post_quit1(&g_windowMgr, 0xd4);
        return;
    case 1: msg = 0x177; break;
    case 2: msg = 0x178; break;
    case 3: msg = 0x195; break;
    case 4: msg = 0xa7f; break;
    default: return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(msg), CHAT_COLOR_YELLOW, 4, 0);
}

static void report_member_status(struct GameMode *self, unsigned aid, unsigned gid, int status)
{
    struct TextView name = player_name_lookup(gid);
    if (name.size != 0)
        mode_send_msg(self, MM_GUILD_MEMBER_STATUS, (intptr_t)name.data, status, 0, 0);
    if (status == 0)
        mode_send_msg(self, MM_GUILD_MEMBER_LEFT, (int)aid, 0, 0, 0);
}

void game_mode_zc_update_charstat2(struct GameMode *self, const struct PACKET_ZC_UPDATE_CHARSTAT2 *packet)
{
    guild_info_set_member_status(&g_guildInfo, packet->status, packet->gid,
                                 packet->sex, packet->head, packet->head_palette);
    report_member_status(self, packet->aid, packet->gid, packet->status);
}

void game_mode_zc_update_charstat(struct GameMode *self, const struct PACKET_ZC_UPDATE_CHARSTAT *packet)
{
    guild_info_set_member_online(&g_guildInfo, packet->aid, packet->status);
    report_member_status(self, packet->aid, packet->gid, packet->status);
}

static void update_guild_id(struct GameMode *self, const struct PACKET_ZC_UPDATE_GDID *packet)
{
    struct Player *player = self->world->player;

    game_actor_set_guild((struct GameActor *)player, packet->gdid, packet->emblem_version);
    if (!g_guildInfo.page_asked[GUILD_PAGE_SKILLS]) {
        g_guildInfo.page_asked[GUILD_PAGE_SKILLS] = 1;
        mode_send_message(client_current_mode(), MM_REQ_GUILD_INFO, 3, 0, 0, 0);
    }
    if (!g_guildInfo.page_asked[GUILD_PAGE_MEMBERS]) {
        g_guildInfo.page_asked[GUILD_PAGE_MEMBERS] = 1;
        mode_send_message(client_current_mode(), MM_REQ_GUILD_INFO, 1, 0, 0, 0);
    }
}

static void store_guild_rights(const struct PACKET_ZC_UPDATE_GDID *packet)
{
    g_guildInfo.guild_id = packet->gdid;
    g_guildInfo.can_invite = packet->rights & GUILD_RIGHT_INVITE;
    g_guildInfo.can_expel = (packet->rights >> 4) & 1;
    g_guildInfo.is_master = packet->is_master;
}

void game_mode_zc_update_gdid2(struct GameMode *self, const struct PACKET_ZC_UPDATE_GDID *packet)
{
    store_guild_rights(packet);
    g_guildInfo.master_gid = packet->extra;
    update_guild_id(self, packet);
}

void game_mode_zc_update_gdid(struct GameMode *self, const struct PACKET_ZC_UPDATE_GDID *packet)
{
    store_guild_rights(packet);
    update_guild_id(self, packet);
}

void game_mode_on_actor_guild(struct GameMode *self, int aid, int guild_id, int emblem_version)
{
    if (self->world) {
        struct GameActor *actor = world_get_game_actor_by_aid(self->world, aid);

        if (actor)
            game_actor_set_guild_id(actor, guild_id);
    }
    if (guild_id)
        guild_mgr_set_emblem_version(g_guildMgr, guild_id, emblem_version);
}

void game_mode_on_emblem_download(int part, int guild_id, int version, const char *data,
                                          int size)
{
    if (size < 0) return;
    guild_mgr_download_chunk(g_guildMgr, part, (unsigned)guild_id, version, data, (size_t)size);
}

#pragma pack(push, 1)
struct PACKET_ZC_CHANGE_GUILD {
    unsigned short id;
    unsigned aid;
    unsigned gdid;
    short emblem_version;
};
#pragma pack(pop)

void game_mode_zc_change_guild(struct GameMode *self, const struct PACKET_ZC_CHANGE_GUILD *packet)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

    if (actor)
        game_actor_set_guild(actor, packet->gdid, packet->emblem_version);
}

#pragma pack(push, 1)
struct PACKET_ZC_GUILD_INFO2 {
    unsigned short id;
    unsigned gdid;
    int level;
    int member_count;
    int max_members;
    int average_level;
    int exp;
    int max_exp;
    int points;
    int honor;
    int virtue;
    int emblem_version;
    char name[24];
    char master_name[24];
    char manage_land[16];
    int zeny;
};
#pragma pack(pop)
STATIC_ASSERT(guild_info2_size, sizeof(struct PACKET_ZC_GUILD_INFO2) == 0x72);

void game_mode_zc_guild_info2(const struct PACKET_ZC_GUILD_INFO2 *packet)
{
    char name[25], master_name[25], manage_land[25];

    memcpy(name, packet->name, 24);
    name[24] = 0;
    memcpy(master_name, packet->master_name, 24);
    master_name[24] = 0;
    memcpy(manage_land, packet->manage_land, 24);
    manage_land[24] = 0;
    string_assign_cstr(&(g_guildInfo.name), name);
    string_assign_cstr(&(g_guildInfo.master_name), master_name);
    g_guildInfo.is_master = (unsigned char)string_equal_cstr(&(g_guildInfo.master_name), session_character_name(&g_session1));
    string_assign_cstr(&(g_guildInfo.castle_name), manage_land);
    g_guildInfo.guild_id = packet->gdid;
    (g_guildInfo.level) = packet->level;
    (g_guildInfo.member_count) = packet->member_count;
    (g_guildInfo.online_count) = packet->member_count;
    (g_guildInfo.max_members) = packet->max_members;
    (g_guildInfo.average_level) = packet->average_level;
    (g_guildInfo.exp) = packet->exp;
    (g_guildInfo.exp_next) = packet->max_exp;
    (g_guildInfo.tax_points) = packet->points;
    (g_guildInfo.honor) = packet->honor;
    (g_guildInfo.virtue) = packet->virtue;
    guild_mgr_set_emblem_version(g_guildMgr, packet->gdid, packet->emblem_version);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

#define MM_GUILD_MEMBER_AT      0xaa
#define MM_GUILD_MEMBER_GONE    0xab

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_POSITION_TO_GUILDM {
    unsigned short id;
    unsigned aid;
    short x;
    short y;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_POSITION_TO_GUILDM, 10);

void game_mode_zc_notify_position_to_guild_m(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_POSITION_TO_GUILDM *packet = (const struct PACKET_ZC_NOTIFY_POSITION_TO_GUILDM *)pkt;
    unsigned aid = packet->aid;

    if (aid != ((unsigned)g_session1.ownAid))
        mode_send_msg(self, MM_GUILD_MEMBER_AT, aid, packet->x, packet->y, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_GUILD_MEMBER_MAP_CHANGE {
    unsigned short id;
    unsigned gdid;
    unsigned aid;
    char map_name[];
};
#pragma pack(pop)

void game_mode_zc_guild_member_map_change(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_GUILD_MEMBER_MAP_CHANGE *packet = (const struct PACKET_ZC_GUILD_MEMBER_MAP_CHANGE *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    if (strcmp(g_session1.mapName, packet->map_name) != 0) {
        mode_send_msg(self, MM_GUILD_MEMBER_GONE, aid, 0, 0, 0);
        actor = world_get_game_actor_by_aid(self->world, aid);
        if (actor)
            game_actor_set_mark_listed(actor, 0);
    } else {
        actor = world_get_game_actor_by_aid(self->world, aid);
        if (actor)
            game_actor_set_mark_listed(actor, 1);
    }
}

#define CHAT_CHANNEL_GUILD      4
#define CHAT_COLOR_RED          0xff
#define UIM_REFRESH             3

static void guild_line(int msg, unsigned color, int channel)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(msg), color, channel, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_GUILD_INFO {
    unsigned short id;
    unsigned guild_id;
    int level;
    int member_count;
    int max_members;
    int average_level;
    int exp;
    int exp_next;
    int tax_points;
    int honor;
    int virtue;
    int emblem_version;
    char name[24];
    char master_name[24];
    char castle_name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_GUILD_INFO, 0x76);

void game_mode_zc_guild_info(const unsigned char *pkt)
{
    const struct PACKET_ZC_GUILD_INFO *packet = (const struct PACKET_ZC_GUILD_INFO *)pkt;
    char name[25], master[25], land[25];

    memcpy(name, packet->name, 24);
    name[24] = 0;
    memcpy(master, packet->master_name, 24);
    master[24] = 0;
    memcpy(land, packet->castle_name, 24);
    land[24] = 0;
    string_assign_cstr(((&g_guildInfo.name)), name);
    string_assign_cstr(((&g_guildInfo.master_name)), master);
    g_guildInfo.is_master = string_equal_cstr(((&g_guildInfo.master_name)), session_character_name(&g_session1));
    string_assign_cstr(((&g_guildInfo.castle_name)), land);
    g_guildInfo.level = packet->level;
    g_guildInfo.member_count = packet->member_count;
    g_guildInfo.max_members = packet->max_members;
    g_guildInfo.average_level = packet->average_level;
    g_guildInfo.exp = packet->exp;
    g_guildInfo.exp_next = packet->exp_next;
    g_guildInfo.tax_points = packet->tax_points;
    g_guildInfo.guild_id = packet->guild_id;
    g_guildInfo.honor = packet->honor;
    g_guildInfo.virtue = packet->virtue;
    guild_mgr_set_emblem_version(g_guildMgr, packet->guild_id, packet->emblem_version);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

#define WND_MSG_EMBLEM_CHANGED  0x3c

#pragma pack(push, 1)
struct PACKET_ZC_GUILD_EMBLEM_IMG {
    unsigned short id;
    short length;
    unsigned guild_id;
    unsigned emblem_version;
    BYTE image[];
};
#pragma pack(pop)

void game_mode_zc_guild_emblem_img(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_GUILD_EMBLEM_IMG *packet = (const struct PACKET_ZC_GUILD_EMBLEM_IMG *)pkt;
    unsigned gdid;

    session_save_emblem(&g_session1, packet->image, packet->length - 0xc,
                                        packet->guild_id, packet->emblem_version);
    gdid = packet->guild_id;
    if (g_guildInfo.guild_id != gdid)
        return;
    game_actor_set_guild((struct GameActor *)self->world->player, gdid, packet->emblem_version);
    if (g_windowMgr.guild_info_manage_wnd)
        ui_window_send(g_windowMgr.guild_info_manage_wnd, NULL, WND_MSG_EMBLEM_CHANGED, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_REQ_JOIN_GUILD {
    unsigned short id;
    BYTE answer;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_REQ_JOIN_GUILD, 3);

void game_mode_zc_ack_req_join_guild(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQ_JOIN_GUILD *packet = (const struct PACKET_ZC_ACK_REQ_JOIN_GUILD *)pkt;
    static const struct { short msg; unsigned color; } answers[5] = {
        { 0x17a, CHAT_COLOR_RED },
        { 0x17b, CHAT_COLOR_RED },
        { 0x17c, CHAT_COLOR_CYAN },
        { 0x17d, CHAT_COLOR_RED },
        { 0x82a, CHAT_COLOR_RED },
    };
    unsigned answer;

    ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
    answer = packet->answer;
    if (answer <= 4)
        guild_line(answers[answer].msg, answers[answer].color, CHAT_CHANNEL_GUILD);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_GUILD_MENUINTERFACE {
    unsigned short id;
    int flags;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_GUILD_MENUINTERFACE, 6);

void game_mode_zc_ack_guild_menu_interface(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_GUILD_MENUINTERFACE *packet = (const struct PACKET_ZC_ACK_GUILD_MENUINTERFACE *)pkt;

    g_guildInfo.menu_flags = packet->flags;
}

#define WND_GUILD_POSITIONS     0x3d
#define WND_MSG_POSITIONS       0x6a

#pragma pack(push, 1)
struct PACKET_ZC_ACK_REQ_CHANGE_MEMBERS_MEMBER {
    unsigned aid;
    unsigned gid;
    unsigned position;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_REQ_CHANGE_MEMBERS_MEMBER, 12);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_REQ_CHANGE_MEMBERS {
    unsigned short id;
    short length;
    struct PACKET_ZC_ACK_REQ_CHANGE_MEMBERS_MEMBER members[];
};
#pragma pack(pop)

void game_mode_zc_ack_req_change_members(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQ_CHANGE_MEMBERS *packet = (const struct PACKET_ZC_ACK_REQ_CHANGE_MEMBERS *)pkt;
    int count = (int)((unsigned)(packet->length - 4) / 12);
    const struct PACKET_ZC_ACK_REQ_CHANGE_MEMBERS_MEMBER *member = packet->members;
    struct UIWindow *wnd;
    int i;

    for (i = 0; i < count; i++, member++) {
        guild_info_set_member_position(&g_guildInfo, member->aid, member->gid, (int)member->position);
        if (member->position == 0)
            guild_info_set_master(&g_guildInfo, member->gid);
    }
    wnd = ui_window_find(WND_GUILD_POSITIONS);
    if (wnd)
        ui_window_send(wnd, NULL, WND_MSG_POSITIONS, 0, 0, 0, 0);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

#define WND_GUILD               0x3b

#pragma pack(push, 1)
struct PACKET_ZC_ACK_DISORGANIZE_GUILD_RESULT {
    unsigned short id;
    int result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_DISORGANIZE_GUILD_RESULT, 6);

void game_mode_zc_ack_disorganize_guild_result(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_DISORGANIZE_GUILD_RESULT *packet = (const struct PACKET_ZC_ACK_DISORGANIZE_GUILD_RESULT *)pkt;

    switch (packet->result) {
    case 0:
        session_refresh_guild_skill_hotkeys(&g_session1);
        guild_info_reset(&g_guildInfo);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_GUILD);
        guild_line(0x190, CHAT_COLOR_YELLOW, CHAT_CHANNEL_GUILD);
        break;
    case 1:
        guild_line(0x191, CHAT_COLOR_YELLOW, 0);
        break;
    case 2:
        guild_line(0x192, CHAT_COLOR_YELLOW, CHAT_CHANNEL_GUILD);
        break;
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_DISORGANIZE_GUILD {
    unsigned short id;
    char reason[40];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_DISORGANIZE_GUILD, 42);

void game_mode_zc_ack_disorganize_guild(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_DISORGANIZE_GUILD *packet = (const struct PACKET_ZC_ACK_DISORGANIZE_GUILD *)pkt;
    char reason[41], line[0x400];

    memcpy(reason, packet->reason, 40);
    reason[40] = 0;
    sprintf(line, msg_str(0x16f), reason);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)line, CHAT_COLOR_YELLOW, CHAT_CHANNEL_GUILD, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_REQ_ALLY_GUILD {
    unsigned short id;
    BYTE answer;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_REQ_ALLY_GUILD, 3);

void game_mode_zc_ack_req_ally_guild(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQ_ALLY_GUILD *packet = (const struct PACKET_ZC_ACK_REQ_ALLY_GUILD *)pkt;
    static const struct { short msg; unsigned color; } answers[6] = {
        { 0x18a, CHAT_COLOR_YELLOW },
        { 0x18b, CHAT_COLOR_YELLOW },
        { 0x18c, CHAT_COLOR_CYAN },
        { 0x18d, CHAT_COLOR_YELLOW },
        { 0x18e, CHAT_COLOR_YELLOW },
        { 0x6b5, CHAT_COLOR_YELLOW },
    };
    unsigned answer = packet->answer;

    if (answer <= 5)
        guild_line(answers[answer].msg, answers[answer].color, CHAT_CHANNEL_GUILD);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_REQ_HOSTILE_GUILD {
    unsigned short id;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_REQ_HOSTILE_GUILD, 3);

void game_mode_zc_ack_req_hostile_guild(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQ_HOSTILE_GUILD *packet = (const struct PACKET_ZC_ACK_REQ_HOSTILE_GUILD *)pkt;
    static const struct { short msg; unsigned color; } results[4] = {
        { 0x1ef, CHAT_COLOR_CYAN },
        { 0x1f0, CHAT_COLOR_RED },
        { 0x1f1, CHAT_COLOR_RED },
        { 0x6b6, CHAT_COLOR_RED },
    };
    unsigned result = packet->result;

    if (result <= 3)
        guild_line(results[result].msg, results[result].color, CHAT_CHANNEL_GUILD);
}

#pragma pack(push, 1)
struct PACKET_ZC_DELETE_RELATED_GUILD {
    unsigned short id;
    unsigned guild_id;
    int relation;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DELETE_RELATED_GUILD, 10);

void game_mode_zc_delete_related_guild(const unsigned char *pkt)
{
    const struct PACKET_ZC_DELETE_RELATED_GUILD *packet = (const struct PACKET_ZC_DELETE_RELATED_GUILD *)pkt;

    if ((g_guildInfo.page_asked[GUILD_PAGE_INFO]))
        guild_info_remove_relation(&g_guildInfo, packet->guild_id, packet->relation);
}

#pragma pack(push, 1)
struct PACKET_ZC_ADD_RELATED_GUILD {
    unsigned short id;
    int relation;
    unsigned guild_id;
    char name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ADD_RELATED_GUILD, 34);

void game_mode_zc_add_related_guild(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_RELATED_GUILD *packet = (const struct PACKET_ZC_ADD_RELATED_GUILD *)pkt;
    char name[25];
    struct GuildRelation guild, copy;

    if (!(g_guildInfo.page_asked[GUILD_PAGE_INFO]))
        return;
    memcpy(name, packet->name, 24);
    name[24] = 0;
    string_init(&guild.name);
    guild.kind = packet->relation;
    guild.id = packet->guild_id;
    string_assign_cstr(&guild.name, name);
    guild_relation_copy(&copy, &guild);
    list_push_back(&g_guildInfo.relations, &copy, sizeof copy);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
    string_free(&guild.name);
}

#pragma pack(push, 1)
struct PACKET_ZC_GUILD_POSITION {
    unsigned short id;
    unsigned short length;
    unsigned aid;
    char position[1];
};
#pragma pack(pop)

void game_mode_zc_guild_position(struct GameMode *self, const struct PACKET_ZC_GUILD_POSITION *packet)
{
    struct String title;
    size_t length = packet->length - 8;

    string_init(&title);
    if (length != 0)
        string_assign(&title, packet->position, length);
    name_card_holder_set_position(&self->actor_names, packet->aid, title);
}
