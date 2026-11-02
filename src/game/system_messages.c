#include "game/packet_handlers.h"
#include "skills/skill_info.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "core/memory.h"
#include "platform/win_types.h"
#include <stdlib.h>
#include <string.h>
#include "game/system_messages.h"
#include "platform/language.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "platform/clientinfo.h"

enum {
    MSG_CALL_BLOCKED = 0xb92,
    CHAT_CHANNEL_CALL_NOTICE = 0x15,
    MSG_ITEM_VALUE = 0xcce,
};

#define UIM_REFRESH  3
#define UIM_LOG_LINE 0x13
#define COLOUR_WHITE 0xffffff
#define COLOUR_RED   0xff
#define COLOUR_YELLOW 0xffff00

static void chat(int msg, const char *text, int colour, int channel)
{
    ui_window_mgr_send_msg_(&g_windowMgr, msg, (intptr_t)text, colour, channel, 0);
}

static int is_warning(unsigned short msg_id)
{
    if (msg_id > 0x758)
        return msg_id == 0x886;
    if (msg_id >= 0x757)
        return 1;
    return msg_id >= 0x4c5 && msg_id <= 0x4c6;
}

static void show_line(unsigned short msg_id, int colour)
{
    chat(UIM_REFRESH, 0, 0, 0);
    chat(UIM_PUSH_INTO_CHAT, msg_str(msg_id), colour, 0);
    chat(UIM_LOG_LINE, msg_str(msg_id), colour, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_MSG {
    unsigned short id;
    unsigned short msg_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MSG, 4);

void game_mode_msg(const unsigned char *pkt)
{
    const struct PACKET_ZC_MSG *packet = (const struct PACKET_ZC_MSG *)pkt;
    unsigned short msg_id = packet->msg_id;

    show_line(msg_id, is_warning(msg_id) ? COLOUR_RED : COLOUR_WHITE);
}

#pragma pack(push, 1)
struct PACKET_ZC_MSG_COLOR {
    unsigned short id;
    unsigned short msg_id;
    int colour;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MSG_COLOR, 8);

void game_mode_msg_color(const unsigned char *pkt)
{
    const struct PACKET_ZC_MSG_COLOR *packet = (const struct PACKET_ZC_MSG_COLOR *)pkt;
    unsigned short msg_id = packet->msg_id;
    int colour = packet->colour;

    show_line(msg_id, is_warning(msg_id) ? COLOUR_RED : colour);
}

#pragma pack(push, 1)
struct PACKET_ZC_MSG_VALUE {
    unsigned short id;
    unsigned short msg_id;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MSG_VALUE, 8);

void game_mode_msg_value(const unsigned char *pkt)
{
    const struct PACKET_ZC_MSG_VALUE *packet = (const struct PACKET_ZC_MSG_VALUE *)pkt;
    char line[500];
    const char *name;
    struct ItemInfo item;
    unsigned short msg_id = packet->msg_id;

    memset(line, 0, sizeof line);
    if (msg_id == MSG_ITEM_VALUE) {
        item_info_construct(&item);
        item_info_set_item_id(&item, packet->value);
        name = item_info_get_name(&item);
        snprintf(line, sizeof line, msg_str(MSG_ITEM_VALUE), name);
        item_info_destruct(&item);
    } else {
        sprintf(line, msg_str(msg_id), packet->value);
    }
    if (strlen(line) != 0)
        chat(UIM_PUSH_INTO_CHAT, line, 0xffff, 0);
}

void game_mode_on_msg_value_string(struct GameMode *self, unsigned short msg_id, int color, const char *value)
{
    struct String line;

    (void)self;
    string_init(&line);
    string_format(&line, msg_str(msg_id), value);
    if (line.size != 0)
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)string_cstr(&line), color,
                            msg_id == MSG_CALL_BLOCKED ? CHAT_CHANNEL_CALL_NOTICE : 0, 0);
    string_free(&line);
}

#pragma pack(push, 1)
struct PACKET_ZC_FORMATSTRING_MSG {
    unsigned short id;
    unsigned short length;
    unsigned short msg_id;
    char strings[];
};
#pragma pack(pop)

void game_mode_format_string_msg(const unsigned char *pkt)
{
    const struct PACKET_ZC_FORMATSTRING_MSG *packet = (const struct PACKET_ZC_FORMATSTRING_MSG *)pkt;
    char line[0x105];
    const char *fmt = msg_str(packet->msg_id);
    const char *p = fmt;
    const char *arg1, *arg2, *arg3;
    unsigned count = 0;

    while (*p != 0) {
        if (platform_code_page_is_system_lead_byte((unsigned char)*p))
            p += 2;
        else if (p[0] == '%' && p[1] == 's') {
            count++;
            p += 2;
        } else
            p++;
    }
    memset(line, 0, sizeof line);
    arg1 = packet->strings;
    switch (count) {
    case 0:
        snprintf(line, sizeof line, fmt);
        break;
    case 1:
        snprintf(line, sizeof line, fmt, arg1);
        break;
    case 2:
        arg2 = arg1 + strlen(arg1) + 1;
        snprintf(line, sizeof line, fmt, arg1, arg2);
        break;
    case 3:
        arg2 = arg1 + strlen(arg1) + 1;
        arg3 = arg2 + strlen(arg2) + 1;
        snprintf(line, sizeof line, fmt, arg1, arg2, arg3);
        break;
    default:
        sprintf(line, "unknown argment list");
        return;
    }
    chat(UIM_PUSH_INTO_CHAT, line, COLOUR_WHITE, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_NORMAL_SYSTEM_MSG {
    unsigned short id;
    unsigned short length;
    char text[];
};
#pragma pack(pop)

void game_mode_zc_normal_system_msg(const unsigned char *pkt)
{
    const struct PACKET_ZC_NORMAL_SYSTEM_MSG *packet = (const struct PACKET_ZC_NORMAL_SYSTEM_MSG *)pkt;
    struct String text;

    string_init(&text);
    string_assign(&text, packet->text, (size_t)((const char *)pkt + packet->length - packet->text));
    chat(UIM_REFRESH, 0, 0, 0);
    chat(UIM_PUSH_INTO_CHAT, string_cstr(&text), 0xffff, 0);
    string_free(&text);
}

#pragma pack(push, 1)
struct PACKET_ZC_DEBUGMSG {
    unsigned short id;
    unsigned short length;
    int colour;
    char text[];
};
#pragma pack(pop)

void game_mode_zc_debug_msg(const unsigned char *pkt)
{
    const struct PACKET_ZC_DEBUGMSG *packet = (const struct PACKET_ZC_DEBUGMSG *)pkt;
    struct String text;

    string_init(&text);
    string_assign(&text, packet->text, (size_t)((const char *)pkt + packet->length - packet->text));
    chat(UIM_PUSH_INTO_CHAT, string_cstr(&text), packet->colour, 0);
    string_free(&text);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_DISCONNECT_CHARACTER {
    unsigned short id;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_DISCONNECT_CHARACTER, 3);

void game_mode_zc_ack_disconnect_character(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_DISCONNECT_CHARACTER *packet = (const struct PACKET_ZC_ACK_DISCONNECT_CHARACTER *)pkt;
    char line[0x400];

    sprintf(line, msg_str(packet->result != 0 ? 0x97 : 0x98));
    chat(UIM_REFRESH, 0, 0, 0);
    chat(UIM_PUSH_INTO_CHAT, line, 0x32ff, 0);
}

static const char *count_in_words(unsigned short count)
{
    if (count <= 1000)
        return msg_str(0x339);
    if (count <= 3000)
        return msg_str(0x33a);
    if (count <= 6000)
        return msg_str(0x33b);
    if (count <= 10000)
        return msg_str(0x33c);
    return msg_str(0x33d);
}
#define SERVICE_TYPE_CHINA 3
#define MSG_USER_COUNT                0xb2

#pragma pack(push, 1)
struct PACKET_ZC_USER_COUNT {
    unsigned short id;
    int count;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_USER_COUNT, 6);

void game_mode_zc_user_count(const unsigned char *pkt)
{
    const struct PACKET_ZC_USER_COUNT *packet = (const struct PACKET_ZC_USER_COUNT *)pkt;
    char line[0x100];

    if (g_serviceType == SERVICE_TYPE_CHINA && !clientinfo_is_admin_aid(g_session1.ownAid))
        strcpy(line, count_in_words((unsigned short)packet->count));
    else
        sprintf(line, msg_str(MSG_USER_COUNT), packet->count);
    chat(UIM_REFRESH, 0, 0, 0);
    chat(UIM_PUSH_INTO_CHAT, line, 0xffff, 0);
}

void game_mode_zc_throw_mvp_item(void)
{
    chat(UIM_REFRESH, 0, 0, 0);
    chat(UIM_PUSH_INTO_CHAT, msg_str(0x93), COLOUR_RED, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_MVP_GETTING_ITEM {
    unsigned short id;
    int item_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MVP_GETTING_ITEM, 6);

void game_mode_item_name_msg(const unsigned char *pkt)
{
    const struct PACKET_ZC_MVP_GETTING_ITEM *packet = (const struct PACKET_ZC_MVP_GETTING_ITEM *)pkt;
    char id_text[25];
    struct ItemInfo item;
    struct String line;
    struct String name = {0};

    snprintf(id_text, sizeof id_text, "%d", packet->item_id);
    id_text[24] = 0;
    item_info_construct(&item);
    string_assign_cstr(&item.id_text, id_text);
    item.refine = 0;
    item.identified = 0;
    chat(UIM_REFRESH, 0, 0, 0);
    const char *after, *before;

    item_info_get_display_name(&item, &name);
    after = msg_str(0x90);
    before = msg_str(0x8f);
    string_init(&line);
    string_format(&line, "%s%s", before, string_cstr(&name), after);
    chat(UIM_PUSH_INTO_CHAT, string_cstr(&line), COLOUR_YELLOW, 0);
    string_free(&line);
    string_free(&name);
    item_info_destruct(&item);
}

void game_mode_zc_mvp_getting_special_exp(const unsigned char *pkt)
{
    char line[0x100];
    const char *first, *second;

    (void)pkt;
    memset(line, 0, sizeof line);
    second = msg_str(0x92);
    first = msg_str(0x91);
    snprintf(line, sizeof line, "%s%s", first, second);
    chat(UIM_REFRESH, 0, 0, 0);
    chat(UIM_PUSH_INTO_CHAT, line, COLOUR_YELLOW, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_SKILLMSG {
    unsigned short id;
    int msg;
};
#pragma pack(pop)

#define SKILLMSG_COLOR 0x9b9bff
#define CHAT_CHANNEL_SKILLMSG 7

void game_mode_zc_skill_msg(struct GameMode *self, const struct PACKET_ZC_SKILLMSG *packet)
{
    int line;

    (void)self;
    switch (packet->msg) {
    case 0x15: case 0x16: case 0x17: case 0x18: case 0x19:
        line = 0x379 + (packet->msg - 0x15);
        break;
    case 0x1c: case 0x1d: case 0x1e: case 0x1f: case 0x20:
        line = 0x37e + (packet->msg - 0x1c);
        break;
    case 0x28:
        line = 0x393;
        break;
    default:
        return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(line), SKILLMSG_COLOR,
                        CHAT_CHANNEL_SKILLMSG, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_HUNTINGLIST {
    unsigned short id;
    short length;
    char text[1];
};
#pragma pack(pop)

#define HUNTING_LIST_MAX        5000
#define HUNTING_FRAME_COLOR     0x9bcd9b
#define HUNTING_TEXT_COLOR      0xcdffcd

void game_mode_zc_hunting_list(struct GameMode *self, const struct PACKET_ZC_HUNTINGLIST *packet)
{
    char line[0x100];
    int len = packet->length - 4;
    char *text;

    (void)self;
    if (len > HUNTING_LIST_MAX)
        len = HUNTING_LIST_MAX;
    text = mem_alloc(len + 1);
    memcpy(text, packet->text, len);
    text[len] = 0;
    sprintf(line, "=========== HUNTING LIST =============");
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)line, HUNTING_FRAME_COLOR, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)text, HUNTING_TEXT_COLOR, 0, 0);
    sprintf(line, "========================================");
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)line, HUNTING_FRAME_COLOR, 0, 0);
}
