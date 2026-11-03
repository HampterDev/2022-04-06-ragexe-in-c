#include "game/packet_handlers.h"
#include "skills/skill_info.h"
#include "std/list.h"
#include "session/state.h"
#include "game/packets.h"
#include "actor/job.h"
#include "session/session_windows.h"

static void merc_redraw(void)
{
    session_invalidate_mer_info_wnd(&g_session1);
}

#pragma pack(push, 1)
struct PacketMerInit {
    unsigned short id;
    unsigned aid;
    short atk, matk, hit, crit, def, mdef, flee, aspd;
    char name[24];
    short level;
    int hp;
    int max_hp;
    int sp;
    int max_sp;
    int expire_time;
    short faith;
    int summon_count;
    int kill_count;
    short attack_range;
};
#pragma pack(pop)
OFFSET_ASSERT(PacketMerInit, name, 0x16);
OFFSET_ASSERT(PacketMerInit, attack_range, 0x4e);

#pragma pack(push, 1)
struct PACKET_ZC_MER_PAR_CHANGE {
    unsigned short packet_type;
    unsigned short var;
    int value;
};
struct PACKET_ZC_EL_INIT {
    unsigned short packet_type;
    unsigned aid;
    int hp;
    int max_hp;
    int sp;
    int max_sp;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MER_PAR_CHANGE, 8);
SIZE_ASSERT(PACKET_ZC_EL_INIT, 0x16);

void game_mode_zc_mer_init(struct GameMode *self, const struct PacketMerInit *pkt)
{
    struct GameActor *merc;
    size_t name_length = 0;

    list_clear(&g_session1.merSkills, skill_info_destroy);
    merc = world_get_game_actor_by_aid(self->world, pkt->aid);
    g_session1.mercAid = (int)pkt->aid;
    g_session1.mercPresent = 1;
    if (merc)
        g_session1.mercJob = game_actor_get_job(merc);
    while (name_length < sizeof pkt->name && pkt->name[name_length])
        ++name_length;
    memcpy(g_session1.mercName, pkt->name, name_length);
    g_session1.mercName[name_length] = 0;
    g_session1.mercStats[0] = pkt->atk;
    g_session1.mercStats[1] = pkt->matk;
    g_session1.mercStats[2] = pkt->hit;
    g_session1.mercStats[3] = pkt->crit;
    g_session1.mercStats[4] = pkt->def;
    g_session1.mercStats[5] = pkt->mdef;
    g_session1.mercStats[6] = pkt->flee;
    g_session1.mercStats[7] = pkt->aspd;
    g_session1.mercLevel = pkt->level;
    g_session1.mercHp = pkt->hp;
    merc_redraw();
    g_session1.mercSp = pkt->sp;
    merc_redraw();
    g_session1.mercMaxHp = pkt->max_hp;
    merc_redraw();
    g_session1.mercMaxSp = pkt->max_sp;
    merc_redraw();
    g_session1.merc_contract_stats[0] = pkt->faith;
    g_session1.merc_contract_stats[1] = pkt->summon_count;
    g_session1.merc_contract_stats[2] = pkt->kill_count;
    g_session1.merc_contract_stats[3] = pkt->attack_range;
    g_session1.merc_contract_stats[4] = pkt->expire_time;
}

enum {
    PAR_HP = 5,
    PAR_MAX_HP = 6,
    PAR_SP = 7,
    PAR_MAX_SP = 8,
    PAR_ATK = 41,
    PAR_HIT = 49,
    PAR_ASPD = 53,
    PAR_MERC_FLEE = 165,
    PAR_MERC_KILLS = 189,
    PAR_MERC_FAITH = 190,
};

void game_mode_zc_mer_par_change(const unsigned char *pkt)
{
    const struct PACKET_ZC_MER_PAR_CHANGE *packet = (const struct PACKET_ZC_MER_PAR_CHANGE *)pkt;
    int value = packet->value;

    if (g_session1.mercPresent == 0)
        return;
    switch (packet->var) {
    case PAR_HP:
        g_session1.mercHp = value;
        merc_redraw();
        break;
    case PAR_MAX_HP:
        g_session1.mercMaxHp = value;
        merc_redraw();
        break;
    case PAR_SP:
        g_session1.mercSp = value;
        merc_redraw();
        break;
    case PAR_MAX_SP:
        g_session1.mercMaxSp = value;
        merc_redraw();
        break;
    case PAR_ATK:
        g_session1.mercStats[0] = value;
        break;
    case PAR_HIT:
        g_session1.mercStats[2] = value;
        break;
    case PAR_ASPD:
        g_session1.mercStats[7] = value;
        break;
    case PAR_MERC_FLEE:
        g_session1.mercStats[6] = value;
        break;
    case PAR_MERC_KILLS:
        g_session1.merc_contract_stats[2] = value;
        break;
    case PAR_MERC_FAITH:
        g_session1.merc_contract_stats[0] = value;
        break;
    }
    merc_redraw();
}
