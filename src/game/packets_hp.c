#include "game/packet_handlers.h"
#include "session/state.h"
#include "game/objects.h"
#include "session/abr.h"
#include "std/integer_map.h"
#include "std/tree.h"

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
struct PACKET_ZC_ABR_INIT {
    unsigned short id;
    int aid;
    int hp;
    int max_hp;
};
struct PACKET_ZC_ABR_PAR_CHANGE {
    unsigned short id;
    int aid;
    unsigned short type;
    int value;
};
#pragma pack(pop)

enum {
    AM_SET_HP_BAR = 0x91,
    AM_REFRESH_HP = 0x22,
};
enum { SP_HP = 5, SP_MAXHP = 6 };

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

static void refresh_abr(struct GameMode *self, int aid)
{
    struct GameActor *actor = world_get_game_actor_by_aid(self->world, aid);

    if (actor != 0)
        render_object_send(actor, NULL, AM_REFRESH_HP, aid, 0, 0, 0);
}

static void set_abr_value(struct Tree *table, long long aid, long long value)
{
    struct Int64Pair *entry = tree_get(table, &aid, sizeof aid, sizeof *entry, tree_less_int64);

    entry->value = value;
}

void game_mode_zc_abr_init(struct GameMode *self, const struct PACKET_ZC_ABR_INIT *packet)
{
    set_abr_value(&g_session1.abrHp, packet->aid, packet->hp);
    set_abr_value(&g_session1.abrMaxHp, packet->aid, packet->max_hp);
    refresh_abr(self, packet->aid);
}

void game_mode_zc_abr_par_change(struct GameMode *self, const struct PACKET_ZC_ABR_PAR_CHANGE *packet)
{
    struct Tree *table;

    if (session_find_abr(&g_session1, (long long)packet->aid) == -1)
        return;
    switch (packet->type) {
    case SP_HP: table = &g_session1.abrHp; break;
    case SP_MAXHP: table = &g_session1.abrMaxHp; break;
    default: table = 0; break;
    }
    if (table != 0)
        set_abr_value(table, packet->aid, packet->value);
    refresh_abr(self, packet->aid);
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
