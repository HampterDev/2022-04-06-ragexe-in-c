#include "game/packet_handlers.h"
#include "game/actor_names.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <stdint.h>
#include <string.h>
#include "base.h"
#include "game/packets.h"
#include "ui/items/pet_windows.h"
#include "core/msg_str.h"
#include "game/packet_data.h"

#define NAME_LENGTH 24

#pragma pack(push, 1)
struct PACKET_ZC_ACK_REQNAME_BYGID {
    uint16_t packet_type;
    int gid;
    char name[NAME_LENGTH];
};
struct PACKET_ZC_ACK_REQNAME_BYGID2 {
    uint16_t packet_type;
    uint16_t result;
    int gid;
    char name[NAME_LENGTH];
};
struct PACKET_ZC_ACK_REQNAMEALL {
    uint16_t packet_type;
    unsigned aid;
    char name[NAME_LENGTH];
    char party[NAME_LENGTH];
    char guild[NAME_LENGTH];
    char position[NAME_LENGTH];
};
struct PACKET_ZC_ACK_REQNAMEALL_NPC {
    uint16_t packet_type;
    unsigned aid;
    int group_id;
    char name[NAME_LENGTH];
    char title[];
};
struct PACKET_ZC_ACK_REQNAME {
    uint16_t packet_type;
    unsigned aid;
    char name[NAME_LENGTH];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_REQNAME_BYGID, 0x1e);
SIZE_ASSERT(PACKET_ZC_ACK_REQNAME_BYGID2, 0x20);
SIZE_ASSERT(PACKET_ZC_ACK_REQNAMEALL, 0x66);
SIZE_ASSERT(PACKET_ZC_ACK_REQNAMEALL_NPC, 0x22);
SIZE_ASSERT(PACKET_ZC_ACK_REQNAME, 0x1e);

static void name_field(struct String *whole, struct String *out, const char *field)
{
    string_init(whole);
    string_assign(whole, field, NAME_LENGTH);
    string_init(out);
    string_assign_cstr(out, string_cstr(whole));
}

static void pet_named(unsigned aid, const char *name)
{
    if (aid != ((unsigned)g_session1.aid))
        return;
    _snprintf_s(g_session1.petName, sizeof g_session1.petName, _TRUNCATE, "%.*s", NAME_LENGTH, name);
    if (g_windowMgr.pet_info_wnd)
        ui_pet_info_set_name(g_windowMgr.pet_info_wnd, name);
}

void game_mode_ack_req_name_by_gid(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQNAME_BYGID *packet = (const struct PACKET_ZC_ACK_REQNAME_BYGID *)pkt;
    struct String whole, name;

    name_field(&whole, &name, packet->name);
    client_post_aid_text(packet->gid, name);
    string_free(&whole);
}

void game_mode_ack_req_name_by_gid2(const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQNAME_BYGID2 *packet = (const struct PACKET_ZC_ACK_REQNAME_BYGID2 *)pkt;
    struct String whole, name, arg;

    switch (packet->result) {
    case 4: {
        const char *unknown = msg_str(0x245);

        string_init(&name);
        string_assign_cstr(&name, unknown);
        break;
    }
    case 3:
        name_field(&whole, &name, packet->name);
        string_free(&whole);
        break;
    default:
        return;
    }
    string_copy(&arg, &name);
    client_post_aid_text(packet->gid, arg);
    string_free(&name);
}

void game_mode_ack_req_name_all(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQNAMEALL *packet = (const struct PACKET_ZC_ACK_REQNAMEALL *)pkt;
    struct String whole_name, whole_party, whole_guild, whole_position;
    struct String name, party, guild, position, achievement_title, title;

    string_init(&title);
    string_init(&achievement_title);
    name_field(&whole_position, &position, packet->position);
    name_field(&whole_guild, &guild, packet->guild);
    name_field(&whole_party, &party, packet->party);
    name_field(&whole_name, &name, packet->name);
    name_card_holder_set_all(&self->actor_names, packet->aid, name, party, guild, position, achievement_title, title, 0);
    string_free(&whole_name);
    string_free(&whole_party);
    string_free(&whole_guild);
    string_free(&whole_position);
}

void game_mode_ack_req_name_all_npc(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQNAMEALL_NPC *packet = (const struct PACKET_ZC_ACK_REQNAMEALL_NPC *)pkt;
    struct String whole_name, name, party, guild, position, achievement_title, title;
    const char *title_text = packet->title;

    string_init(&title);
    string_assign_cstr(&title, title_text);
    string_init(&achievement_title);
    string_init(&position);
    string_init(&guild);
    string_init(&party);
    name_field(&whole_name, &name, packet->name);
    name_card_holder_set_all(&self->actor_names, packet->aid, name, party, guild, position, achievement_title, title,
                      packet->group_id);
    string_free(&whole_name);
    pet_named(packet->aid, packet->name);
}

void game_mode_ack_req_name(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ACK_REQNAME *packet = (const struct PACKET_ZC_ACK_REQNAME *)pkt;
    struct String whole, name;

    name_field(&whole, &name, packet->name);
    name_card_holder_set_name(&self->actor_names, packet->aid, name);
    string_free(&whole);
    pet_named(packet->aid, packet->name);
}

struct String *game_mode_get_actor_name(struct GameMode *self, struct String *out, int aid)
{
    name_card_holder_name(&self->actor_names, out, aid);
    return out;
}
