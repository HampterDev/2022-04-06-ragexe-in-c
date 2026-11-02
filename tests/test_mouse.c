#include "contents/mouse.h"
#include "render/device.h"
#include "mouse_fixture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct C3dDevice g_3dDevice;
HWND g_hMainWnd;
static DWORD now;
static POINT cursor;
static unsigned captures, releases;

uint32_t mouse_test_time(void) { return now; }
BOOL mouse_test_cursor(POINT *point) { *point = cursor; return TRUE; }
BOOL mouse_test_client(HWND window, POINT *point)
{
    assert(window == g_hMainWnd);
    point->x -= 20;
    point->y -= 30;
    return TRUE;
}
HWND mouse_test_capture(HWND window)
{
    assert(window == g_hMainWnd);
    ++captures;
    return NULL;
}
BOOL mouse_test_release(void) { ++releases; return TRUE; }

void mem_free(void *memory)
{
    free(memory);
}

static void message(UINT message, WPARAM value)
{
    mouse_on_message(&g_mouse, g_hMainWnd, message, value, 0);
}

int main(void)
{
    assert(g_mouse.ready == 1 && g_mouse.double_click_ms == 500);
    assert(g_mouse.x == 0 && g_mouse.y == 0);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_UP);
    assert(g_mouse.button[MOUSE_RIGHT] == MOUSE_BUTTON_UP);
    assert(g_mouse.wheel == 0 && g_mouse.wheel_pending == 0);

    now = 2000;
    cursor.x = 100;
    cursor.y = 200;
    message(WM_MOUSEMOVE, 0);
    assert(g_mouse.x == 0 && g_mouse.y == 0);
    mouse_read_state(&g_mouse);
    assert(g_mouse.x == 80 && g_mouse.y == 170);
    assert(g_mouse.dx == 80 && g_mouse.dy == 170);

    message(WM_LBUTTONDOWN, 0);
    mouse_read_state(&g_mouse);
    assert(captures == 1);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_PRESSED);
    mouse_read_state(&g_mouse);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_HELD);
    message(WM_LBUTTONUP, 0);
    mouse_read_state(&g_mouse);
    assert(releases == 1);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_RELEASED);
    mouse_read_state(&g_mouse);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_UP);

    now += 100;
    message(WM_LBUTTONDOWN, 0);
    mouse_read_state(&g_mouse);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_DOUBLE);
    message(WM_RBUTTONDOWN, 0);
    mouse_read_state(&g_mouse);
    assert(g_mouse.button[MOUSE_RIGHT] == MOUSE_BUTTON_PRESSED);

    message(WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-120));
    assert(g_mouse.wheel == 0);
    mouse_read_state(&g_mouse);
    assert(g_mouse.wheel == -120 && g_mouse.wheel_pending == 0);
    mouse_read_state(&g_mouse);
    assert(g_mouse.wheel == 0);

    g_mouse.ready = 0;
    message(WM_RBUTTONUP, 0);
    assert(g_mouse.ready == 1);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_UP);
    assert(g_mouse.down[MOUSE_LEFT] == 0 && g_mouse.was_down[MOUSE_LEFT] == 0);
    assert(g_mouse.down[MOUSE_RIGHT] == 0 && g_mouse.was_down[MOUSE_RIGHT] == 0);
    g_mouse.ready = 0;
    g_mouse.cur_x = 400;
    mouse_read_state(&g_mouse);
    assert(g_mouse.x == 80);

    g_3dDevice.width = 1280;
    g_3dDevice.height = 720;
    mouse_center(&g_mouse);
    assert(g_mouse.x == 640 && g_mouse.cur_x == 640);
    assert(g_mouse.y == 360 && g_mouse.cur_y == 360);
    mouse_set_x(&g_mouse, -100);
    mouse_set_y(&g_mouse, -100);
    assert(g_mouse.x == -100 && g_mouse.cur_x == -100);
    assert(g_mouse.y == -100 && g_mouse.cur_y == -100);

    puts("mouse: global initialization, frame input, wheel, acquisition and positioning passed");
    return 0;
}
