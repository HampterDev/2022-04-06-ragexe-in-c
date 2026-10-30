#include "ui/window_manager_lifecycle.h"
#include "render/device.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type.h"
#include "core/memory.h"
#include "render/render_api.h"
#include "ui/window_types.h"
#include "ui/controls/check_radio.h"
#include "actor/status_icons.h"
#include <string.h>
#include "core/app.h"
#include "core/msg_str.h"
#include "session/session_options.h"
#include "ui/render/graphic_setting.h"


#define MSG_APPLY_AGREE    0xc5f

enum { KIND_WINDOWED, KIND_EXCLUSIVE, KIND_BORDERLESS };
static int current_kind(const struct C3dDevice *d);
static int requested_kind(int fullscreen);
int borderless_setting = 1;
static struct UICheckBox *borderlessBox;

static int live_request(const struct UIGraphicSettingWnd *wnd, int *w, int *h, int *fullscreen)
{
    if (!g_3dDevice.dev)
        return 0;
    if (!wnd->modes.first || wnd->mode < 0)
        return 0;
    *w = ((int (*)[3])wnd->modes.first)[wnd->mode][0];
    *h = ((int (*)[3])wnd->modes.first)[wnd->mode][1];
    *fullscreen = wnd->box_fullscreen && wnd->box_fullscreen->base.state;
    return *w > 0 && *h > 0;
}

char *display_prompt_text(struct UIGraphicSettingWnd *wnd, int restartId)
{
    int w, h, fs;
    return msg_str(live_request(wnd, &w, &h, &fs) ? MSG_APPLY_AGREE : restartId);
}

int apply_display_mode(struct UIGraphicSettingWnd *wnd)
{
    int w, h, fs;

    g_fullscreenMode = wnd->box_fullscreen ? wnd->box_fullscreen->base.state : 0;
    if (borderlessBox)
        borderless_setting = borderlessBox->base.state != 0;
    if (!live_request(wnd, &w, &h, &fs))
        return 0;
    return set_display(w, h, fs, &wnd->adapter_guid);
}

static int moved(int pos, int size, int oldExtent, int newExtent)
{
    int nearEdge = pos, farEdge = oldExtent - (pos + size), out;
    if (size >= oldExtent)
        out = pos;
    else if (nearEdge - farEdge <= oldExtent / 8 && farEdge - nearEdge <= oldExtent / 8)
        out = (newExtent - size) / 2;
    else if (farEdge < nearEdge)
        out = newExtent - size - farEdge;
    else
        out = pos;
    if (out + size > newExtent)
        out = newExtent - size;
    if (out < 0)
        out = 0;
    return out;
}

static int class_is(const char *cls, const char *name)
{
    return !strcmp(cls, name);
}

static int placed_by_the_game(const struct UIWindow *win, int oldW, int oldH)
{
    const char *cls = object_type_name(win->ops);
    if (win->x + win->width <= 0 || win->y + win->height <= 0 || win->x >= oldW || win->y >= oldH)
        return 1;
    return class_is(cls, "UIPcGage") || class_is(cls, "UIPlayerGage") || class_is(cls, "UIMonsterGage")
        || class_is(cls, "UIMerchantShopTitle") || class_is(cls, "UIChatRoomTitle") || class_is(cls, "CBmpObjWnd")
        || class_is(cls, "UIBalloonText") || class_is(cls, "UINameBalloonText") || class_is(cls, "UICharInfoBalloonText")
        || class_is(cls, "UIColorTransBalloonText") || class_is(cls, "UIAchBalloonText");
}

static void move_window(struct UIWindow *win, int x, int y)
{
    win->ops->move(win, x, y);
}

static void relayout_ui(int w, int h)
{
    struct UIWindowMgr *m = &g_windowMgr;
    struct ListNode *head = m->windows.head, *n;
    int oldW = m->screen_width, oldH = m->screen_height;
    ui_window_mgr_set_screen_size(m, w, h);
    if (!head || oldW <= 0 || oldH <= 0 || (oldW == w && oldH == h))
        return;
    for (n = head->next; n && n != head; n = n->next) {
        struct UIWindow *win = *LIST_VALUE(n, struct UIWindow *);
        int x, y, skip;
        if (!win)
            continue;
        skip = placed_by_the_game(win, oldW, oldH);
        x = skip ? win->x : moved(win->x, win->width, oldW, w);
        y = skip ? win->y : moved(win->y, win->height, oldH, h);
        if (!skip && (x != win->x || y != win->y))
            move_window(win, x, y);
    }
}

static void relaunch_state_icons(void)
{
    if (game_hide_player_status_icons()) {
        game_show_player_status_icons();
    }
}

#define WINDOWED_STYLE   (WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN)
#define FULLSCREEN_STYLE WS_POPUP

static HMONITOR target_monitor(HWND hwnd, const GUID *adapter)
{
    HMONITOR m = gfx_adapter_monitor(adapter);
    return m ? m : MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
}

static void monitor_rects(HMONITOR m, RECT *mon, RECT *work)
{
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (m && GetMonitorInfoA(m, &mi)) {
        *mon = mi.rcMonitor;
        *work = mi.rcWork;
    } else {
        SetRect(mon, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        *work = *mon;
    }
}

int borderless_rect(const GUID *adapter, RECT *rc)
{
    RECT work;
    HMONITOR m = gfx_adapter_monitor(adapter);
    if (!m || !borderless_setting)
        return 0;
    monitor_rects(m, rc, &work);
    return 1;
}

static int requested_kind(int fullscreen)
{
    if (!fullscreen)
        return KIND_WINDOWED;
    return borderless_setting ? KIND_BORDERLESS : KIND_EXCLUSIVE;
}

static int current_kind(const struct C3dDevice *d)
{
    if (d->fullscreen)
        return KIND_EXCLUSIVE;
    return (GetWindowLongA(d->hwnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION ? KIND_WINDOWED : KIND_BORDERLESS;
}

static void place_windowed(HWND hwnd, HMONITOR on, int x, int y, int clientW, int clientH, int centre)
{
    RECT rc, mon, work;
    int fw, fh;
    monitor_rects(on, &mon, &work);
    SetWindowLongA(hwnd, GWL_STYLE, WINDOWED_STYLE | (GetWindowLongA(hwnd, GWL_STYLE) & WS_VISIBLE));
    SetRect(&rc, 0, 0, clientW, clientH);
    AdjustWindowRect(&rc, WINDOWED_STYLE, FALSE);
    fw = rc.right - rc.left;
    fh = rc.bottom - rc.top;
    if (centre) {
        x = work.left + (work.right - work.left - fw) / 2;
        y = work.top + (work.bottom - work.top - fh) / 2;
    }
    if (x + fw > work.right)  x = work.right - fw;
    if (y + fh > work.bottom) y = work.bottom - fh;
    if (x < work.left) x = work.left;
    if (y < work.top)  y = work.top;
    SetWindowPos(hwnd, NULL, x, y, fw, fh, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

static void place_frameless(HWND hwnd, HMONITOR on, int w, int h)
{
    RECT mon, work;
    monitor_rects(on, &mon, &work);
    SetWindowLongA(hwnd, GWL_STYLE, FULLSCREEN_STYLE | WS_VISIBLE);
    SetWindowPos(hwnd, HWND_TOP, mon.left, mon.top, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

static void read_geometry(struct C3dDevice *d, int fullscreen, int w, int h)
{
    RECT rc;
    if (fullscreen) {
        SetRect(&d->rcWindow, 0, 0, w, h);
        d->width = w;
        d->height = h;
        d->rcClient = d->rcWindow;
    } else {
        GetClientRect(d->hwnd, &d->rcWindow);
        d->width = d->rcWindow.right;
        d->height = d->rcWindow.bottom;
        d->rcClient = *window_client_rect_on_monitor(&rc, d->hwnd);
    }
}

int set_display(int w, int h, int fullscreen, const GUID *adapter)
{
    struct C3dDevice *d = &g_3dDevice;
    struct Renderer *r = g_renderer;
    RECT old;
    HMONITOR was, on;
    int oldW = d->width, oldH = d->height, moving, kind, wasKind, devFull, wasDevFull;

    if (!d->hwnd || !d->dev || w <= 0 || h <= 0)
        return 0;
    moving = adapter && memcmp(adapter, &init_adapter, sizeof(GUID)) != 0;
    kind = requested_kind(fullscreen);
    wasKind = current_kind(d);
    devFull = kind == KIND_EXCLUSIVE;
    wasDevFull = d->fullscreen != 0;
    GetWindowRect(d->hwnd, &old);
    was = target_monitor(d->hwnd, &init_adapter);
    on = target_monitor(d->hwnd, adapter ? adapter : &init_adapter);
    if (kind == KIND_BORDERLESS) {
        RECT mon, work;
        monitor_rects(on, &mon, &work);
        w = mon.right - mon.left;
        h = mon.bottom - mon.top;
    }
    if (w == oldW && h == oldH && kind == wasKind && !moving)
        return 1;
    if (devFull) {
        place_frameless(d->hwnd, on, w, h);
        read_geometry(d, 1, w, h);
    } else if (!wasDevFull) {
        if (kind == KIND_BORDERLESS)
            place_frameless(d->hwnd, on, w, h);
        else
            place_windowed(d->hwnd, on, old.left, old.top, w, h, moving || wasKind == KIND_BORDERLESS);
        read_geometry(d, 0, w, h);
    } else {
        read_geometry(d, 1, w, h);
    }
    if (gfx_set_mode(d, adapter, d->width, d->height, devFull) != 0) {
        if (wasDevFull || wasKind == KIND_BORDERLESS)
            place_frameless(d->hwnd, was, oldW, oldH);
        else
            place_windowed(d->hwnd, was, old.left, old.top, oldW, oldH, 0);
        read_geometry(d, wasDevFull, oldW, oldH);
        return 0;
    }
    if (adapter)
        init_adapter = *adapter;
    d->fullscreen = devFull;
    if (!devFull && wasDevFull) {
        if (kind == KIND_BORDERLESS)
            place_frameless(d->hwnd, on, w, h);
        else
            place_windowed(d->hwnd, on, 0, 0, w, h, 1);
        read_geometry(d, 0, w, h);
        if (d->width != w || d->height != h)
            gfx_set_mode(d, NULL, d->width, d->height, 0);
    }
    if (devFull && !wasDevFull)
        ShowCursor(FALSE);
    else if (!devFull && wasDevFull)
        ShowCursor(TRUE);
    if (r) {
        renderer_set_projection(r, d->width, d->height);
        if (r->clipLeft == -40.0f) {
            r->clipRight = (float)(r->width + 640);
            r->clipBottom = (float)(r->height + 480);
        }
    }
    relayout_ui(d->width, d->height);
    relaunch_state_icons();
    if (g_session1.mouseExclusiveOption && GetForegroundWindow() == d->hwnd) {
        RECT rc;
        POINT p0 = { 0, 0 }, p1;
        GetClientRect(d->hwnd, &rc);
        p1.x = rc.right;
        p1.y = rc.bottom;
        ClientToScreen(d->hwnd, &p0);
        ClientToScreen(d->hwnd, &p1);
        SetRect(&rc, p0.x, p0.y, p1.x, p1.y);
        ClipCursor(&rc);
    }
    return 1;
}

#define BORDERLESS_ROW_Y   0x100
#define TRILINEAR_ROW_Y    0xc0
#define ROW_HIDDEN        -200

void settings_add_row(struct UIGraphicSettingWnd *wnd, struct UIWindow *fullscreenBox)
{
    struct UICheckBox *box;

    ui_window_add_child((struct UIWindow *)wnd, fullscreenBox);
    box = (struct UICheckBox *)mem_alloc(sizeof *box);
    borderlessBox = NULL;
    if (!box)
        return;
    ui_check_box_construct(&box->base);
    ui_window_create((&box->base), 60, 12);
    ui_check_box_set_bitmaps(&box->base, "유저인터페이스\\checkbox_1.bmp", "유저인터페이스\\checkbox_0.bmp", msg_str(0xc69));
    ui_check_box_set_check(&box->base, borderless_setting != 0);
    ui_window_add_child((struct UIWindow *)wnd, (&box->base));
    borderlessBox = box;
}

int settings_is_display_box(struct UIWindow *sender, struct UIGraphicSettingWnd *wnd)
{
    return sender == (&wnd->box_fullscreen->base) || sender == (&wnd->box_mouse->base)
        || (borderlessBox && sender == (&borderlessBox->base));
}

void settings_hide_rows(struct UIGraphicSettingWnd *wnd)
{
    move_window((&wnd->box_trilinear->base), ROW_HIDDEN, ROW_HIDDEN);
    if (borderlessBox)
        move_window((&borderlessBox->base), ROW_HIDDEN, ROW_HIDDEN);
}

void settings_place_rows(struct UIGraphicSettingWnd *wnd)
{
    const struct UIWindow *at = (const struct UIWindow *)wnd->device_box;

    move_window((&wnd->box_trilinear->base), at->x, at->y + TRILINEAR_ROW_Y);
    if (borderlessBox)
        move_window((&borderlessBox->base), at->x, at->y + BORDERLESS_ROW_Y);
}

void settings_draw_labels(struct UIGraphicSettingWnd *wnd)
{
    ui_window_text_out((struct UIWindow *)wnd, 0x12, wnd->box_trilinear->base.y,
               msg_str(0x7ce), 0, 0, 0xc, 0, 0, 0);
    if (borderlessBox)
        ui_window_text_out((struct UIWindow *)wnd, 0x12, borderlessBox->base.y,
                   "Borderless", 0, 0, 0xc, 0, 0, 0);
}

int settings_display_changed(struct UIGraphicSettingWnd *wnd)
{
    if (wnd->box_fullscreen->base.state != g_fullscreenMode)
        return 1;
    return borderlessBox && (borderlessBox->base.state != 0) != (borderless_setting != 0);
}

int read_display_options(void *session, const char *name, int def)
{
    borderless_setting = session_get_option_int(session, "ISBORDERLESS", 1) != 0;
    return session_get_option_int(session, name, def);
}

void write_display_options(void *session, void *stream, const char *name, int value)
{
    session_write_option_int(session, stream, name, value);
    session_write_option_int(session, stream, "ISBORDERLESS", borderless_setting != 0);
}
