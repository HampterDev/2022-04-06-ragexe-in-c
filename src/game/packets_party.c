#include "game/packet_handlers.h"
#include "ui/window_types.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include <windows.h>
#include <string.h>
#include "game/packets.h"
#include "game/chat.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "ui/character/character.h"
#include "ui/social/party_board.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

#pragma pack(push, 1)
struct PACKET_ZC_PARTY_CONFIG {
    unsigned short id;
    BYTE refuse_invitations;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PARTY_CONFIG, 3);

void game_mode_zc_party_config(const unsigned char *pkt)
{
    const struct PACKET_ZC_PARTY_CONFIG *packet = (const struct PACKET_ZC_PARTY_CONFIG *)pkt;

    g_session1.partyInviteRefused = packet->refuse_invitations;
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(packet->refuse_invitations == 0 ? 0x52e : 0x52d), 0x64ffff, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_PARTY_JOIN_REQ_ACK_FROM_MASTER {
    unsigned short id;
    char player_name[24];
    char party_name[24];
    unsigned aid;
    unsigned result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PARTY_JOIN_REQ_ACK_FROM_MASTER, 58);

void game_mode_zc_party_join_req_ack_from_master(const unsigned char *pkt)
{
    const struct PACKET_ZC_PARTY_JOIN_REQ_ACK_FROM_MASTER *packet = (const struct PACKET_ZC_PARTY_JOIN_REQ_ACK_FROM_MASTER *)pkt;
    const char *player_name = packet->player_name;
    const char *party_name = packet->party_name;
    char line[500];
    const char *text;
    int color = 0xff;

    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    switch (packet->result) {
    case 0:
        memset(line, 0, sizeof line);
        if (g_session1.partyLeader == 1)
            _snprintf_s(line, sizeof line, (size_t)-1, msg_str(0xdad), player_name);
        else
            _snprintf_s(line, sizeof line, (size_t)-1, msg_str(0xdaa), party_name);
        text = line;
        color = 0xffff64;
        break;
    case 1:
        memset(line, 0, sizeof line);
        if (g_session1.partyLeader == 1)
            _snprintf_s(line, sizeof line, (size_t)-1, msg_str(0xdae), player_name);
        else
            _snprintf_s(line, sizeof line, (size_t)-1, msg_str(0xdab), party_name);
        text = line;
        break;
    case 2: text = msg_str(0xda9); break;
    case 3: text = msg_str(0xd72); break;
    case 4: text = msg_str(0xda8); break;
    case 5:
    case 6: text = msg_str(0xda4); break;
    case 7:
        memset(line, 0, sizeof line);
        _snprintf_s(line, sizeof line, (size_t)-1, msg_str(0xdac), player_name);
        text = line;
        break;
    case 8:
        memset(line, 0, sizeof line);
        _snprintf_s(line, sizeof line, (size_t)-1, msg_str(0xdaf), player_name);
        text = line;
        break;
    case 9:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0xda6), 0xff, 0, 0);
        return;
    case 10: text = msg_str(0xd73); break;
    case 11: text = msg_str(0xda7); break;
    case 12:
        memset(line, 0, sizeof line);
        _snprintf_s(line, sizeof line, (size_t)-1, msg_str(0x260), player_name);
        text = line;
        break;
    case 13: text = msg_str(0x718); break;
    default:
        return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)text, color, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_PARTY_REQ_MASTER_TO_JOIN {
    unsigned short id;
    unsigned aid;
    unsigned request_id;
    char name[24];
    unsigned short level;
    unsigned short job;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PARTY_REQ_MASTER_TO_JOIN, 38);

void game_mode_zc_party_req_master_to_join(const unsigned char *pkt)
{
    const struct PACKET_ZC_PARTY_REQ_MASTER_TO_JOIN *packet = (const struct PACKET_ZC_PARTY_REQ_MASTER_TO_JOIN *)pkt;
    char name[25];

    strncpy(name, packet->name, 24);
    name[24] = 0;
    ui_request_join_party_mgr_add_request(g_requestJoinPartyMgr, packet->request_id, packet->aid, name,
               packet->level, packet->job);
}

#define WND_PARTY_INVITATION    0x23

#pragma pack(push, 1)
struct PACKET_ZC_REQ_JOIN_GROUP {
    unsigned short id;
    int party_id;
    char party_name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_JOIN_GROUP, 30);

void game_mode_zc_req_join_group(const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_JOIN_GROUP *packet = (const struct PACKET_ZC_REQ_JOIN_GROUP *)pkt;
    char name[25];
    void *wnd;

    strncpy(name, packet->party_name, 24);
    name[24] = 0;
    string_assign_cstr(&g_session1.partyName, name);
    wnd = ui_window_open(WND_PARTY_INVITATION);
    wnd_send(wnd, 0x38, packet->party_id, (int)name, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_PARTY_JOIN_REQ_ACK {
    unsigned short id;
    char player_name[24];
    int result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_PARTY_JOIN_REQ_ACK, 30);

void game_mode_party_join_ack(const unsigned char *pkt)
{
    const struct PACKET_ZC_PARTY_JOIN_REQ_ACK *packet = (const struct PACKET_ZC_PARTY_JOIN_REQ_ACK *)pkt;
    const char *player_name = packet->player_name;
    struct String who, line;
    char text[256];
    int result = packet->result;

    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    string_init(&who);
    string_assign_cstr(&who, player_name);
    switch (result) {
    case 0:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x50), 0xff, 0, 0);
        break;
    case 1:
        string_copy(&line, &who);
        string_append_cstr(&line, msg_str(0x80c));
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&line), 0xff, 0, 0);
        string_free(&line);
        break;
    case 2:
        string_copy(&line, &who);
        string_append_cstr(&line, msg_str(0x80d));
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&line), 0xffff00, 0, 0);
        string_free(&line);
        break;
    case 3:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x53), 0xff, 0, 0);
        break;
    case 4:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x260), 0xff, 0, 0);
        break;
    }
    switch (result) {
    case 5:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x52c), 0xff, 0, 0);
        break;
    case 7:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x47), 0xff, 0, 0);
        break;
    case 8:
        sprintf(text, msg_str(0x56c), player_name);
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)text, 0xff, 0, 0);
        break;
    case 9:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x74f), 0xff, 0, 0);
        break;
    case 10:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0xbd3), 0xff, 0, 0);
        break;
    case 11:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0xc9a), 0xff, 0, 0);
        break;
    case 18:
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0xe32), 0xff, 0, 0);
        break;
    }
    string_free(&who);
}

#define CHAT_COLOR_YELLOW       0xffff

static void party_setting_line(const char *line)
{
    if (strlen(line) != 0)
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)line, CHAT_COLOR_YELLOW, 0xb, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_REQ_GROUPINFO_CHANGE_V2 {
    unsigned short id;
    int exp_option;
    BYTE item_pickup_rule;
    BYTE item_division_rule;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_GROUPINFO_CHANGE_V2, 8);

void game_mode_zc_req_groupinfo_change_v2(const unsigned char *pkt)
{
    const struct PACKET_ZC_REQ_GROUPINFO_CHANGE_V2 *packet = (const struct PACKET_ZC_REQ_GROUPINFO_CHANGE_V2 *)pkt;
    char line[0x80];
    const char *setting, *what, *how, *note;

    g_session1.partyExpOption = packet->exp_option;
    g_session1.partyItemPickupRule = packet->item_pickup_rule;
    g_session1.partyItemDivisionRule = packet->item_division_rule;
    if (g_windowMgr.party_setting_wnd)
        wnd_send(g_windowMgr.party_setting_wnd, 0x3c, 0, 0, 0, 0);
    memset(line, 0, sizeof line);
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    switch (g_session1.partyExpOption) {
    case 0:
    case 1:
        how = msg_str(g_session1.partyExpOption == 0 ? 0x11f : 0x120);
        what = msg_str(0x124);
        setting = msg_str(0x123);
        sprintf(line, "%s - %s : %s", setting, what, how);
        break;
    case 2:
        note = msg_str(0x5ff);
        how = msg_str(0x11f);
        what = msg_str(0x124);
        setting = msg_str(0x123);
        sprintf(line, "%s - %s : %s(%s)", setting, what, how, note);
        break;
    }
    party_setting_line(line);
    if (g_session1.partyItemPickupRule == 0 || g_session1.partyItemPickupRule == 1) {
        how = msg_str(g_session1.partyItemPickupRule == 0 ? 0x11f : 0x122);
        what = msg_str(0x125);
        setting = msg_str(0x123);
        sprintf(line, "%s - %s : %s", setting, what, how);
    }
    party_setting_line(line);
    if (g_session1.partyItemDivisionRule == 0 || g_session1.partyItemDivisionRule == 1) {
        how = msg_str(g_session1.partyItemDivisionRule == 0 ? 0x11f : 0x120);
        what = msg_str(0x2e2);
        setting = msg_str(0x123);
        sprintf(line, "%s - %s : %s", setting, what, how);
    }
    party_setting_line(line);
}

#define WND_PARTY               0x22

#pragma pack(push, 1)
struct PACKET_ZC_ACK_MAKE_GROUP {
    unsigned short id;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_MAKE_GROUP, 3);

void game_mode_zc_ack_make_group(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_MAKE_GROUP *packet = (const struct PACKET_ZC_ACK_MAKE_GROUP *)pkt;
    static const short refusal[4] = { 0, 0x4e, 0x4f, 0x56b };
    unsigned result = packet->result;

    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    if (result > 3)
        return;
    if (result == 0) {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x4d), 0xffff00, 0, 0);
        ui_window_open(WND_PARTY);
    } else {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(refusal[result]), 0xff, 0, 0);
    }
}

#define ACTOR_MSG_HP            0x22
#define PARTY_WND_NOTIFY        6
#define PARTY_NOTIFY_HP         0x22
#define MM_PARTY_MEMBER_AT      0x3f

static void member_hp(struct GameMode *self, unsigned aid, int hp, int max_hp)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, aid);

    if (actor)
        actor_send(actor, ACTOR_MSG_HP, hp, max_hp);
    if (g_windowMgr.messenger_group_wnd)
        wnd_send(g_windowMgr.messenger_group_wnd, PARTY_WND_NOTIFY, PARTY_NOTIFY_HP, aid, hp, max_hp);
    ui_window_mgr_command_plate_of(&g_windowMgr, aid, hp, max_hp);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_HP_TO_GROUPM {
    unsigned short id;
    unsigned aid;
    short hp;
    short max_hp;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_HP_TO_GROUPM, 10);

void game_mode_zc_notify_hp_to_group_m(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_HP_TO_GROUPM *packet = (const struct PACKET_ZC_NOTIFY_HP_TO_GROUPM *)pkt;
    unsigned aid = packet->aid;

    if (!session_is_mercenary_aid(&g_session1, aid))
        member_hp(self, aid, packet->hp, packet->max_hp);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_HP_TO_GROUPM_R2 {
    unsigned short id;
    unsigned aid;
    int hp;
    int max_hp;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_HP_TO_GROUPM_R2, 14);

void game_mode_zc_notify_hp_to_group_m_r2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_HP_TO_GROUPM_R2 *packet = (const struct PACKET_ZC_NOTIFY_HP_TO_GROUPM_R2 *)pkt;
    unsigned aid = packet->aid;

    if (!session_is_own_unit_aid(&g_session1, aid))
        member_hp(self, aid, packet->hp, packet->max_hp);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_POSITION_TO_GROUPM {
    unsigned short id;
    int aid;
    short x;
    short y;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_POSITION_TO_GROUPM, 10);

void game_mode_zc_notify_position_to_group_m(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_POSITION_TO_GROUPM *packet = (const struct PACKET_ZC_NOTIFY_POSITION_TO_GROUPM *)pkt;

    mode_send_msg(self, MM_PARTY_MEMBER_AT, packet->aid, packet->x,
                 packet->y, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_GROUPINFO_CHANGE {
    unsigned short id;
    int exp_option;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_GROUPINFO_CHANGE, 6);

void game_mode_zc_group_info_change(const unsigned char *pkt)
{
    const struct PACKET_ZC_GROUPINFO_CHANGE *packet = (const struct PACKET_ZC_GROUPINFO_CHANGE *)pkt;
    char line[0x80];
    const char *setting, *what, *how;

    g_session1.partyExpOption = packet->exp_option;
    if (g_windowMgr.party_setting_wnd)
        wnd_send(g_windowMgr.party_setting_wnd, 0x3c, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
    switch (g_session1.partyExpOption) {
    case 0:
    case 1:
        how = msg_str(g_session1.partyExpOption == 0 ? 0x11f : 0x120);
        what = msg_str(0x124);
        setting = msg_str(0x123);
        sprintf(line, "%s - %s : %s", setting, what, how);
        break;
    case 2:
        how = msg_str(0x120);
        what = msg_str(0x124);
        setting = msg_str(0x175);
        _snprintf_s(line, sizeof line, (size_t)-1, setting, what, how);
        break;
    default:
        return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)line, CHAT_COLOR_YELLOW, 0xb, 0);
}
