#include "game/packet_handlers.h"
#include "game/objects.h"

#pragma pack(push, 1)
struct PACKET_ZC_HP_INFO {
    unsigned short id;
    unsigned aid;
    int hp;
    int max_hp;
};
struct PACKET_ZC_HP_INFO_TINY {
    unsigned short id;
    unsigned aid;
    signed char hp_percent;
};
#pragma pack(pop)

enum {
    AM_SET_HP_BAR = 0x91,
};

void game_mode_zc_hp_info(struct GameMode *self, const struct PACKET_ZC_HP_INFO *packet)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

    if (actor != 0)
        render_object_send(actor, NULL, AM_SET_HP_BAR, packet->hp, packet->max_hp, 0, 0);
}

void game_mode_zc_hp_info_tiny(struct GameMode *self, const struct PACKET_ZC_HP_INFO_TINY *packet)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);

    if (actor != 0)
        render_object_send(actor, NULL, AM_SET_HP_BAR, packet->hp_percent, 20, 0, 0);
}

int game_mode_zc_battlefield_notify_hp_sp(struct GameMode *self, int aid, int hp, int max_hp, int sp,
                                                   int max_sp)
{
    (void)self; (void)aid; (void)hp; (void)max_hp; (void)sp; (void)max_sp;
    return -1;
}

void game_mode_zc_notify_hp_sp_to_group_m_r2(struct GameMode *self, int aid, int hp, int max_hp, int sp,
                                                     int max_sp)
{
    (void)self; (void)aid; (void)hp; (void)max_hp; (void)sp; (void)max_sp;
}
