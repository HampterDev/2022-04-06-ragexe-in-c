#include "game/packet_handlers.h"
#include "game/pickup_requests.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include <string.h>
#include "game/gamemode.h"
#include "game/session_bridge.h"
#include "world/world.h"
#include <stdint.h>

enum { ACTOR_MSG_SHOW_SCRIPT = 7 };

#pragma pack(push, 1)
struct PacketShowScript {
    unsigned short packet_type;
    short packet_length;
    int aid;
    char text[1];
};
#pragma pack(pop)

void game_mode_on_show_script(struct GameMode *self, const struct PacketShowScript *packet)
{
    int length = packet->packet_length - 8;
    struct GameActor *actor;
    char text[256];

    if (length == 0)
        return;
    if (g_session1.ownAid == packet->aid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (actor == 0)
        return;
    memcpy(text, packet->text, length);
    render_object_send(actor, NULL, ACTOR_MSG_SHOW_SCRIPT, (intptr_t)text, 0, 0, 0);
}

void game_mode_on_remove_neighborhood_characters(struct GameMode *self, int count, const int *aids)
{
    int i;

    for (i = 0; i < count; i++) {
        int aid = aids[i];

        if (world_remove_actor_by_aid(self->world, aid) == 1)
            mode_send_msg(self, MODE_MSG_ACTOR_REMOVED, aid, 0, 0, 0);
        else if (world_remove_item(self->world, aid) == 1)
            game_mode_remove_pickup_request(self, aid);
        else
            world_remove_skill(self->world, aid);
    }
}

#pragma pack(push, 1)
struct PacketPrNpcState {
    unsigned short packet_type;
    unsigned char state;
};
#pragma pack(pop)

void game_mode_on_pr_npc_state(struct GameMode *self, const struct PacketPrNpcState *packet)
{
    (void)self;
    if (packet->state != 1 && packet->state != 2)
        g_navi.routePrNpcState = 0;
}

#pragma pack(push, 1)
struct PACKET_ZC_SKILL_DISAPPEAR {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SKILL_DISAPPEAR, 6);

void game_mode_zc_skill_disappear(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_SKILL_DISAPPEAR *packet = (const struct PACKET_ZC_SKILL_DISAPPEAR *)pkt;
    int aid = packet->aid;
    int *unit = (int *)self->own_double_aids.first;

    while (unit != (int *)self->own_double_aids.last && *unit != aid)
        unit++;
    if (unit == (int *)self->own_double_aids.last) {
        world_remove_skill(self->world, aid);
        return;
    }
    if (world_remove_actor_by_aid(self->world, aid) != 1)
        return;
    mode_send_msg(self, MODE_MSG_ACTOR_REMOVED, packet->aid, 0, 0, 0);
    memmove(unit, unit + 1, (char *)self->own_double_aids.last - (char *)(unit + 1));
    self->own_double_aids.last = (int *)self->own_double_aids.last - 1;
}
