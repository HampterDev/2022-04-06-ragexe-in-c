#include "replay/replay.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "ui/wndmgr/make_window_internal.h"
#include "session/session_jobs.h"
#include "ui/character/gauge.h"
#include "ui/render/graphic_setting.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Run the production basic-info factory over saved "shown" flags and record
   which windows it reopens. Every request enters the ESC window-ID list, so it
   must name a window that a factory can create. */
int g_replayPlaying;
int g_spriteMode, g_textureMode;
struct Session g_session1;
struct SnapMgr g_snapMgr;
static struct UIWindowMgr mgr;
static struct UIWindow basic_info;
static int requested[32], request_count, shows, plate_restores, camera_free, tracked_achievements;

struct UIWindow *ui_window_mgr_make_window(struct UIWindowMgr *self, int id)
{
    assert(self == &mgr && request_count < (int)(sizeof requested / sizeof *requested));
    requested[request_count++] = id;
    return NULL;
}
void wm_show(struct UIWindow *wnd) { assert(wnd == &basic_info); ++shows; }
int game_camera_is_free(void) { return camera_free; }
BYTE ui_window_mgr_show_option_window(struct UIWindowMgr *self, int window_id, int show)
{
    assert(self == &mgr && (window_id == 0x1 || window_id == 0x24 || window_id == 0xf));
    (void)show;
    return 1;
}
BYTE session_has_tracked_achievements(struct Session *self)
{
    assert(self == &g_session1);
    return (BYTE)tracked_achievements;
}
void ui_window_mgr_restore_plates(struct UIWindowMgr *self) { assert(self == &mgr); ++plate_restores; }

/* Window creation is not exercised: the basic-info window already exists. */
struct UIWindow *ui_basic_info_wnd_new(void) { assert(!"exists"); return NULL; }
void ui_window_create(struct UIWindow *wnd, int width, int height) { (void)wnd; (void)width; (void)height; assert(!"exists"); }
int session_get_drawn_job(struct Session *self) { (void)self; assert(!"exists"); return 0; }
BYTE session_is_fourth_job(struct Session *self, int job) { (void)self; (void)job; assert(!"exists"); return 0; }
void wm_move(struct UIWindow *wnd, int x, int y) { (void)wnd; (void)x; (void)y; assert(!"exists"); }
void ui_window_mgr_add_window(struct UIWindowMgr *self, struct UIWindow *wnd) { (void)self; (void)wnd; assert(!"exists"); }
void snap_mgr_register_root(struct SnapMgr *self, struct UIWindow *wnd) { (void)self; (void)wnd; assert(!"exists"); }
struct UIWindow *ui_window_open(int id) { (void)id; assert(!"exists"); return NULL; }
void ui_window_to_screen(struct UIWindow *wnd, int *x, int *y) { (void)wnd; (void)x; (void)y; assert(!"exists"); }
struct UIWindow *ui_window_mgr_get_window(struct UIWindowMgr *self, int id) { (void)self; (void)id; assert(!"live"); return NULL; }
struct ReplayRecorder *replay_recorder_instance(void) { assert(!"live"); return NULL; }

#include "basic_info_restore.inc"

static void expect(const int *ids, int count)
{
    int i;

    request_count = 0;
    assert(make_basic_info(&mgr) == &basic_info);
    assert(request_count == count);
    for (i = 0; i < count; i++)
        assert(requested[i] == ids[i]);
}

int main(void)
{
    static const int none[1] = {0};
    static const int all[] = {0x8, 0xa, 0xb, 0x25, 0x141, 0xe, 0x45, 0x10f, 0x11e};
    static const int status_only[] = {0xb};

    mgr.basic_info_wnd = &basic_info;
    mgr.option_info.show = 1;
    expect(none, 0);

    mgr.status_info.show = 1;
    expect(status_only, 1);

    mgr.item_info.show = 1;
    mgr.equip_info.show = 1;
    mgr.skill_list_info.show = 1;
    mgr.quest_info.show = 1;
    mgr.messenger_group_info.show = 1;
    mgr.macro_reporter_info.open = 1;
    camera_free = tracked_achievements = 1;
    expect(all, (int)(sizeof all / sizeof *all));

    mgr.option_info.show = 0;
    expect(all, (int)(sizeof all / sizeof *all));
    assert(shows == 4 && plate_restores == 4);
    puts("Basic info restore: saved windows reopened; stale option-window flag requests nothing.");
    return 0;
}
