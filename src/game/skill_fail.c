#include "game/packet_handlers.h"
#include "skills/skill_info.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include <string.h>
#include "game/gamemode.h"
#include "game/client.h"
#include "game/item_info.h"
#include "core/msg_str.h"
#include "core/text_util.h"

#pragma pack(push, 1)
struct PACKET_ZC_ACK_TOUSESKILL {
    unsigned short id;
    unsigned short skill_id;
    int number;
    unsigned item_id;
    unsigned char flag;
    unsigned char cause;
};
#pragma pack(pop)

#define SKILL_FAIL_VERBOSE (g_session1.showSkillFail == 1)

#define SKILL_FAIL_CHANNEL 0xa
enum { NV_BASIC = 1, AL_WARP = 0x1b, TF_STEAL = 0x32, TF_POISON = 0x34 };
enum { ACTOR_MSG_EFFECT = 0x8f };

static const unsigned short g_plainCauseMessage[112] = {
      0,     0xca,  0xcb,  0x328, 0,     0xe9,  0xef,  0xf6,  0xf7,  0x244,
      0,     0x574, 0x575, 0x576, 0x577, 0x578, 0x579, 0x583, 0x584, 0x585,
      0x586, 0x587, 0x588, 0x589, 0x591, 0x748, 0x593, 0x594, 0x789, 0x78c,
      0x791, 0x599, 0x59a, 0x59b, 0x79c, 0x7a0, 0x79f, 0x59f, 0,     0,
      0x5aa, 0,     0,     0x5ad, 0,     0,     0,     0,     0,     0,
      0x5b6, 0x5c2, 0x5c3, 0x5c4, 0x5c5, 0,     0,     0x5ef, 0,     0,
      0x5f2, 0,     0,     0,     0,     0,     0,     0,     0,     0,
      0,     0,     0,     0,     0,     0,     0,     0,     0,     0x6d4,
      0x71f, 0x720, 0x721, 0x7a2, 0x9a2, 0,     0,     0xa03, 0xa42, 0xba6,
      0,     0,     0,     0xd9a, 0xd9b, 0xdc9, 0xdca, 0xdcd, 0,     0xe14,
      0xe3b, 0,     0xf75, 0xf76, 0xf77, 0xf78, 0xf79, 0xf7a, 0xf7b, 0xf7c,
      0xf7d, 0xf80,
};

static void fail_text(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)text, CHAT_COLOR_RED, SKILL_FAIL_CHANNEL, 0);
}

static void fail_line(int msg)
{
    fail_text(msg_str(msg));
}

static void fail_formatted(int msg, int number)
{
    struct String line;

    string_init(&line);
    string_format(&line, msg_str(msg), number);
    fail_text(string_cstr(&line));
    string_free(&line);
}

static void basic_skill_failed(struct GameMode *self, const struct PACKET_ZC_ACK_TOUSESKILL *packet)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
    if (packet->skill_id == NV_BASIC) {
        switch (packet->number) {
        case 0: fail_line(0x9f); break;
        case 1:
            fail_line(0xa0);
            break;
        case 2: fail_line(0xa1); break;
        case 3: fail_line(0xa2); break;
        case 4: fail_line(0xa3); break;
        case 5: fail_line(0xa4); break;
        case 6: fail_line(0xa5); break;
        case 7:
            fail_line(0x17f);
            fail_line(0x518);
            break;
        case 8: fail_line(0x518); break;
        }
        return;
    }
    switch (packet->skill_id) {
    case AL_WARP: fail_line(0xd6); break;
    case TF_STEAL: fail_line(0xcd); break;
    case TF_POISON: fail_line(0xcf); break;
    default:
        if (SKILL_FAIL_VERBOSE)
            fail_line(0xcc);
        break;
    }
}

static void item_failed(const struct PACKET_ZC_ACK_TOUSESKILL *packet, int with_number)
{
    struct ItemInfo item;
    struct String line;
    struct String name = {0};

    item_info_construct(&item);
    item_info_set_item_id(&item, (int)packet->item_id);
    if (!with_number)
        item.identified = 1;
    item_info_get_display_name(&item, &name);
    string_init(&line);
    if (with_number)
        string_format(&line, msg_str(0x600), string_cstr(&name), packet->number);
    else
        string_format(&line, msg_str(0x602), string_cstr(&name));
    fail_text(string_cstr(&line));
    string_free(&line);
    string_free(&name);
    item_info_destruct(&item);
}

static void sub_reason_failed(int number)
{
    switch (number) {
    case 0x15: fail_line(0xa01); break;
    case 0x11: fail_line(0x9fd); break;
    case 0x12: fail_line(0x9fc); break;
    case 0x14: fail_line(0x9fb); break;
    case 0x13: fail_line(0x9fa); break;
    case 0xb:  fail_line(0x69b); break;
    case 0xd:
    case 0xe:  fail_line(0x69c); break;
    case 4:    fail_line(0x747); break;
    case 0x16: fail_line(0x7ca); break;
    }
}

void game_mode_ack_to_use_skill(struct GameMode *self, const struct PACKET_ZC_ACK_TOUSESKILL *packet)
{
    struct Player *player = self->world->player;
    char text[0x100];
    const char *name;

    player->motion_held = 0;
    if (packet->flag != 0 || packet->cause > 111)
        return;
    switch (packet->cause) {
    case 0:
        basic_skill_failed(self, packet);
        return;
    case 4:
        if (!SKILL_FAIL_VERBOSE)
            return;
        name = skill_name(packet->skill_id);
        sprintf(text, msg_str(0x5b4), name);
        fail_text(text);
        return;
    case 10:
        switch (packet->skill_id) {
        }
        if (SKILL_FAIL_VERBOSE)
            fail_line(0x11d);
        return;
    case 71:
        item_failed(packet, 1);
        return;
    case 72:
        item_failed(packet, 0);
        return;
    case 73:
        if (packet->number == 0) {
            sprintf(text, msg_str(0x618), 0x618);
        } else {
            name = skill_name(packet->number);
            sprintf(text, msg_str(0x617), name);
        }
        fail_text(text);
        return;
    case 74:
        if (packet->number == 0)
            sprintf(text, msg_str(0x61a), 0x61a);
        else
            sprintf(text, msg_str(0x619), packet->number);
        fail_text(text);
        return;
    case 78:
        sub_reason_failed(packet->number);
        return;
    case 85:
        memset(text, 0, sizeof text);
        if (packet->number > 0)
            snprintf(text, sizeof text, msg_str(0x9f9), packet->number);
        else
            snprintf(text, sizeof text, msg_str(0x9f8), 0x9f8);
        fail_text(text);
        return;
    case 86:
        fail_line(packet->number);
        return;
    case 90: fail_formatted(0xbb5, packet->number); return;
    case 91: fail_formatted(0xbbe, packet->number); return;
    case 92: fail_formatted(0xd82, packet->number); return;
    case 98: fail_formatted(0xe00, packet->number); return;
    case 101: fail_formatted(0xebd, packet->number); return;
    }
    if (g_plainCauseMessage[packet->cause] != 0)
        fail_line(g_plainCauseMessage[packet->cause]);
}
