#include "game/packet_handlers.h"
#include "actor/actor_ops.h"
#include "contents/player_names.h"
#include "session/character_name.h"
#include "contents/manner_filter.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "core/markup_tags.h"
#include <string.h>
#include "game/packets_guild.h"
#include "core/msg_str.h"
#include "session/session_options.h"
#include "std/tree.h"
#include "ui/ui_std.h"

enum { MSG_UNKNOWN_MEMBER = 0x245 };

#pragma pack(push, 1)
struct GuildMemberInfo {
    unsigned aid;
    unsigned gid;
    short head;
    short head_palette;
    short sex;
    short job;
    short level;
    int contribution_exp;
    int current_state;
    int position;
    unsigned last_login;
};
struct PacketMemberMgrInfo {
    short packet_type;
    short length;
    struct GuildMemberInfo members[1];
};
struct PacketGuildMember {
    unsigned short packet_type;
    struct GuildMemberInfo member;
};
#pragma pack(pop)

static void record_fill(struct GuildMember *record, const struct GuildMemberInfo *info)
{
    record->hair_style = info->head;
    record->hair_color = info->head_palette;
    record->job = info->job;
    record->sex = info->sex;
    record->position = info->position;
    record->level = info->level;
    record->exp = info->contribution_exp;
    record->online = info->current_state;
    record->last_login = info->last_login;
}

void game_mode_on_member_mgr_info(const struct PacketMemberMgrInfo *packet)
{
    int count = (int)((unsigned)(packet->length - 4) / sizeof(struct GuildMemberInfo));
    int i;

    g_guildInfo.member_count = count;
    guild_info_clear_members(&g_guildInfo);
    for (i = 0; i < count; i++) {
        const struct GuildMemberInfo *info = &packet->members[i];
        struct GuildMember record;

        string_init(&record.name);
        string_init(&record.position_name);
        string_init(&record.memo);
        record.aid = info->aid;
        record.gid = info->gid;
        record_fill(&record, info);
        record.exp_share = 0;
        if (info->gid == (unsigned)g_session1.ownGid) {
            const char *name = session_character_name(&g_session1);

            string_assign_cstr(&record.name, name);
        }
        string_clear(&record.position_name);
        guild_info_add_member(&g_guildInfo, &record);
        guild_member_record_destroy(&record);
    }
    guild_info_update_member_shares(&g_guildInfo);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void game_mode_on_guild_member(const struct PacketGuildMember *packet)
{
    const struct GuildMemberInfo *info = &packet->member;
    struct GuildMember record;

    if (g_guildInfo.page_asked[GUILD_PAGE_MEMBERS] == 0)
        return;
    string_init(&record.name);
    string_init(&record.position_name);
    string_init(&record.memo);
    record.aid = info->aid;
    record.gid = info->gid;
    {
        struct TextView cached_name = player_name_lookup(info->gid);
        string_assign(&record.name, cached_name.data, cached_name.size);
    }
    record_fill(&record, info);
    if (record.name.size == 0) {
        string_assign_cstr(&record.name, msg_str(MSG_UNKNOWN_MEMBER));
        string_append(&record.name, string_cstr(&text_tag_NAMELESS), text_tag_NAMELESS.size);
    }
    guild_info_add_member(&g_guildInfo, &record);
    guild_info_update_member_shares(&g_guildInfo);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
    guild_member_record_destroy(&record);
}

#pragma pack(push, 1)
struct PacketGuildLeave {
    unsigned short packet_type;
    char name[24];
    char reason[40];
};
struct PacketGuildBanByGid {
    unsigned short packet_type;
    char reason[40];
    unsigned gid;
};
struct PacketGuildLeaveByGid {
    unsigned short packet_type;
    unsigned gid;
    char reason[40];
};
#pragma pack(pop)

enum {
    WID_GUILD = 0x3b,
    CHAT_CHANNEL_GUILD = 4,
    MM_GUILD_DROPPED = 0xac,
    MSG_EXPELLED_US = 0x1ea,
    MSG_WE_LEFT = 0x1e9,
    MSG_MEMBER_LEFT = 0x16c,
    MSG_LEAVE_REASON = 0x16d,
    MSG_MEMBER_EXPELLED = 0x172,
    MSG_EXPEL_REASON = 0x173,
    CHAT_LINE_MAX = 0x400,
};

struct GuildExpelled *guild_ban_construct(struct GuildExpelled *ban)
{
    string_init(&ban->name);
    string_init(&ban->account_name);
    string_init(&ban->reason);
    return ban;
}

static void guild_chat(const char *text, int color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)text, color, CHAT_CHANNEL_GUILD, 0);
}

static char is_our_name(const char *name)
{
    struct String text;
    const char *ours;
    char same;

    string_init(&text);
    string_assign_cstr(&text, name);
    ours = session_character_name(&g_session1);
    same = string_equal_cstr(&text, ours);
    string_free(&text);
    return same;
}

static void censor_filtered_text(char *line)
{
    if (word_filter_contains(&g_nameFilter, line) == 1) {
        word_filter_censor(&g_nameFilter, line);
    }
}

static void name_or_unknown(struct String *name)
{
    if (name->size == 0) {
        string_assign_cstr(name, msg_str(MSG_UNKNOWN_MEMBER));
        string_append(name, string_cstr(&text_tag_NAMELESS), text_tag_NAMELESS.size);
    }
}

static void report_expelled(const char *name, const char *reason)
{
    char line[CHAT_LINE_MAX];
    struct GuildExpelled ban;

    memset(line, 0, sizeof line);
    sprintf(line, msg_str(MSG_MEMBER_EXPELLED), name);
    guild_chat(line, CHAT_COLOR_YELLOW);
    memset(line, 0, sizeof line);
    sprintf_s(line, sizeof line, msg_str(MSG_EXPEL_REASON), reason);
    censor_filtered_text(line);
    guild_chat(line, CHAT_COLOR_YELLOW);
    guild_ban_construct(&ban);
    string_assign_cstr(&ban.name, name);
    string_assign_cstr(&ban.reason, reason);
    guild_info_add_expelled(&g_guildInfo, &ban);
    string_free(&ban.reason);
    string_free(&ban.account_name);
    string_free(&ban.name);
}

static void report_left(const char *name, const char *reason)
{
    char line[CHAT_LINE_MAX];

    sprintf(line, msg_str(MSG_MEMBER_LEFT), name);
    guild_chat(line, CHAT_COLOR_YELLOW);
    sprintf_s(line, sizeof line, msg_str(MSG_LEAVE_REASON), reason);
    censor_filtered_text(line);
    guild_chat(line, CHAT_COLOR_YELLOW);
}

static void we_were_expelled(struct GameMode *self)
{
    g_guildInfo.guild_id = 0;
    ui_window_mgr_post_quit1(&g_windowMgr, WID_GUILD);
    guild_chat(msg_str(MSG_EXPELLED_US), CHAT_COLOR_RED);
    mode_send_msg(self, MM_GUILD_DROPPED, 0, 0, 0, 0);
}

static void we_left(struct GameMode *self)
{
    guild_info_reset(&g_guildInfo);
    ui_window_mgr_post_quit1(&g_windowMgr, WID_GUILD);
    guild_chat(msg_str(MSG_WE_LEFT), CHAT_COLOR_RED);
    game_actor_set_guild((struct GameActor *)self->world->player, 0, 0);
    mode_send_msg(self, MM_GUILD_DROPPED, 0, 0, 0, 0);
}

void game_mode_zc_ack_ban_guild_sso(struct GameMode *self, const struct PacketGuildLeave *packet)
{
    char name[25], reason[41];

    memcpy(reason, packet->reason, 40);
    reason[40] = 0;
    memcpy(name, packet->name, 24);
    name[24] = 0;
    if (is_our_name(name)) {
        we_were_expelled(self);
        return;
    }
    if (g_guildInfo.page_asked[GUILD_PAGE_MEMBERS])
        guild_info_remove_member_by_name(&g_guildInfo, name);
    report_expelled(name, reason);
}

void game_mode_zc_ack_ban_guild_del_name(struct GameMode *self, const struct PacketGuildBanByGid *packet)
{
    char reason[41];
    struct String name;
    struct Player *player;

    memcpy(reason, packet->reason, 40);
    reason[40] = 0;
    if (packet->gid == (unsigned)g_session1.ownGid) {
        session_refresh_guild_skill_hotkeys(&g_session1);
        we_were_expelled(self);
        if (self->world != 0 && (player = self->world->player) != 0)
            game_actor_set_guild_id((struct GameActor *)player, 0);
        guild_info_clear_castles(&g_guildInfo);
        return;
    }
    if (g_guildInfo.page_asked[GUILD_PAGE_MEMBERS])
        guild_info_remove_member(&g_guildInfo, packet->gid);
    {
        struct TextView cached_name = player_name_lookup(packet->gid);
        string_init(&name);
        string_assign(&name, cached_name.data, cached_name.size);
    }
    name_or_unknown(&name);
    report_expelled(string_cstr(&name), reason);
    string_free(&name);
}

void game_mode_zc_ack_leave_guild(struct GameMode *self, const struct PacketGuildLeave *packet)
{
    char name[25], reason[41];

    memcpy(reason, packet->reason, 40);
    reason[40] = 0;
    memcpy(name, packet->name, 24);
    name[24] = 0;
    if (is_our_name(name)) {
        we_left(self);
        return;
    }
    if (g_guildInfo.page_asked[GUILD_PAGE_MEMBERS])
        guild_info_remove_member_by_name(&g_guildInfo, name);
    report_left(name, reason);
}

void game_mode_zc_ack_leave_guild_del_name(struct GameMode *self, const struct PacketGuildLeaveByGid *packet)
{
    char reason[41];
    struct String name;
    struct GameActor *actor;

    strcpy_s(reason, 40, packet->reason);
    reason[40] = 0;
    if (packet->gid == (unsigned)g_session1.ownGid) {
        session_refresh_guild_skill_hotkeys(&g_session1);
        we_left(self);
        return;
    }
    if (g_guildInfo.page_asked[GUILD_PAGE_MEMBERS])
        guild_info_remove_member(&g_guildInfo, packet->gid);
    string_init(&name);
    if (packet->gid == (unsigned)g_session1.ownGid) {
        const char *ours = session_character_name(&g_session1);

        string_assign_cstr(&name, ours);
    } else {
        {
            struct TextView cached_name = player_name_lookup(packet->gid);
            string_assign(&name, cached_name.data, cached_name.size);
        }
    }
    name_or_unknown(&name);
    report_left(string_cstr(&name), reason);
    actor = world_get_game_actor_by_aid(self->world, packet->gid);
    if (actor != 0)
        game_actor_set_guild(actor, 0, 0);
    string_free(&name);
}

#pragma pack(push, 1)
struct GuildPositionEntry {
    int position_id;
    unsigned rights;
    int ranking;
    int pay_rate;
    char title[24];
};
struct PacketPositionInfo {
    unsigned short packet_type;
    short length;
    struct GuildPositionEntry entries[1];
};
#pragma pack(pop)
SIZE_ASSERT(GuildPositionEntry, 40);

struct String *guild_position_title(int position_id)
{
    struct GuildPositionName *entry = tree_get(&g_guildInfo.position_names, &position_id, sizeof position_id,
                                               sizeof *entry, tree_less_int);

    return &entry->name;
}

void game_mode_zc_ack_change_guild_position_info(const struct PacketPositionInfo *packet)
{
    int count, i;

    if (g_guildInfo.page_asked[GUILD_PAGE_POSITIONS] == 0)
        return;
    count = (int)((unsigned)(packet->length - 4) / sizeof(struct GuildPositionEntry));
    for (i = 0; i < count; i++) {
        struct GuildPositionEntry entry = packet->entries[i];
        struct GuildPosition record;
        struct String title, *kept;
        char name[25];

        memcpy(name, entry.title, 24);
        name[24] = 0;
        if (word_filter_contains(&g_nameFilter, name)) {
            word_filter_censor(&g_nameFilter, name);
        }
        kept = guild_position_title(entry.position_id);
        string_assign_cstr(kept, name);
        string_init(&title);
        kept = guild_position_title(entry.position_id);
        string_assign(&title, string_cstr(kept), kept->size);
        record.ranking = entry.ranking;
        record.position_id = entry.position_id;
        string_copy(&record.name, &title);
        record.can_invite = entry.rights & 1;
        record.can_expel = (entry.rights >> 4) & 1;
        record.exp_tax = entry.pay_rate;
        record.can_use_storage = (entry.rights >> 8) & 1;
        guild_info_update_position(&g_guildInfo, record);
        string_free(&title);
    }
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}
