#include "game/companion_packets.h"
#include "game/packets.h"
#include <stdint.h>
#include <string.h>
#include "base.h"
#include "platform/language.h"
#include "core/msg_str.h"
#include "session/session_windows.h"

#pragma pack(push, 1)
struct HomunPropertyHead {
    uint16_t packet_type;
    char name[24];
    uint8_t renamed;
    short level;
    short fullness;
    short intimacy;
};
struct PACKET_ZC_PROPERTY_HOMUN {
    struct HomunPropertyHead head;
    int item;
    short stats[8];
    short hp;
    short max_hp;
    short sp;
    short max_sp;
    int exp;
    int max_exp;
    short skill_points;
    short attack_range;
};
struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY {
    struct HomunPropertyHead head;
    short stats[8];
    int hp;
    int max_hp;
    short sp;
    short max_sp;
    int exp;
    int max_exp;
    short skill_points;
    short attack_range;
};
struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY2 {
    struct HomunPropertyHead head;
    short stats[8];
    int hp;
    int max_hp;
    int sp;
    int max_sp;
    int exp;
    int max_exp;
    short skill_points;
    short attack_range;
};
struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY3 {
    struct HomunPropertyHead head;
    short stats[8];
    int hp;
    int max_hp;
    int sp;
    int max_sp;
    long long exp;
    long long max_exp;
    short skill_points;
    short attack_range;
};
#pragma pack(pop)
SIZE_ASSERT(HomunPropertyHead, 0x21);
SIZE_ASSERT(PACKET_ZC_PROPERTY_HOMUN, 0x49);
SIZE_ASSERT(PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY, 0x49);
SIZE_ASSERT(PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY2, 0x4d);
SIZE_ASSERT(PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY3, 0x55);

static void homun_redraw(void)
{
    session_invalidate_homun_info_wnd(&g_session1);
}

static void homun_name(const struct HomunPropertyHead *head)
{
    if (g_serviceType == 10 || g_serviceType == 12 || g_serviceType == 15)
        sprintf(g_session1.homunName, "%s", head->name);
    else
        sprintf(g_session1.homunName, head->name);
}

static void homun_hunger_warning(const struct HomunPropertyHead *head)
{
    short fullness = head->fullness;

    if ((fullness == 1 || fullness == 5 || fullness == 10) && g_session1.homun_ai_state[1] != fullness) {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(0x3fa), 0xff, 0, 0);
        g_session1.homun_ai_state[1] = head->fullness;
    }
}

static void homun_window_show(void)
{
    if (g_windowMgr.homun_skill_list_wnd)
        wnd_send(g_windowMgr.homun_skill_list_wnd, 0x17, 0, 0, 0, 0);
}

static void homun_stats(const short *stat)
{
    g_session1.homunStats[0] = stat[0];
    g_session1.homunStats[1] = stat[1];
    g_session1.homunStats[2] = stat[2];
    g_session1.homunStats[3] = stat[3];
    g_session1.homunStats[4] = stat[4];
    g_session1.homunStats[5] = stat[5];
    g_session1.homunStats[6] = stat[6];
    g_session1.homunStats[7] = stat[7];
}

void game_mode_zc_property_homun(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PROPERTY_HOMUN *packet = (const struct PACKET_ZC_PROPERTY_HOMUN *)pkt;

    (void)self;
    homun_name(&packet->head);
    g_session1.homunIntimacy = packet->head.intimacy;
    g_session1.homunItem = packet->item;
    g_session1.homunRenamed = packet->head.renamed;
    homun_stats(packet->stats);
    g_session1.homunLevel = packet->head.level;
    g_session1.homunHp = packet->hp;
    homun_redraw();
    g_session1.homunSp = packet->sp;
    homun_redraw();
    g_session1.homunMaxHp = packet->max_hp;
    homun_redraw();
    g_session1.homunMaxSp = packet->max_sp;
    homun_redraw();
    g_session1.homunExp = packet->exp;
    homun_redraw();
    g_session1.homunMaxExp = packet->max_exp;
    homun_redraw();
    g_session1.homunFullness = packet->head.fullness;
    homun_redraw();
    g_session1.homunSkillPoints = packet->skill_points;
    session_invalidate_windows(&g_session1);
    g_session1.homun_ai_state[2] = packet->attack_range;
    homun_window_show();
    homun_hunger_warning(&packet->head);
}

void game_mode_zc_property_homun_no_accessory(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY *packet =
        (const struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY *)pkt;

    (void)self;
    homun_name(&packet->head);
    g_session1.homunIntimacy = packet->head.intimacy;
    g_session1.homunRenamed = packet->head.renamed;
    homun_stats(packet->stats);
    g_session1.homunLevel = packet->head.level;
    g_session1.homunHp = packet->hp;
    homun_redraw();
    g_session1.homunSp = packet->sp;
    homun_redraw();
    g_session1.homunMaxHp = packet->max_hp;
    homun_redraw();
    g_session1.homunMaxSp = packet->max_sp;
    homun_redraw();
    g_session1.homunExp = packet->exp;
    homun_redraw();
    g_session1.homunMaxExp = packet->max_exp;
    homun_redraw();
    g_session1.homunFullness = packet->head.fullness;
    homun_redraw();
    g_session1.homunSkillPoints = packet->skill_points;
    session_invalidate_windows(&g_session1);
    g_session1.homun_ai_state[2] = packet->attack_range;
    homun_window_show();
    homun_hunger_warning(&packet->head);
}

static void homun_v2_head(const struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY2 *packet)
{
    snprintf(g_session1.homunName, 0x20, "%s", packet->head.name);
    g_session1.homunRenamed = packet->head.renamed;
    g_session1.homunLevel = packet->head.level;
    g_session1.homunFullness = packet->head.fullness;
    homun_redraw();
    homun_hunger_warning(&packet->head);
    g_session1.homunIntimacy = packet->head.intimacy;
    homun_stats(packet->stats);
    g_session1.homunHp = packet->hp;
    homun_redraw();
    g_session1.homunMaxHp = packet->max_hp;
    homun_redraw();
    g_session1.homunSp = packet->sp;
    homun_redraw();
    g_session1.homunMaxSp = packet->max_sp;
    homun_redraw();
}

void game_mode_zc_property_homun_no_accessory2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY2 *packet =
        (const struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY2 *)pkt;

    (void)self;
    if (!pkt)
        return;
    homun_v2_head(packet);
    g_session1.homunExp = packet->exp;
    homun_redraw();
    g_session1.homunMaxExp = packet->max_exp;
    homun_redraw();
    g_session1.homunSkillPoints = packet->skill_points;
    session_invalidate_windows(&g_session1);
    homun_window_show();
    g_session1.homun_ai_state[2] = packet->attack_range;
}

void game_mode_zc_property_homun_no_accessory3(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY3 *packet =
        (const struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY3 *)pkt;

    (void)self;
    if (!pkt)
        return;
    homun_v2_head((const struct PACKET_ZC_PROPERTY_HOMUN_NO_ACCESSORY2 *)pkt);
    g_session1.homunExp = packet->exp;
    homun_redraw();
    g_session1.homunMaxExp = packet->max_exp;
    homun_redraw();
    g_session1.homunSkillPoints = packet->skill_points;
    session_invalidate_windows(&g_session1);
    homun_window_show();
    g_session1.homun_ai_state[2] = packet->attack_range;
}
