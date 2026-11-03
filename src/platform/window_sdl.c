#ifndef _WIN32
#include "platform/window.h"
#include <stdlib.h>
#include <string.h>
#include "platform/code_page.h"
#include "platform/vk.h"
#include "platform/system.h"
#include <SDL3/SDL.h>

static unsigned char g_keys[256];
static unsigned char g_toggles[256];
static int g_cursorCount;
static int g_cursorX, g_cursorY;

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

/* Text between the client's code page (CODE_PAGE_SYSTEM, the language's, as on Windows the ANSI
   code page is) and SDL's UTF-8. A text that does not convert keeps its ASCII and shows '?' for the
   rest. */
static char *recode(const char *text, size_t length, unsigned int to, unsigned int from)
{
    uint16_t *wide;
    char *out;
    int units = 0, bytes = 0;
    size_t i;
    if (length > (size_t)INT32_MAX / 4)
        return NULL;
    wide = malloc((length + 1) * sizeof *wide);
    out = malloc(length * 3 + 1);
    if (!wide || !out) {
        free(wide);
        free(out);
        return NULL;
    }
    if (length)
        units = platform_code_page_to_utf16(from, text, (int)length, wide, (int)length + 1);
    if (units > 0)
        bytes = platform_code_page_from_utf16(to, wide, units, out, (int)(length * 3));
    if (length && bytes <= 0) {
        for (i = 0; i < length; i++)
            out[i] = (unsigned char)text[i] < 0x80 ? text[i] : '?';
        bytes = (int)length;
    }
    out[bytes] = 0;
    free(wide);
    return out;
}

static SDL_MessageBoxFlags message_box_kind(unsigned flags)
{
    switch (flags & 0xf0) {
    case MB_ICONERROR: return SDL_MESSAGEBOX_ERROR;
    case MB_ICONWARNING: return SDL_MESSAGEBOX_WARNING;
    default: return SDL_MESSAGEBOX_INFORMATION;
    }
}

void platform_message_box(PlatformWindow owner, const char *text, const char *caption, unsigned flags)
{
    char *utf8Text = recode(text, strlen(text), CODE_PAGE_UTF8, CODE_PAGE_SYSTEM);
    char *utf8Caption = recode(caption, strlen(caption), CODE_PAGE_UTF8, CODE_PAGE_SYSTEM);
    const char *shownText = utf8Text ? utf8Text : text, *shownCaption = utf8Caption ? utf8Caption : caption;
    SDL_Log("%s: %s", shownCaption, shownText);
    SDL_ShowSimpleMessageBox(message_box_kind(flags), shownCaption, shownText, owner);
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

int platform_clipboard_set(PlatformWindow owner, const char *text, size_t length)
{
    char *utf8 = recode(text, length, CODE_PAGE_UTF8, CODE_PAGE_SYSTEM);
    int done;
    (void)owner;
    if (!utf8)
        return 0;
    done = SDL_SetClipboardText(utf8);
    free(utf8);
    return done;
}

char *platform_clipboard_get(PlatformWindow owner)
{
    char *utf8 = SDL_GetClipboardText(), *text;
    (void)owner;
    if (!utf8)
        return NULL;
    text = recode(utf8, strlen(utf8), CODE_PAGE_SYSTEM, CODE_PAGE_UTF8);
    SDL_free(utf8);
    return text;
}

void platform_cursor_client_pos(PlatformWindow window, POINT *out)
{
    (void)window;
    out->x = g_cursorX;
    out->y = g_cursorY;
}

void platform_mouse_capture(PlatformWindow window, int capture)
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

void platform_clip_cursor_to_client(PlatformWindow window, int clip)
{
    if (window)
        SDL_SetWindowMouseGrab(window, clip != 0);
}

void platform_client_rect(PlatformWindow window, RECT *out)
{
    int w = 0, h = 0;
    if (window)
        SDL_GetWindowSize(window, &w, &h);
    *out = (RECT){0, 0, w, h};
}

void platform_window_rect(PlatformWindow window, RECT *out)
{
    int x = 0, y = 0, w = 0, h = 0, top = 0, left = 0, bottom = 0, right = 0;
    if (window) {
        SDL_GetWindowPosition(window, &x, &y);
        SDL_GetWindowSize(window, &w, &h);
        SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right);
    }
    *out = (RECT){x - left, y - top, x + w + right, y + h + bottom};
}

static SDL_DisplayID display_or_primary(PlatformDisplay display)
{
    return display ? display : SDL_GetPrimaryDisplay();
}

void platform_monitor_rect(PlatformDisplay display, RECT *out)
{
    SDL_Rect bounds = {0, 0, 0, 0};
    SDL_GetDisplayBounds(display_or_primary(display), &bounds);
    *out = (RECT){bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h};
}

void platform_monitor_rects(PlatformDisplay display, RECT *mon, RECT *work)
{
    SDL_Rect bounds = {0, 0, 1024, 768}, usable;
    SDL_DisplayID id = display_or_primary(display);
    SDL_GetDisplayBounds(id, &bounds);
    if (!SDL_GetDisplayUsableBounds(id, &usable))
        usable = bounds;
    *mon = (RECT){bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h};
    *work = (RECT){usable.x, usable.y, usable.x + usable.w, usable.y + usable.h};
}

PlatformDisplay platform_window_monitor(PlatformWindow window)
{
    return window ? SDL_GetDisplayForWindow(window) : SDL_GetPrimaryDisplay();
}

void platform_client_screen_rect(PlatformWindow window, RECT *out)
{
    int x = 0, y = 0;
    platform_client_rect(window, out);
    if (window)
        SDL_GetWindowPosition(window, &x, &y);
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

void platform_focus_window(PlatformWindow window)
{
    if (window)
        SDL_RaiseWindow(window);
}

void platform_window_frameless(PlatformWindow window, const RECT *area)
{
    if (!window)
        return;
    SDL_SetWindowBordered(window, false);
    SDL_SetWindowPosition(window, area->left, area->top);
    SDL_SetWindowSize(window, area->right - area->left, area->bottom - area->top);
    SDL_SyncWindow(window);
}

void platform_window_fullscreen(PlatformWindow window)
{
    if (!window)
        return;
    SDL_SetWindowFullscreen(window, true);
    SDL_SyncWindow(window);
}

void platform_ime_cancel(PlatformWindow window)
{
    if (window)
        SDL_ClearComposition(window);
}

void platform_ime_alphanumeric(PlatformWindow window)
{
    (void)window;
}

WORD platform_keyboard_language(void)
{
    return (WORD)platform_user_language();
}

int platform_window_has_caption(PlatformWindow window)
{
    return window && !(SDL_GetWindowFlags(window) & SDL_WINDOW_BORDERLESS);
}

int platform_window_is_foreground(PlatformWindow window)
{
    return window && (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS);
}

void platform_window_place_windowed(PlatformWindow window, PlatformDisplay on, int x, int y, int clientW,
                                    int clientH, int centre)
{
    RECT mon, work;
    if (!window)
        return;
    platform_monitor_rects(on, &mon, &work);
    SDL_SetWindowFullscreen(window, false);
    SDL_SetWindowBordered(window, true);
    SDL_SetWindowSize(window, clientW, clientH);
    if (centre) {
        x = work.left + (work.right - work.left - clientW) / 2;
        y = work.top + (work.bottom - work.top - clientH) / 2;
    }
    if (x + clientW > work.right)  x = work.right - clientW;
    if (y + clientH > work.bottom) y = work.bottom - clientH;
    if (x < work.left) x = work.left;
    if (y < work.top)  y = work.top;
    SDL_SetWindowPosition(window, x, y);
    SDL_SyncWindow(window);
}

void platform_window_place_frameless(PlatformWindow window, PlatformDisplay on, int w, int h)
{
    RECT mon, work;
    if (!window)
        return;
    platform_monitor_rects(on, &mon, &work);
    SDL_SetWindowFullscreen(window, false);
    SDL_SetWindowBordered(window, false);
    SDL_SetWindowPosition(window, mon.left, mon.top);
    SDL_SetWindowSize(window, w, h);
    SDL_SyncWindow(window);
}
#endif
