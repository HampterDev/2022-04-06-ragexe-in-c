#include "core/user_input.h"
#include "core/window_state.h"
#include "core/window_events.h"
#include "contents/mouse.h"
#include "platform/vk.h"
#include "platform/window.h"
#include "ui/window_keyboard.h"
#include "ui/window_manager_state.h"

int input_key_lparam(const struct InputKey *key)
{
    unsigned value = (unsigned)(key->count > 0 ? key->count & 0xffff : 1);
    value |= (unsigned)(key->scancode & 0xff) << 16;
    if (key->scancode & 0x100)
        value |= 1u << 24;
    if (key->alt)
        value |= 1u << 29;
    if (key->repeat || !key->down)
        value |= 1u << 30;
    if (!key->down)
        value |= 1u << 31;
    return (int)value;
}

int input_key(const struct InputKey *key)
{
    int lparam = input_key_lparam(key);
    platform_note_key(key->vk, key->down);
    if (!key->down) {
        window_events_key_up(key->vk, lparam);
        return 1;
    }
    if (key->alt || key->vk == VK_F10)
        return ui_window_mgr_on_key_down(&g_windowMgr, key->vk, lparam, !key->repeat);
    window_events_key_down(key->vk, lparam);
    return 1;
}

void input_char(int ch)
{
    window_events_char(ch);
}

void input_pointer(int x, int y)
{
    platform_note_cursor(x, y);
    mouse_on_move(&g_mouse);
}

void input_button(int button, int down, int x, int y)
{
    static const int keys[] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};
    if (button < INPUT_BUTTON_LEFT || button > INPUT_BUTTON_MIDDLE)
        return;
    platform_note_cursor(x, y);
    platform_note_key(keys[button], down);
    if (button == INPUT_BUTTON_LEFT)
        mouse_on_button(&g_mouse, g_hMainWnd, MOUSE_LEFT, down);
    else if (button == INPUT_BUTTON_RIGHT)
        mouse_on_button(&g_mouse, g_hMainWnd, MOUSE_RIGHT, down);
}

void input_wheel(int delta)
{
    mouse_on_wheel(&g_mouse, delta);
}
