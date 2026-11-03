#ifdef _WIN32
#include "core/app.h"
#include "core/drive_state.h"
#include "platform/filesystem.h"
#include "platform/language.h"
#include "platform/window.h"
#include "session/session_options.h"
#include <imm.h>

unsigned char g_windowDragging;
RECT g_windowDragRect;
POINT g_windowDragCursor;
HWND g_windowDragHwnd;

static void window_drag_follow(void)
{
    POINT cursor;
    int dx, dy;

    if (g_windowDragging != 1)
        return;
    GetCursorPos(&cursor);
    dy = cursor.y - g_windowDragCursor.y;
    dx = cursor.x - g_windowDragCursor.x;
    if (dx != 0 || dy != 0)
        MoveWindow(g_windowDragHwnd, g_windowDragRect.left + dx, g_windowDragRect.top + dy,
                   g_windowDragRect.right - g_windowDragRect.left, g_windowDragRect.bottom - g_windowDragRect.top,
                   TRUE);
    if ((platform_async_key_state(VK_LBUTTON) & 0x8000) == 0)
        g_windowDragging = 0;
}

static void pump_messages(void)
{
    MSG msg;

    if (!PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        return;
    do {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_PROCESSKEY) {
            UINT key = ImmGetVirtualKey(msg.hwnd);

            language_on_reading_key(g_language, (BYTE)key);
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    } while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE));
}

void check_system_message(void)
{
    pump_messages();
    window_drag_follow();
    drive_state_tick();
}

void app_last_pump(void)
{
    pump_messages();
    window_drag_follow();
    DestroyWindow(g_hMainWnd);
}

int app_create_window(HINSTANCE instance)
{
    WNDCLASSA wc;
    RECT rect;
    int x, y;

    g_hInstance = instance;
    wc.style = 0;
    wc.lpfnWndProc = window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(0x77));
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = g_windowClassName;
    wc.lpszMenuName = 0;
    RegisterClassA(&wc);

    rect.right = g_screenWidth;
    rect.bottom = g_screenHeight;
    rect.left = 0;
    rect.top = 0;
    AdjustWindowRect(&rect, 0x2c20000, FALSE);

    x = 0;
    y = 0;
    if (g_fullscreenMode == 0) {
        GetSystemMetrics(SM_CXSCREEN);
        if (g_savedWidth == g_screenWidth && g_savedHeight == g_screenHeight) {
            x = session_get_option_int(&g_session1, "Window_XPos", (GetSystemMetrics(SM_CXSCREEN) - g_screenWidth) / 2);
            y = session_get_option_int(&g_session1, "Window_YPos", (GetSystemMetrics(SM_CYSCREEN) - g_screenHeight) / 2);
        } else {
            x = (GetSystemMetrics(SM_CXSCREEN) - g_screenWidth) / 2;
            y = (GetSystemMetrics(SM_CYSCREEN) - g_screenHeight) / 2;
        }
        if (x > GetSystemMetrics(SM_CXSCREEN) - g_screenWidth)
            x = (GetSystemMetrics(SM_CXSCREEN) - g_screenWidth) / 2;
        if (x < 0)
            x = 0;
        if (y > GetSystemMetrics(SM_CYSCREEN) - g_screenHeight)
            y = (GetSystemMetrics(SM_CYSCREEN) - g_screenHeight) / 2;
        if (y < 0)
            y = 0;
    }

    g_hMainWnd = CreateWindowExA(0, wc.lpszClassName, g_windowClassName, 0x2c20000, x, y,
                                   rect.right - rect.left, rect.bottom - rect.top, 0, 0, instance, 0);
    if (!g_hMainWnd) {
        platform_message_box(0, "Window Creation Failed", "ERROR", 0);
        return 0;
    }
    ShowWindow(g_hMainWnd, SW_SHOW);
    UpdateWindow(g_hMainWnd);
    platform_current_directory(g_clientDir, 0x80);
    return 1;
}
#endif
