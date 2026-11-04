#include "actor/object_ops.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "session/state.h"
#include "../src/game/banking.c"
#include "../src/game/packets_mercenary.c"

static DWORD clock_now;
static uint32_t test_clock(void) { return clock_now; }
#define timeGetTime test_clock
#define platform_ticks_ms test_clock
#include "../src/game/screen_counters.c"
#undef timeGetTime
#undef platform_ticks_ms

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
struct Mouse g_mouse;
struct ResMgr *g_resMgr(void) { return NULL; }
void *res_mgr_get(struct ResMgr *manager, const char *name)
{ (void)manager; (void)name; return NULL; }
unsigned actres_frame_count(const struct ActRes *act, unsigned action)
{ (void)act; (void)action; assert(0); return 0; }
static unsigned glyph_count;
static int glyphs[14];
void screen_sprite_frame(struct Renderer *target, int x, int y, struct ActRes *act,
    struct SprRes *spr, int action, int motion, float scale, float angle, DWORD colour, int flags)
{
    (void)x; (void)y; (void)act; (void)spr; (void)motion; (void)scale; (void)angle; (void)colour; (void)flags;
    assert(target == &renderer && glyph_count < 14);
    glyphs[glyph_count++] = action;
}
static struct GameActor companion;
static struct World world;
static unsigned expected_aid;
static int actor_present, cleared, redraws, actor_messages;
static int notices, invalidations, close_calls, open_calls, closed_window, last_message;
static int observed_hp[4], observed_sp[4], observed_max_hp[4], observed_max_sp[4];

void session_refresh_basic_info(struct Session *session)
{ assert(session == &g_session1); ++invalidations; }
void session_invalidate_windows(struct Session *session)
{ assert(session == &g_session1); ++notices; }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *manager, int id)
{ assert(manager == &g_windowMgr && id == 0x113); ++close_calls; return (BYTE)closed_window; }
struct UIWindow *ui_window_open(int id)
{ assert(id == 0x113); ++open_calls; return NULL; }
char *msg_str(int id) { last_message = id; return "bank failure"; }
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *manager, int message, intptr_t text, intptr_t colour, intptr_t c, intptr_t d)
{
    assert(manager == &g_windowMgr && message == UIM_CHAT_LINE && colour == 0xff && !c && !d);
    assert(!strcmp((const char *)text, "bank failure"));
    return 0;
}
void skill_info_destroy(void *skill) { (void)skill; }
void list_clear(struct List *list, ListDestroyFn destroy)
{ assert(list == &g_session1.merSkills && destroy == skill_info_destroy); ++cleared; }
struct GameActor *world_get_game_actor_by_aid(struct World *owner, unsigned aid)
{ assert(owner == &world && aid == expected_aid); return actor_present ? &companion : NULL; }
int game_actor_get_job(struct GameActor *actor)
{ assert(actor == &companion); return 6123; }
void session_invalidate_mer_info_wnd(struct Session *session)
{
    assert(session == &g_session1);
    if (redraws < 4) {
        observed_hp[redraws] = session->mercHp;
        observed_sp[redraws] = session->mercSp;
        observed_max_hp[redraws] = session->mercMaxHp;
        observed_max_sp[redraws] = session->mercMaxSp;
    }
    ++redraws;
}
static void companion_message(struct GameObject *actor, struct GameObject *sender, RenderMsgArg message,
    RenderMsgArg a, RenderMsgArg b, RenderMsgArg c, RenderMsgArg d)
{
    assert((void *)actor == &companion && !sender && message == 0x22 && !a && !b && !c && !d);
    ++actor_messages;
}

static void banking(void)
{
    unsigned char bytes[16] = {0, 0, 0, 0, 0xef, 0xcd, 0xab, 0x89, 0x44, 0x33, 0x22, 0x11, 0x40, 0xe2, 1, 0};
    struct PacketBankingAck ack;
    struct PacketBankingCheck check = {0};
    const int deposit_messages[] = {0, 0x996, 0x998, 0x9a1, 0x9b9, 0x996};
    const int withdraw_messages[] = {0, 0x997, 0x996, 0x9b9, 0x996, 0x996};
    unsigned i;
    memcpy(&ack, bytes, sizeof ack);
    game_mode_ack_banking_deposit(NULL, &ack);
    assert(g_session1.bankZeny == 0x1122334489abcdefLL && g_session1.zeny == 123456);
    assert(notices == 1 && invalidations == 1);
    ack.balance = -1; ack.zeny = 765;
    game_mode_ack_banking_withdraw(NULL, &ack);
    assert(g_session1.bankZeny == -1 && g_session1.zeny == 765);
    assert(notices == 2 && invalidations == 2);
    for (i = 1; i < sizeof deposit_messages / sizeof deposit_messages[0]; ++i) {
        ack.result = (unsigned short)i;
        game_mode_ack_banking_deposit(NULL, &ack);
        assert(last_message == deposit_messages[i]);
        game_mode_ack_banking_withdraw(NULL, &ack);
        assert(last_message == withdraw_messages[i]);
        assert(g_session1.bankZeny == -1 && g_session1.zeny == 765);
        assert(notices == 2 && invalidations == 2);
    }
    check.balance = 0x7fffffffffffffffLL;
    game_mode_banking_check(NULL, &check);
    assert(g_session1.bankZeny == check.balance && close_calls == 1 && open_calls == 1);
    closed_window = 1; check.balance = 0;
    game_mode_banking_check(NULL, &check);
    assert(!g_session1.bankZeny && close_calls == 2 && open_calls == 1);
    check.reason = 2; check.balance = 456;
    game_mode_banking_check(NULL, &check);
    assert(last_message == 0xbd7 && !g_session1.bankZeny && close_calls == 2);
    check.reason = 1;
    game_mode_banking_check(NULL, &check);
    assert(last_message == 0x996 && !g_session1.bankZeny);
}

static void companions(void)
{
    struct GameMode mode = {0};
    struct PacketMerInit packet = {0};
    const struct GameObjectOps ops = {.send_msg = companion_message};
    unsigned char change[8] = {0};
    int value;
    unsigned i;
    const unsigned short parameters[] = {5, 6, 7, 8, 41, 49, 53, 165, 189, 190, 65535};
    const int stats[] = {-1, 2, -3, 4, -5, 6, -7, 8};
    mode.world = &world; actor_present = 1;
    companion.ops = &ops;
    expected_aid = packet.aid = 0xf1234567u;
    memcpy(packet.name, "%s%n literal companion!!", sizeof packet.name);
    packet.atk = -1; packet.matk = 2; packet.hit = -3; packet.crit = 4;
    packet.def = -5; packet.mdef = 6; packet.flee = -7; packet.aspd = 8;
    packet.level = 99; packet.hp = 10; packet.max_hp = 20; packet.sp = 30; packet.max_sp = 40;
    packet.faith = -123; packet.summon_count = 234; packet.kill_count = 345; packet.attack_range = 12; packet.expire_time = 456;
    g_session1.mercJob = -1;
    game_mode_zc_mer_init(&mode, &packet);
    assert((unsigned)g_session1.mercAid == expected_aid && g_session1.mercPresent == 1 && g_session1.mercJob == 6123);
    assert(!memcmp(g_session1.mercName, packet.name, sizeof packet.name) && !g_session1.mercName[24]);
    assert(!memcmp(g_session1.mercStats, stats, sizeof stats) && g_session1.mercLevel == 99);
    assert(g_session1.mercHp == 10 && g_session1.mercMaxHp == 20 && g_session1.mercSp == 30 && g_session1.mercMaxSp == 40);
    assert(g_session1.merc_contract_stats[0] == -123 && g_session1.merc_contract_stats[1] == 234 && g_session1.merc_contract_stats[2] == 345
        && g_session1.merc_contract_stats[3] == 12 && g_session1.merc_contract_stats[4] == 456);
    assert(cleared == 1 && redraws == 4);
    assert(observed_hp[0] == 10 && !observed_sp[0] && !observed_max_hp[0] && !observed_max_sp[0]);
    assert(observed_hp[1] == 10 && observed_sp[1] == 30 && !observed_max_hp[1] && !observed_max_sp[1]);
    assert(observed_max_hp[2] == 20 && !observed_max_sp[2] && observed_max_sp[3] == 40);
    actor_present = 0; packet.name[4] = 0; packet.name[5] = 'x'; g_session1.mercJob = 321;
    game_mode_zc_mer_init(&mode, &packet);
    assert(!strcmp(g_session1.mercName, "%s%n") && g_session1.mercJob == 321 && cleared == 2);
    for (i = 0; i < sizeof parameters / sizeof parameters[0]; ++i) {
        int before = redraws;
        value = 1000 + (int)i;
        memcpy(change + 2, &parameters[i], 2); memcpy(change + 4, &value, 4);
        game_mode_zc_mer_par_change(change);
        assert(redraws - before == (i < 4 ? 2 : 1));
    }
    assert(g_session1.mercHp == 1000 && g_session1.mercMaxHp == 1001 && g_session1.mercSp == 1002 && g_session1.mercMaxSp == 1003);
    assert(g_session1.mercStats[0] == 1004 && g_session1.mercStats[2] == 1005 && g_session1.mercStats[7] == 1006 && g_session1.mercStats[6] == 1007);
    assert(g_session1.merc_contract_stats[2] == 1008 && g_session1.merc_contract_stats[0] == 1009);
    g_session1.mercPresent = 0; i = (unsigned)redraws;
    game_mode_zc_mer_par_change(change); assert((unsigned)redraws == i);
}

static void clocks(void)
{
    struct PACKET_ZC_SHOWDIGIT packet = {0};
    struct GameMode mode = {0};
    packet.type = TIMER_FOR_5S; packet.value = -1;
    game_mode_zc_show_digit(NULL, &packet);
    assert(g_session1.countdownTicks == 0xfffffc18u && g_session1.timerKind == TIMER_FOR_5S && g_session1.timerShown == 1);
    clock_now = 0xfffffff0u; packet.type = TIMER_SINCE; packet.value = 1;
    game_mode_zc_show_digit(NULL, &packet);
    assert(g_session1.countdownTicks == 984u && g_session1.timerKind == TIMER_SINCE);
    clock_now = 16; packet.type = TIMER_UNTIL;
    game_mode_zc_show_digit(NULL, &packet);
    assert(g_session1.countdownTicks == 0xfffffc28u && g_session1.timerKind == TIMER_UNTIL);
    packet.type = TIMER_COUNTDOWN; packet.value = INT_MAX;
    game_mode_zc_show_digit(NULL, &packet);
    assert(g_session1.countdownTicks == 0xffffffffu && g_session1.timerKind == TIMER_COUNTDOWN);
    packet.value = 0; game_mode_zc_show_digit(NULL, &packet); assert(g_session1.countdownTicks == 29);
    packet.type = 255; g_session1.timerShown = 0;
    game_mode_zc_show_digit(NULL, &packet);
    assert(g_session1.countdownTicks == 29 && g_session1.timerKind == TIMER_COUNTDOWN && g_session1.timerShown == 1);
    g_session1.countdownTicks = 0; glyph_count = 0;
    game_mode_draw_time_attack(&mode, 0, TIMER_COUNTDOWN);
    assert(g_session1.countdownTicks == UINT_MAX && !g_session1.timerShown && glyph_count == 2 && !glyphs[0] && !glyphs[1]);
    clock_now = 984; glyph_count = 0;
    game_mode_draw_time_attack(&mode, 0xfffffff0u, TIMER_SINCE);
    assert(glyph_count == 2 && glyphs[0] == 0 && glyphs[1] == 1);
    {
        static const int invalid[] = {INT_MIN, -1, 4, 255, INT_MAX};
        struct GameMode other = {0};
        struct GameMode *modes[] = {&mode, &other};
        unsigned i, j;
        for (i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
            for (j = 0; j < 2; ++j) {
                modes[j]->timer_shown_since = 123;
                g_session1.timerShown = 1;
                g_session1.countdownTicks = 456;
                glyph_count = 0;
                game_mode_draw_time_attack(modes[j], 987, invalid[i]);
                assert(!glyph_count && !g_session1.timerShown && !modes[j]->timer_shown_since);
                assert(g_session1.countdownTicks == 456);
            }
        }
    }
}

int main(void)
{
    banking(); companions(); clocks();
    puts("Session scalar packets: 64-bit bank balances, failure paths, companion fields and bounded names, redraw order and timer wraparound passed");
    return 0;
}
