#include "../src/ui/social/agency_results.c"
#include <assert.h>
#include <stdio.h>

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct UIAdvenPartyBoardWnd board;
static struct UIApplyForPartyWnd apply;
static int board_visible, apply_visible, refreshes, joins, quits, chats, members, own_ad;
static int expected_aid, expected_gid, mode_cookie;

struct UIWindow *ui_window_find(int id)
{
    if (id == 0x144 && board_visible) return &board.frame.base;
    if (id == 0x159 && apply_visible) return &apply.frame.base;
    return NULL;
}
static intptr_t on_board_message(struct UIWindow *w, struct UIWindow *sender, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    (void)sender; (void)b; (void)c; (void)d;
    assert(w == &board.frame.base && (msg == 0x3c || (msg == 6 && a == 0x197)));
    ++refreshes; return 0;
}
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{ (void)a; (void)b; (void)c; (void)d; assert(mgr == &g_windowMgr && msg == 1); ++chats; return 0; }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *mgr, int id)
{ assert(mgr == &g_windowMgr && id == 0x159); ++quits; return 1; }
char *msg_str(int id) { (void)id; return "message"; }
void ui_adven_party_board_wnd_set_own_ad(struct UIAdvenPartyBoardWnd *w, const struct AdvenPartyInfo *ad)
{ assert(w == &board && ad->aid == expected_aid); own_ad = 1; }
void ui_adven_party_board_wnd_set_has_ad(struct UIAdvenPartyBoardWnd *w, BYTE has)
{ assert(w == &board); own_ad = has; }
int ui_apply_for_party_wnd_set_ad(struct UIApplyForPartyWnd *w, const struct AdvenPartyInfo *ad)
{ assert(w == &apply); w->ad = *ad; return 1; }
int session_get_party_member_count(struct Session *s) { assert(s == &g_session1); return members; }
void ui_button_set_show(struct UIButton *button, BYTE show) { (void)button; assert(show == 0); }
struct Mode *client_current_mode(void) { return (struct Mode *)&mode_cookie; }
intptr_t mode_send_message(struct Mode *mode, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(mode == (struct Mode *)&mode_cookie && msg == 0x109 && c == 0 && d == 0);
    assert(*(int *)a == expected_aid && *(int *)b == expected_gid); ++joins; return 1;
}

int main(void)
{
    struct UIWindowOps ops = {0};
    struct AdventurerAgencyMgr manager = {0};
    struct AdvenPartyInfo ad = {0};
    struct PartyBookingList data = {&ad, 1};
    int previous;
    ops.send_msg = on_board_message; board.frame.base.ops = &ops;
    expected_aid = ad.aid = 100; expected_gid = ad.gid = 200;
    ad.min_level = 30; ad.max_level = 90; g_session1.charBaseLevel = 50;
    manager.page = 1; manager.pages = 1;
    /* Closed windows remain closed and cannot trigger an application. */
    agency_present(&manager, AGENCY_LIST, 0, &data);
    agency_present(&manager, AGENCY_OWN, 0, &data);
    agency_present(&manager, AGENCY_DETAIL, 0, &data);
    agency_present(&manager, AGENCY_REGISTER, 0, &data);
    assert(!refreshes && !joins && !own_ad);
    board_visible = 1; board.page = 8; board.pages = 20;
    agency_present(&manager, AGENCY_LIST, AGENCY_UNAVAILABLE, NULL);
    assert(refreshes == 1 && board.page == 1 && board.pages == 1);
    agency_present(&manager, AGENCY_LIST, 0, &data); assert(refreshes == 2);
    agency_present(&manager, AGENCY_OWN, 0, &data); assert(own_ad);
    data.count = 0; agency_present(&manager, AGENCY_OWN, 0, &data); assert(!own_ad);
    data.count = 1; apply_visible = 1; apply.ad.aid = 999;
    agency_present(&manager, AGENCY_DETAIL, 0, &data); assert(!joins && quits == 1);
    apply.ad.aid = ad.aid; g_session1.charBaseLevel = 29;
    agency_present(&manager, AGENCY_DETAIL, 0, &data); assert(!joins);
    g_session1.charBaseLevel = 50; members = 1;
    agency_present(&manager, AGENCY_DETAIL, 0, &data); assert(!joins);
    members = 0;
    agency_present(&manager, AGENCY_DETAIL, 0, &data); assert(joins == 1 && quits == 2);
    previous = refreshes; agency_present(&manager, AGENCY_DELETE, 0, &data); assert(refreshes == previous + 1);
    assert(chats >= 4);
    puts("Agency UI: closed/stale windows do not apply; errors recover board controls; empty own ads and existing join checks passed.");
    return 0;
}
