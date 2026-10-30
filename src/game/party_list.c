#include "game/packet_handlers.h"
#include "session/character_name.h"
#include "contents/manner_filter.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include "game/party_list.h"
#include "actor/game_actor.h"
#include "core/msg_str.h"
#include "game/chat.h"
#include "session/session_party.h"
#include "session/session_state.h"
#include "session/session_windows.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

#define PARTY_COLOR_ONLINE      0x493a37
#define PARTY_COLOR_OFFLINE     0xa38a86
#define PARTY_COLOR_OUR_LEAD    0x6f3007

void game_mode_set_party_list(struct GameMode *self, const char *party_name, const struct Vec *members)
{
    const struct PartyListMember *m;
    char name[25];

    (void)self;
    session_clear_party_members(&g_session1);
    memcpy(name, party_name, 24);
    name[24] = 0;
    if (word_filter_contains(&g_nameFilter, name)) {
        word_filter_censor(&g_nameFilter, name);
    }
    string_assign_cstr(&g_session1.partyName, name);

    for (m = (const struct PartyListMember *)members->first; m != (const struct PartyListMember *)members->last; m++) {
        struct PartyMember member;
        char member_name[25], map_name[17];

        memcpy(member_name, m->name, 24);
        member_name[24] = 0;
        memcpy(map_name, m->map_name, 16);
        map_name[16] = 0;
        member.found = 1;
        string_init(&member.name);
        string_init(&member.map_name);
        member.dead = 0;
        member.aid = m->aid;
        member.char_id = m->char_id;
        string_assign_cstr(&member.name, member_name);
        string_assign_cstr(&member.map_name, map_name);
        member.role = m->role;
        member.state = m->state;
        member.job = m->job;
        member.level = m->level;
        member.color = m->state == 0 ? PARTY_COLOR_ONLINE : PARTY_COLOR_OFFLINE;
        if (member.aid == g_session1.ownAid && m->role == 0) {
            member.color = PARTY_COLOR_OUR_LEAD;
            g_session1.partyLeader = 1;
        }
        session_add_party_member(&g_session1, &member);
        party_member_info_destroy(&member);
    }
    session_load_mini_party_info(&g_session1);
    ui_window_mgr_create_party_plates(&g_windowMgr);
    session_notify_messenger_group_and_plates(NULL);
}

#pragma pack(push, 1)
struct PacketGroupList2 {
    unsigned short packet_type;
    short packet_length;
    char party_name[24];
    struct {
        int aid;
        char name[24];
        char map_name[16];
        unsigned char role;
        unsigned char state;
        unsigned short job;
        unsigned short level;
    } members[1];
};
#pragma pack(pop)

void game_mode_on_group_list2(struct GameMode *self, const struct PacketGroupList2 *packet)
{
    struct Vec members = { 0, 0, 0 };
    int count = (int)((unsigned)(packet->packet_length - 0x1c) / 50);
    int i;

    for (i = 0; i < count; i++) {
        struct PartyListMember member;
        unsigned char *p = (unsigned char *)&member;
        int k;

        for (k = 0; k < (int)sizeof member; k++)
            p[k] = 0;
        member.aid = packet->members[i].aid;
        for (k = 0; k < 24; k++)
            member.name[k] = packet->members[i].name[k];
        for (k = 0; k < 16; k++)
            member.map_name[k] = packet->members[i].map_name[k];
        member.role = packet->members[i].role;
        member.state = packet->members[i].state;
        member.job = packet->members[i].job;
        member.level = packet->members[i].level;
        vec_push(&members, &member, sizeof(struct PartyListMember));
    }
    game_mode_set_party_list(self, packet->party_name, &members);
    vec_free(&members);
}

#pragma pack(push, 1)
struct PacketGroupList3 {
    unsigned short packet_type;
    short packet_length;
    char party_name[24];
    struct PartyListMember members[1];
};
#pragma pack(pop)

void game_mode_on_group_list3(struct GameMode *self, const struct PacketGroupList3 *packet)
{
    struct Vec members = { 0, 0, 0 };
    unsigned count = (unsigned)(packet->packet_length - 0x1c) / 0x36;
    const struct PartyListMember *src = packet->members;

    if ((int)count >= 1) {
        do {
            vec_push(&members, src, sizeof(struct PartyListMember));
            src++;
        } while (--count != 0);
    }
    game_mode_set_party_list(self, packet->party_name, &members);
    vec_free(&members);
}

#pragma pack(push, 1)
struct PacketNotifyMemberInfoToGroupm {
    unsigned short packet_type;
    int aid;
    unsigned short job;
    unsigned short level;
};
#pragma pack(pop)

void game_mode_on_notify_member_info_to_groupm(struct GameMode *self,
                                                    const struct PacketNotifyMemberInfoToGroupm *packet)
{
    (void)self;
    session_set_party_member_job_level(&g_session1, packet->aid, packet->job, packet->level);
    session_notify_messenger_group_and_plates(NULL);
}

#define GAMEMODE_MSG_MEMBER_GONE 0x40
#define GAMEMODE_MSG_PARTY_LEFT  0x41
#define WINDOW_PARTY             0x35
#define WINDOW_MEMORIAL_DUNGEON  0x89
#define ACTOR_MSG_CHAT           7

static void chat_line(const char *text, int colour, int channel)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)text, colour, channel, 0);
}

static void chat_close_dialog(void)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CLOSE_DIALOG, 0, 0, 0, 0);
}

static void censor_party_name(char *name)
{
    if (word_filter_contains(&g_nameFilter, name)) {
        word_filter_censor(&g_nameFilter, name);
    }
}

#pragma pack(push, 1)
struct GroupListEntry {
    int aid;
    char name[24];
    char map_name[16];
    unsigned char role;
    unsigned char state;
};
#pragma pack(pop)
SIZE_ASSERT(GroupListEntry, 46);

#pragma pack(push, 1)
struct PACKET_ZC_GROUP_LIST {
    unsigned short id;
    short length;
    char party_name[24];
    struct GroupListEntry entries[];
};
#pragma pack(pop)

void game_mode_group_list(const unsigned char *pkt)
{
    const struct PACKET_ZC_GROUP_LIST *packet = (const struct PACKET_ZC_GROUP_LIST *)pkt;
    char party_name[25], name[25], map_name[17];
    struct PartyMember m;
    const struct GroupListEntry *e;
    short len;
    int n;

    session_clear_party_members(&g_session1);
    memcpy(party_name, packet->party_name, 24);
    party_name[24] = 0;
    censor_party_name(party_name);
    string_assign_cstr(&g_session1.partyName, party_name);
    len = packet->length;
    if (len == 0x1c)
        return;
    n = (len - 0x1c) / (int)sizeof *e;
    for (e = packet->entries; n > 0; n--, e++) {
        struct GroupListEntry entry = *e;

        memcpy(name, entry.name, 24);
        name[24] = 0;
        memcpy(map_name, entry.map_name, 16);
        map_name[16] = 0;
        m.found = 1;
        string_init(&m.name);
        string_init(&m.map_name);
        m.dead = 0;
        m.aid = entry.aid;
        string_assign_cstr(&m.name, name);
        string_assign_cstr(&m.map_name, map_name);
        m.role = entry.role;
        m.state = entry.state;
        session_add_party_member(&g_session1, &m);
        if (m.aid == g_session1.ownAid && m.role == 0)
            g_session1.partyLeader = 1;
        party_member_info_destroy(&m);
    }
    session_notify_messenger_group_and_plates(NULL);
}

struct PartyMember *party_member_info_construct(struct PartyMember *m)
{
    m->found = 1;
    string_init(&m->name);
    string_init(&m->map_name);
    m->dead = 0;
    return m;
}

#pragma pack(push, 1)
struct PACKET_ZC_ADD_MEMBER_TO_GROUP {
    unsigned short id;
    int aid;
    int role;
    short x;
    short y;
    BYTE state;
    char party_name[24];
    char name[24];
    char map_name[16];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ADD_MEMBER_TO_GROUP, 79);

void game_mode_add_member_to_group(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_MEMBER_TO_GROUP *packet = (const struct PACKET_ZC_ADD_MEMBER_TO_GROUP *)pkt;
    char name[25], party_name[25], map_name[17];
    struct PartyMember m;
    struct String member_map, our_map;
    unsigned char state;

    memcpy(name, packet->name, 24);
    name[24] = 0;
    strncpy(party_name, packet->party_name, 24);
    party_name[24] = 0;
    censor_party_name(party_name);
    string_assign_cstr(&g_session1.partyName, party_name);
    memcpy(map_name, packet->map_name, 16);
    map_name[16] = 0;
    party_member_info_construct(&m);
    string_assign_cstr(&m.name, name);
    m.aid = packet->aid;
    string_assign_cstr(&m.map_name, map_name);
    state = packet->state;
    m.state = state;
    m.role = packet->role;
    if (m.aid == g_session1.ownAid) {
        if (m.role == 0)
            g_session1.partyLeader = 1;
    } else if (state == 1) {
        mode_send_msg(self, GAMEMODE_MSG_MEMBER_GONE, packet->aid, 0, 0, 0);
    } else {
        string_copy(&member_map, &m.map_name);
        string_init(&our_map);
        string_assign_cstr(&our_map, self->map_name);
        if (member_map.size > 4)
            string_resize(&member_map, member_map.size - 3, 0);
        if (our_map.size > 4)
            string_resize(&our_map, our_map.size - 3, 0);
        if (!string_equal(&member_map, &our_map))
            mode_send_msg(self, GAMEMODE_MSG_MEMBER_GONE, packet->aid, 0, 0, 0);
        string_free(&our_map);
        string_free(&member_map);
    }
    session_add_party_member(&g_session1, &m);
    session_notify_messenger_group_and_plates(NULL);
    party_member_info_destroy(&m);
}

#pragma pack(push, 1)
struct PACKET_ZC_DELETE_MEMBER_FROM_GROUP {
    unsigned short id;
    int aid;
    char name[24];
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DELETE_MEMBER_FROM_GROUP, 31);

void game_mode_delete_member_from_group(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_DELETE_MEMBER_FROM_GROUP *packet = (const struct PACKET_ZC_DELETE_MEMBER_FROM_GROUP *)pkt;
    char name[25];
    struct GameActor *actor;

    memcpy(name, packet->name, 24);
    name[24] = 0;
    switch (packet->result) {
    case 0:
    case 1:
        mode_send_msg(self, GAMEMODE_MSG_MEMBER_GONE, session_get_party_member_aid(&g_session1, name), 0, 0, 0);
        session_remove_party_member(&g_session1, name);
        break;
    case 2:
        chat_close_dialog();
        chat_line(msg_str(0x750), 0xff, 0);
        return;
    case 3:
        chat_close_dialog();
        chat_line(msg_str(0x751), 0xff, 0);
        return;
    }
    if (strcmp(session_character_name(&g_session1), name) == 0) {
        g_session1.partyLeader = 0;
        session_clear_party_members(&g_session1);
        string_clear(&g_session1.partyName);
        chat_close_dialog();
        chat_line(msg_str(0x54), 0xffff00, 0);
        ui_close_window(WINDOW_PARTY);
        mode_send_msg(self, GAMEMODE_MSG_PARTY_LEFT, 0, 0, 0, 0);
        if (g_windowMgr.memorial_dun_wnd != 0)
            ui_close_window(WINDOW_MEMORIAL_DUNGEON);
        session_clear_plate_records(&g_session1);
    }
    session_notify_messenger_group_and_plates(NULL);
    actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (actor != 0)
        game_actor_set_mark_listed(actor, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_CHANGE_GROUP_MASTER {
    unsigned short id;
    int old_aid;
    int new_aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CHANGE_GROUP_MASTER, 10);

void game_mode_change_group_master(const unsigned char *pkt)
{
    const struct PACKET_ZC_CHANGE_GROUP_MASTER *packet = (const struct PACKET_ZC_CHANGE_GROUP_MASTER *)pkt;
    struct PartyMember m;
    char text[512];

    session_get_party_member(&g_session1, &m, packet->new_aid);
    if (m.found != 0) {
        session_set_party_member_role(&g_session1, packet->old_aid, 1);
        session_set_party_member_role(&g_session1, packet->new_aid, 0);
        g_session1.partyLeader = packet->new_aid == g_session1.ownAid;
        sprintf(text, msg_str(0x610), string_cstr(&m.name));
        chat_line(text, 0xffff, 0xb);
        session_notify_messenger_group_and_plates(NULL);
        if (g_windowMgr.memorial_dun_wnd != 0)
            ui_window_send(g_windowMgr.memorial_dun_wnd, NULL, 0x3c, 0, 0, 0, 0);
    }
    party_member_info_destroy(&m);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_CHAT_PARTY {
    unsigned short id;
    unsigned short length;
    int aid;
    char text[];
};
#pragma pack(pop)

void game_mode_notify_chat_party(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_CHAT_PARTY *packet = (const struct PACKET_ZC_NOTIFY_CHAT_PARTY *)pkt;
    size_t len = packet->length - 8;
    struct Vec text;
    struct GameActor *speaker;
    char *line;

    if (len - 1 > 0x168)
        return;
    vec_init(&text);
    vec_resize(&text, len + 1, 1);
    line = text.first;
    memcpy_s(text.first, (char *)text.last - (char *)text.first, packet->text, len);
    if (word_filter_contains(&g_nameFilter, line) == 1) {
        word_filter_censor(&g_nameFilter, line);
    }
    if (packet->aid == g_session1.ownAid) {
        chat_line(line, 0xc8ff, 3);
        render_object_send(self->world->player, NULL, ACTOR_MSG_CHAT, (int)line, 0, 0, 0);
    } else {
        chat_close_dialog();
        chat_line(line, 0xc8c8ff, 3);
        speaker = world_get_game_actor_by_aid(self->world, packet->aid);
        if (speaker != 0)
            render_object_send(speaker, NULL, ACTOR_MSG_CHAT, (int)line, 0, 0, 0);
    }
    if (g_session1.autoSaveChat != 0)
        game_mode_log_whisper(self, line);
    vec_free(&text);
}
