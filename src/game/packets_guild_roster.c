#include "game/packet_handlers.h"
#include "contents/player_names.h"
#include "session/character_name.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "core/markup_tags.h"
#include <string.h>
#include "game/packets_guild.h"
#include "core/msg_str.h"
#include "game/packet_data.h"
#include "std/std.h"
#include "ui/social/guild_member_sort.h"
#include "ui/ui_std.h"

enum {
    MSG_UNKNOWN_MEMBER = 0x245,
    NAME_LENGTH = 24,
    MEMO_LENGTH = 50,
};

#pragma pack(push, 1)
struct GuildMemberBase {
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
};
struct GuildMemberFull {
    struct GuildMemberBase base;
    char memo[MEMO_LENGTH];
    char name[NAME_LENGTH];
};
struct GuildMemberMemo {
    struct GuildMemberBase base;
    char memo[MEMO_LENGTH];
};
struct GuildMemberNamed {
    struct GuildMemberBase base;
    unsigned last_login;
    char name[NAME_LENGTH];
};
struct PacketMemberList {
    unsigned short packet_type;
    short length;
    unsigned char entries[1];
};
struct PacketMemberFull {
    unsigned short packet_type;
    struct GuildMemberFull member;
};
struct PacketMemberMemo {
    unsigned short packet_type;
    struct GuildMemberMemo member;
};
struct PacketMemberNamed {
    unsigned short packet_type;
    struct GuildMemberNamed member;
};
#pragma pack(pop)
SIZE_ASSERT(GuildMemberFull, 0x68);
SIZE_ASSERT(GuildMemberMemo, 0x50);
SIZE_ASSERT(GuildMemberNamed, 0x3a);

static void record_init(struct GuildMember *record)
{
    string_init(&record->name);
    string_init(&record->position_name);
    string_init(&record->memo);
}

static void record_fill(struct GuildMember *record, const struct GuildMemberBase *info)
{
    record->hair_style = info->head;
    record->hair_color = info->head_palette;
    record->job = info->job;
    record->sex = info->sex;
    record->position = info->position;
    record->level = info->level;
    record->exp = info->contribution_exp;
    record->online = info->current_state;
}

static void copy_text(char *out, const char *field, size_t length)
{
    memcpy(out, field, length);
    out[length] = 0;
}

static void name_from_table(struct String *name, unsigned gid)
{
    {
        struct TextView cached_name = player_name_lookup(gid);
        string_assign(name, cached_name.data, cached_name.size);
    }
}

static void name_unknown(struct String *name)
{
    string_assign_cstr(name, msg_str(MSG_UNKNOWN_MEMBER));
    string_append(name, string_cstr(&text_tag_NAMELESS), text_tag_NAMELESS.size);
}

static void post_unknown_name(unsigned gid, const char name[NAME_LENGTH])
{
    struct String raw, text;

    if (player_names_has(gid))
        return;
    string_init(&raw);
    string_assign(&raw, name, NAME_LENGTH);
    string_init(&text);
    string_assign_cstr(&text, string_cstr(&raw));
    client_post_aid_text((int)gid, text);
    string_free(&raw);
}

static void name_member(struct GuildMember *record, unsigned gid, const char *posted_name)
{
    if (gid == (unsigned)g_session1.ownGid) {
        string_assign_cstr(&record->name, session_character_name(&g_session1));
        return;
    }
    if (posted_name != 0)
        post_unknown_name(gid, posted_name);
    name_from_table(&record->name, gid);
    if (record->name.size == 0)
        name_unknown(&record->name);
}

static void list_done(void)
{
    guild_info_update_member_shares(&g_guildInfo);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void game_mode_zc_member_mgr_info(struct GameMode *self, const struct PacketMemberList *packet)
{
    const struct GuildMemberFull *entry = (const struct GuildMemberFull *)packet->entries;
    int count = (int)((unsigned)(packet->length - 4) / sizeof *entry);
    int i;

    (void)self;
    g_guildInfo.member_count = count;
    guild_info_clear_members(&g_guildInfo);
    for (i = 0; i < count; i++, entry++) {
        struct GuildMember record;
        char memo[MEMO_LENGTH + 1], name[NAME_LENGTH + 1];

        record_init(&record);
        record.aid = entry->base.aid;
        record.gid = entry->base.gid;
        record_fill(&record, &entry->base);
        record.exp_share = 0;
        copy_text(memo, entry->memo, MEMO_LENGTH);
        copy_text(name, entry->name, NAME_LENGTH);
        string_assign_cstr(&record.name, name);
        string_assign_cstr(&record.memo, memo);
        string_clear(&record.position_name);
        guild_info_add_member(&g_guildInfo, &record);
        guild_member_record_destroy(&record);
    }
    list_done();
}

void game_mode_zc_member_mgr_info_del_name(struct GameMode *self, const struct PacketMemberList *packet)
{
    const struct GuildMemberMemo *entry = (const struct GuildMemberMemo *)packet->entries;
    int count = (int)((unsigned)(packet->length - 4) / sizeof *entry);
    int i;

    (void)self;
    g_guildInfo.member_count = count;
    guild_info_clear_members(&g_guildInfo);
    for (i = 0; i < count; i++, entry++) {
        struct GuildMember record;
        char memo[MEMO_LENGTH + 1];

        record_init(&record);
        record.aid = entry->base.aid;
        record.gid = entry->base.gid;
        record_fill(&record, &entry->base);
        record.exp_share = 0;
        name_member(&record, entry->base.gid, 0);
        copy_text(memo, entry->memo, MEMO_LENGTH);
        string_assign_cstr(&record.memo, memo);
        string_clear(&record.position_name);
        guild_info_add_member(&g_guildInfo, &record);
        guild_member_record_destroy(&record);
    }
    list_done();
}

void game_mode_zc_member_mgr_info_new(struct GameMode *self, const struct PacketMemberList *packet)
{
    const struct GuildMemberNamed *entry = (const struct GuildMemberNamed *)packet->entries;
    int count = (packet->length - 4) / (int)sizeof *entry;
    int i;

    (void)self;
    g_guildInfo.member_count = count;
    guild_info_clear_members(&g_guildInfo);
    for (i = 0; i < count; i++, entry++) {
        struct GuildMember record;

        record_init(&record);
        record.aid = entry->base.aid;
        record.gid = entry->base.gid;
        record_fill(&record, &entry->base);
        record.last_login = entry->last_login;
        record.exp_share = 0;
        name_member(&record, entry->base.gid, entry->name);
        string_clear(&record.position_name);
        guild_info_add_member(&g_guildInfo, &record);
        guild_member_record_destroy(&record);
    }
    list_done();
}

void game_mode_zc_member_add(struct GameMode *self, const struct PacketMemberFull *packet)
{
    const struct GuildMemberFull *member = &packet->member;
    struct GuildMember record;
    char memo[MEMO_LENGTH + 1], name[NAME_LENGTH + 1];

    (void)self;
    if (g_guildInfo.page_asked[GUILD_PAGE_MEMBERS] == 0)
        return;
    copy_text(memo, member->memo, MEMO_LENGTH);
    copy_text(name, member->name, NAME_LENGTH);
    guild_member_construct(&record);
    record.aid = member->base.aid;
    record.gid = member->base.gid;
    string_assign_cstr(&record.name, name);
    record_fill(&record, &member->base);
    string_assign_cstr(&record.memo, memo);
    guild_info_add_member(&g_guildInfo, &record);
    player_names_set(member->base.gid, text_view_cstr(name));
    list_done();
    guild_member_record_destroy(&record);
}

void game_mode_zc_member_add_del_name(struct GameMode *self, const struct PacketMemberMemo *packet)
{
    const struct GuildMemberMemo *member = &packet->member;
    struct GuildMember record;
    char memo[MEMO_LENGTH + 1];

    (void)self;
    if (g_guildInfo.page_asked[GUILD_PAGE_MEMBERS] == 0)
        return;
    copy_text(memo, member->memo, MEMO_LENGTH);
    guild_member_construct(&record);
    record.aid = member->base.aid;
    record.gid = member->base.gid;
    {
        struct TextView cached_name = player_name_lookup(member->base.gid);
        string_assign(&record.name, cached_name.data, cached_name.size);
    }
    record_fill(&record, &member->base);
    string_assign_cstr(&record.memo, memo);
    if (record.name.size == 0) {
        string_assign_cstr(&record.name, msg_str(MSG_UNKNOWN_MEMBER));
        string_append(&record.name, string_cstr(&text_tag_NAMELESS), text_tag_NAMELESS.size);
    }
    guild_info_add_member(&g_guildInfo, &record);
    list_done();
    guild_member_record_destroy(&record);
}

void game_mode_zc_member_add_new(struct GameMode *self, const struct PacketMemberNamed *packet)
{
    const struct GuildMemberNamed *member = &packet->member;
    struct GuildMember record;

    (void)self;
    if (g_guildInfo.page_asked[GUILD_PAGE_MEMBERS] == 0)
        return;
    record_init(&record);
    record.aid = member->base.aid;
    record.gid = member->base.gid;
    record_fill(&record, &member->base);
    record.last_login = member->last_login;
    post_unknown_name(member->base.gid, member->name);
    {
        struct TextView cached_name = player_name_lookup(member->base.gid);
        string_assign(&record.name, cached_name.data, cached_name.size);
    }
    if (record.name.size == 0)
        name_unknown(&record.name);
    guild_info_add_member(&g_guildInfo, &record);
    list_done();
    guild_member_record_destroy(&record);
}
