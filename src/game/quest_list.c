#include "ui/window_messages.h"
#include "game/quest_packets.h"
#include <limits.h>
#include "session/map_names.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <string.h>
#include <time.h>
#include "contents/quest.h"
#include "game/gamemode.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_names.h"
#include <stdint.h>

#define UIM_CHAT_LINE 1
#define QUEST_CHAT_COLOR 0xffff
#define CHAT_CHANNEL_QUEST 0x12
#define WINDOW_NOTIFY_QUEST 0x8f

#define QUEST_HUNT_NAME RENEW_QUEST_HUNT_NAME

static void quest_info_init(struct RenewQuestB *info)
{
    quest_hunt_construct(info);
}

static void quest_info_destroy(struct RenewQuestB *info)
{
    quest_hunt_destroy(info);
}

static void quest_info_set_end_time(struct RenewQuestB *info, int seconds, int has_time)
{
    __time32_t timestamp = seconds;
    struct tm local;

    if (_localtime32_s(&local, &timestamp) != 0) {
        info->plain.deadline.year = info->plain.deadline.month = info->plain.deadline.day = 0;
        info->plain.deadline.hour = info->plain.deadline.minute = 0;
        return;
    }
    info->plain.deadline.year = (unsigned short)(local.tm_year + 1900);
    info->plain.deadline.month = (unsigned short)(local.tm_mon + 1);
    info->plain.deadline.day = (unsigned short)local.tm_mday;
    info->plain.deadline.hour = has_time ? (unsigned short)local.tm_hour : 0;
    info->plain.deadline.minute = has_time ? (unsigned short)local.tm_min : 0;
}

static void quest_info_set(struct RenewQuestB *info, unsigned char active, int end_time, unsigned char hunts)
{
    info->base.active = active;
    quest_info_set_end_time(info, end_time < 0 ? 0 : end_time, end_time != 0);
    info->plain.hunt_count = hunts;
}

static void quest_info_set_hunt(struct RenewQuestB *info, int i, int64_t hunt_id, int mob_type, int mob_id,
                                short killed, short to_kill, short min_level, short max_level)
{
    if ((unsigned)i >= RENEW_QUEST_HUNT_MAX)
        return;
    info->plain.hunt_id[i] = hunt_id;
    info->plain.hunt_flags[i] = mob_type;
    info->plain.hunt_mob[i] = mob_id;
    info->plain.hunt_done[i] = killed;
    info->plain.hunt_goal[i] = to_kill;
    info->plain.hunt_min_level[i] = min_level;
    info->plain.hunt_max_level[i] = max_level;
}

static void quest_hunt_name(char *out, int mob_id, const char *packet_name)
{
    struct String rsw;
    const char *name;
    char packet_text[25];

    memcpy(packet_text, packet_name, 24);
    packet_text[24] = 0;
    packet_name = packet_text;

    if ((mob_id > 1000 && mob_id < 3999) || (unsigned)(mob_id - 20021) <= 0x2eca) {
        name = packet_name;
    } else if (mob_id == 0x1099) {
        name = msg_str(0xde6);
    } else if (mob_id == 0x109a) {
        name = msg_str(0xde7);
    } else if (mob_id == 0x287b) {
        string_init(&rsw);
        string_format(&rsw, "%s.rsw", packet_name);
        name = session_map_display_name(string_cstr(&rsw));
        _snprintf_s(out, QUEST_HUNT_NAME, (size_t)-1, "%s", name);
        string_free(&rsw);
        return;
    } else {
        name = session_get_job_name(&g_session1, mob_id, 0x63);
    }
    _snprintf_s(out, QUEST_HUNT_NAME, (size_t)-1, "%s", name);
}

#pragma pack(push, 1)
struct PacketQuestList {
    unsigned short packet_type;
    unsigned short length;
    int count;
    unsigned char quests[1];
};
struct PacketQuest {
    int quest_id;
    unsigned char active;
    int start_time;
    int end_time;
    short hunt_count;
};
struct PacketQuestHunt3 {
    int hunt_id;
    int mob_type;
    int mob_id;
    short min_level, max_level;
    short killed, to_kill;
    char mob_name[24];
};
struct PacketQuestHunt4 {
    int64_t hunt_id;
    int mob_type;
    int mob_id;
    short min_level, max_level;
    short killed, to_kill;
    char mob_name[24];
};
#pragma pack(pop)

static int quest_list_valid(const void *packet, size_t size, int wide)
{
    struct PacketQuestList header;
    size_t offset = offsetof(struct PacketQuestList, quests);
    size_t hunt_size = wide ? sizeof(struct PacketQuestHunt4) : sizeof(struct PacketQuestHunt3);
    int n;
    if (!packet || size < offset || size > USHRT_MAX)
        return 0;
    memcpy(&header, packet, offset);
    if (header.length != size || header.count < 0 ||
        (size_t)header.count > (size - offset) / sizeof(struct PacketQuest))
        return 0;
    for (n = 0; n < header.count; ++n) {
        struct PacketQuest quest;
        size_t bytes;
        if (size - offset < sizeof quest)
            return 0;
        memcpy(&quest, (const unsigned char *)packet + offset, sizeof quest);
        offset += sizeof quest;
        if (quest.hunt_count < 0 || quest.hunt_count > RENEW_QUEST_HUNT_MAX)
            return 0;
        bytes = (size_t)quest.hunt_count * hunt_size;
        if (bytes > size - offset)
            return 0;
        offset += bytes;
    }
    return offset == size;
}

static void quest_read_list(const void *packet, size_t size, int wide)
{
    struct PacketQuestList header;
    size_t offset = offsetof(struct PacketQuestList, quests);
    size_t hunt_size = wide ? sizeof(struct PacketQuestHunt4) : sizeof(struct PacketQuestHunt3);
    int n;
    if (!quest_list_valid(packet, size, wide))
        return;
    memcpy(&header, packet, offset);
    for (n = 0; n < header.count; ++n) {
        struct PacketQuest quest;
        struct RenewQuestB info;
        const unsigned char *hunts;
        int i;
        memcpy(&quest, (const unsigned char *)packet + offset, sizeof quest);
        offset += sizeof quest;
        hunts = (const unsigned char *)packet + offset;
        offset += (size_t)quest.hunt_count * hunt_size;
        if (renew_quest_set_active(g_questTable, quest.quest_id, quest.active))
            continue;
        quest_info_init(&info);
        quest_info_set(&info, quest.active, quest.end_time, (unsigned char)quest.hunt_count);
        for (i = 0; i < quest.hunt_count; ++i) {
            if (wide) {
                struct PacketQuestHunt4 hunt;
                memcpy(&hunt, hunts + (size_t)i * hunt_size, sizeof hunt);
                quest_info_set_hunt(&info, i, hunt.hunt_id, hunt.mob_type, hunt.mob_id,
                    hunt.killed, hunt.to_kill, hunt.min_level, hunt.max_level);
                quest_hunt_name(info.plain.hunt_name[i], hunt.mob_id, hunt.mob_name);
            } else {
                struct PacketQuestHunt3 hunt;
                memcpy(&hunt, hunts + (size_t)i * hunt_size, sizeof hunt);
                quest_info_set_hunt(&info, i, hunt.hunt_id, hunt.mob_type, hunt.mob_id,
                    hunt.killed, hunt.to_kill, hunt.min_level, hunt.max_level);
                quest_hunt_name(info.plain.hunt_name[i], hunt.mob_id, hunt.mob_name);
            }
        }
        renew_quest_read_ongoing(g_questTable, quest.quest_id, &info);
        quest_info_destroy(&info);
    }
    renew_quest_refresh_windows();
}

void game_mode_all_quest_list3(struct GameMode *self, const void *packet, size_t size)
{
    (void)self;
    quest_read_list(packet, size, 0);
}

void game_mode_all_quest_list4(struct GameMode *self, const void *packet, size_t size)
{
    (void)self;
    quest_read_list(packet, size, 1);
}

#pragma pack(push, 1)
struct PacketAddQuest {
    unsigned short packet_type;
    int quest_id;
    unsigned char active;
    int start_time;
    int end_time;
    short hunt_count;
    unsigned char hunts[1];
};
struct PacketAddQuestHunt {
    int hunt_id;
    int mob_type;
    int mob_id;
    short min_level, max_level;
    short killed;
    char mob_name[24];
};
struct PacketAddQuestHunt2 {
    int64_t hunt_id;
    int mob_type;
    int mob_id;
    short min_level, max_level;
    short killed;
    char mob_name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PacketAddQuestHunt, 0x2a);
SIZE_ASSERT(PacketAddQuestHunt2, 0x2e);

static void quest_info_set_added(struct RenewQuestB *info, const struct PacketAddQuest *pkt)
{
    info->base.active = pkt->active;
    info->plain.hunt_count = (unsigned char)pkt->hunt_count;
    quest_info_set_end_time(info, pkt->end_time, pkt->end_time != 0);
}

static void quest_added(const struct PacketAddQuest *pkt, struct RenewQuestB *info)
{
    renew_quest_read_ongoing(g_questTable, pkt->quest_id, info);
    renew_quest_refresh_windows();
    ui_window_open(WINDOW_NOTIFY_QUEST);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(0x55e), QUEST_CHAT_COLOR,
                          CHAT_CHANNEL_QUEST, 0);
}

static void quest_read_added(const void *packet, size_t size, int wide)
{
    struct PacketAddQuest header;
    struct RenewQuestB info;
    size_t offset = offsetof(struct PacketAddQuest, hunts);
    size_t hunt_size = wide ? sizeof(struct PacketAddQuestHunt2) : sizeof(struct PacketAddQuestHunt);
    int i;
    if (!packet || size != offset + RENEW_QUEST_HUNT_MAX * hunt_size)
        return;
    memcpy(&header, packet, offset);
    if (header.hunt_count < 0 || header.hunt_count > RENEW_QUEST_HUNT_MAX)
        return;
    if (renew_quest_set_active(g_questTable, header.quest_id, header.active)) {
        renew_quest_refresh_windows();
        return;
    }
    quest_info_init(&info);
    quest_info_set_added(&info, &header);
    for (i = 0; i < header.hunt_count; ++i) {
        const unsigned char *source = (const unsigned char *)packet + offset + (size_t)i * hunt_size;
        if (wide) {
            struct PacketAddQuestHunt2 hunt;
            memcpy(&hunt, source, sizeof hunt);
            quest_info_set_hunt(&info, i, hunt.hunt_id, hunt.mob_type, hunt.mob_id,
                hunt.killed, 0, hunt.min_level, hunt.max_level);
            quest_hunt_name(info.plain.hunt_name[i], hunt.mob_id, hunt.mob_name);
        } else {
            struct PacketAddQuestHunt hunt;
            memcpy(&hunt, source, sizeof hunt);
            quest_info_set_hunt(&info, i, hunt.hunt_id, hunt.mob_type, hunt.mob_id,
                hunt.killed, 0, hunt.min_level, hunt.max_level);
            quest_hunt_name(info.plain.hunt_name[i], hunt.mob_id, hunt.mob_name);
        }
    }
    quest_added(&header, &info);
    quest_info_destroy(&info);
}

void game_mode_zc_add_quest_ex2(struct GameMode *self, const void *packet, size_t size)
{
    (void)self;
    quest_read_added(packet, size, 1);
}

void game_mode_zc_add_quest_ex(struct GameMode *self, const void *packet, size_t size)
{
    (void)self;
    quest_read_added(packet, size, 0);
}

static void quest_table_erase(unsigned quest_id)
{
    renew_quest_remove(g_questTable, quest_id);
}

void game_mode_del_quest(struct GameMode *self, const void *packet, size_t size)
{
    const unsigned char *pkt = packet;
    unsigned quest_id;
    void *quests;
    struct String title, line;

    (void)self;
    if (!pkt || size != 6)
        return;
    memcpy(&quest_id, pkt + 2, sizeof quest_id);
    string_copy(&title, &renew_quest_ongoing_info(g_questTable, quest_id)->base.text[RENEW_QUEST_TEXT_TITLE]);
    quests = g_questTable;
    quest_table_erase(quest_id);
    renew_quest_refresh_windows();
    string_init(&line);
    string_format(&line, msg_str(0x64b), string_cstr(&title));
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)string_cstr(&line), QUEST_CHAT_COLOR,
                          CHAT_CHANNEL_QUEST, 0);
    string_free(&line);
    string_free(&title);
}

void game_mode_zc_active_quest(const void *packet, size_t size)
{
    const unsigned char *bytes = packet;
    unsigned quest_id;
    if (!bytes || size != 7)
        return;
    memcpy(&quest_id, bytes + 2, sizeof quest_id);
    renew_quest_set_active(g_questTable, quest_id, (char)bytes[6]);
    renew_quest_refresh_windows();
}
