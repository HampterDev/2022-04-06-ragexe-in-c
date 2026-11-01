#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
static DWORD now;
static DWORD test_time(void) { return now; }
#define timeGetTime test_time
#include "attendance_banner_functions.inc"
#undef timeGetTime

static unsigned notifications, redraws;
static int selector_message(struct UIWindow *window, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    (void)window; (void)sender; (void)b; (void)c; (void)d;
    assert(msg == UI_MSG_SET_STATE && (a == 0 || a == 1));
    ++notifications;
    return 0;
}
static const struct UIWindowOps selector_ops = {.send_msg = selector_message};
void ui_window_invalidate_base(struct UIWindow *window) { (void)window; ++redraws; }
int ui_window_send_msg_base(struct UIWindow *window, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    (void)window; (void)sender; (void)msg; (void)a; (void)b; (void)c; (void)d;
    return 0;
}

static void verify(void)
{
    struct UICheckAttendanceWnd_Banner banner = {0};
    struct StdBanner38 rows[3] = {0};
    struct UIWindow controls[3] = {0};
    struct UIWindow *buttons[] = {controls, controls + 1, controls + 2};
    unsigned count;

    for (count = 0; count < 3; ++count)
        controls[count].ops = &selector_ops;
    for (count = 0; count <= 1; ++count) {
        banner.banners.first = rows;
        banner.banners.last = banner.banners.end = rows + (1 - count);
        now = 6000;
        ui_check_attendance_wnd_banner_on_process(&banner.base);
        assert(!notifications && !redraws && !banner.current && !banner.shown_at);
    }
    banner.banners.last = banner.banners.end = rows + 3;
    banner.buttons.first = buttons;
    banner.buttons.last = banner.buttons.end = buttons + 3;
    now = 5000;
    ui_check_attendance_wnd_banner_on_process(&banner.base);
    assert(!notifications && !redraws && !banner.current);
    now = 5001;
    ui_check_attendance_wnd_banner_on_process(&banner.base);
    assert(banner.current == 1 && banner.shown_at == now && notifications == 3 && redraws == 1);
    banner.current = 2;
    now = 10002;
    ui_check_attendance_wnd_banner_on_process(&banner.base);
    assert(!banner.current && banner.shown_at == now && notifications == 6 && redraws == 2);
    puts("PASS: zero/single banners need no selectors; multiple banners retain timing, selection and wraparound.");
}

int main(void)
{
    __try { verify(); }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        puts("FAIL: banner update read an absent selector.");
        return 1;
    }
    return 0;
}
