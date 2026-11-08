#include "game/packet_handlers.h"
#include "ui/window_access.h"
#include "ui/window_manager_state.h"
#include "contents/lapine_upgrade.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct UIWindowMgr g_windowMgr;
static struct UIWindow synthesis_box, upgrade_box;
static struct UIWindow *open_synthesis, *open_upgrade;
static void *waiting_window;
static int waiting_calls, waiting_value, quit_calls, quit_id, cases;

struct UIWindow *ui_window_find(int id)
{
    if (id == 0x122)
        return open_synthesis;
    if (id == 0x12e)
        return open_upgrade;
    assert(!"only the Lapine windows are looked up");
    return NULL;
}

void lapine_upgrade_set_waiting(struct LapineUpgradeBox *self, char on)
{
    waiting_window = self;
    waiting_value = on;
    ++waiting_calls;
}

BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *self, int window_id)
{
    assert(self == &g_windowMgr);
    quit_id = window_id;
    ++quit_calls;
    return 1;
}

#include "lapine_upgrade_result.inc"

static void check(int synthesis, int upgrade, unsigned result, struct UIWindow *expected_waiting, int expected_quit)
{
    open_synthesis = synthesis ? &synthesis_box : NULL;
    open_upgrade = upgrade ? &upgrade_box : NULL;
    waiting_window = NULL;
    waiting_calls = quit_calls = quit_id = 0;
    waiting_value = -1;
    game_mode_zc_ack_random_upgrade_item(NULL, result);
    if (expected_waiting) {
        assert(waiting_calls == 1 && waiting_window == expected_waiting && waiting_value == 0);
    } else {
        assert(waiting_calls == 0);
    }
    if (expected_quit) {
        assert(quit_calls == 1 && quit_id == expected_quit);
    } else {
        assert(quit_calls == 0);
    }
    ++cases;
}

int main(void)
{
    check(0, 1, 1, &upgrade_box, 0);
    check(1, 1, 1, &upgrade_box, 0);
    check(1, 0, 1, NULL, 0);
    check(0, 0, 1, NULL, 0);
    check(0, 1, 0xffff, &upgrade_box, 0);
    check(0, 1, 0, NULL, 0x12e);
    check(1, 1, 0, NULL, 0x12e);
    check(1, 0, 0, NULL, 0);
    check(0, 1, 0x10000, NULL, 0x12e);
    printf("Lapine upgrade result: %d cases passed (failure releases the upgrade box, success closes it).\n",
           cases);
    return 0;
}
