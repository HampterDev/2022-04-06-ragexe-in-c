#ifdef _WIN32
#include "platform/window.h"
#include "platform/win_utf8.h"
#include "core/utf8.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <imm.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "imm32.lib")

/* The client's text is UTF-8; win_utf8_message_box shows it through MessageBoxW (text that is not
   UTF-8, such as a CP949 resource name, as CP949). */
void platform_message_box(PlatformWindow owner, const char *text, const char *caption, unsigned flags)
{
    win_utf8_message_box(owner, text, caption, flags);
}

short platform_async_key_state(int vk)
{
    return GetAsyncKeyState(vk);
}

short platform_key_state(int vk)
{
    return GetKeyState(vk);
}

void platform_note_key(int vk, int down)
{
    (void)vk;
    (void)down;
}

void platform_note_cursor(int x, int y)
{
    (void)x;
    (void)y;
}

/* The clipboard holds CF_UNICODETEXT; the client's text is UTF-8, whatever the system code page. */
int platform_clipboard_set(PlatformWindow owner, const char *text, size_t length)
{
    int units = 0, copied = 0;
    HGLOBAL block;
    WCHAR *wide;

    if (length > INT_MAX)
        return 0;
    if (length && !(units = MultiByteToWideChar(CP_UTF8, 0, text, (int)length, NULL, 0)))
        return 0;
    if (!OpenClipboard(owner))
        return 0;
    block = GlobalAlloc(GMEM_MOVEABLE, ((size_t)units + 1) * sizeof(WCHAR));
    if (block) {
        wide = GlobalLock(block);
        if (wide) {
            if (units)
                MultiByteToWideChar(CP_UTF8, 0, text, (int)length, wide, units);
            wide[units] = 0;
            GlobalUnlock(block);
            if (EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, block)) {
                block = NULL;
                copied = 1;
            }
        }
        if (block)
            GlobalFree(block);
    }
    CloseClipboard();
    return copied;
}

char *platform_clipboard_get(PlatformWindow owner)
{
    HANDLE data;
    const WCHAR *wide;
    char *text = NULL;
    size_t count;

    if (!OpenClipboard(owner))
        return NULL;
    data = GetClipboardData(CF_UNICODETEXT);
    if (data) {
        wide = (const WCHAR *)GlobalLock(data);
        if (wide) {
            count = wcsnlen(wide, GlobalSize(data) / sizeof(WCHAR));
            if (count <= (SIZE_MAX - 1) / 3 && (text = malloc(count * 3 + 1)) != NULL)
                utf8_from_utf16(text, count * 3 + 1, wide, count);
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return text;
}

void platform_cursor_client_pos(PlatformWindow window, POINT *out)
{
    GetCursorPos(out);
    ScreenToClient(window, out);
}

void platform_mouse_capture(PlatformWindow window, int capture)
{
    if (capture)
        SetCapture(window);
    else
        ReleaseCapture();
}

int platform_show_cursor(int show)
{
    return ShowCursor(show ? TRUE : FALSE);
}

void platform_client_screen_rect(PlatformWindow window, RECT *out)
{
    GetClientRect(window, out);
    ClientToScreen(window, (POINT *)&out->left);
    ClientToScreen(window, (POINT *)&out->right);
}

void platform_clip_cursor_to_client(PlatformWindow window, int clip)
{
    RECT client, area;
    POINT top_left, bottom_right;
    if (!clip) {
        ClipCursor(NULL);
        return;
    }
    GetClientRect(window, &client);
    top_left.x = client.left;
    top_left.y = client.top;
    bottom_right.x = client.right;
    bottom_right.y = client.bottom;
    ClientToScreen(window, &top_left);
    ClientToScreen(window, &bottom_right);
    area.left = top_left.x;
    area.top = top_left.y;
    area.right = bottom_right.x;
    area.bottom = bottom_right.y;
    ClipCursor(&area);
}

void platform_client_rect(PlatformWindow window, RECT *out)
{
    GetClientRect(window, out);
}

void platform_window_rect(PlatformWindow window, RECT *out)
{
    GetWindowRect(window, out);
}

void platform_monitor_rect(PlatformDisplay monitor, RECT *out)
{
    MONITORINFOEXA info;
    info.cbSize = sizeof(info);
    GetMonitorInfoA(monitor, (MONITORINFO *)&info);
    *out = info.rcMonitor;
}

void platform_screen_size(int *width, int *height)
{
    *width = GetSystemMetrics(SM_CXSCREEN);
    *height = GetSystemMetrics(SM_CYSCREEN);
}

void platform_focus_window(PlatformWindow window)
{
    SetFocus(window);
}

void platform_window_frameless(PlatformWindow window, const RECT *area)
{
    SetWindowLongA(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowPos(window, HWND_TOP, area->left, area->top, area->right - area->left, area->bottom - area->top,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void platform_window_fullscreen(PlatformWindow window)
{
    SetWindowLongA(window, GWL_STYLE, WS_POPUP);
    ShowWindow(window, SW_MAXIMIZE);
}

void platform_ime_cancel(PlatformWindow window)
{
    HIMC imc = ImmGetContext(window);
    if (imc) {
        ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        ImmReleaseContext(window, imc);
    }
}

WORD platform_keyboard_language(void)
{
    return LOWORD(GetKeyboardLayout(0));
}

void platform_ime_alphanumeric(PlatformWindow window)
{
    HIMC imc = ImmGetContext(window);
    DWORD conversion, sentence;
    ImmGetConversionStatus(imc, &conversion, &sentence);
    ImmSetConversionStatus(imc, 0, sentence);
    ImmReleaseContext(window, imc);
}


#define WINDOWED_STYLE   (WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN)
#define FULLSCREEN_STYLE WS_POPUP

void platform_monitor_rects(PlatformDisplay m, RECT *mon, RECT *work)
{
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (m && GetMonitorInfoA(m, &mi)) {
        *mon = mi.rcMonitor;
        *work = mi.rcWork;
    } else {
        *mon = (RECT){0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
        *work = *mon;
    }
}


PlatformDisplay platform_window_monitor(PlatformWindow window)
{
    return MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
}

int platform_window_has_caption(PlatformWindow window)
{
    return (GetWindowLongA(window, GWL_STYLE) & WS_CAPTION) == WS_CAPTION;
}

int platform_window_is_foreground(PlatformWindow window)
{
    return GetForegroundWindow() == window;
}

void platform_window_place_windowed(PlatformWindow hwnd, PlatformDisplay on, int x, int y, int clientW, int clientH, int centre)
{
    RECT rc, mon, work;
    int fw, fh;
    platform_monitor_rects(on, &mon, &work);
    SetWindowLongA(hwnd, GWL_STYLE, WINDOWED_STYLE | (GetWindowLongA(hwnd, GWL_STYLE) & WS_VISIBLE));
    rc = (RECT){0, 0, clientW, clientH};
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

void platform_window_place_frameless(PlatformWindow hwnd, PlatformDisplay on, int w, int h)
{
    RECT mon, work;
    platform_monitor_rects(on, &mon, &work);
    SetWindowLongA(hwnd, GWL_STYLE, FULLSCREEN_STYLE | WS_VISIBLE);
    SetWindowPos(hwnd, HWND_TOP, mon.left, mon.top, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}
#endif
