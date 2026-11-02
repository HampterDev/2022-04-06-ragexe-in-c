#ifndef _WIN32
#include "core/app.h"
#include "core/window_events.h"
#include "core/drive_state.h"
#include "contents/mouse.h"
#include "contents/frame_timer.h"
#include "platform/code_page.h"
#include "platform/filesystem.h"
#include "platform/language.h"
#include "platform/window.h"
#include "render/device.h"
#include "session/session_options.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_keyboard.h"
#include "core/window_state.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *g_sdlWindow;
static int g_composing;

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

static int key_code(const SDL_KeyboardEvent *key)
{
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
    return (unsigned)key->scancode <= SDL_SCANCODE_RGUI ? g_scanKeys[key->scancode] : 0;
}

static int key_lparam(const SDL_KeyboardEvent *key)
{
    unsigned code = (unsigned)key->scancode <= SDL_SCANCODE_RGUI ? g_scanCodes[key->scancode] : 0;
    int lparam = 1 | (int)((code & 0xff) << 16);
    if (code & 0x100)
        lparam |= 1 << 24;
    if (key->mod & SDL_KMOD_ALT)
        lparam |= 1 << 29;
    if (key->repeat)
        lparam |= 1 << 30;
    return lparam;
}

static char *client_text(const char *utf8)
{
    uint16_t *wide = (uint16_t *)SDL_iconv_string("UTF-16LE", "UTF-8", utf8, strlen(utf8) + 1);
    char *text = NULL;
    int length = 0, capacity;
    if (!wide)
        return NULL;
    while (wide[length])
        length++;
    capacity = length * 4 + 1;
    text = malloc((size_t)capacity);
    if (text) {
        int written = platform_code_page_from_utf16(g_language->code_page, wide, length, text, capacity - 1);
        if (written <= 0 && length) {
            free(text);
            text = NULL;
        } else {
            text[written > 0 ? written : 0] = 0;
        }
    }
    SDL_free(wide);
    return text;
}

static void scene_point(float x, float y, int *outX, int *outY)
{
    int w = 0, h = 0;
    SDL_GetWindowSize(g_sdlWindow, &w, &h);
    if (w > 0 && h > 0 && g_3dDevice.width > 0 && g_3dDevice.height > 0) {
        *outX = (int)(x * g_3dDevice.width / w);
        *outY = (int)(y * g_3dDevice.height / h);
    } else {
        *outX = (int)x;
        *outY = (int)y;
    }
}

static void on_key(const SDL_KeyboardEvent *key, int down)
{
    int vk = key_code(key), lparam = key_lparam(key);
    if (!vk)
        return;
    platform_note_key(vk, down);
    if (!down) {
        window_events_key_up(vk, lparam | (int)0xc0000000u);
        return;
    }
    if ((key->mod & SDL_KMOD_ALT) || vk == VK_F10) {
        ui_window_mgr_on_key_down(&g_windowMgr, vk, lparam, !key->repeat);
        return;
    }
    window_events_key_down(vk, lparam);
    if (vk == VK_BACK && !g_composing)
        window_events_char(VK_BACK);
}

static void on_text(const char *utf8)
{
    char *text;
    if (!g_language || (!g_windowMgr.key_as_text && !g_composing))
        return;
    text = client_text(utf8);
    if (!text)
        return;
    if (!g_composing && text[0] && !text[1])
        window_events_char((unsigned char)text[0]);
    else
        language_commit(g_language, text);
    free(text);
    g_composing = 0;
    window_events_language_changed();
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
    g_composing = text[0] != 0;
    language_set_composition(g_language, text, start);
    free(text);
    window_events_language_changed();
}

static void dispatch(const SDL_Event *event)
{
    int x, y;
    switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
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
        window_events_activate(0);
        app_activate(0);
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
        platform_note_cursor(x, y);
        mouse_on_move(&g_mouse);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        int down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        scene_point(event->button.x, event->button.y, &x, &y);
        platform_note_cursor(x, y);
        if (event->button.button == SDL_BUTTON_LEFT) {
            platform_note_key(VK_LBUTTON, down);
            mouse_on_button(&g_mouse, g_hMainWnd, MOUSE_LEFT, down);
        } else if (event->button.button == SDL_BUTTON_RIGHT) {
            platform_note_key(VK_RBUTTON, down);
            mouse_on_button(&g_mouse, g_hMainWnd, MOUSE_RIGHT, down);
        } else if (event->button.button == SDL_BUTTON_MIDDLE) {
            platform_note_key(VK_MBUTTON, down);
        }
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
        mouse_on_wheel(&g_mouse, (int)(event->wheel.y * 120.0f));
        break;
    default:
        break;
    }
}

static void pump_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        dispatch(&event);
}

void check_system_message(void)
{
    pump_events();
    drive_state_tick();
}

void app_last_pump(void)
{
    pump_events();
    SDL_StopTextInput(g_sdlWindow);
    SDL_DestroyWindow(g_sdlWindow);
    g_sdlWindow = NULL;
    g_hMainWnd = NULL;
    platform_set_main_window(NULL);
}

int app_create_window(HINSTANCE instance)
{
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL;
    int screenW, screenH, x, y;
    g_hInstance = instance;
    SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        platform_message_box(0, SDL_GetError(), "ERROR", 0);
        return 0;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    g_sdlWindow = SDL_CreateWindow(g_windowClassName, g_screenWidth, g_screenHeight, flags);
    if (!g_sdlWindow) {
        platform_message_box(0, "Window Creation Failed", "ERROR", 0);
        return 0;
    }
    platform_set_main_window(g_sdlWindow);
    g_hMainWnd = (HWND)g_sdlWindow;
    if (g_fullscreenMode == 0) {
        platform_screen_size(&screenW, &screenH);
        x = (screenW - g_screenWidth) / 2;
        y = (screenH - g_screenHeight) / 2;
        if (g_savedWidth == g_screenWidth && g_savedHeight == g_screenHeight) {
            x = session_get_option_int(&g_session1, "Window_XPos", x);
            y = session_get_option_int(&g_session1, "Window_YPos", y);
        }
        if (x > screenW - g_screenWidth)
            x = (screenW - g_screenWidth) / 2;
        if (x < 0)
            x = 0;
        if (y > screenH - g_screenHeight)
            y = (screenH - g_screenHeight) / 2;
        if (y < 0)
            y = 0;
        SDL_SetWindowPosition(g_sdlWindow, x, y);
    }
    SDL_ShowWindow(g_sdlWindow);
    SDL_StartTextInput(g_sdlWindow);
    platform_current_directory(g_clientDir, 0x80);
    return 1;
}
#endif
