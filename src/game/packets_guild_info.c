#include "game/packet_handlers.h"
#include "contents/player_names.h"
#include "session/character_name.h"
#include "contents/guild_emblem_api.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "core/memory.h"
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
    REASON_LENGTH = 40,
    NOTICE_SUBJECT_LENGTH = 60,
    NOTICE_TEXT_LENGTH = 120,
    NOTICE_ECHO_COLOR = 0x80ffff,
};

static void copy_text(char *out, const char *field, size_t length)
{
    memcpy(out, field, length);
    out[length] = 0;
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

static void check_master(const struct String *master)
{
    const char *ours = session_character_name(&g_session1);

    if (string_equal_cstr(master, ours))
        g_guildInfo.is_master = 1;
}

#pragma pack(push, 1)
struct PacketGuildInfo {
    unsigned short packet_type;
    unsigned guild_id;
    int level;
    int member_count;
    int member_max;
    int average_level;
    int exp;
    int exp_max;
    int points;
    int honor;
    int virtue;
    int emblem_version;
    char name[NAME_LENGTH];
    char managed_land[16];
    int zeny;
    unsigned master_gid;
};
struct PacketGuildInfoNew {
    struct PacketGuildInfo info;
    char master_name[NAME_LENGTH];
};
#pragma pack(pop)

static void guild_info_store(const struct PacketGuildInfo *packet, const char *land)
{
    g_guildInfo.master_gid = packet->master_gid;
    string_assign_cstr(&(g_guildInfo.castle_name), land);
    g_guildInfo.guild_id = packet->guild_id;
    g_guildInfo.level = packet->level;
    g_guildInfo.online_count = packet->member_count;
    g_guildInfo.max_members = packet->member_max;
    g_guildInfo.average_level = packet->average_level;
    g_guildInfo.exp = packet->exp;
    g_guildInfo.exp_next = packet->exp_max;
    g_guildInfo.tax_points = packet->points;
    g_guildInfo.honor = packet->honor;
    g_guildInfo.virtue = packet->virtue;
    guild_mgr_set_emblem_version(g_guildMgr, packet->guild_id, packet->emblem_version);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void game_mode_zc_guild_info_del_name(struct GameMode *self, const struct PacketGuildInfo *packet)
{
    struct String master;
    char name[NAME_LENGTH + 1], land[NAME_LENGTH + 1];

    (void)self;
    {
        struct TextView cached_name = player_name_lookup(packet->master_gid);
        string_init(&master);
        string_assign(&master, cached_name.data, cached_name.size);
    }
    if (master.size == 0)
        name_unknown(&master);
    copy_text(name, packet->name, NAME_LENGTH);
    copy_text(land, packet->managed_land, NAME_LENGTH);
    check_master(&master);
    string_assign_cstr(&(g_guildInfo.name), name);
    string_assign(&(g_guildInfo.master_name), string_cstr(&master), master.size);
    guild_info_store(packet, land);
    string_free(&master);
}

void game_mode_zc_guild_info_new(struct GameMode *self, const struct PacketGuildInfoNew *packet)
{
    const struct PacketGuildInfo *info = &packet->info;
    char name[NAME_LENGTH + 1], land[NAME_LENGTH + 1];

    (void)self;
    post_unknown_name(info->master_gid, packet->master_name);
    {
        struct TextView cached_name = player_name_lookup(info->master_gid);
        string_assign(&(g_guildInfo.master_name), cached_name.data, cached_name.size);
    }
    if ((g_guildInfo.master_name).size == 0)
        name_unknown(&(g_guildInfo.master_name));
    check_master(&(g_guildInfo.master_name));
    copy_text(name, info->name, NAME_LENGTH);
    copy_text(land, info->managed_land, NAME_LENGTH);
    string_assign_cstr(&(g_guildInfo.name), name);
    guild_info_store(info, land);
}

#pragma pack(push, 1)
struct PacketGuildNotice {
    unsigned short packet_type;
    char subject[NOTICE_SUBJECT_LENGTH];
    char text[NOTICE_TEXT_LENGTH];
};
#pragma pack(pop)

static void notice_echo(struct String *line, char *buf, size_t size, const char *text)
{
    memset(buf, 0, size);
    strcpy_s(buf, size, text);
    string_init(line);
    string_assign_cstr(line, "[ ");
    string_append_cstr(line, buf);
    string_append_cstr(line, " ]");
    if (strlen(buf) != 0)
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)string_cstr(line), NOTICE_ECHO_COLOR, 0, 0);
}

void game_mode_zc_guild_notice(struct GameMode *self, struct PacketGuildNotice *packet)
{
    char subject[NOTICE_SUBJECT_LENGTH], text[NOTICE_TEXT_LENGTH];
    struct String subject_line, text_line;

    (void)self;
    packet->text[NOTICE_TEXT_LENGTH - 1] = 0;
    packet->subject[NOTICE_SUBJECT_LENGTH - 1] = 0;
    string_assign_cstr(&(g_guildInfo.notice_subject), packet->subject);
    string_assign_cstr(&(g_guildInfo.notice_body), packet->text);
    if (g_session1.showLoginNotices != 1)
        return;
    notice_echo(&subject_line, subject, sizeof subject, packet->subject);
    notice_echo(&text_line, text, sizeof text, packet->text);
    string_assign_cstr(&(g_guildInfo.notice_subject), subject);
    string_assign_cstr(&(g_guildInfo.notice_body), text);
    string_free(&text_line);
    string_free(&subject_line);
}

#pragma pack(push, 1)
struct GuildRelationEntry {
    int kind;
    unsigned guild_id;
    char name[NAME_LENGTH];
};
struct PacketGuildRelations {
    unsigned short packet_type;
    short length;
    struct GuildRelationEntry entries[1];
};
#pragma pack(pop)

static void relation_destroy(void *record)
{
    string_free(&((struct GuildRelation *)record)->name);
}

static void position_destroy(void *record)
{
    string_free(&((struct GuildPosition *)record)->name);
}

static int position_less(const void *a, const void *b)
{
    return ((const struct GuildPosition *)a)->position_id < ((const struct GuildPosition *)b)->position_id;
}

void game_mode_zc_my_guild_basic_info(struct GameMode *self, const struct PacketGuildRelations *packet)
{
    unsigned count = (unsigned)(packet->length - 4) / sizeof(struct GuildRelationEntry);
    struct List *relations = &(g_guildInfo.relations);
    int i;

    (void)self;
    list_clear(relations, relation_destroy);
    for (i = 0; (unsigned)i < count; i++) {
        const struct GuildRelationEntry *entry = &packet->entries[i];
        struct GuildRelation relation, copy;
        char name[NAME_LENGTH + 1];

        copy_text(name, entry->name, NAME_LENGTH);
        string_init(&relation.name);
        relation.kind = entry->kind;
        relation.id = entry->guild_id;
        string_assign_cstr(&relation.name, name);
        guild_relation_copy(&copy, &relation);
        list_push_back(relations, &copy, sizeof copy);
        string_free(&relation.name);
    }
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

#pragma pack(push, 1)
struct PositionNameEntry {
    int position_id;
    char title[NAME_LENGTH];
};
struct PacketPositionNames {
    unsigned short packet_type;
    short length;
    struct PositionNameEntry entries[1];
};
struct PositionEntry {
    int position_id;
    unsigned rights;
    int ranking;
    int pay_rate;
};
struct PacketPositions {
    unsigned short packet_type;
    short length;
    struct PositionEntry entries[1];
};
#pragma pack(pop)
SIZE_ASSERT(PositionNameEntry, 28);

void game_mode_zc_position_id_name_info(struct GameMode *self, const struct PacketPositionNames *packet)
{
    int count = (int)((unsigned)(packet->length - 4) / sizeof(struct PositionNameEntry));
    int i;

    (void)self;
    for (i = 0; i < count; i++) {
        char title[NAME_LENGTH + 1];

        copy_text(title, packet->entries[i].title, NAME_LENGTH);
        string_assign_cstr(guild_position_title(packet->entries[i].position_id), title);
    }
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void game_mode_zc_position_info(struct GameMode *self, const struct PacketPositions *packet)
{
    unsigned count = (unsigned)(packet->length - 4) / sizeof(struct PositionEntry);
    struct List *positions = &(g_guildInfo.positions);
    int i;

    (void)self;
    list_clear(positions, position_destroy);
    for (i = 0; (unsigned)i < count; i++) {
        const struct PositionEntry *entry = &packet->entries[i];
        struct GuildPosition record, copy;
        struct String *title;

        string_init(&record.name);
        record.position_id = entry->position_id;
        title = guild_position_title(entry->position_id);
        string_assign(&record.name, string_cstr(title), title->size);
        record.exp_tax = entry->pay_rate;
        record.ranking = entry->ranking;
        record.can_invite = entry->rights & 1;
        record.can_expel = (entry->rights >> 4) & 1;
        record.can_use_storage = (entry->rights >> 8) & 1;
        guild_position_copy(&copy, &record);
        list_push_back(positions, &copy, sizeof copy);
        string_free(&record.name);
    }
    list_sort(positions, position_less);
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

#pragma pack(push, 1)
struct BanEntry {
    char name[NAME_LENGTH];
    char reason[REASON_LENGTH];
};
struct BanEntryById {
    unsigned gid;
    char reason[REASON_LENGTH];
};
struct BanEntryNamed {
    unsigned gid;
    char reason[REASON_LENGTH];
    char name[NAME_LENGTH];
};
struct PacketBanList {
    unsigned short packet_type;
    short length;
    unsigned char entries[1];
};
#pragma pack(pop)
SIZE_ASSERT(BanEntry, 0x40);
SIZE_ASSERT(BanEntryById, 0x2c);
SIZE_ASSERT(BanEntryNamed, 0x44);

static void ban_init(struct GuildExpelled *ban)
{
    string_init(&ban->name);
    string_init(&ban->account_name);
    string_init(&ban->reason);
}

static void expelled_destroy(void *record)
{
    guild_expelled_destruct(record);
}

static void ban_keep_and_destroy(struct GuildExpelled *ban, int keep)
{
    if (keep) {
        struct GuildExpelled copy;

        guild_expelled_copy(&copy, ban);
        list_push_back(&g_guildInfo.expelled, &copy, sizeof copy);
    }
    string_free(&ban->reason);
    string_free(&ban->account_name);
    string_free(&ban->name);
}

void game_mode_zc_ban_list(struct GameMode *self, const struct PacketBanList *packet)
{
    const struct BanEntry *entry = (const struct BanEntry *)packet->entries;
    int count = (int)((unsigned)(packet->length - 4) / sizeof *entry);
    int i;

    (void)self;
    list_clear(&g_guildInfo.expelled, expelled_destroy);
    for (i = 0; i < count; i++, entry++) {
        struct GuildExpelled ban;
        char name[NAME_LENGTH + 1], reason[REASON_LENGTH + 1];

        copy_text(name, entry->name, NAME_LENGTH);
        copy_text(reason, entry->reason, REASON_LENGTH);
        ban_init(&ban);
        string_assign_cstr(&ban.name, name);
        string_assign_cstr(&ban.reason, reason);
        ban_keep_and_destroy(&ban, 1);
    }
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void game_mode_zc_ban_list_del_name(struct GameMode *self, const struct PacketBanList *packet)
{
    const struct BanEntryById *entry = (const struct BanEntryById *)packet->entries;
    int count = (int)((unsigned)(packet->length - 4) / sizeof *entry);
    int i;

    (void)self;
    list_clear(&g_guildInfo.expelled, expelled_destroy);
    for (i = 0; i < count; i++, entry++) {
        struct GuildExpelled ban;
        char reason[REASON_LENGTH + 1];

        copy_text(reason, entry->reason, REASON_LENGTH);
        ban_init(&ban);
        ban.gid = entry->gid;
        {
            struct TextView cached_name = player_name_lookup(entry->gid);
            string_assign(&ban.name, cached_name.data, cached_name.size);
        }
        string_assign_cstr(&ban.reason, reason);
        if (ban.name.size == 0)
            name_unknown(&ban.name);
        ban_keep_and_destroy(&ban, ban.gid != 0);
    }
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}

void game_mode_zc_ban_list_new(struct GameMode *self, const struct PacketBanList *packet)
{
    const struct BanEntryNamed *entry = (const struct BanEntryNamed *)packet->entries;
    int count = (packet->length - 4) / (int)sizeof *entry;
    int i;

    (void)self;
    list_clear(&g_guildInfo.expelled, expelled_destroy);
    for (i = 0; i < count; i++, entry++) {
        struct GuildExpelled ban;
        char reason[REASON_LENGTH + 1];

        copy_text(reason, entry->reason, REASON_LENGTH);
        ban_init(&ban);
        ban.gid = entry->gid;
        string_assign_cstr(&ban.reason, reason);
        post_unknown_name(entry->gid, entry->name);
        {
            struct TextView cached_name = player_name_lookup(entry->gid);
            string_assign(&ban.name, cached_name.data, cached_name.size);
        }
        if (ban.name.size == 0)
            name_unknown(&ban.name);
        ban_keep_and_destroy(&ban, ban.gid != 0);
    }
    window_mgr_invalidate_guild_wnd(&g_windowMgr);
}
