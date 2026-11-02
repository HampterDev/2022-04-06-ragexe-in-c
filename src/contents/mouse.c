#include "contents/mouse.h"
#include "render/device.h"
#include "core/app.h"
#include <string.h>
#include "platform/clock.h"

struct Mouse g_mouse = {
    .double_click_ms = 500,
    .ready = 1,
};

void mouse_read_state(struct Mouse *self)
{
    DWORD now;
    int i;

    if (!self->ready)
        return;
    self->dx = self->cur_x - self->x;
    self->dy = self->cur_y - self->y;
    self->x = self->cur_x;
    self->y = self->cur_y;
    now = platform_ticks_ms();
    for (i = 0; i < 2; i++) {
        if (self->dx != 0 || self->dy != 0)
            self->press_time[i] = now - self->double_click_ms;
        if (!self->down[i]) {
            self->button[i] = self->was_down[i] ? MOUSE_BUTTON_RELEASED : MOUSE_BUTTON_UP;
        } else if (self->was_down[i]) {
            self->button[i] = MOUSE_BUTTON_HELD;
        } else if (now < self->press_time[i] + self->double_click_ms) {
            self->button[i] = MOUSE_BUTTON_DOUBLE;
            self->press_time[i] = now - self->double_click_ms;
        } else {
            self->press_time[i] = now;
            self->button[i] = MOUSE_BUTTON_PRESSED;
        }
    }
    self->wheel = self->wheel_pending;
    self->wheel_pending = 0;
    self->was_down[MOUSE_LEFT] = self->down[MOUSE_LEFT];
    self->was_down[MOUSE_RIGHT] = self->down[MOUSE_RIGHT];
}

void mouse_reset_buttons(struct Mouse *self)
{
    self->button[MOUSE_RIGHT] = 0;
    self->button[MOUSE_LEFT] = 0;
    memset(self->was_down, 0, sizeof self->was_down);
    memset(self->down, 0, sizeof self->down);
}

void mouse_center(struct Mouse *self)
{
    self->cur_x = self->x = g_3dDevice.width / 2;
    self->cur_y = self->y = g_3dDevice.height / 2;
}

void mouse_set_x(struct Mouse *self, int x)
{
    self->cur_x = self->x = x;
}

void mouse_set_y(struct Mouse *self, int y)
{
    self->cur_y = self->y = y;
}

void mouse_on_message(struct Mouse *self, HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    POINT pt;

    (void)lparam;
    if (g_mouse.ready != 1) {
        mouse_reset_buttons(&g_mouse);
        g_mouse.ready = 1;
    }
    switch (msg) {
    case WM_MOUSEMOVE:
        GetCursorPos(&pt);
        ScreenToClient(g_hMainWnd, &pt);
        self->cur_x = pt.x;
        self->cur_y = pt.y;
        break;
    case WM_LBUTTONDOWN:
        self->down[MOUSE_LEFT] = 1;
        SetCapture(wnd);
        break;
    case WM_LBUTTONUP:
        self->down[MOUSE_LEFT] = 0;
        ReleaseCapture();
        break;
    case WM_RBUTTONDOWN:
        self->down[MOUSE_RIGHT] = 1;
        break;
    case WM_RBUTTONUP:
        self->down[MOUSE_RIGHT] = 0;
        break;
    case WM_MOUSEWHEEL:
        self->wheel_pending = (short)HIWORD(wparam);
        break;
    }
}
