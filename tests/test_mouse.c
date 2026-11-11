#include "contents/mouse.h"
#include "render/device.h"
#include "platform/window.h"
#include "mouse_fixture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* contents/mouse.c with the SDL platform layer: the pointer is where the input last put it
   (platform_note_cursor, in client coordinates), and holding the left button captures the mouse
   through SDL. */
struct C3dDevice g_3dDevice;
PlatformWindow g_hMainWnd;
static DWORD now;
static unsigned captures, releases;

uint32_t mouse_test_time(void) { return now; }
bool mouse_test_capture(bool capture)
{
    if (capture)
        ++captures;
    else
        ++releases;
    return true;
}

void mem_free(void *memory)
{
    free(memory);
}

static void button(int which, int down)
{
    mouse_on_button(&g_mouse, g_hMainWnd, which, down);
}

int main(void)
{
    assert(g_mouse.ready == 1 && g_mouse.double_click_ms == 500);
    assert(g_mouse.x == 0 && g_mouse.y == 0);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_UP);
    assert(g_mouse.button[MOUSE_RIGHT] == MOUSE_BUTTON_UP);
    assert(g_mouse.wheel == 0 && g_mouse.wheel_pending == 0);

    now = 2000;
    platform_note_cursor(80, 170);
    mouse_on_move(&g_mouse);
    assert(g_mouse.x == 0 && g_mouse.y == 0);
    mouse_read_state(&g_mouse);
    assert(g_mouse.x == 80 && g_mouse.y == 170);
    assert(g_mouse.dx == 80 && g_mouse.dy == 170);

    button(MOUSE_LEFT, 1);
    mouse_read_state(&g_mouse);
    assert(captures == 1);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_PRESSED);
    mouse_read_state(&g_mouse);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_HELD);
    button(MOUSE_LEFT, 0);
    mouse_read_state(&g_mouse);
    assert(releases == 1);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_RELEASED);
    mouse_read_state(&g_mouse);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_UP);

    now += 100;
    button(MOUSE_LEFT, 1);
    mouse_read_state(&g_mouse);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_DOUBLE);
    button(MOUSE_RIGHT, 1);
    mouse_read_state(&g_mouse);
    assert(g_mouse.button[MOUSE_RIGHT] == MOUSE_BUTTON_PRESSED);

    mouse_on_wheel(&g_mouse, -120);
    assert(g_mouse.wheel == 0);
    mouse_read_state(&g_mouse);
    assert(g_mouse.wheel == -120 && g_mouse.wheel_pending == 0);
    mouse_read_state(&g_mouse);
    assert(g_mouse.wheel == 0);

    g_mouse.ready = 0;
    button(MOUSE_RIGHT, 0);
    assert(g_mouse.ready == 1);
    assert(g_mouse.button[MOUSE_LEFT] == MOUSE_BUTTON_UP);
    assert(g_mouse.down[MOUSE_LEFT] == 0 && g_mouse.was_down[MOUSE_LEFT] == 0);
    assert(g_mouse.down[MOUSE_RIGHT] == 0 && g_mouse.was_down[MOUSE_RIGHT] == 0);
    g_mouse.ready = 0;
    g_mouse.cur_x = 400;
    mouse_read_state(&g_mouse);
    assert(g_mouse.x == 80);

    mouse_set_x(&g_mouse, -100);
    mouse_set_y(&g_mouse, -100);
    assert(g_mouse.x == -100 && g_mouse.cur_x == -100);
    assert(g_mouse.y == -100 && g_mouse.cur_y == -100);

    puts("mouse: global initialization, frame input, wheel, acquisition and positioning passed");
    return 0;
}
