#include "render/renderer.h"
#include "session/session_jobs.h"
#include "status_wnd_info.inc"

#include <assert.h>
#include <stdio.h>

/* Restore saved status-window layouts through the production UI_MSG_SET_INFO
   path. A window saved as a title bar must come back as a title bar. The
   trait expand command does nothing, so the restored expand flag is the
   inverse of the saved one and the height is not changed by it. */
#define SAVED_EXPANDED_HEIGHT 0xf1

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
struct SnapMgr g_snapMgr;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
static struct UIBitmapButton plus_buttons[STAT_COUNT], trait_buttons[STAT_COUNT];
static int cases;

int session_get_drawn_job(struct Session *self) { (void)self; return 0; }
BYTE session_is_fourth_job(struct Session *self, int job) { (void)self; (void)job; return 0; }
int session_get_param(struct Session *self, int type) { assert(self == &g_session1); (void)type; return 0; }

void ui_window_resize_base(struct UIWindow *self, int width, int height)
{
    self->width = width;
    self->height = height;
}
void ui_window_move_base(struct UIWindow *self, int x, int y) { self->x = x; self->y = y; }
void ui_frame_wnd_move(struct UIWindow *self, int x, int y) { self->x = x; self->y = y; }
void ui_window_invalidate_base(struct UIWindow *self) { (void)self; }
void ui_window_mgr_update_snap_rect(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    assert(self == &g_windowMgr && wnd);
}
void ui_window_mgr_raise_window(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    assert(self == &g_windowMgr && wnd);
}
void snap_mgr_move_chain(struct SnapMgr *self, struct UIWindow *wnd, int dx, int dy, int side)
{
    assert(self == &g_snapMgr && wnd && dx == 0 && dy != 0 && side == 3);
}
int ui_frame_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3,
                          int val4)
{
    (void)self; (void)sender; (void)val1; (void)val2; (void)val3; (void)val4;
    assert(msg == UI_MSG_RESIZE);
    return 0;
}
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *self, int window_id)
{
    (void)self; (void)window_id;
    assert(!"not closed");
    return 0;
}
struct Mode *client_current_mode(void) { assert(!"no stat requests"); return NULL; }
int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{
    (void)mode; (void)message; (void)a; (void)b; (void)c; (void)d;
    assert(!"no stat requests");
    return 0;
}

/* The state the status-window factory leaves before it sends UI_MSG_SET_INFO. */
static void create(struct UIStatusWnd *self)
{
    int i;

    memset(self, 0, sizeof *self);
    self->frame.base.width = STATUS_WIDTH;
    self->frame.base.height = STATUS_HEIGHT;
    self->title_height = TITLE_HEIGHT;
    for (i = 0; i < STAT_COUNT; i++) {
        self->plus[i] = &plus_buttons[i];
        self->trait_plus[i] = &trait_buttons[i];
    }
    self->full_height = self->frame.base.height;
}

static void restore(struct UIStatusWnd *self, struct StatusWndInfo *info)
{
    create(self);
    ui_status_wnd_send_msg(&self->frame.base, NULL, UI_MSG_SET_INFO, (int)(INT_PTR)info, 0, 0, 0);
}

static void expect(struct UIStatusWnd *self, int x, int y, int height, int full_height, int expanded)
{
    assert(self->frame.base.x == x && self->frame.base.y == y);
    assert(self->frame.base.width == STATUS_WIDTH && self->frame.base.height == height);
    assert(self->full_height == full_height && self->expanded == expanded);
    ++cases;
}

static void toggle_title_bar(struct UIStatusWnd *self)
{
    ui_status_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
}

static void press_expand(struct UIStatusWnd *self)
{
    ui_status_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, ID_EXPAND, 0, 0, 0);
}

int main(void)
{
    static struct UIStatusWnd wnd;
    struct StatusWndInfo info;

    renderer.width = 1024;
    renderer.height = 768;

    info = (struct StatusWndInfo){ { 10, 20, STATUS_WIDTH, STATUS_HEIGHT }, STATUS_HEIGHT, 1, 0 };
    restore(&wnd, &info);
    expect(&wnd, 10, 20, STATUS_HEIGHT, STATUS_HEIGHT, 1);
    press_expand(&wnd);
    expect(&wnd, 10, 20, STATUS_HEIGHT, STATUS_HEIGHT, 1);

    info = (struct StatusWndInfo){ { 30, 40, STATUS_WIDTH, SAVED_EXPANDED_HEIGHT }, SAVED_EXPANDED_HEIGHT, 1, 1 };
    restore(&wnd, &info);
    expect(&wnd, 30, 40, STATUS_HEIGHT, STATUS_HEIGHT, 0);
    press_expand(&wnd);
    expect(&wnd, 30, 40, STATUS_HEIGHT, STATUS_HEIGHT, 0);

    info = (struct StatusWndInfo){ { 50, 60, STATUS_WIDTH, TITLE_HEIGHT }, STATUS_HEIGHT, 1, 0 };
    restore(&wnd, &info);
    expect(&wnd, 50, 60, TITLE_HEIGHT, STATUS_HEIGHT, 1);
    toggle_title_bar(&wnd);
    expect(&wnd, 50, 60, STATUS_HEIGHT, STATUS_HEIGHT, 1);

    info = (struct StatusWndInfo){ { 70, 80, STATUS_WIDTH, TITLE_HEIGHT }, SAVED_EXPANDED_HEIGHT, 1, 1 };
    restore(&wnd, &info);
    expect(&wnd, 70, 80, TITLE_HEIGHT, STATUS_HEIGHT, 0);
    ui_status_wnd_store_info(&wnd.frame.base);
    assert(info.rect.height == TITLE_HEIGHT && info.org_height == STATUS_HEIGHT && info.expanded == 0);
    restore(&wnd, &info);
    expect(&wnd, 70, 80, TITLE_HEIGHT, STATUS_HEIGHT, 1);
    toggle_title_bar(&wnd);
    expect(&wnd, 70, 80, STATUS_HEIGHT, STATUS_HEIGHT, 1);
    toggle_title_bar(&wnd);
    expect(&wnd, 70, 80, TITLE_HEIGHT, STATUS_HEIGHT, 1);

    info = (struct StatusWndInfo){ { 90, 100, 0x100, TITLE_HEIGHT }, STATUS_HEIGHT, 1, 0 };
    restore(&wnd, &info);
    expect(&wnd, 0xb9, 0x12c, STATUS_HEIGHT, STATUS_HEIGHT, 0);

    printf("Status window info: %d cases passed (saved title bar restored, expand command ignored).\n", cases);
    return 0;
}
