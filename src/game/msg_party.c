#include "web/service_managers.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include <string.h>
#include <mbstring.h>
#include "game/mode_msg.h"
#include "game/session_bridge.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "ui/ui_std.h"

#define CZ_PARTY_LEAVE              0x100
#define CZ_REQ_EXPEL_GROUP_MEMBER   0x103
#define CZ_MAKE_GROUP2              0x1e8
#define CZ_PARTY_JOIN_REQ           0x2c4
#define CZ_PARTY_JOIN_REQ_ACK       0x2c7

#define WID_PARTY_NAME              0x35
#define WID_PARTY_NAME_2            0xcf
#define WID_PARTY_INVITE_LIST       0xd0
#define WID_MESSENGER_GROUP               0x22

#define COLOR_NO_PARTY_HERE 0x6e96ff

int game_mode_msg_make_party(const char *name)
{
    if (g_session1.map_rule_flags[4]) {
        chat_print_msg(0x56b, COLOR_NO_PARTY_HERE);
        return 0;
    }
    if (session_get_party_member_count(&g_session1) != 0)
        return 0;
    if (_mbsstr((const unsigned char *)name, (const unsigned char *)"#")) {
        message_box_msg(0xb17);
        return 0;
    }
    if (strlen(name) >= 0x18) {
        message_box_msg(0x213);
        return 0;
    }
    close_window(WID_PARTY_NAME);
    ui_window_open(WID_PARTY_NAME);
    string_assign_cstr(&g_session1.newPartyName, name);
    return 0;
}

int game_mode_msg_set_party_name(const char *name)
{
    if (g_session1.map_rule_flags[4]) {
        chat_print_msg(0x56b, COLOR_NO_PARTY_HERE);
        return 0;
    }
    if (session_get_party_member_count(&g_session1) != 0)
        return 0;
    close_window(WID_PARTY_NAME_2);
    string_assign_cstr(&g_session1.newPartyName, name);
    return 0;
}

int game_mode_msg_make_party_with_options(int item_pickup, int item_share)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char name[24];
        unsigned char item_pickup;
        unsigned char item_share;
    } packet;
#pragma pack(pop)

    packet.id = CZ_MAKE_GROUP2;
    strncpy(packet.name, string_cstr(&g_session1.newPartyName), 0x18);
    packet.item_pickup = (unsigned char)item_pickup;
    packet.item_share = (unsigned char)item_share;
    send_packet(CZ_MAKE_GROUP2, &packet);
    return 0;
}

int game_mode_msg_party_invite(const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char name[24];
    } packet;
#pragma pack(pop)
    struct String who, line;

    packet.id = CZ_PARTY_JOIN_REQ;
    if (strlen(name) >= 0x18) {
        message_box_msg(0x213);
        return 0;
    }
    strncpy(packet.name, name, 0x18);
    send_packet(packet.id, &packet);
    if (!g_session1.partyLeader)
        return 0;
    string_init(&who);
    string_assign_cstr(&who, name);
    string_copy(&line, &who);
    string_append_cstr(&line, msg_str(0x80b));
    chat_print(string_cstr(&line), 0xffff00);
    string_free(&line);
    string_free(&who);
    return 0;
}

int game_mode_msg_party_invite_answer(int party_id, int accept)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int party_id;
        unsigned char accept;
    } packet;
#pragma pack(pop)

    if (!accept)
        string_clear(&g_session1.partyName);
    else
        ui_window_open(WID_MESSENGER_GROUP);
    packet.id = CZ_PARTY_JOIN_REQ_ACK;
    packet.party_id = party_id;
    packet.accept = accept != 0;
    send_packet(CZ_PARTY_JOIN_REQ_ACK, &packet);
    return 0;
}

int game_mode_msg_leave_party(struct GameMode *self)
{
    struct PartyMember member;
    int count, i;
    char found;

    if (g_session1.mdState == 1 || g_session1.mdState == 2)
        chat_print_msg(0xbd3, CHAT_COLOR_RED);
    count = session_get_party_member_count(&g_session1);
    if (!g_session1.partyLeader)
        goto leave;
    if (count <= 1) {
        if (count == 1)
            adventurer_agency_cancel_own_booking(g_adventurerAgency);
        goto leave;
    }
    found = 0;
    for (i = 0; i < count; i++) {
        session_party_member_at(&g_session1, &member, i);
        if (member.aid != g_session1.ownAid && member.state == 0) {
            found = 1;
            if (!!string_equal_cstr(&member.map_name, self->map_name)) {
                mode_send_message(client_current_mode(), MODE_MSG_CHANGE_GROUP_MASTER, member.aid, 0, 0, 0);
                party_member_info_destroy(&member);
                goto leave;
            }
        }
        party_member_info_destroy(&member);
    }
    if (found) {
        chat_print_msg(0xcc9, CHAT_COLOR_RED);
        return 0;
    }
    if (ui_window_mgr_message_box(&g_windowMgr, msg_str(0xcb9), 2, 0, 0, 0, "메시지", 0x118, 0x78, 0) != 0xb3)
        return 0;
leave:
    if (g_windowMgr.party_invitation_to_wnd)
        close_window(WID_PARTY_INVITE_LIST);
    send_id(CZ_PARTY_LEAVE);
    return 0;
}

int game_mode_msg_expel_party_member(const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned aid;
        char name[24];
    } packet;
#pragma pack(pop)
    struct PartyMember member;

    if (!g_session1.partyLeader)
        return 0;
    packet.id = CZ_REQ_EXPEL_GROUP_MEMBER;
    strncpy(packet.name, name, 0x18);
    session_get_party_member_by_name(&g_session1, &member, name);
    packet.aid = member.aid;
    send_packet(CZ_REQ_EXPEL_GROUP_MEMBER, &packet);
    party_member_info_destroy(&member);
    return 0;
}

int game_mode_msg_party_join_master_ack(int aid, int accept, const int *char_id)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int aid;
        int char_id;
        unsigned char accept;
    } packet;
#pragma pack(pop)

    packet.id = 0xaf8;
    packet.aid = aid;
    packet.accept = accept != 0;
    packet.char_id = *char_id;
    send_packet(0xaf8, &packet);
    return 0;
}

int game_mode_msg_party_req_master_to_join(const int *char_id, const int *party_id)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int party_id;
        int char_id;
    } packet;
#pragma pack(pop)

    packet.id = 0xae6;
    packet.char_id = *char_id;
    packet.party_id = *party_id;
    send_packet(0xae6, &packet);
    return 0;
}
