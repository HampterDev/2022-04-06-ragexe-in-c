#ifndef GAMEMODE_CHARACTER_STATS_H
#define GAMEMODE_CHARACTER_STATS_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/client.h"

#define STAT_BASE(i)        (g_session1.stats[i])
#define STAT_COST(i)        (g_session1.statCosts[i])
#define TRAIT_COST(i)       (g_session1.traitCost[i])

enum {
    SP_STR = 13,
    SP_ATK1 = 41, SP_ATK2, SP_MATK1, SP_MATK2, SP_DEF1, SP_DEF2, SP_MDEF1, SP_MDEF2, SP_HIT, SP_FLEE1,
    SP_FLEE2, SP_CRITICAL, SP_ASPD, SP_ASPD2, SP_JOBLEVEL,
    SP_USTR = 32,
    SP_UPOW = 247,
};

struct PACKET_ZC_STATUS;
struct PACKET_ZC_STATUS_CHANGE;
struct PACKET_ZC_STATUS_CHANGE_ACK;

struct PACKET_ZC_ACK_STATUS_GM;

struct PACKET_ZC_COUPLESTATUS;

#endif
