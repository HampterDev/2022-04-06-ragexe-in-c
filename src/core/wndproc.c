#include "contents/adventurer_agency.h"
#include "contents/character_settings.h"
#include "contents/account_settings.h"
#include "ui/window_keyboard.h"
#include "ui/window.h"
#include "platform/language.h"
#include "contents/frame_timer.h"
#include "render/device.h"
#include "ui/window_messages.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "platform/win_types.h"
#include "std/std.h"
#include "render/render_api.h"
#include "core/app.h"
#include "session/session_options.h"
#include "ui/controls/edit.h"
#include "core/window_events.h"
#include "core/user_input.h"

static LRESULT on_activate(HWND hwnd, WPARAM wparam)
{
    WORD state = LOWORD(wparam);

    if (state == WA_INACTIVE)
        window_events_activate(0);
    else if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
        window_events_activate(1);
    if (g_3dDevice.fullscreen != 1) {
        if (g_windowActive == 1)
            g_activatedNow = 1;
        return 1;
    }
    if (g_windowActive == 1) {
        g_activatedNow = 1;
        ShowWindow(hwnd, SW_MAXIMIZE);
        return 1;
    }
    ShowWindow(hwnd, SW_MINIMIZE);
    return 1;
}

static LRESULT on_set_cursor(LPARAM lparam)
{
    if (g_3dDevice.fullscreen != 0)
        return 0;
    if (g_session1.mouseExclusiveOption != 0 || LOWORD(lparam) == HTCLIENT) {
        SetCursor(0);
        return 0;
    }
    SetCursor(LoadCursorA(0, IDC_ARROW));
    mouse_set_x(&g_mouse, -100);
    mouse_set_y(&g_mouse, -100);
    return 0;
}

static int sys_command_passes(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    unsigned int cmd = wparam & 0xfff0;

    switch (cmd) {
    case SC_MOVE:
        if (g_3dDevice.fullscreen != 0)
            return 0;
        g_windowDragHwnd = hwnd;
        GetCursorPos(&g_windowDragCursor);
        GetWindowRect(hwnd, &g_windowDragRect);
        g_windowDragging = 1;
        return 0;
    case SC_MINIMIZE:
    case 0xf090:
    case SC_RESTORE:
    case SC_MONITORPOWER:
        return g_3dDevice.fullscreen == 0;
    case SC_KEYMENU:
        return lparam == ' ';
    case SC_CLOSE:
        return 1;
    default:
        return 0;
    }
}

/* The key message parameter, unpacked for core/user_input.c, which packs it again for the client. */
static int on_key(WPARAM wparam, LPARAM lparam, int down)
{
    struct InputKey key;
    key.vk = (int)wparam;
    key.scancode = (int)(lparam >> 16 & 0xff) | (lparam & 1 << 24 ? 0x100 : 0);
    key.down = down;
    key.repeat = down && (lparam & 1 << 30) != 0;
    key.alt = (lparam & 1 << 29) != 0;
    key.count = (int)(lparam & 0xffff);
    return input_key(&key);
}

static void on_mouse(UINT msg, WPARAM wparam, LPARAM lparam)
{
    int x = (short)LOWORD(lparam), y = (short)HIWORD(lparam);
    switch (msg) {
    case WM_MOUSEMOVE:
        input_pointer(x, y);
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        input_button(INPUT_BUTTON_LEFT, msg == WM_LBUTTONDOWN, x, y);
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        input_button(INPUT_BUTTON_RIGHT, msg == WM_RBUTTONDOWN, x, y);
        break;
    case WM_MOUSEWHEEL:
        input_wheel((short)HIWORD(wparam));
        break;
    }
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    unsigned char handled = 0;
    LRESULT result;

    result = language_on_ime_message(g_language, hwnd, msg, wparam, lparam, &handled);
    if (handled) {
        window_events_language_changed();
        return result;
    }
    switch (msg) {
    case WM_CREATE:
    case WM_DESTROY:
        return 0;
    case WM_MOVE:
        if (g_windowActive == 1)
            return 0;
        if (g_3dDevice.fullscreen != 0)
            return 0;
        c3d_device_show_frame(&g_3dDevice, 1);
        return 0;
    case WM_ACTIVATE:
        return on_activate(hwnd, wparam);
    case WM_PAINT:
        ValidateRect(hwnd, 0);
        return 0;
    case WM_CLOSE:
        window_events_close();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SETCURSOR:
        return on_set_cursor(lparam);
    case WM_NCACTIVATE:
        app_activate(LOWORD(wparam));
        if (wparam != 0)
            frame_timer_reset();
        break;
    case WM_KEYDOWN:
        on_key(wparam, lparam, 1);
        return 0;
    case WM_KEYUP:
        on_key(wparam, lparam, 0);
        return 0;
    case WM_CHAR:
        input_char((int)wparam);
        return 0;
    case WM_SYSKEYDOWN:
        if (on_key(wparam, lparam, 1))
            return 0;
        break;
    case WM_SYSCOMMAND:
        if (!sys_command_passes(hwnd, wparam, lparam))
            return 0;
        break;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEWHEEL:
        on_mouse(msg, wparam, lparam);
        break;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
        app_activate(0);
        break;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
        if (GetActiveWindow() != hwnd && IsIconic(hwnd))
            app_activate(0);
        else
            app_activate(1);
        break;
    case WM_IME_SETCONTEXT:
        wparam = 1;
        lparam = 0;
        break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}
