#include "core/app.h"
#include "core/window_events.h"
#include "core/drive_state.h"
#include "core/user_input.h"
#include "core/touch.h"
#include "contents/frame_timer.h"
#include "contents/mouse.h"
#include "platform/code_page.h"
#include "platform/filesystem.h"
#include "platform/language.h"
#include "platform/vk.h"
#include "platform/window.h"
#include "render/device.h"
#include "render/display_options.h"
#include "render/present_fit.h"
#include "render/render_api.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "ui/window.h"
#include "ui/window_types.h"
#include "core/window_state.h"
#include <SDL3/SDL.h>
#ifdef _WIN32
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#endif
#include <stdlib.h>
#include <string.h>

/* The client's window on every platform: SDL's window, its events turned into the client's input
   (core/user_input.h), text input and the IME. */

static struct PresentCover g_cover;
static int g_composing, g_compositionEnded;
/* AltGr is held (on_key). */
static int g_altGr;
#ifdef _WIN32
/* Whether the last right Alt that went down came with the left Ctrl Windows sends for AltGr
   (window_message), and the event that carries a key the IME took (VK_PROCESSKEY). */
static int g_rightAltIsAltGr;
static Uint32 g_processKeyEvent;
#endif

static const unsigned short g_scanCodes[SDL_SCANCODE_RGUI + 1] = {
    [SDL_SCANCODE_A] = 0x1e, [SDL_SCANCODE_B] = 0x30, [SDL_SCANCODE_C] = 0x2e, [SDL_SCANCODE_D] = 0x20,
    [SDL_SCANCODE_E] = 0x12, [SDL_SCANCODE_F] = 0x21, [SDL_SCANCODE_G] = 0x22, [SDL_SCANCODE_H] = 0x23,
    [SDL_SCANCODE_I] = 0x17, [SDL_SCANCODE_J] = 0x24, [SDL_SCANCODE_K] = 0x25, [SDL_SCANCODE_L] = 0x26,
    [SDL_SCANCODE_M] = 0x32, [SDL_SCANCODE_N] = 0x31, [SDL_SCANCODE_O] = 0x18, [SDL_SCANCODE_P] = 0x19,
    [SDL_SCANCODE_Q] = 0x10, [SDL_SCANCODE_R] = 0x13, [SDL_SCANCODE_S] = 0x1f, [SDL_SCANCODE_T] = 0x14,
    [SDL_SCANCODE_U] = 0x16, [SDL_SCANCODE_V] = 0x2f, [SDL_SCANCODE_W] = 0x11, [SDL_SCANCODE_X] = 0x2d,
    [SDL_SCANCODE_Y] = 0x15, [SDL_SCANCODE_Z] = 0x2c,
    [SDL_SCANCODE_1] = 0x02, [SDL_SCANCODE_2] = 0x03, [SDL_SCANCODE_3] = 0x04, [SDL_SCANCODE_4] = 0x05,
    [SDL_SCANCODE_5] = 0x06, [SDL_SCANCODE_6] = 0x07, [SDL_SCANCODE_7] = 0x08, [SDL_SCANCODE_8] = 0x09,
    [SDL_SCANCODE_9] = 0x0a, [SDL_SCANCODE_0] = 0x0b,
    [SDL_SCANCODE_RETURN] = 0x1c, [SDL_SCANCODE_ESCAPE] = 0x01, [SDL_SCANCODE_BACKSPACE] = 0x0e,
    [SDL_SCANCODE_TAB] = 0x0f, [SDL_SCANCODE_SPACE] = 0x39, [SDL_SCANCODE_MINUS] = 0x0c,
    [SDL_SCANCODE_EQUALS] = 0x0d, [SDL_SCANCODE_LEFTBRACKET] = 0x1a, [SDL_SCANCODE_RIGHTBRACKET] = 0x1b,
    [SDL_SCANCODE_BACKSLASH] = 0x2b, [SDL_SCANCODE_SEMICOLON] = 0x27, [SDL_SCANCODE_APOSTROPHE] = 0x28,
    [SDL_SCANCODE_GRAVE] = 0x29, [SDL_SCANCODE_COMMA] = 0x33, [SDL_SCANCODE_PERIOD] = 0x34,
    [SDL_SCANCODE_SLASH] = 0x35, [SDL_SCANCODE_CAPSLOCK] = 0x3a,
    [SDL_SCANCODE_F1] = 0x3b, [SDL_SCANCODE_F2] = 0x3c, [SDL_SCANCODE_F3] = 0x3d, [SDL_SCANCODE_F4] = 0x3e,
    [SDL_SCANCODE_F5] = 0x3f, [SDL_SCANCODE_F6] = 0x40, [SDL_SCANCODE_F7] = 0x41, [SDL_SCANCODE_F8] = 0x42,
    [SDL_SCANCODE_F9] = 0x43, [SDL_SCANCODE_F10] = 0x44, [SDL_SCANCODE_F11] = 0x57, [SDL_SCANCODE_F12] = 0x58,
    [SDL_SCANCODE_PRINTSCREEN] = 0x137, [SDL_SCANCODE_SCROLLLOCK] = 0x46, [SDL_SCANCODE_PAUSE] = 0x45,
    [SDL_SCANCODE_INSERT] = 0x152, [SDL_SCANCODE_HOME] = 0x147, [SDL_SCANCODE_PAGEUP] = 0x149,
    [SDL_SCANCODE_DELETE] = 0x153, [SDL_SCANCODE_END] = 0x14f, [SDL_SCANCODE_PAGEDOWN] = 0x151,
    [SDL_SCANCODE_RIGHT] = 0x14d, [SDL_SCANCODE_LEFT] = 0x14b, [SDL_SCANCODE_DOWN] = 0x150, [SDL_SCANCODE_UP] = 0x148,
    [SDL_SCANCODE_NUMLOCKCLEAR] = 0x145, [SDL_SCANCODE_KP_DIVIDE] = 0x135, [SDL_SCANCODE_KP_MULTIPLY] = 0x37,
    [SDL_SCANCODE_KP_MINUS] = 0x4a, [SDL_SCANCODE_KP_PLUS] = 0x4e, [SDL_SCANCODE_KP_ENTER] = 0x11c,
    [SDL_SCANCODE_KP_1] = 0x4f, [SDL_SCANCODE_KP_2] = 0x50, [SDL_SCANCODE_KP_3] = 0x51, [SDL_SCANCODE_KP_4] = 0x4b,
    [SDL_SCANCODE_KP_5] = 0x4c, [SDL_SCANCODE_KP_6] = 0x4d, [SDL_SCANCODE_KP_7] = 0x47, [SDL_SCANCODE_KP_8] = 0x48,
    [SDL_SCANCODE_KP_9] = 0x49, [SDL_SCANCODE_KP_0] = 0x52, [SDL_SCANCODE_KP_PERIOD] = 0x53,
    [SDL_SCANCODE_LCTRL] = 0x1d, [SDL_SCANCODE_LSHIFT] = 0x2a, [SDL_SCANCODE_LALT] = 0x38, [SDL_SCANCODE_LGUI] = 0x15b,
    [SDL_SCANCODE_RCTRL] = 0x11d, [SDL_SCANCODE_RSHIFT] = 0x36, [SDL_SCANCODE_RALT] = 0x138, [SDL_SCANCODE_RGUI] = 0x15c,
};

static const unsigned char g_scanKeys[SDL_SCANCODE_RGUI + 1] = {
    [SDL_SCANCODE_RETURN] = VK_RETURN, [SDL_SCANCODE_ESCAPE] = VK_ESCAPE, [SDL_SCANCODE_BACKSPACE] = VK_BACK,
    [SDL_SCANCODE_TAB] = VK_TAB, [SDL_SCANCODE_SPACE] = VK_SPACE, [SDL_SCANCODE_MINUS] = VK_OEM_MINUS,
    [SDL_SCANCODE_EQUALS] = VK_OEM_PLUS, [SDL_SCANCODE_LEFTBRACKET] = VK_OEM_4, [SDL_SCANCODE_RIGHTBRACKET] = VK_OEM_6,
    [SDL_SCANCODE_BACKSLASH] = VK_OEM_5, [SDL_SCANCODE_SEMICOLON] = VK_OEM_1, [SDL_SCANCODE_APOSTROPHE] = VK_OEM_7,
    [SDL_SCANCODE_GRAVE] = VK_OEM_3, [SDL_SCANCODE_COMMA] = VK_OEM_COMMA, [SDL_SCANCODE_PERIOD] = VK_OEM_PERIOD,
    [SDL_SCANCODE_SLASH] = VK_OEM_2, [SDL_SCANCODE_CAPSLOCK] = VK_CAPITAL,
    [SDL_SCANCODE_F1] = VK_F1, [SDL_SCANCODE_F2] = VK_F2, [SDL_SCANCODE_F3] = VK_F3, [SDL_SCANCODE_F4] = VK_F4,
    [SDL_SCANCODE_F5] = VK_F5, [SDL_SCANCODE_F6] = VK_F6, [SDL_SCANCODE_F7] = VK_F7, [SDL_SCANCODE_F8] = VK_F8,
    [SDL_SCANCODE_F9] = VK_F9, [SDL_SCANCODE_F10] = VK_F10, [SDL_SCANCODE_F11] = VK_F11, [SDL_SCANCODE_F12] = VK_F12,
    [SDL_SCANCODE_PRINTSCREEN] = VK_SNAPSHOT, [SDL_SCANCODE_SCROLLLOCK] = VK_SCROLL, [SDL_SCANCODE_PAUSE] = VK_PAUSE,
    [SDL_SCANCODE_INSERT] = VK_INSERT, [SDL_SCANCODE_HOME] = VK_HOME, [SDL_SCANCODE_PAGEUP] = VK_PRIOR,
    [SDL_SCANCODE_DELETE] = VK_DELETE, [SDL_SCANCODE_END] = VK_END, [SDL_SCANCODE_PAGEDOWN] = VK_NEXT,
    [SDL_SCANCODE_RIGHT] = VK_RIGHT, [SDL_SCANCODE_LEFT] = VK_LEFT, [SDL_SCANCODE_DOWN] = VK_DOWN, [SDL_SCANCODE_UP] = VK_UP,
    [SDL_SCANCODE_NUMLOCKCLEAR] = VK_NUMLOCK, [SDL_SCANCODE_KP_DIVIDE] = VK_DIVIDE, [SDL_SCANCODE_KP_MULTIPLY] = VK_MULTIPLY,
    [SDL_SCANCODE_KP_MINUS] = VK_SUBTRACT, [SDL_SCANCODE_KP_PLUS] = VK_ADD, [SDL_SCANCODE_KP_ENTER] = VK_RETURN,
    [SDL_SCANCODE_KP_1] = VK_NUMPAD1, [SDL_SCANCODE_KP_2] = VK_NUMPAD2, [SDL_SCANCODE_KP_3] = VK_NUMPAD3,
    [SDL_SCANCODE_KP_4] = VK_NUMPAD4, [SDL_SCANCODE_KP_5] = VK_NUMPAD5, [SDL_SCANCODE_KP_6] = VK_NUMPAD6,
    [SDL_SCANCODE_KP_7] = VK_NUMPAD7, [SDL_SCANCODE_KP_8] = VK_NUMPAD8, [SDL_SCANCODE_KP_9] = VK_NUMPAD9,
    [SDL_SCANCODE_KP_0] = VK_NUMPAD0, [SDL_SCANCODE_KP_PERIOD] = VK_DECIMAL,
    [SDL_SCANCODE_LCTRL] = VK_CONTROL, [SDL_SCANCODE_LSHIFT] = VK_SHIFT, [SDL_SCANCODE_LALT] = VK_MENU,
    [SDL_SCANCODE_LGUI] = VK_LWIN, [SDL_SCANCODE_RCTRL] = VK_CONTROL, [SDL_SCANCODE_RSHIFT] = VK_SHIFT,
    [SDL_SCANCODE_RALT] = VK_MENU, [SDL_SCANCODE_RGUI] = VK_RWIN,
};

/* The keypad with NumLock off, as Windows names its keys then: the arrows, Home, End, Page Up and
   Down, Insert, Delete and Clear. */
static int keypad_navigation(SDL_Scancode scancode)
{
    switch (scancode) {
    case SDL_SCANCODE_KP_1: return VK_END;
    case SDL_SCANCODE_KP_2: return VK_DOWN;
    case SDL_SCANCODE_KP_3: return VK_NEXT;
    case SDL_SCANCODE_KP_4: return VK_LEFT;
    case SDL_SCANCODE_KP_5: return VK_CLEAR;
    case SDL_SCANCODE_KP_6: return VK_RIGHT;
    case SDL_SCANCODE_KP_7: return VK_HOME;
    case SDL_SCANCODE_KP_8: return VK_UP;
    case SDL_SCANCODE_KP_9: return VK_PRIOR;
    case SDL_SCANCODE_KP_0: return VK_INSERT;
    case SDL_SCANCODE_KP_PERIOD: return VK_DELETE;
    default: return 0;
    }
}

static int key_code(const SDL_KeyboardEvent *key)
{
    int navigation;
    if (key->key >= 'a' && key->key <= 'z')
        return (int)(key->key - 'a' + 'A');
    if (key->key >= '0' && key->key <= '9')
        return (int)key->key;
    if (key->scancode >= SDL_SCANCODE_A && key->scancode <= SDL_SCANCODE_Z)
        return 'A' + (key->scancode - SDL_SCANCODE_A);
    if (key->scancode >= SDL_SCANCODE_1 && key->scancode <= SDL_SCANCODE_9)
        return '1' + (key->scancode - SDL_SCANCODE_1);
    if (key->scancode == SDL_SCANCODE_0)
        return '0';
#ifdef __ANDROID__
    if (key->scancode == SDL_SCANCODE_AC_BACK)
        return VK_ESCAPE;
#endif
    if (!(key->mod & SDL_KMOD_NUM) && (navigation = keypad_navigation(key->scancode)) != 0)
        return navigation;
    return (unsigned)key->scancode <= SDL_SCANCODE_RGUI ? g_scanKeys[key->scancode] : 0;
}

/* SDL's text is UTF-8, as the client's is (core/utf8.h). */
static char *client_text(const char *utf8)
{
    size_t length = strlen(utf8);
    char *text = malloc(length + 1);
    if (text)
        memcpy(text, utf8, length + 1);
    return text;
}

/* SDL's pointer is in window coordinates, the game's in the device's pixels (present_point). */
static void scene_point(float x, float y, int *outX, int *outY)
{
    int w = 0, h = 0;
    SDL_GetWindowSize(g_hMainWnd, &w, &h);
    present_point(w, h, g_3dDevice.width, g_3dDevice.height, &g_cover, x, y, outX, outY);
}

/* AltGr types characters as Ctrl+Alt does, and Windows reports it as a left Ctrl and a right Alt.
   SDL reports it as SDLK_MODE on X11 and Wayland, and on Windows as the right Alt alone, leaving
   out the left Ctrl (window_message tells which right Alt is AltGr). */
static int is_altgr(const SDL_KeyboardEvent *key)
{
    if (key->key == SDLK_MODE)
        return 1;
#ifdef _WIN32
    return key->scancode == SDL_SCANCODE_RALT && g_rightAltIsAltGr;
#else
    return 0;
#endif
}

/* AltGr as the Win32 client saw it: Ctrl and Alt held, so that edit boxes take the keys typed with
   it as characters (ui/wndmgr/key_down.c), and those keys without Alt. */
static void on_altgr(const struct InputKey *alt)
{
    struct InputKey ctrl = *alt;
    ctrl.vk = VK_CONTROL;
    ctrl.scancode = 0x1d;
    if (alt->down) {
        input_key(&ctrl);
        input_key(alt);
        return;
    }
    input_key(alt);
    /* Windows sends the left Ctrl's release first, which SDL passes on */
    if (platform_key_state(VK_CONTROL) & 0x8000)
        input_key(&ctrl);
    g_altGr = 0;
}

/* SDL's keys as Windows delivers them: Print Screen only as it goes up, and Backspace also as a
   character. */
static void on_key(const SDL_KeyboardEvent *key, int down)
{
    struct InputKey in;
    int altgr_key = key->scancode == SDL_SCANCODE_RALT || key->key == SDLK_MODE;
    if (down && !key->repeat && altgr_key && is_altgr(key))
        g_altGr = 1;
    in.vk = key_code(key);
    in.scancode = (unsigned)key->scancode <= SDL_SCANCODE_RGUI ? g_scanCodes[key->scancode] : 0;
    in.down = down;
    in.repeat = down && key->repeat;
    in.alt = (key->mod & (g_altGr ? SDL_KMOD_LALT : SDL_KMOD_ALT)) != 0;
    in.count = 1;
    if (!in.vk)
        return;
    if (g_altGr && altgr_key) {
        on_altgr(&in);
        return;
    }
    if (down && in.vk == VK_SNAPSHOT) {
        platform_note_key(in.vk, 1);
        return;
    }
    if (down)
        g_compositionEnded = 0;
    input_key(&in);
    if (down && in.vk == VK_BACK && !g_composing)
        input_char(VK_BACK);
}

#ifdef _WIN32
/* A key the IME took, which Windows reports as VK_PROCESSKEY with the key's scan code, and SDL leaves
   out: the client maps it back to its letter by the scan code for the hotkeys
   (ui/wndmgr/keyboard.c), as it did with the Win32 window. */
static void on_process_key(Uint32 lparam)
{
    struct InputKey in;
    in.vk = VK_PROCESSKEY;
    in.scancode = (int)(lparam >> 16 & 0xff) | (lparam & 1u << 24 ? 0x100 : 0);
    in.down = 1;
    in.repeat = (lparam & 1u << 30) != 0;
    in.alt = (lparam & 1u << 29) != 0;
    in.count = (int)(lparam & 0xffff);
    input_key(&in);
}
#endif

static void on_text(const char *utf8)
{
    char *text;
    int composed = g_composing || g_compositionEnded;
    g_compositionEnded = 0;
    if (!g_language || (!g_windowMgr.key_as_text && !composed))
        return;
    text = client_text(utf8);
    if (!text)
        return;
    if (!composed && text[0] && !text[1])
        input_char((unsigned char)text[0]);
    else
        language_commit(g_language, text);
    free(text);
    g_composing = 0;
}

static void on_editing(const char *utf8, int start)
{
    char *text;
    if (!g_language)
        return;
    text = client_text(utf8);
    if (!text)
        return;
    if (text[0] && !g_composing)
        language_delete_selection(g_language);
    g_compositionEnded = g_composing && !text[0];
    g_composing = text[0] != 0;
    language_set_composition(g_language, text, start);
    free(text);
}

static int mouse_button(Uint8 button)
{
    switch (button) {
    case SDL_BUTTON_LEFT: return INPUT_BUTTON_LEFT;
    case SDL_BUTTON_RIGHT: return INPUT_BUTTON_RIGHT;
    case SDL_BUTTON_MIDDLE: return INPUT_BUTTON_MIDDLE;
    default: return -1;
    }
}

/* Touch screens go to core/touch.c; trackpads, which also report touches, stay the mouse they
   already are. */
static void on_finger(const SDL_TouchFingerEvent *finger)
{
    int w = 0, h = 0, x, y;
    uint32_t ms = (uint32_t)SDL_NS_TO_MS(finger->timestamp);
    if (finger->touchID == SDL_MOUSE_TOUCHID || SDL_GetTouchDeviceType(finger->touchID) != SDL_TOUCH_DEVICE_DIRECT)
        return;
    SDL_GetWindowSize(g_hMainWnd, &w, &h);
    scene_point(finger->x * (float)w, finger->y * (float)h, &x, &y);
    if (finger->type == SDL_EVENT_FINGER_DOWN)
        touch_down(finger->fingerID, x, y, ms);
    else if (finger->type == SDL_EVENT_FINGER_MOTION)
        touch_move(finger->fingerID, x, y, ms);
    else if (finger->type == SDL_EVENT_FINGER_UP)
        touch_up(finger->fingerID, x, y, ms);
    else
        touch_cancel();
}

static void dispatch(const SDL_Event *event)
{
    int x, y;
#ifdef _WIN32
    if (g_processKeyEvent && event->type == g_processKeyEvent) {
        on_process_key((Uint32)event->user.code);
        return;
    }
#endif
    switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        /* SDL follows the last window's close request with a quit; the client closes once */
        if (!g_quitRequested)
            window_events_close();
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        window_events_activate(1);
        app_activate(1);
        frame_timer_reset();
        if (g_windowActive == 1)
            g_activatedNow = 1;
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        touch_cancel();
        window_events_activate(0);
        app_activate(0);
        break;
#ifdef _WIN32
    case SDL_EVENT_WINDOW_MOVED:
        /* A window that is not active still draws, but shows its last frame again at once when it
           is moved, as the Win32 window did. */
        if (g_windowActive != 1)
            c3d_device_show_frame(&g_3dDevice, 1);
        break;
#endif
#ifndef __ANDROID__
    case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED:
        /* Fullscreen keeps covering its display when the display's own resolution changes, which
           SDL leaves to the client. */
        if (g_fullscreenMode && event->display.displayID == SDL_GetDisplayForWindow(g_hMainWnd))
            platform_window_fullscreen(g_hMainWnd);
        break;
#endif
    case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        /* the smallest size in pixels, at the pixel density of the display the window is now on */
        platform_window_minimum(g_hMainWnd, DISPLAY_MIN_WIDTH, DISPLAY_MIN_HEIGHT);
        break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        /* The pointer went onto the window's frame or off the window, where the system's cursor
           shows: the client's own cursor, drawn in the scene, leaves the scene. */
        if (!g_session1.mouseExclusiveOption) {
            mouse_set_x(&g_mouse, -100);
            mouse_set_y(&g_mouse, -100);
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        on_key(&event->key, 1);
        break;
    case SDL_EVENT_KEY_UP:
        on_key(&event->key, 0);
        break;
    case SDL_EVENT_TEXT_INPUT:
        on_text(event->text.text);
        break;
    case SDL_EVENT_TEXT_EDITING:
        on_editing(event->edit.text, event->edit.start);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        scene_point(event->motion.x, event->motion.y, &x, &y);
        input_pointer(x, y);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        scene_point(event->button.x, event->button.y, &x, &y);
        input_button(mouse_button(event->button.button), event->type == SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        input_wheel((int)(event->wheel.y * 120.0f));
        break;
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        on_finger(&event->tfinger);
        break;
    default:
        break;
    }
}

#ifdef _WIN32
/* Windows moves a window dragged by its caption in a loop of its own, and the game would stand
   still until the button went up. As the Win32 client did, the client moves the window itself:
   SDL's message hook takes the press on the caption, and every pump moves the window as far as the
   pointer has gone, until the left button is up. */
static struct {
    int on, left, top;
    float x, y;
} g_drag;

/* SDL's hook sees the messages its pump takes from the queue, and those of a modal loop (the
   window's system menu): the game counts as inactive while such a loop runs, as it did with the
   Win32 window. */
static bool SDLCALL window_message(void *userdata, MSG *msg)
{
    HWND hwnd = g_hMainWnd ? SDL_GetPointerProperty(SDL_GetWindowProperties(g_hMainWnd),
                                                    SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL) : NULL;
    (void)userdata;
    if (!hwnd || msg->hwnd != hwnd)
        return true;
    switch (msg->message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (msg->wParam == VK_MENU && (msg->lParam & 1 << 24) && !(msg->lParam & 1 << 30))
            g_rightAltIsAltGr = (GetKeyState(VK_LCONTROL) & 0x8000) != 0;
        if (msg->wParam == VK_PROCESSKEY && g_processKeyEvent) {
            /* SDL leaves out the keys the IME takes; the client still gets them, in order */
            SDL_Event event;
            SDL_zero(event);
            event.type = g_processKeyEvent;
            event.user.windowID = SDL_GetWindowID(g_hMainWnd);
            event.user.code = (Sint32)(Uint32)msg->lParam;
            SDL_PushEvent(&event);
        }
        return true;
    case WM_NCLBUTTONDOWN:
        /* a maximised window is Windows' to drag: dragging restores it */
        if (msg->wParam != HTCAPTION || (SDL_GetWindowFlags(g_hMainWnd) & SDL_WINDOW_MAXIMIZED))
            return true;
        SDL_GetGlobalMouseState(&g_drag.x, &g_drag.y);
        SDL_GetWindowPosition(g_hMainWnd, &g_drag.left, &g_drag.top);
        g_drag.on = 1;
        return false;
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE:
        app_activate(0);
        return true;
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE:
        app_activate(GetActiveWindow() != hwnd && IsIconic(hwnd) ? 0 : 1);
        return true;
    default:
        return true;
    }
}

static void follow_drag(void)
{
    float x, y;
    SDL_MouseButtonFlags buttons;
    int dx, dy;
    if (!g_drag.on)
        return;
    buttons = SDL_GetGlobalMouseState(&x, &y);
    dx = (int)(x - g_drag.x);
    dy = (int)(y - g_drag.y);
    if (dx || dy)
        SDL_SetWindowPosition(g_hMainWnd, g_drag.left + dx, g_drag.top + dy);
    if (!(buttons & SDL_BUTTON_LMASK))
        g_drag.on = 0;
}
#endif

static void pump_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        dispatch(&event);
#ifdef _WIN32
    follow_drag();
#endif
    touch_frame((uint32_t)SDL_GetTicks());
}

#ifdef __ANDROID__
#include <jni.h>

#define KEYBOARD_MARGIN 8

enum { COVER_LEFT, COVER_TOP, COVER_RIGHT, COVER_BOTTOM, COVER_KEYBOARD, COVER_COUNT };

/* Window pixels along each edge that a desktop's caption bar covers, and at the bottom the on-screen
   keyboard, from the activity's window insets (android/app/src/main/java/local/ragexe/client/
   ClientActivity.java). */
static SDL_AtomicInt g_windowCover[COVER_COUNT];

JNIEXPORT void JNICALL Java_local_ragexe_client_ClientActivity_onWindowCover(JNIEnv *env, jclass cls, jint left,
                                                                           jint top, jint right, jint bottom,
                                                                           jint keyboard)
{
    (void)env;
    (void)cls;
    SDL_SetAtomicInt(&g_windowCover[COVER_LEFT], left);
    SDL_SetAtomicInt(&g_windowCover[COVER_TOP], top);
    SDL_SetAtomicInt(&g_windowCover[COVER_RIGHT], right);
    SDL_SetAtomicInt(&g_windowCover[COVER_BOTTOM], bottom);
    SDL_SetAtomicInt(&g_windowCover[COVER_KEYBOARD], keyboard);
}

static int cover_points(int which, int points, int pixels)
{
    int cover = SDL_GetAtomicInt(&g_windowCover[which]);
    return pixels > 0 ? (int)((long long)cover * points / pixels) : cover;
}

/* The scene keeps out from under the caption bar a desktop draws over a maximised window. */
static void sync_window_cover(void)
{
    int w = 0, h = 0, pixels_w = 0, pixels_h = 0;
    SDL_GetWindowSize(g_hMainWnd, &w, &h);
    SDL_GetWindowSizeInPixels(g_hMainWnd, &pixels_w, &pixels_h);
    g_cover.left = cover_points(COVER_LEFT, w, pixels_w);
    g_cover.top = cover_points(COVER_TOP, h, pixels_h);
    g_cover.right = cover_points(COVER_RIGHT, w, pixels_w);
    g_cover.bottom = cover_points(COVER_BOTTOM, h, pixels_h);
}

static void screen_keyboard(int show)
{
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "1");
    SDL_StopTextInput(g_hMainWnd);
    if (show)
        SDL_StartTextInput(g_hMainWnd);
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "0");
}

/* Text input is on while an edit box has the focus, as the client's edit boxes take a hardware
   keyboard's typing whenever they have it. The on-screen keyboard is another matter, as the client
   keeps the chat line focused all game long: it comes up when the player taps the focused edit box
   and goes when they tap anything else or the box loses the focus, and while it is up the scene is
   raised so that it does not cover the box. */
static void sync_text_input(void)
{
    static SDL_Rect told;
    struct UIWindow *edit = ui_window_mgr_get_focus_edit(&g_windowMgr);
    int w = 0, h = 0, pixels_h = 0, cover, fx, fy, fw, fh, x = 0, y = 0, tx, ty, bottom, lift = 0;
    int shown = SDL_ScreenKeyboardShown(g_hMainWnd);
    struct PresentCover bars = g_cover;
    SDL_Rect area;
    if (!edit || g_3dDevice.width <= 0 || g_3dDevice.height <= 0) {
        g_cover.lift = 0;
        if (shown)
            screen_keyboard(0);
        if (SDL_TextInputActive(g_hMainWnd))
            SDL_StopTextInput(g_hMainWnd);
        return;
    }
    ui_window_to_screen(edit, &x, &y);
    if (touch_pressed(&tx, &ty)) {
        int on_edit = tx >= x && tx < x + edit->width && ty >= y && ty < y + edit->height;
        if (on_edit != shown) {
            screen_keyboard(on_edit);
            shown = on_edit;
        }
    }
    SDL_GetWindowSize(g_hMainWnd, &w, &h);
    SDL_GetWindowSizeInPixels(g_hMainWnd, NULL, &pixels_h);
    cover = shown ? cover_points(COVER_KEYBOARD, h, pixels_h) : 0;
    bars.lift = 0;
    present_fit(w, h, g_3dDevice.width, g_3dDevice.height, &bars, &fx, &fy, &fw, &fh);
    bottom = fy + (y + edit->height) * fh / g_3dDevice.height;
    if (cover > 0 && bottom + KEYBOARD_MARGIN > h - cover)
        lift = bottom + KEYBOARD_MARGIN - (h - cover);
    g_cover.lift = lift;
    area.x = fx + x * fw / g_3dDevice.width;
    area.y = fy - lift + y * fh / g_3dDevice.height;
    area.w = edit->width * fw / g_3dDevice.width;
    area.h = edit->height * fh / g_3dDevice.height;
    if (memcmp(&area, &told, sizeof area) || !SDL_TextInputActive(g_hMainWnd)) {
        told = area;
        SDL_SetTextInputArea(g_hMainWnd, &area, 0);
    }
    if (!SDL_TextInputActive(g_hMainWnd))
        SDL_StartTextInput(g_hMainWnd);
}
#endif

#ifndef __ANDROID__
/* The IME draws its candidate list where the edit box last put it (language_place_candidates): below
   the composition's line, which SDL's text input area keeps clear, as CFS_EXCLUDE does on Windows. */
static void sync_candidate_area(void)
{
    static struct LanguageCandidatePos told;
    static int toldW, toldH, toldSceneW, toldSceneH;
    const struct LanguageCandidatePos *pos;
    int w = 0, h = 0, fx, fy, fw, fh;
    SDL_Rect area;
    if (!g_language || !g_composing || !(pos = &g_language->candidate_pos)->placed
        || g_3dDevice.width <= 0 || g_3dDevice.height <= 0)
        return;
    SDL_GetWindowSize(g_hMainWnd, &w, &h);
    /* the same place in the scene is somewhere else in a window or a scene of another size */
    if (told.placed && told.x == pos->x && told.top == pos->top && told.bottom == pos->bottom && toldW == w
        && toldH == h && toldSceneW == g_3dDevice.width && toldSceneH == g_3dDevice.height)
        return;
    told = *pos;
    toldW = w;
    toldH = h;
    toldSceneW = g_3dDevice.width;
    toldSceneH = g_3dDevice.height;
    present_fit(w, h, g_3dDevice.width, g_3dDevice.height, &g_cover, &fx, &fy, &fw, &fh);
    area.x = fx + pos->x * fw / g_3dDevice.width;
    area.y = fy + pos->top * fh / g_3dDevice.height;
    area.w = 1;
    area.h = (pos->bottom - pos->top) * fh / g_3dDevice.height;
    SDL_SetTextInputArea(g_hMainWnd, &area, 0);
}
#endif

void check_system_message(void)
{
    pump_events();
    display_follow_window();
#ifdef __ANDROID__
    sync_window_cover();
    sync_text_input();
    gfx_gl_set_cover(&g_cover);
#else
    sync_candidate_area();
#endif
    drive_state_tick();
}

void app_last_pump(void)
{
    pump_events();
    SDL_StopTextInput(g_hMainWnd);
    SDL_DestroyWindow(g_hMainWnd);
    g_hMainWnd = NULL;
}

/* The display a client that starts fullscreen goes fullscreen on: the one its saved windowed
   position is on, or the primary display when no display holds that point or none was saved. */
static SDL_DisplayID saved_display(void)
{
    SDL_Point at;
    SDL_Rect bounds;
    SDL_DisplayID id;
    if (g_windowX == DISPLAY_NO_POSITION || g_windowY == DISPLAY_NO_POSITION)
        return SDL_GetPrimaryDisplay();
    at.x = g_windowX;
    at.y = g_windowY;
    id = SDL_GetDisplayForPoint(&at);
    if (!id || !SDL_GetDisplayBounds(id, &bounds) || !SDL_PointInRect(&at, &bounds))
        return SDL_GetPrimaryDisplay();
    return id;
}

int app_create_window(void)
{
    /* With a high pixel density the window has as many pixels as the display gives it, also where
       SDL's window coordinates are points (macOS, scaled Wayland); the game draws at its size in
       pixels and the windowed size is in pixels (render/display_options.h). */
    SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_DisplayID on;
    int screenW, screenH, x = 0, y = 0, top = 0, left = 0, w, h;
    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
    /* Touches reach the client through core/touch.c only, a mouse and a pen as a mouse. */
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_PEN_TOUCH_EVENTS, "0");
#ifdef __ANDROID__
    /* The back button is Escape, as on the keyboard, instead of leaving the game; the on-screen
       keyboard comes up only when the player taps an edit box (sync_text_input). */
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "0");
#endif
#ifdef _WIN32
    /* As the Win32 window did: the click that activates the window reaches the game, and the window
       class keeps the client's name and the icon of its resources (0x77). */
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON, "119");
    SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON_SMALL, "119");
    SDL_RegisterApp(g_windowClassName, CS_BYTEALIGNCLIENT | CS_OWNDC, NULL);
    SDL_SetWindowsMessageHook(window_message, NULL);
#endif
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        platform_message_box(0, SDL_GetError(), "ERROR", 0);
        return 0;
    }
#ifdef _WIN32
    g_processKeyEvent = SDL_RegisterEvents(1);
#endif
#ifndef _WIN32
    /* Direct3D 9 draws into the window's HWND on Windows; elsewhere the GL backend draws through
       an OpenGL context of the window. */
    flags |= SDL_WINDOW_OPENGL;
#if defined(__ANDROID__) || defined(RAGE_GLES)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
#endif
#ifdef __ANDROID__
    /* An Android window always covers the screen, so the client runs in its fullscreen mode, at the
       window's size, without the system bars. */
    g_fullscreenMode = 1;
    flags |= SDL_WINDOW_FULLSCREEN;
#else
    /* Shown once it is in its place; the player can resize and maximise it when it is windowed. */
    flags |= SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
#endif
    on = g_fullscreenMode ? saved_display() : SDL_GetPrimaryDisplay();
    w = platform_window_units(NULL, on, g_displayWidth, 0);
    h = platform_window_units(NULL, on, g_displayHeight, 0);
    g_hMainWnd = SDL_CreateWindow(g_windowClassName, w, h, flags);
    if (!g_hMainWnd) {
        platform_message_box(NULL, "Window Creation Failed", "ERROR", 0);
        return 0;
    }
    platform_window_minimum(g_hMainWnd, DISPLAY_MIN_WIDTH, DISPLAY_MIN_HEIGHT);
    if (g_fullscreenMode == 0) {
        int placed = g_savedWidth == g_displayWidth && g_savedHeight == g_displayHeight
                     && g_windowX != DISPLAY_NO_POSITION;
        int displayW, displayH, roomW, roomH, fitW = g_displayWidth, fitH = g_displayHeight;
        /* a saved size the display has no room for opens at the largest that fits */
        platform_window_room(g_hMainWnd, &displayW, &displayH, &roomW, &roomH);
        display_fit(&fitW, &fitH, roomW, roomH);
        if (fitW != g_displayWidth || fitH != g_displayHeight) {
            g_displayWidth = fitW;
            g_displayHeight = fitH;
            w = platform_window_units(g_hMainWnd, 0, fitW, 0);
            h = platform_window_units(g_hMainWnd, 0, fitH, 0);
            SDL_SetWindowSize(g_hMainWnd, w, h);
        }
        /* x and y are the window's outer corner, as Window_XPos and Window_YPos keep it
           (platform_window_rect); SDL places the client area, inside the frame. */
        platform_screen_size(&screenW, &screenH);
        x = (screenW - w) / 2;
        y = (screenH - h) / 2;
        if (placed) {
            x = g_windowX;
            y = g_windowY;
        }
        if (x > screenW - w)
            x = (screenW - w) / 2;
        if (x < 0)
            x = 0;
        if (y > screenH - h)
            y = (screenH - h) / 2;
        if (y < 0)
            y = 0;
        SDL_GetWindowBordersSize(g_hMainWnd, &top, &left, NULL, NULL);
        SDL_SetWindowPosition(g_hMainWnd, x + left, y + top);
        /* maximised once it is shown, and restored to this size and place */
        if (g_windowMaximized)
            SDL_MaximizeWindow(g_hMainWnd);
    } else {
        /* covers the display it is placed on once it is shown; the device takes its size
           (render/c3ddevice.c) */
        SDL_SetWindowPosition(g_hMainWnd, SDL_WINDOWPOS_CENTERED_DISPLAY(on), SDL_WINDOWPOS_CENTERED_DISPLAY(on));
        platform_window_fullscreen(g_hMainWnd);
    }
    SDL_ShowWindow(g_hMainWnd);
    SDL_SyncWindow(g_hMainWnd);
    if (g_fullscreenMode == 0) {
        /* X11 knows the frame only once the window manager has framed the shown window */
        int shownTop = 0, shownLeft = 0;
        SDL_GetWindowBordersSize(g_hMainWnd, &shownTop, &shownLeft, NULL, NULL);
        if (shownTop != top || shownLeft != left)
            SDL_SetWindowPosition(g_hMainWnd, x + shownLeft, y + shownTop);
    }
    /* The client draws its own cursor: the system's shows only off the client area. */
    SDL_HideCursor();
#ifndef __ANDROID__
    SDL_StartTextInput(g_hMainWnd);
#endif
    platform_current_directory(g_clientDir, 0x80);
    return 1;
}
