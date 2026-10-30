#include "ui/window_manager_state.h"
#include "ui/window_mgr.h"
#include <assert.h>
#include <string.h>

struct UIWindowMgr g_windowMgr;

static const struct UIWindow *actors_told;
static int actors_calls;

void actors_forget_window(const struct UIWindow *wnd)
{
    actors_told = wnd;
    actors_calls++;
}

int main(void)
{
    struct UIWindow *gone = (struct UIWindow *)0x1000, *kept = (struct UIWindow *)0x2000;
    struct UIWindowMgr *mgr = &g_windowMgr;

    memset(mgr, 0, sizeof *mgr);
    mgr->loading_wnd = gone;
    mgr->apply_for_party_wnd = gone;
    mgr->quit_wnd = gone;
    mgr->item_store_sub_wnd[3] = gone;
    mgr->modal_wnd = gone;
    mgr->capture = gone;
    mgr->capture_root = gone;
    mgr->focus_edit = gone;
    mgr->mouse_wnd = gone;
    mgr->replay_wnd = gone;
    mgr->active_top_wnd = gone;
    mgr->login_wnd = kept;
    mgr->item_store_sub_wnd[4] = kept;
    mgr->choose_wnd = kept;

    ui_window_mgr_forget_window(mgr, gone);

    assert(mgr->loading_wnd == NULL && mgr->apply_for_party_wnd == NULL && mgr->quit_wnd == NULL);
    assert(mgr->item_store_sub_wnd[3] == NULL);
    assert(mgr->modal_wnd == NULL && mgr->capture == NULL && mgr->capture_root == NULL);
    assert(mgr->focus_edit == NULL && mgr->mouse_wnd == NULL && mgr->replay_wnd == NULL);
    assert(mgr->active_top_wnd == NULL);
    assert(mgr->login_wnd == kept && mgr->item_store_sub_wnd[4] == kept && mgr->choose_wnd == kept);
    assert(actors_told == gone && actors_calls == 1);
    return 0;
}
