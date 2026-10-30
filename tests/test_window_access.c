#include "iteminfo/item_api.h"
#include "iteminfo/item_list.h"
#include "contents/ui_window.h"
#include "ui/window_access.h"
#include "ui/window_types.h"
#include "session/csession.h"
#include "ui/window_types.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <assert.h>
#include <stdio.h>

struct UIWindowMgr g_windowMgr;
static struct UIWindow window, sender;
static struct UIWindowOps methods;
static struct UIWindow *available;
static int finds, opens, sends, shows, raises, moves, refreshes;

struct UIWindow *ui_window_mgr_get_window(struct UIWindowMgr *self, int id)
{
    assert(self == &g_windowMgr && id == 37);
    ++finds;
    return available;
}

struct UIWindow *ui_window_mgr_make_window(struct UIWindowMgr *self, int id)
{
    assert(self == &g_windowMgr && id == 37);
    ++opens;
    return available;
}

void ui_window_mgr_raise_window(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    assert(self == &g_windowMgr && wnd == &window);
    ++raises;
}

static int send(struct UIWindow *wnd, struct UIWindow *source,
                           int msg, int a, int b, int c, int d)
{
    assert(wnd == &window && source == &sender && msg == 73);
    assert(a == 10 && b == 20 && c == 30 && d == 40);
    ++sends;
    return 81;
}

static void show(struct UIWindow *wnd, int visible)
{
    assert(wnd == &window && visible == 1);
    ++shows;
}

static void move(struct UIWindow *wnd, int x, int y)
{
    assert(wnd == &window && x == 12 && y == 34);
    ++moves;
}

static void refresh(struct UIWindow *wnd)
{
    assert(wnd == &window);
    ++refreshes;
}

static struct UIWindow *notification_target;
static int notifications;

static int notify(struct UIWindow *wnd, struct UIWindow *source,
                  int message, int a, int b, int c, int d)
{
    assert(wnd == notification_target && source == NULL);
    assert(message == UIMSG_REFRESH && a == 0 && b == 0 && c == 0 && d == 0);
    ++notifications;
    return 17;
}

static void check_session_notifications(void)
{
    struct UIWindow *slot = NULL;
    notify_item_window(&slot);
    assert(notifications == 0);
    methods.send_msg = notify;
    sender.ops = &methods;
    notification_target = slot = &window;
    notify_item_window(&slot);
    assert(notifications == 1);
    notification_target = slot = &sender;
    notify_item_window(&slot);
    assert(notifications == 2);
    slot = NULL;
    notify_item_window(&slot);
    assert(notifications == 2);
}

int main(void)
{
    methods.send_msg = send;
    methods.set_show = show;
    methods.move = move;
    methods.invalidate = refresh;
    window.ops = &methods;

    assert(ui_window_find(37) == NULL && finds == 1);
    assert(ui_window_send(ui_window_open(37), &sender, 73, 10, 20, 30, 40) == 0);
    ui_window_show(NULL, 1);
    ui_window_raise(NULL);
    ui_window_move_safe(NULL, 12, 34);
    ui_window_refresh(NULL);
    assert(opens == 1 && sends == 0 && shows == 0 && raises == 0 && moves == 0 && refreshes == 0);

    available = &window;
    assert(ui_window_find(37) == &window && finds == 2);
    assert(ui_window_send(ui_window_open(37), &sender, 73, 10, 20, 30, 40) == 81);
    ui_window_show(&window, 1);
    ui_window_raise(&window);
    ui_window_move_safe(&window, 12, 34);
    ui_window_refresh(&window);
    assert(opens == 2 && sends == 1 && shows == 1 && raises == 1 && moves == 1 && refreshes == 1);

    available = NULL;
    assert(ui_window_send(ui_window_find(37), &sender, 73, 10, 20, 30, 40) == 0);
    assert(sends == 1);
    check_session_notifications();
    puts("Window interfaces: shared record, coexisting headers and notifications use the current nullable window passed");
    puts("window access: nullable queries, one creation per request, message arguments and optional operations passed");
    return 0;
}
