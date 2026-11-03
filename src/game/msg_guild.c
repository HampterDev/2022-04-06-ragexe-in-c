#include "contents/guild_info.h"
#include "session/state.h"
#include <string.h>
#include "game/mode_msg.h"
#include "game/context.h"
#include "platform/language.h"
#include "core/text_util.h"
#include "std/tree.h"

#define CZ_REGISTER_GUILD_EMBLEM_IMG    0x153
#define CZ_REQ_CHANGE_MEMBERPOS         0x155
#define CZ_REG_CHANGE_GUILD_POSITIONINFO 0x161
#define CZ_REQ_DISORGANIZE_GUILD        0x15d
#define CZ_REQ_LEAVE_GUILD              0x159
#define CZ_REQ_BAN_GUILD                0x15b
#define CZ_REQ_MAKE_GUILD               0x165
#define CZ_REQ_JOIN_GUILD               0x168
#define CZ_GUILD_NOTICE                 0x16e
#define CZ_REQ_ALLY_GUILD               0x170
#define CZ_REQ_JOIN_GUILD2              0x916

#define LIST_PACKET_MAX 0x800

static char *charset_prefix(char *field)
{
    strcpy(field, language_charset_tag(g_language, 0));
    return field + 3;
}

int game_mode_msg_register_guild_emblem(const void *image, int size)
{
    struct {
        unsigned short id;
        unsigned short len;
        unsigned char image[LIST_PACKET_MAX - 4];
    } packet;
    short len;

    if (!g_guildInfo.is_master)
        return 0;
    if (size + 4 > LIST_PACKET_MAX)
        return 0;
    len = (short)((unsigned short)size + 4);
    packet.id = CZ_REGISTER_GUILD_EMBLEM_IMG;
    packet.len = len;
    memcpy(packet.image, image, size);
    send_raw(len, &packet);
    return 0;
}

int game_mode_msg_change_guild_member_pos(void)
{
    struct {
        unsigned short id;
        unsigned short len;
        int entries[(LIST_PACKET_MAX - 4) / 4];
    } packet;
    struct Tree *changes = &g_guildInfo.member_position_edits;
    struct TreeNode *node;
    int *out;
    int len;

    if (!g_guildInfo.is_master)
        return 0;
    len = changes->size * 12 + 4;
    if ((short)len > LIST_PACKET_MAX)
        return 0;
    packet.id = CZ_REQ_CHANGE_MEMBERPOS;
    packet.len = (unsigned short)len;
    out = packet.entries;
    for (node = tree_first(changes); node != changes->head; node = tree_next(node)) {
        const struct GuildMemberChange *change = &TREE_VALUE(node, struct GuildMemberEdit)->change;

        out[0] = change->aid;
        out[1] = change->gid;
        out[2] = change->position;
        out += 3;
    }
    tree_clear(changes, NULL);
    send_raw((short)len, &packet);
    return 0;
}

int game_mode_msg_change_guild_positions(void)
{
#pragma pack(push, 1)
    struct PositionInfoEntry {
        int position_id;
        int mode;
        int ranking;
        int pay_rate;
        char title[24];
    };
    struct {
        unsigned short id;
        unsigned short len;
        struct PositionInfoEntry entries[(LIST_PACKET_MAX - 4) / sizeof(struct PositionInfoEntry)];
    } packet;
#pragma pack(pop)
    struct Tree *changes = &g_guildInfo.position_edits;
    struct TreeNode *node;
    struct PositionInfoEntry entry, *out;
    int len;

    if (!g_guildInfo.is_master)
        return 0;
    len = changes->size * sizeof(struct PositionInfoEntry) + 4;
    if ((short)len > LIST_PACKET_MAX)
        return 0;
    packet.id = CZ_REG_CHANGE_GUILD_POSITIONINFO;
    packet.len = (unsigned short)len;
    out = packet.entries;
    for (node = tree_first(changes); node != changes->head; node = tree_next(node)) {
        const struct GuildPosition *pos = &TREE_VALUE(node, struct GuildPositionEdit)->position;
        int mode = pos->can_invite != 0;

        if (pos->can_expel)
            mode |= 0x10;
        if (pos->can_use_storage)
            mode |= 0x100;
        entry.position_id = pos->position_id;
        entry.mode = mode;
        entry.ranking = pos->ranking;
        entry.pay_rate = pos->exp_tax;
        strncpy(entry.title, string_cstr(&pos->name), 0x17);
        entry.title[23] = 0;
        *out++ = entry;
    }
    tree_clear(changes, guild_position_edit_destroy);
    send_raw((short)len, &packet);
    return 0;
}

int game_mode_msg_disorganize_guild(const char *key)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char key[40];
    } packet;
#pragma pack(pop)

    if (!g_guildInfo.is_master)
        return 0;
    packet.id = CZ_REQ_DISORGANIZE_GUILD;
    strncpy(packet.key, key, 0x27);
    packet.key[39] = 0;
    if (strcmp(key, string_cstr(((&g_guildInfo.name)))) != 0) {
        message_box_msg(0x16e);
        return 0;
    }
    send_packet(CZ_REQ_DISORGANIZE_GUILD, &packet);
    return 0;
}

int game_mode_msg_leave_guild(const char *reason)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned guild_id;
        unsigned aid;
        unsigned char_id;
        char reason[40];
    } packet;
#pragma pack(pop)

    packet.id = CZ_REQ_LEAVE_GUILD;
    packet.guild_id = g_guildInfo.guild_id;
    packet.aid = g_session1.ownAid;
    packet.char_id = g_session1.ownGid;
    if (g_useCharsetTag)
        strncpy(charset_prefix(packet.reason), reason, 0x24);
    else
        strncpy(packet.reason, reason, 0x27);
    packet.reason[39] = 0;
    send_packet(CZ_REQ_LEAVE_GUILD, &packet);
    return 0;
}

int game_mode_msg_ban_guild_member(const struct GuildBanTarget *who, const char *reason)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned guild_id;
        unsigned aid;
        unsigned char_id;
        char reason[40];
    } packet;
#pragma pack(pop)

    if (!g_guildInfo.is_master && !g_guildInfo.can_expel)
        return 0;
    packet.id = CZ_REQ_BAN_GUILD;
    packet.guild_id = who->guild_id;
    packet.aid = who->aid;
    packet.char_id = who->char_id;
    if (g_useCharsetTag)
        strncpy(charset_prefix(packet.reason), reason, 0x24);
    else
        strncpy(packet.reason, reason, 0x27);
    packet.reason[39] = 0;
    send_packet(CZ_REQ_BAN_GUILD, &packet);
    return 0;
}

#define MAKE_GUILD_REFUSED_COLOR 0x6e96ff
#define FULLWIDTH_SPACE "　"

int game_mode_msg_make_guild(const char *name, int b)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned char_id;
        char name[24];
    } packet;
#pragma pack(pop)
    char text[0x100];
    char pair[4];
    struct Player *player = game_current_player();

    if (!player)
        return 0;
    memcpy(pair, &b, 4);
    memcpy(pair, name, 2);
    if (strcmp(pair, FULLWIDTH_SPACE) == 0)
        goto no_space;
    memcpy(pair, name + strlen(name) - 2, 2);
    if (strcmp(pair, FULLWIDTH_SPACE) == 0)
        goto no_space;
    if (strchr(name, ' '))
        goto no_space;

    if (strlen(name) >= 0x18) {
        message_box_msg(0x213);
        return 0;
    }
    packet.id = CZ_REQ_MAKE_GUILD;
    packet.char_id = g_session1.ownGid;
    strncpy(packet.name, name, 0x17);
    packet.name[23] = 0;
    send_packet(CZ_REQ_MAKE_GUILD, &packet);
    return 0;

no_space:
    sprintf(text, msg_str(0x97b));
    if (strstr(text, "NO MSG"))
        sprintf(text, "Can NOT use space in Guild Name !!");
    chat_print(text, MAKE_GUILD_REFUSED_COLOR);
    return 0;
}

int game_mode_msg_guild_invite(struct GameMode *self, int aid)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int aid;
        int my_aid;
        int my_char_id;
    } packet;
#pragma pack(pop)
    struct GameActor *actor = mode_actor(self, aid);

    if (!actor)
        return 0;
    packet.id = CZ_REQ_JOIN_GUILD;
    packet.aid = aid;
    packet.my_char_id = g_session1.ownGid;
    packet.my_aid = g_session1.ownAid;
    send_packet(CZ_REQ_JOIN_GUILD, &packet);
    return 0;
}

int game_mode_msg_guild_invite_by_name(const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char name[24];
    } packet;
#pragma pack(pop)
    struct String line;

    if (!g_guildInfo.guild_id || !g_guildInfo.can_invite)
        return 0;
    packet.id = CZ_REQ_JOIN_GUILD2;
    if (strlen(name) >= 0x18) {
        message_box_msg(0x213);
        return 0;
    }
    strncpy(packet.name, name, 0x18);
    send_packet(CZ_REQ_JOIN_GUILD2, &packet);
    string_init(&line);
    string_format(&line, msg_str(0x17e), name);
    chat_print(string_cstr(&line), 0xffff00);
    string_free(&line);
    return 0;
}

int game_mode_msg_guild_notice(int guild_id, const char *subject, const char *notice)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int guild_id;
        char subject[60];
        char notice[120];
    } packet;
#pragma pack(pop)

    packet.id = CZ_GUILD_NOTICE;
    packet.guild_id = guild_id;
    if (g_useCharsetTag) {
        strncpy(charset_prefix(packet.subject), subject, 0x38);
        strncpy(charset_prefix(packet.notice), notice, 0x74);
    } else {
        strncpy(packet.subject, subject, 0x3b);
        strncpy(packet.notice, notice, 0x77);
    }
    packet.subject[59] = 0;
    packet.notice[119] = 0;
    send_packet(CZ_GUILD_NOTICE, &packet);
    return 0;
}

int game_mode_msg_give_manner_point(int aid, int type, int points)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int aid;
        unsigned char type;
        unsigned short points;
    } packet;
#pragma pack(pop)
    int clamped;

    if (g_serverType == 3)
        return 0;
    packet.id = 0x149;
    packet.aid = aid;
    packet.type = (unsigned char)type;
    clamped = points > 0 ? points : 0;
    if (clamped >= 30000)
        packet.points = 30000;
    else
        packet.points = points > 0 ? (unsigned short)points : 0;
    send_packet(0x149, &packet);
    return 0;
}

int game_mode_msg_guild_ally(int aid)
{
#pragma pack(push, 1)
    struct { unsigned short id; int aid, my_aid, my_char_id; } packet;
#pragma pack(pop)

    packet.id = CZ_REQ_ALLY_GUILD;
    packet.my_aid = g_session1.ownAid;
    packet.my_char_id = g_session1.ownGid;
    packet.aid = aid;
    send_packet(CZ_REQ_ALLY_GUILD, &packet);
    return 0;
}
