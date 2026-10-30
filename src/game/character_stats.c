#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include "game/character_stats.h"
#include "game/status_values.h"
#include "session/session_windows.h"

enum { UIM_STATUS_REFRESH = 0x23 };
#define WINDOW_JOB_LEVEL_UP 0x31

static void refresh_status_window(void)
{
    if (g_windowMgr.status_wnd != 0)
        UIWINDOW_SENDMSG(g_windowMgr.status_wnd, UIM_STATUS_REFRESH, 0, 0, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_STATUS {
    unsigned short id;
    short point;
    struct {
        unsigned char value;
        unsigned char cost;
    } stats[6];
    short atk;
    short refine_atk;
    short matk_max;
    short matk_min;
    short def;
    short plus_def;
    short mdef;
    short plus_mdef;
    short hit;
    short flee;
    short plus_flee;
    short critical;
    short aspd;
    short plus_aspd;
};
struct PACKET_ZC_STATUS_CHANGE {
    unsigned short id;
    unsigned short sp;
    unsigned char value;
};
struct PACKET_ZC_STATUS_CHANGE_ACK {
    unsigned short id;
    unsigned short sp;
    unsigned char ok;
    unsigned char value;
};
#pragma pack(pop)

void game_mode_zc_status(struct GameMode *self, const struct PACKET_ZC_STATUS *packet)
{
    int i;

    (void)self;
    for (i = 0; i < 6; i++) {
        STAT_BASE(i) = packet->stats[i].value;
        session_invalidate_windows(&g_session1);
    }
    for (i = 0; i < 6; i++) {
        STAT_COST(i) = packet->stats[i].cost;
        session_invalidate_windows(&g_session1);
    }
    g_session1.statusPoints = packet->point; session_invalidate_windows(&g_session1);
    g_session1.atk = packet->atk; session_invalidate_windows(&g_session1);
    g_session1.refineAtk = packet->refine_atk; session_invalidate_windows(&g_session1);
    g_session1.matkMax = packet->matk_max; session_invalidate_windows(&g_session1);
    g_session1.matkMin = packet->matk_min; session_invalidate_windows(&g_session1);
    g_session1.def = packet->def; session_invalidate_windows(&g_session1);
    g_session1.plusDef = packet->plus_def; session_invalidate_windows(&g_session1);
    g_session1.mdef = packet->mdef; session_invalidate_windows(&g_session1);
    g_session1.plusMdef = packet->plus_mdef; session_invalidate_windows(&g_session1);
    g_session1.hit = packet->hit; session_invalidate_windows(&g_session1);
    g_session1.flee = packet->flee; session_invalidate_windows(&g_session1);
    g_session1.plusFlee = packet->plus_flee; session_invalidate_windows(&g_session1);
    g_session1.critical = packet->critical; session_invalidate_windows(&g_session1);
    g_session1.aspd = packet->aspd; session_invalidate_windows(&g_session1);
    g_session1.plusAspd = packet->plus_aspd; session_invalidate_windows(&g_session1);
    refresh_status_window();
}

void game_mode_zc_status_change(struct GameMode *self, const struct PACKET_ZC_STATUS_CHANGE *packet)
{
    unsigned sp = packet->sp;

    (void)self;
    if (sp >= SP_USTR && sp < SP_USTR + 6) {
        STAT_COST(sp - SP_USTR) = packet->value;
        session_invalidate_windows(&g_session1);
    } else if (sp >= SP_UPOW && sp < SP_UPOW + 6) {
        TRAIT_COST(sp - SP_UPOW) = packet->value;
        session_invalidate_windows(&g_session1);
    }
    refresh_status_window();
}

void game_mode_zc_status_change_ack(struct GameMode *self, const struct PACKET_ZC_STATUS_CHANGE_ACK *packet)
{
    struct UIWindow *wnd = g_windowMgr.status_wnd;
    unsigned sp;

    self->status_request_pending = 0;
    if (packet->ok == 0)
        return;
    sp = packet->sp;
    if (sp >= SP_STR && sp < SP_STR + 6) {
        STAT_BASE(sp - SP_STR) = packet->value;
        session_invalidate_windows(&g_session1);
        if (wnd != 0)
            UIWINDOW_SENDMSG(wnd, UIM_STATUS_REFRESH, 0, 0, 0, 0);
        return;
    }
    switch (sp) {
    case SP_ATK1: g_session1.atk = packet->value; break;
    case SP_ATK2: g_session1.refineAtk = packet->value; break;
    case SP_MATK1: g_session1.matkMax = packet->value; break;
    case SP_MATK2: g_session1.matkMin = packet->value; break;
    case SP_DEF1: g_session1.def = packet->value; break;
    case SP_DEF2: g_session1.plusDef = packet->value; break;
    case SP_MDEF1: g_session1.mdef = packet->value; break;
    case SP_MDEF2: g_session1.plusMdef = packet->value; break;
    case SP_HIT: g_session1.hit = packet->value; break;
    case SP_FLEE1: g_session1.flee = packet->value; break;
    case SP_FLEE2: g_session1.plusFlee = packet->value; break;
    case SP_CRITICAL: g_session1.critical = packet->value; break;
    case SP_ASPD: g_session1.aspd = packet->value; break;
    case SP_ASPD2: g_session1.plusAspd = packet->value; break;
    case SP_JOBLEVEL:
        g_session1.jobLevel = packet->value;
        session_invalidate_windows(&g_session1);
        ui_window_open(WINDOW_JOB_LEVEL_UP);
        return;
    default:
        return;
    }
    session_invalidate_windows(&g_session1);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_STATUS_GM {
    unsigned short id;
    struct { unsigned char value, standard; } stat[6];
    short att_power;
    short refining_power;
    short max_matt_power;
    short min_matt_power;
    short itemdef_power;
    short plusdef_power;
    short mdef_power;
    short plusmdef_power;
    short hit;
    short avoid;
    short plus_avoid;
    short critical;
    short aspd;
    short plus_aspd;
};
#pragma pack(pop)
STATIC_ASSERT(PACKET_ZC_ACK_STATUS_GM_plus_aspd, offsetof(struct PACKET_ZC_ACK_STATUS_GM, plus_aspd) == 0x28);

#define GM_STATUS_YELLOW    0xffff
#define GM_STATUS_GREEN     0xff37

static void gm_status_line(const char *text, unsigned color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)text, (int)color, 0, 0);
}

void game_mode_ack_status_gm(const struct PACKET_ZC_ACK_STATUS_GM *packet)
{
    char line[0x104];

    sprintf(line, "             STR=%3d      AGI=%3d      VIT=%3d      INT=%3d      DEX=%3d      LUK=%d",
                packet->stat[0].value, packet->stat[1].value, packet->stat[2].value, packet->stat[3].value,
                packet->stat[4].value, packet->stat[5].value);
    gm_status_line(line, GM_STATUS_YELLOW);
    sprintf(line, "standard STR=%3d      AGI=%3d      VIT=%3d      INT=%3d      DEX=%3d      LUK=%2d",
                packet->stat[0].standard, packet->stat[1].standard, packet->stat[2].standard,
                packet->stat[3].standard, packet->stat[4].standard, packet->stat[5].standard);
    gm_status_line(line, GM_STATUS_GREEN);
    sprintf(line, "    attPower=%3d    refiningPow=%3d    MAXmatPow=%3d    MINmatPower=%3d       ASPD=%3d",
                packet->att_power, packet->refining_power, packet->max_matt_power, packet->min_matt_power,
                packet->aspd);
    gm_status_line(line, GM_STATUS_YELLOW);
    sprintf(line, "itemdefPow=%3d     plusdefPow=%3d    mdefPower=%3d    plusmdefPow=%3d    plusASPD=%3d",
                packet->itemdef_power, packet->plusdef_power, packet->mdef_power, packet->plusmdef_power,
                packet->plus_aspd);
    gm_status_line(line, GM_STATUS_GREEN);
    sprintf(line, "hitSuccessVal=%3d    avoidSuccessVal=%3d    plusAvoidSuccessValue=%3d",
                packet->hit, packet->avoid, packet->plus_avoid);
    gm_status_line(line, GM_STATUS_YELLOW);
}

#pragma pack(push, 1)
struct PACKET_ZC_COUPLESTATUS {
    unsigned short id;
    unsigned sp;
    int value;
    int bonus;
};
#pragma pack(pop)

enum { SP_POW = 219 };

#define STAT_BONUS(i)       (g_session1.statBonus[i])
#define TRAIT_BONUS(i)      (g_session1.traitBonus[i])

void game_mode_zc_couple_status(struct GameMode *self, const struct PACKET_ZC_COUPLESTATUS *packet)
{
    unsigned sp = packet->sp;

    (void)self;
    if (sp >= SP_STR && sp < SP_STR + 6) {
        STAT_BASE(sp - SP_STR) = packet->value;
        session_invalidate_windows(&g_session1);
        STAT_BONUS(sp - SP_STR) = packet->bonus;
        session_invalidate_windows(&g_session1);
    } else if (sp >= SP_POW && sp < SP_POW + 6) {
        session_set_advanced_status(&g_session1, (int)sp, packet->value);
        TRAIT_BONUS(sp - SP_POW) = packet->bonus;
        session_invalidate_windows(&g_session1);
    }
}
