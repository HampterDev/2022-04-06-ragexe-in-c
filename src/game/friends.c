#include "game/packet_handlers.h"
#include "contents/player_names.h"
#include <string.h>
#include "game/client.h"
#include "game/gamemode.h"
#include "game/session_bridge.h"
#include "core/msg_str.h"
#include "game/packet_data.h"
#include "session/session_party.h"
#include "session/session_windows.h"
#include "ui/ui_std.h"

#define NAME_LENGTH 24
#define FRIEND_ONLINE  0
#define FRIEND_OFFLINE 1

static void chat(const char *text, unsigned colour)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)text, colour, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_FRIENDS_LIST_ENTRY {
    unsigned aid;
    unsigned char_id;
    char name[NAME_LENGTH];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_FRIENDS_LIST_ENTRY, 32);

#pragma pack(push, 1)
struct PACKET_ZC_FRIENDS_LIST {
    unsigned short id;
    unsigned short length;
    struct PACKET_ZC_FRIENDS_LIST_ENTRY friends[];
};
#pragma pack(pop)

void game_mode_friends_list(const unsigned char *pkt)
{
    const struct PACKET_ZC_FRIENDS_LIST *packet = (const struct PACKET_ZC_FRIENDS_LIST *)pkt;
    struct PACKET_ZC_FRIENDS_LIST_ENTRY entry;
    char name[NAME_LENGTH + 1];
    struct String text, arg;
    struct PartyMember friend_;
    long long count, i;
    int len;

    session_clear_friends(&g_session1);
    len = packet->length;
    if (len == 4)
        return;
    count = ((long long)len - 4) / 32;
    for (i = 0; i < count; i++) {
        memcpy(&entry, &packet->friends[(int)i], sizeof entry);
        memset(name, 0, sizeof name);
        snprintf(name, sizeof name, "%s", entry.name);
        if (!player_names_has(entry.char_id)) {
            string_init(&text);
            string_assign(&text, name, sizeof name);
            string_init(&arg);
            string_assign_cstr(&arg, string_cstr(&text));
            client_post_aid_text(entry.char_id, arg);
            string_free(&text);
        }
        friend_.found = 1;
        string_init(&friend_.name);
        string_init(&friend_.map_name);
        friend_.dead = 0;
        string_assign_cstr(&friend_.name, name);
        friend_.aid = entry.aid;
        friend_.char_id = entry.char_id;
        friend_.state = FRIEND_OFFLINE;
        session_add_friend(&g_session1, &friend_);
        party_member_info_destroy(&friend_);
    }
    session_notify_messenger_group_wnd(&g_session1);
}

#pragma pack(push, 1)
struct PACKET_ZC_ADD_FRIENDS_LIST {
    unsigned short id;
    short result;
    int aid;
    int char_id;
    char name[NAME_LENGTH];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ADD_FRIENDS_LIST, 36);

void game_mode_add_friends_list(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_FRIENDS_LIST *packet = (const struct PACKET_ZC_ADD_FRIENDS_LIST *)pkt;
    char name[NAME_LENGTH + 1];
    char line[0x80];
    struct PartyMember friend_;
    int msg;

    memcpy(name, packet->name, NAME_LENGTH);
    name[NAME_LENGTH] = 0;
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
    switch (packet->result) {
    case 0:
        sprintf(line, msg_str(0x335), name);
        chat(line, 0xffff00);
        party_member_info_construct(&friend_);
        string_assign_cstr(&friend_.name, name);
        friend_.aid = packet->aid;
        friend_.char_id = packet->char_id;
        friend_.state = FRIEND_ONLINE;
        session_add_friend(&g_session1, &friend_);
        session_notify_messenger_group_wnd(&g_session1);
        party_member_info_destroy(&friend_);
        return;
    case 2:
        chat(msg_str(0x333), 0xff);
        return;
    case 1: msg = 0x336; break;
    case 3: msg = 0x334; break;
    case 4: msg = 0xab1; break;
    case 6: msg = 0xd78; break;
    case 7: msg = 0xd79; break;
    case 8: msg = 0xd7a; break;
    default:
        return;
    }
    sprintf(line, msg_str(msg), name);
    chat(line, 0xff);
}

#pragma pack(push, 1)
struct PACKET_ZC_FRIENDS_STATE {
    unsigned short id;
    unsigned aid;
    unsigned char_id;
    BYTE state;
    char name[NAME_LENGTH];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_FRIENDS_STATE, 35);

void game_mode_friends_state(const unsigned char *pkt)
{
    const struct PACKET_ZC_FRIENDS_STATE *packet = (const struct PACKET_ZC_FRIENDS_STATE *)pkt;

    session_rename_friend(&g_session1, (int)packet->char_id, packet->name);
    if (session_set_friend_state(&g_session1, packet->aid, packet->char_id, packet->state))
        session_notify_messenger_group_wnd(&g_session1);
}
