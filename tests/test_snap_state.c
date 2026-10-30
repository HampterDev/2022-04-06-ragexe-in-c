#include "ui/misc/snap_mgr.h"
#include "ui/window_types.h"
#include "ui/window_manager_state.h"
#include "world_state_memory.h"
#include <stdio.h>

struct UIWindowMgr g_windowMgr;
struct Renderer *g_renderer;
static unsigned moves, saves, updates;

static void save_position(struct UIWindow *w) { assert(w); ++saves; }
static void move_delta(struct UIWindow *w, int x, int y)
{ w->x += x; w->y += y; ++moves; }
void ui_window_mgr_update_snap_rect(struct UIWindowMgr *mgr, struct UIWindow *w)
{ assert(mgr == &g_windowMgr && w); ++updates; }
void ui_window_mgr_get_snap_delta(struct UIWindowMgr *mgr, struct UIWindow *w, int *x, int *y, int flag)
{ assert(mgr == &g_windowMgr && w && flag == 1); *x = *y = 0; }

int main(void)
{
    struct UIWindowOps slots = {0};
    struct UIWindow a = {0}, b = {0}, c = {0}, distant = {0};
    unsigned round;
    slots.save_pos = save_position;
    slots.move_delta = move_delta;
    a.ops = b.ops = c.ops = distant.ops = &slots;
    a.width = b.width = c.width = distant.width = 100;
    a.height = b.height = c.height = distant.height = 50;
    snap_mgr_construct(&g_snapMgr);
    for (round = 0; round < 8; ++round) {
        a.x = a.y = b.y = c.x = 0;
        b.x = 100; c.y = 50;
        distant.x = distant.y = 500;
        snap_mgr_register_root(&g_snapMgr, &a);
        snap_mgr_register(&g_snapMgr, &b);
        snap_mgr_register(&g_snapMgr, &c);
        snap_mgr_register(&g_snapMgr, &distant);
        snap_mgr_register(&g_snapMgr, &b);
        assert(g_snapMgr.objects.size == 4);
        assert(snap_mgr_is_root_window(&g_snapMgr, &a));
        assert(snap_mgr_are_connected(&g_snapMgr, &a, &b));
        assert(snap_mgr_in_root_group(&g_snapMgr, &c));
        assert(!snap_mgr_in_root_group(&g_snapMgr, &distant));
        moves = saves = updates = 0;
        snap_mgr_save_group_positions(&g_snapMgr, &a);
        snap_mgr_move_group(&g_snapMgr, &a, 10, 20);
        assert(moves == 2 && saves == 2 && updates == 2);
        assert(a.x == 0 && b.x == 110 && c.y == 70 && distant.x == 500);
        snap_mgr_unregister(&g_snapMgr, &b);
        assert(!snap_mgr_find(&g_snapMgr, &b) && g_snapMgr.objects.size == 3);
        snap_mgr_unregister(&g_snapMgr, &a);
        assert(g_snapMgr.root == NULL && !snap_mgr_in_root_group(&g_snapMgr, &c));
        snap_mgr_clear(&g_snapMgr);
        assert(g_snapMgr.objects.size == 0 && live_blocks == 1);
    }
    snap_mgr_destruct(&g_snapMgr);
    assert(live_blocks == 0);
    puts("snap state: registration, connected movement, unlinking and guarded list ownership passed");
    return 0;
}
