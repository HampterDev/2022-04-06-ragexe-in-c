#include "platform/window.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <imm.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "imm32.lib")

void platform_message_box(HWND owner, const char *text, const char *caption, unsigned flags)
{
    MessageBoxA(owner, text, caption, flags);
}

short platform_async_key_state(int vk)
{
    return GetAsyncKeyState(vk);
}

short platform_key_state(int vk)
{
    return GetKeyState(vk);
}

int platform_clipboard_set(HWND owner, const char *text, size_t length)
{
    HGLOBAL block;
    char *copy;
    if (!OpenClipboard(owner))
        return 0;
    block = GlobalAlloc(GMEM_MOVEABLE, length + 1);
    if (block) {
        copy = GlobalLock(block);
        if (copy) {
            memcpy(copy, text, length);
            copy[length] = 0;
            GlobalUnlock(block);
            if (EmptyClipboard() && SetClipboardData(CF_TEXT, block))
                block = NULL;
        }
        if (block)
            GlobalFree(block);
    }
    CloseClipboard();
    return 1;
}

char *platform_clipboard_get(HWND owner)
{
    HANDLE data;
    const char *text;
    char *copy = NULL;
    if (!OpenClipboard(owner))
        return NULL;
    data = GetClipboardData(CF_TEXT);
    if (data) {
        text = (const char *)GlobalLock(data);
        if (text) {
            size_t length = strlen(text);
            copy = malloc(length + 1);
            if (copy)
                memcpy(copy, text, length + 1);
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return copy;
}

void platform_cursor_client_pos(HWND window, POINT *out)
{
    GetCursorPos(out);
    ScreenToClient(window, out);
}

void platform_mouse_capture(HWND window, int capture)
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

void platform_client_screen_rect(HWND window, RECT *out)
{
    GetClientRect(window, out);
    ClientToScreen(window, (POINT *)&out->left);
    ClientToScreen(window, (POINT *)&out->right);
}

void platform_clip_cursor_to_client(HWND window, int clip)
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

void platform_client_rect(HWND window, RECT *out)
{
    GetClientRect(window, out);
}

void platform_window_rect(HWND window, RECT *out)
{
    GetWindowRect(window, out);
}

void platform_monitor_rect(HMONITOR monitor, RECT *out)
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

void platform_focus_window(HWND window)
{
    SetFocus(window);
}

void platform_window_frameless(HWND window, const RECT *area)
{
    SetWindowLongA(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowPos(window, HWND_TOP, area->left, area->top, area->right - area->left, area->bottom - area->top,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void platform_window_fullscreen(HWND window)
{
    SetWindowLongA(window, GWL_STYLE, WS_POPUP);
    ShowWindow(window, SW_MAXIMIZE);
}

void platform_ime_cancel(HWND window)
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

void platform_ime_alphanumeric(HWND window)
{
    HIMC imc = ImmGetContext(window);
    DWORD conversion, sentence;
    ImmGetConversionStatus(imc, &conversion, &sentence);
    ImmSetConversionStatus(imc, 0, sentence);
    ImmReleaseContext(window, imc);
}


#define WINDOWED_STYLE   (WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN)
#define FULLSCREEN_STYLE WS_POPUP

void platform_monitor_rects(HMONITOR m, RECT *mon, RECT *work)
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


HMONITOR platform_window_monitor(HWND window)
{
    return MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
}

int platform_window_has_caption(HWND window)
{
    return (GetWindowLongA(window, GWL_STYLE) & WS_CAPTION) == WS_CAPTION;
}

int platform_window_is_foreground(HWND window)
{
    return GetForegroundWindow() == window;
}

void platform_window_place_windowed(HWND hwnd, HMONITOR on, int x, int y, int clientW, int clientH, int centre)
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

void platform_window_place_frameless(HWND hwnd, HMONITOR on, int w, int h)
{
    RECT mon, work;
    platform_monitor_rects(on, &mon, &work);
    SetWindowLongA(hwnd, GWL_STYLE, FULLSCREEN_STYLE | WS_VISIBLE);
    SetWindowPos(hwnd, HWND_TOP, mon.left, mon.top, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

#else
#include "platform/vk.h"
#include "platform/system.h"
#include <SDL3/SDL.h>
#include <iconv.h>

static SDL_Window *g_window;
static unsigned char g_keys[256];
static unsigned char g_toggles[256];
static int g_cursorCount;
static int g_cursorX, g_cursorY;

void platform_set_main_window(void *window)
{
    g_window = window;
}

void platform_note_key(int vk, int down)
{
    if (vk <= 0 || vk > 255)
        return;
    if (down && !g_keys[vk])
        g_toggles[vk] ^= 1;
    g_keys[vk] = down != 0;
}

void platform_note_cursor(int x, int y)
{
    g_cursorX = x;
    g_cursorY = y;
}

static char *convert(const char *text, size_t length, const char *to, const char *from)
{
    iconv_t cd = iconv_open(to, from);
    size_t capacity = length * 4 + 4, left = capacity - 1, in = length;
    char *out, *dst, *src = (char *)text;
    if (cd == (iconv_t)-1)
        return NULL;
    out = malloc(capacity);
    if (!out) {
        iconv_close(cd);
        return NULL;
    }
    dst = out;
    while (in && iconv(cd, &src, &in, &dst, &left) == (size_t)-1) {
        if (!in || !left)
            break;
        src++;
        in--;
        *dst++ = '?';
        left--;
    }
    *dst = 0;
    iconv_close(cd);
    return out;
}

void platform_message_box(HWND owner, const char *text, const char *caption, unsigned flags)
{
    char *utf8Text = convert(text, strlen(text), "UTF-8", "CP949");
    char *utf8Caption = convert(caption, strlen(caption), "UTF-8", "CP949");
    (void)owner;
    (void)flags;
    SDL_Log("%s: %s", utf8Caption ? utf8Caption : caption, utf8Text ? utf8Text : text);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, utf8Caption ? utf8Caption : caption, utf8Text ? utf8Text : text,
                             g_window);
    free(utf8Text);
    free(utf8Caption);
}

short platform_async_key_state(int vk)
{
    return (vk > 0 && vk < 256 && g_keys[vk]) ? (short)0x8000 : 0;
}

short platform_key_state(int vk)
{
    if (vk <= 0 || vk > 255)
        return 0;
    return (short)((g_keys[vk] ? 0xff80 : 0) | g_toggles[vk]);
}

int platform_clipboard_set(HWND owner, const char *text, size_t length)
{
    char *utf8 = convert(text, length, "UTF-8", "CP949");
    int done;
    (void)owner;
    if (!utf8)
        return 0;
    done = SDL_SetClipboardText(utf8);
    free(utf8);
    return done;
}

char *platform_clipboard_get(HWND owner)
{
    char *utf8 = SDL_GetClipboardText(), *text;
    (void)owner;
    if (!utf8)
        return NULL;
    text = convert(utf8, strlen(utf8), "CP949", "UTF-8");
    SDL_free(utf8);
    return text;
}

void platform_cursor_client_pos(HWND window, POINT *out)
{
    (void)window;
    out->x = g_cursorX;
    out->y = g_cursorY;
}

void platform_mouse_capture(HWND window, int capture)
{
    (void)window;
    SDL_CaptureMouse(capture != 0);
}

int platform_show_cursor(int show)
{
    g_cursorCount += show ? 1 : -1;
    if (g_cursorCount >= 0)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
    return g_cursorCount;
}

void platform_clip_cursor_to_client(HWND window, int clip)
{
    (void)window;
    if (g_window)
        SDL_SetWindowMouseGrab(g_window, clip != 0);
}

void platform_client_rect(HWND window, RECT *out)
{
    int w = 0, h = 0;
    (void)window;
    if (g_window)
        SDL_GetWindowSize(g_window, &w, &h);
    out->left = 0;
    out->top = 0;
    out->right = w;
    out->bottom = h;
}

void platform_window_rect(HWND window, RECT *out)
{
    int x = 0, y = 0, w = 0, h = 0, top = 0, left = 0, bottom = 0, right = 0;
    (void)window;
    if (g_window) {
        SDL_GetWindowPosition(g_window, &x, &y);
        SDL_GetWindowSize(g_window, &w, &h);
        SDL_GetWindowBordersSize(g_window, &top, &left, &bottom, &right);
    }
    out->left = x - left;
    out->top = y - top;
    out->right = x + w + right;
    out->bottom = y + h + bottom;
}

void platform_monitor_rect(HMONITOR monitor, RECT *out)
{
    SDL_Rect bounds = {0, 0, 0, 0};
    (void)monitor;
    SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &bounds);
    out->left = bounds.x;
    out->top = bounds.y;
    out->right = bounds.x + bounds.w;
    out->bottom = bounds.y + bounds.h;
}

void platform_client_screen_rect(HWND window, RECT *out)
{
    int x = 0, y = 0;
    platform_client_rect(window, out);
    if (g_window)
        SDL_GetWindowPosition(g_window, &x, &y);
    out->left += x;
    out->right += x;
    out->top += y;
    out->bottom += y;
}

void platform_screen_size(int *width, int *height)
{
    const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    *width = mode ? mode->w : 1024;
    *height = mode ? mode->h : 768;
}

void platform_focus_window(HWND window)
{
    (void)window;
    if (g_window)
        SDL_RaiseWindow(g_window);
}

void platform_window_frameless(HWND window, const RECT *area)
{
    (void)window;
    if (!g_window)
        return;
    SDL_SetWindowBordered(g_window, false);
    SDL_SetWindowPosition(g_window, area->left, area->top);
    SDL_SetWindowSize(g_window, area->right - area->left, area->bottom - area->top);
    SDL_SyncWindow(g_window);
}

void platform_window_fullscreen(HWND window)
{
    (void)window;
    if (!g_window)
        return;
    SDL_SetWindowFullscreen(g_window, true);
    SDL_SyncWindow(g_window);
}

void platform_ime_cancel(HWND window)
{
    (void)window;
    if (g_window)
        SDL_ClearComposition(g_window);
}

void platform_ime_alphanumeric(HWND window)
{
    (void)window;
}

WORD platform_keyboard_language(void)
{
    return (WORD)platform_user_language();
}
void platform_monitor_rects(HMONITOR monitor, RECT *mon, RECT *work)
{
    SDL_Rect bounds = {0, 0, 1024, 768}, usable;
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    (void)monitor;
    SDL_GetDisplayBounds(display, &bounds);
    if (!SDL_GetDisplayUsableBounds(display, &usable))
        usable = bounds;
    *mon = (RECT){bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h};
    *work = (RECT){usable.x, usable.y, usable.x + usable.w, usable.y + usable.h};
}

HMONITOR platform_window_monitor(HWND window)
{
    (void)window;
    return NULL;
}

int platform_window_has_caption(HWND window)
{
    (void)window;
    return g_window && !(SDL_GetWindowFlags(g_window) & SDL_WINDOW_BORDERLESS);
}

int platform_window_is_foreground(HWND window)
{
    (void)window;
    return g_window && (SDL_GetWindowFlags(g_window) & SDL_WINDOW_INPUT_FOCUS);
}

void platform_window_place_windowed(HWND hwnd, HMONITOR on, int x, int y, int clientW, int clientH, int centre)
{
    RECT mon, work;
    (void)hwnd;
    if (!g_window)
        return;
    platform_monitor_rects(on, &mon, &work);
    SDL_SetWindowFullscreen(g_window, false);
    SDL_SetWindowBordered(g_window, true);
    SDL_SetWindowSize(g_window, clientW, clientH);
    if (centre) {
        x = work.left + (work.right - work.left - clientW) / 2;
        y = work.top + (work.bottom - work.top - clientH) / 2;
    }
    if (x + clientW > work.right)  x = work.right - clientW;
    if (y + clientH > work.bottom) y = work.bottom - clientH;
    if (x < work.left) x = work.left;
    if (y < work.top)  y = work.top;
    SDL_SetWindowPosition(g_window, x, y);
    SDL_SyncWindow(g_window);
}

void platform_window_place_frameless(HWND hwnd, HMONITOR on, int w, int h)
{
    RECT mon, work;
    (void)hwnd;
    if (!g_window)
        return;
    platform_monitor_rects(on, &mon, &work);
    SDL_SetWindowFullscreen(g_window, false);
    SDL_SetWindowBordered(g_window, false);
    SDL_SetWindowPosition(g_window, mon.left, mon.top);
    SDL_SetWindowSize(g_window, w, h);
    SDL_SyncWindow(g_window);
}
#endif
