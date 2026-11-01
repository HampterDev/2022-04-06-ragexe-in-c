#include "game/packet_handlers.h"
#include "std/string.h"
#include "session/state.h"
#include <string.h>
#include "game/packets_world.h"
#include "game/minimap.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "session/session_windows.h"
#include "ui/wndmgr/wndmgr.h"

void game_mode_add_party_member(struct GameMode *self, const char *name, unsigned aid, const char *map_name,
                                        int role, unsigned char state, unsigned short job, unsigned short level,
                                        const char *party_name, unsigned char share_1, unsigned char share_2,
                                        unsigned char_id);

#define GAMEMODE_MSG_MEMBER_GONE 0x40

#pragma pack(push, 1)
struct PacketAddMemberToGroup {
    unsigned short packet_type;
    unsigned aid;
    int role;
    unsigned short job;
    unsigned short level;
    short x;
    short y;
    unsigned char state;
    char party_name[24];
    char name[24];
    char map_name[16];
    unsigned char share_1;
    unsigned char share_2;
};
struct PacketPartyMemberState {
    unsigned short packet_type;
    unsigned aid;
    unsigned char state;
};
struct PacketListFlagClear {
    unsigned short packet_type;
    unsigned short id;
};
struct PacketAddMemberToGroup2 {
    unsigned short packet_type;
    unsigned aid;
    unsigned gid;
    int role;
    unsigned short job;
    unsigned short level;
    short x;
    short y;
    unsigned char state;
    char party_name[24];
    char name[24];
    char map_name[16];
    unsigned char share_1;
    unsigned char share_2;
};
#pragma pack(pop)

void game_mode_on_add_member_to_group(struct GameMode *self, const struct PacketAddMemberToGroup *packet)
{
    game_mode_add_party_member(self, packet->name, packet->aid, packet->map_name, packet->role, packet->state,
                            packet->job, packet->level, packet->party_name, packet->share_1, packet->share_2, 0);
}

void game_mode_on_add_member_to_group2(struct GameMode *self, const struct PacketAddMemberToGroup2 *packet)
{
    game_mode_add_party_member(self, packet->name, packet->aid, packet->map_name, packet->role, packet->state,
                            packet->job, packet->level, packet->party_name, packet->share_1, packet->share_2,
                            packet->gid);
}

void game_mode_on_party_member_state(struct GameMode *self, const struct PacketPartyMemberState *packet)
{
    struct PartyMember member;

    session_get_party_member(&g_session1, &member, packet->aid);
    if (member.found != 0) {
        session_set_party_member_state(&g_session1, packet->aid, packet->state);
        session_notify_messenger_group_and_plates(NULL);
    }
    string_free(&member.map_name);
    string_free(&member.name);
}

void game_mode_on_list_flag_clear(struct GameMode *self, const struct PacketListFlagClear *packet)
{
    session_clear_inventory_move_pending(&g_session1, packet->id);
    self->item_move_ready = 1;
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
    session_clear_cart_move_pending(&g_session1, packet->id);
    session_refresh_party_ui(&g_session1);
    session_clear_storage_move_pending(&g_session1, packet->id);
    session_refresh_friend_ui(&g_session1);
}

static void drop_extension(struct String *s)
{
    if (s->size > 4)
        string_resize(s, s->size - 3, 0);
}

void game_mode_add_party_member(struct GameMode *self, const char *name, unsigned aid, const char *map_name,
                                        int role, unsigned char state, unsigned short job, unsigned short level,
                                        const char *party_name, unsigned char share_1, unsigned char share_2,
                                        unsigned char_id)
{
    struct PartyMember m;
    char party[25];

    m.found = 1;
    string_init(&m.name);
    string_init(&m.map_name);
    m.dead = 0;
    string_assign_cstr(&m.name, name);
    m.aid = (int)aid;
    m.char_id = (int)char_id;
    string_assign_cstr(&m.map_name, map_name);
    m.role = role;
    m.state = state;
    m.job = job;
    m.level = level;
    strncpy(party, party_name, 24);
    party[24] = 0;
    string_assign_cstr(&g_session1.partyName, party);

    if (aid == (unsigned)((unsigned)g_session1.ownAid)) {
        if (m.role == 0)
            g_session1.partyLeader = 1;
    } else if (m.state == 1) {
        mode_send_msg(self, GAMEMODE_MSG_MEMBER_GONE, (int)aid, 0, 0, 0);
        game_mode_set_actor_mark_flag(self, (int)aid, 0);
    } else {
        struct String member_map, our_map;

        string_copy(&member_map, &m.map_name);
        string_init(&our_map);
        string_assign_cstr(&our_map, self->map_name);
        drop_extension(&member_map);
        drop_extension(&our_map);
        if (!string_equal(&member_map, &our_map)) {
            mode_send_msg(self, GAMEMODE_MSG_MEMBER_GONE, (int)aid, 0, 0, 0);
            game_mode_set_actor_mark_flag(self, (int)aid, 0);
        } else {
            game_mode_set_actor_mark_flag(self, (int)aid, 1);
        }
        string_free(&our_map);
        string_free(&member_map);
    }
    g_session1.partyItemDivisionRule = share_2 != 0;
    g_session1.partyItemPickupRule = share_1 != 0;
    session_get_party_member_count(&g_session1);
    session_add_party_member(&g_session1, &m);
    session_notify_messenger_group_and_plates(&g_session1);
    game_mode_set_actor_mark_flag(self, (int)aid, 1);
    party_member_info_destroy(&m);
}
