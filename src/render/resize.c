#include "ui/window_manager_lifecycle.h"
#include "render/device.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type.h"
#include "render/render_api.h"
#include "ui/window_types.h"
#include "actors/status_icons.h"
#include <string.h>
#include "core/window_state.h"
#include "render/display_options.h"
#include "render/render_options.h"
#include "platform/window.h"

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
        || class_is(cls, "UIMerchantShopTitle") || class_is(cls, "UIChatRoomTitle")
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

/* After the device did not take the window's size (it had no memory for its targets at that
   size): that size, which it is not asked to take again; it tries again when the window's size
   changes. */
static struct {
    int pending, width, height;
} failed;

/* custom: the device at the window's client size in pixels (render/display_options.h), and the
   projection and the UI windows with it; 0 when the device did not take the size. */
static int fit_device(struct C3dDevice *d)
{
    struct Renderer *r = g_renderer;
    RECT client;

    platform_client_rect(d->hwnd, &client);
    if (client.right <= 0 || client.bottom <= 0 || (client.right == d->width && client.bottom == d->height)) {
        failed.pending = 0;
        return 1;
    }
    if (gfx_resize(client.right, client.bottom) != 0) {
        failed.pending = 1;
        failed.width = client.right;
        failed.height = client.bottom;
        return 0;
    }
    failed.pending = 0;
    d->width = client.right;
    d->height = client.bottom;
    if (r) {
        renderer_set_projection(r, d->width, d->height);
        if (r->clipLeft == -40.0f) {
            r->clipRight = (float)(r->width + 640);
            r->clipBottom = (float)(r->height + 480);
        }
    }
    relayout_ui(d->width, d->height);
    relaunch_state_icons();
    return 1;
}

/* Whether to give the device the window's size now. */
static int should_fit(struct C3dDevice *d)
{
    RECT client;

    if (!failed.pending)
        return 1;
    platform_client_rect(d->hwnd, &client);
    return client.right != failed.width || client.bottom != failed.height;
}

/* The display options follow what the window is (render/display_options.h). */
static void track_window(struct C3dDevice *d)
{
    struct DisplayOptions options = { g_fullscreenMode, g_windowMaximized, g_displayWidth, g_displayHeight,
                                      g_windowX, g_windowY, g_displayWidth, g_displayHeight };
    RECT client, frame;

    platform_client_rect(d->hwnd, &client);
    platform_window_rect(d->hwnd, &frame);
    display_options_track(&options, platform_window_is_fullscreen(d->hwnd), platform_window_is_maximized(d->hwnd),
                          platform_window_is_minimized(d->hwnd), client.right, client.bottom, frame.left,
                          frame.top);
    g_fullscreenMode = options.fullscreen;
    g_windowMaximized = options.maximized;
    g_displayWidth = options.width;
    g_displayHeight = options.height;
    g_windowX = options.x;
    g_windowY = options.y;
}

/* custom: the window in its mode and the device at its size. Fullscreen covers the display the
   window is on. Windowed, the window is maximised, or its client area is w by h, made to fit the
   room its display has, with the frame's corner where the window last was neither maximised nor
   fullscreen, or centred on its display when no such place is known. Either way the maximised state
   is kept, for when the window is windowed. A device that does not take the size yet follows the
   window later (display_follow_window). */
void set_display(int fullscreen, int maximized, int w, int h)
{
    struct C3dDevice *d = &g_3dDevice;
    int wasFullscreen, wasMaximized, displayW, displayH, roomW, roomH;

    if (!d->hwnd || !d->target || w <= 0 || h <= 0)
        return;
    g_windowMaximized = maximized != 0;
    wasFullscreen = platform_window_is_fullscreen(d->hwnd);
    wasMaximized = platform_window_is_maximized(d->hwnd);
    if (fullscreen && !wasFullscreen) {
        platform_window_fullscreen(d->hwnd);
    } else if (!fullscreen && (wasFullscreen || maximized != wasMaximized
                               || (!maximized && (w != d->width || h != d->height)))) {
        int known = g_windowX != DISPLAY_NO_POSITION && g_windowY != DISPLAY_NO_POSITION;
        RECT frame;
        platform_window_rect(d->hwnd, &frame);
        platform_window_room(d->hwnd, &displayW, &displayH, &roomW, &roomH);
        display_fit(&w, &h, roomW, roomH);
        g_displayWidth = w;
        g_displayHeight = h;
        platform_window_place_windowed(d->hwnd, known ? g_windowX : frame.left, known ? g_windowY : frame.top, w, h,
                                       wasFullscreen && !known, maximized);
    }
    fit_device(d);
    track_window(d);
    if (g_session1.mouseExclusiveOption && platform_window_is_foreground(d->hwnd))
        platform_clip_cursor_to_client(d->hwnd, 1);
}

/* Once a pump (core/app_sdl.c): the device follows the window's size in pixels, which the player
   (dragging the frame, maximising, restoring), the system or SDL may change at any time, and the
   display options follow what the window is. */
void display_follow_window(void)
{
    struct C3dDevice *d = &g_3dDevice;

    if (!d->hwnd || !d->target)
        return;
    if (!platform_window_is_minimized(d->hwnd) && should_fit(d))
        fit_device(d);
    track_window(d);
}
