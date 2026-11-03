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
#include "render/present_fit.h"
#include "session/session_options.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_keyboard.h"
#include "ui/window_access.h"
#include "ui/window.h"
#include "ui/window_types.h"
#include "core/window_state.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *g_sdlWindow;
static int g_composing, g_compositionEnded;

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
#ifdef __ANDROID__
    if (key->scancode == SDL_SCANCODE_AC_BACK)
        return VK_ESCAPE;
#endif
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
    int w = 0, h = 0, fx, fy, fw, fh;
    SDL_GetWindowSize(g_sdlWindow, &w, &h);
    present_fit(w, h, g_3dDevice.width, g_3dDevice.height, &g_presentCover, &fx, &fy, &fw, &fh);
    if (fw > 0 && fh > 0 && g_3dDevice.width > 0 && g_3dDevice.height > 0) {
        *outX = (int)((x - fx) * g_3dDevice.width / fw);
        *outY = (int)((y - fy) * g_3dDevice.height / fh);
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
    if (vk == VK_SNAPSHOT)
        return;
    g_compositionEnded = 0;
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
    int composed = g_composing || g_compositionEnded;
    g_compositionEnded = 0;
    if (!g_language || (!g_windowMgr.key_as_text && !composed))
        return;
    text = client_text(utf8);
    if (!text)
        return;
    if (!composed && text[0] && !text[1])
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
    g_compositionEnded = g_composing && !text[0];
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

static int is_touch_button(const SDL_Event *event)
{
    return (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP)
        && event->button.which == SDL_TOUCH_MOUSEID;
}

/* Where in the scene the last touch pressed this frame, for the on-screen keyboard. */
static int g_touchPressed, g_touchX, g_touchY;

static void dispatch_touch(const SDL_Event *event)
{
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && is_touch_button(event)) {
        g_touchPressed = 1;
        scene_point(event->button.x, event->button.y, &g_touchX, &g_touchY);
    }
    dispatch(event);
}

/* A touch moves the pointer and presses in one go, but the client finds the window under the pointer
   once a frame, as a mouse gives it time to: a touch's press and release wait for the next frame
   when the pointer moved in this one, and come one a frame. */
static void pump_events(void)
{
    static SDL_Event held;
    static int holding;
    SDL_Event event;
    int moved = 0, touched = 0;
    g_touchPressed = 0;
    if (holding) {
        holding = 0;
        dispatch_touch(&held);
        touched = 1;
    }
    while (SDL_PollEvent(&event)) {
        if (is_touch_button(&event) && (moved || touched)) {
            held = event;
            holding = 1;
            break;
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
            moved = 1;
        touched |= is_touch_button(&event);
        dispatch_touch(&event);
    }
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
    SDL_GetWindowSize(g_sdlWindow, &w, &h);
    SDL_GetWindowSizeInPixels(g_sdlWindow, &pixels_w, &pixels_h);
    g_presentCover.left = cover_points(COVER_LEFT, w, pixels_w);
    g_presentCover.top = cover_points(COVER_TOP, h, pixels_h);
    g_presentCover.right = cover_points(COVER_RIGHT, w, pixels_w);
    g_presentCover.bottom = cover_points(COVER_BOTTOM, h, pixels_h);
}

static void screen_keyboard(int show)
{
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "1");
    SDL_StopTextInput(g_sdlWindow);
    if (show)
        SDL_StartTextInput(g_sdlWindow);
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
    int w = 0, h = 0, pixels_h = 0, cover, fx, fy, fw, fh, x = 0, y = 0, bottom, lift = 0;
    int shown = SDL_ScreenKeyboardShown(g_sdlWindow);
    struct PresentCover bars = g_presentCover;
    SDL_Rect area;
    if (!edit || g_3dDevice.width <= 0 || g_3dDevice.height <= 0) {
        g_presentCover.lift = 0;
        if (shown)
            screen_keyboard(0);
        if (SDL_TextInputActive(g_sdlWindow))
            SDL_StopTextInput(g_sdlWindow);
        return;
    }
    ui_window_to_screen(edit, &x, &y);
    if (g_touchPressed) {
        int on_edit = g_touchX >= x && g_touchX < x + edit->width && g_touchY >= y && g_touchY < y + edit->height;
        if (on_edit != shown) {
            screen_keyboard(on_edit);
            shown = on_edit;
        }
    }
    SDL_GetWindowSize(g_sdlWindow, &w, &h);
    SDL_GetWindowSizeInPixels(g_sdlWindow, NULL, &pixels_h);
    cover = shown ? cover_points(COVER_KEYBOARD, h, pixels_h) : 0;
    bars.lift = 0;
    present_fit(w, h, g_3dDevice.width, g_3dDevice.height, &bars, &fx, &fy, &fw, &fh);
    bottom = fy + (y + edit->height) * fh / g_3dDevice.height;
    if (cover > 0 && bottom + KEYBOARD_MARGIN > h - cover)
        lift = bottom + KEYBOARD_MARGIN - (h - cover);
    g_presentCover.lift = lift;
    area.x = fx + x * fw / g_3dDevice.width;
    area.y = fy - lift + y * fh / g_3dDevice.height;
    area.w = edit->width * fw / g_3dDevice.width;
    area.h = edit->height * fh / g_3dDevice.height;
    if (memcmp(&area, &told, sizeof area) || !SDL_TextInputActive(g_sdlWindow)) {
        told = area;
        SDL_SetTextInputArea(g_sdlWindow, &area, 0);
    }
    if (!SDL_TextInputActive(g_sdlWindow))
        SDL_StartTextInput(g_sdlWindow);
}
#endif

void check_system_message(void)
{
    pump_events();
#ifdef __ANDROID__
    sync_window_cover();
    sync_text_input();
#endif
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
#ifdef __ANDROID__
    /* The back button is Escape, as on the keyboard, instead of leaving the game; the on-screen
       keyboard comes up only when the player taps an edit box (sync_text_input). */
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "0");
#endif
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        platform_message_box(0, SDL_GetError(), "ERROR", 0);
        return 0;
    }
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
#ifdef __ANDROID__
    /* An Android window always covers the screen, so the client runs in its full-screen mode: at its
       own resolution, scaled to the screen with the aspect kept, without the system bars. */
    g_fullscreenMode = 1;
    flags |= SDL_WINDOW_FULLSCREEN;
#endif
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
#ifndef __ANDROID__
    SDL_StartTextInput(g_sdlWindow);
#endif
    platform_current_directory(g_clientDir, 0x80);
    return 1;
}
#endif
