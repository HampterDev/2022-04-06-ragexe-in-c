#include "game/packet_handlers.h"
#include "iteminfo/item_api.h"
#include "ui/window_access.h"
#include <string.h>
#include <stdlib.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "game/item_info.h"
#include "game/guild_storage.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include <stdint.h>
#include "core/cstr.h"

enum {
    WID_GUILD_STORAGE = 0x21,
    WID_GUILD_STORAGE_LOG = 0xfd,
};
enum { MSG_WND_CLOSE_REQUEST = 6 };

static void log_push_copy(struct Vec *log, const struct GuildStorageLogEntry *entry)
{
    struct GuildStorageLogEntry copy;

    guild_log_entry_construct_copy(&copy, entry);
    vec_push(log, &copy, sizeof copy);
}

static void storage_line(int msg)
{
    char text[0x100];

    memset(text, 0, 0xfa);
    sprintf(text, msg_str(msg));
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)text, CHAT_COLOR_RED, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_OPEN_GUILD_STORAGE {
    unsigned short id;
    unsigned short result;
    short count;
};
struct PACKET_ZC_ACK_CLOSE_GUILD_STORAGE {
    unsigned short id;
    unsigned short result;
};
#pragma pack(pop)

void game_mode_ack_open_guild_storage(const struct PACKET_ZC_ACK_OPEN_GUILD_STORAGE *packet)
{
    switch (packet->result) {
    case 0:
        storage_line(0x9c0);
        break;
    case 2:
        storage_line(0x9c1);
        break;
    case 3:
        g_session1.guildStorageInUse = 1;
        storage_line(0x9c2);
        break;
    case 1:
        g_session1.guildStorageOpen = 1;
        g_session1.guildStorageCount = packet->count;
        if (packet->count == 0)
            storage_line(0x9c5);
        break;
    }
}

void game_mode_ack_close_guild_storage(const struct PACKET_ZC_ACK_CLOSE_GUILD_STORAGE *packet)
{
    struct UIWindow *wnd;

    if (packet->result == 2) {
        ui_window_mgr_message_box(&g_windowMgr, msg_str(0x9f5), 0, 1, 1, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
        wnd = ui_window_find(WID_GUILD_STORAGE);
    } else {
        wnd = ui_window_find(WID_GUILD_STORAGE);
        if (wnd == 0)
            goto closed;
    }
    ui_window_send(wnd, NULL, MSG_WND_CLOSE_REQUEST, 0xc1, 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WID_GUILD_STORAGE);
closed:
    session_clear_storage(&g_session1);
    g_session1.guildStorageOpen = 0;
    g_session1.guildStorageCount = 0;
    g_session1.guildStorageInUse = 0;
}

#pragma pack(push, 1)
struct GuildStorageLogRecord {
    int id;
    int item_id;
    int amount;
    unsigned char action;
    int refine;
    int64_t unique_id;
    unsigned char identified;
    short item_type;
    int cards[4];
    char name[24];
    char time[24];
    unsigned char attribute;
};
struct PACKET_ZC_ACK_GUILDSTORAGE_LOG {
    unsigned short id;
    unsigned short length;
    unsigned short result;
    short count;
    struct GuildStorageLogRecord records[1];
};
struct LogText {
    int id;
    int amount;
    char action;
    char name[24];
    char time[24];
};
#pragma pack(pop)
SIZE_ASSERT(GuildStorageLogRecord, 0x5d);

static void log_append(const struct GuildStorageLogRecord *record)
{
    struct ItemInfo item;
    struct LogText staged;
    struct GuildStorageLogEntry entry;
    struct String display_name = {0};
    char digits[16];

    item_info_construct(&item);
    staged.id = record->id;
    staged.amount = record->amount;
    staged.action = record->action;
    strcpy(staged.name, record->name);
    strcpy(staged.time, record->time);
    memset(digits, 0, sizeof digits);
    snprintf(digits, sizeof digits, "%d", record->item_id);
    string_assign_cstr(&item.id_text, digits);
    item.identified = record->identified;
    item.type = record->item_type;
    item.refine = record->refine;
    memcpy(item.slot, record->cards, sizeof item.slot);

    string_init(&entry.item_name);
    entry.id = 0;
    entry.amount = 0;
    entry.action = 0;
    entry.identified = 0;
    entry.item_type = 0;
    entry.refine = 0;
    entry.attribute = 0;
    memset(entry.name, 0, sizeof entry.name);
    memset(entry.time, 0, sizeof entry.time);
    memset(entry.cards, 0, sizeof entry.cards);

    entry.id = staged.id;
    entry.amount = staged.amount;
    entry.action = staged.action;
    strcpy(entry.name, staged.name);
    strcpy(entry.time, staged.time);
    item_info_get_display_name(&item, &display_name);
    string_assign(&entry.item_name, string_cstr(&display_name), display_name.size);
    string_free(&display_name);
    entry.identified = item.identified;
    entry.item_type = item.type;
    entry.refine = item.refine;
    entry.attribute = record->attribute;
    memcpy(entry.cards, item.slot, sizeof entry.cards);
    log_push_copy(&g_session1.guildStorageLog, &entry);
    string_free(&entry.item_name);
    item_info_destruct(&item);
}

void game_mode_ack_guild_storage_log(const struct PACKET_ZC_ACK_GUILDSTORAGE_LOG *packet)
{
    int i;

    if (packet->result == 0) {
        for (i = 0; i < packet->count; i++)
            log_append(&packet->records[i]);
    } else if (packet->result == 1) {
        struct GuildStorageLogEntry probe;
        struct GuildStorageLogEntry *first, *last, *e;
        void *comparator;

        for (i = 0; i < packet->count; i++)
            log_append(&packet->records[i]);
        guild_log_entry_construct(&probe);
        last = g_session1.guildStorageLog.last;
        first = g_session1.guildStorageLog.first;
        guild_log_sort(first, last, (int)(last - first), *std_make_move_iterator(&comparator, &probe));
        string_free(&probe.item_name);
        for (e = g_session1.guildStorageLog.first; e != g_session1.guildStorageLog.last; e++)
            log_push_copy(e->action == 1 ? &g_session1.guildStorageLogPut : &g_session1.guildStorageLogTaken, e);
        ui_window_open(WID_GUILD_STORAGE_LOG);
    } else {
        storage_line(packet->result == 2 ? 0x9ef : 0x718);
        guild_log_clear(&g_session1.guildStorageLog);
        guild_log_clear(&g_session1.guildStorageLogPut);
        guild_log_clear(&g_session1.guildStorageLogTaken);
    }
}

#pragma pack(push, 1)
struct GuildStorageLogRecord2 {
    int id;
    int item_id;
    int amount;
    unsigned char action;
    int64_t unique_id;
    unsigned char identified;
    short item_type;
    int cards[4];
    char name[24];
    char time[24];
    unsigned char attribute;
    unsigned char refine;
    unsigned char grade;
};
struct PACKET_ZC_ACK_GUILDSTORAGE_LOG2 {
    unsigned short id;
    unsigned short length;
    unsigned short result;
    short count;
    struct GuildStorageLogRecord2 records[1];
};
#pragma pack(pop)
SIZE_ASSERT(GuildStorageLogRecord2, 0x5b);

struct GuildLogItem {
    struct ItemInfo item;
    unsigned char attribute;
};

static void storage_line_s(int msg)
{
    char text[0x100];

    memset(text, 0, 0xfa);
    snprintf(text, 0xfa, msg_str(msg));
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)text, CHAT_COLOR_RED, 0, 0);
}

void game_mode_enter_guild_storage_log(int result, const struct Vec *items)
{
    const struct GuildLogItem *it;

    for (it = items->first; it != items->last; it++) {
        struct GuildStorageLogEntry entry;
        struct String display_name = {0};

        string_init(&entry.item_name);
        entry.id = 0;
        entry.amount = 0;
        entry.action = 0;
        entry.identified = 0;
        entry.item_type = 0;
        entry.refine = 0;
        entry.attribute = 0;
        memset(entry.name, 0, sizeof entry.name);
        memset(entry.time, 0, sizeof entry.time);
        memset(entry.cards, 0, sizeof entry.cards);

        entry.id = it->item.guild_log.id;
        entry.amount = it->item.guild_log.amount;
        entry.action = it->item.guild_log.action;
        cstr_copy(entry.name, sizeof entry.name, it->item.guild_log.name);
        cstr_copy(entry.time, sizeof entry.time, it->item.guild_log.time);
        item_info_get_display_name(&it->item, &display_name);
        string_assign(&entry.item_name, string_cstr(&display_name), display_name.size);
        string_free(&display_name);
        entry.identified = it->item.identified;
        entry.item_type = it->item.type;
        entry.refine = it->item.refine;
        entry.attribute = it->attribute;
        memcpy(entry.cards, it->item.slot, sizeof entry.cards);
        log_push_copy(&g_session1.guildStorageLog, &entry);
        string_free(&entry.item_name);
    }
    if ((short)result == 0)
        return;
    if ((short)result == 1) {
        struct GuildStorageLogEntry probe;
        struct GuildStorageLogEntry *first, *last, *e;
        void *comparator;

        guild_log_entry_construct(&probe);
        last = g_session1.guildStorageLog.last;
        first = g_session1.guildStorageLog.first;
        guild_log_sort(first, last, (int)(last - first), *std_make_move_iterator(&comparator, &probe));
        string_free(&probe.item_name);
        for (e = g_session1.guildStorageLog.first; e != g_session1.guildStorageLog.last; e++)
            log_push_copy(e->action == 1 ? &g_session1.guildStorageLogPut : &g_session1.guildStorageLogTaken, e);
        ui_window_open(WID_GUILD_STORAGE_LOG);
        return;
    }
    storage_line_s((short)result == 2 ? 0x9ef : 0x718);
    guild_log_clear(&g_session1.guildStorageLog);
    guild_log_clear(&g_session1.guildStorageLogPut);
    guild_log_clear(&g_session1.guildStorageLogTaken);
}

void game_mode_ack_guild_storage_log2(const struct PACKET_ZC_ACK_GUILDSTORAGE_LOG2 *packet)
{
    struct Vec items = { 0, 0, 0 };
    struct GuildLogItem *logged;
    unsigned count = (unsigned)(int)packet->count;
    int i;

    vec_reserve(&items, count, sizeof(struct GuildLogItem));
    if (packet->result == 0 || packet->result == 1) {
        for (i = 0; i < packet->count; i++) {
            const struct GuildStorageLogRecord2 *record = &packet->records[i];
            struct ItemInfo item;
            struct GuildLogItem staged;
            char digits[16];

            item_info_construct(&item);
            item.guild_log.id = record->id;
            item.guild_log.amount = record->amount;
            item.guild_log.action = record->action;
            cstr_copy(item.guild_log.name, sizeof item.guild_log.name, record->name);
            cstr_copy(item.guild_log.time, sizeof item.guild_log.time, record->time);
            memset(digits, 0, sizeof digits);
            snprintf(digits, sizeof digits, "%d", record->item_id);
            string_assign_cstr(&item.id_text, digits);
            item.identified = record->identified;
            item.type = record->item_type;
            item.refine = record->refine;
            memcpy(item.slot, record->cards, sizeof item.slot);
            item_info_copy(&staged.item, &item);
            staged.attribute = record->attribute;
            vec_push(&items, &staged, sizeof staged);
            item_info_destruct(&item);
        }
    }
    game_mode_enter_guild_storage_log(packet->result, &items);
    for (logged = items.first; logged != (struct GuildLogItem *)items.last; logged++)
        item_info_destruct(&logged->item);
    vec_free(&items);
}

void guild_log_clear(struct Vec *v)
{
    struct GuildStorageLogEntry *it = v->first, *last = v->last;

    for (; it != last; it++)
        string_free(&it->item_name);
    vec_clear(v);
}

void guild_log_free(struct Vec *v)
{
    guild_log_clear(v);
    vec_free(v);
}
