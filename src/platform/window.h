#ifndef PLATFORM_WINDOW_H
#define PLATFORM_WINDOW_H

#include "platform/win_types.h"
#include <stddef.h>

void platform_message_box(HWND owner, const char *text, const char *caption, unsigned flags);
short platform_async_key_state(int vk);
short platform_key_state(int vk);
int platform_clipboard_set(HWND owner, const char *text, size_t length);
char *platform_clipboard_get(HWND owner);
void platform_cursor_client_pos(HWND window, POINT *out);
void platform_mouse_capture(HWND window, int capture);
int platform_show_cursor(int show);
void platform_clip_cursor_to_client(HWND window, int clip);
void platform_client_screen_rect(HWND window, RECT *out);
void platform_client_rect(HWND window, RECT *out);
void platform_window_rect(HWND window, RECT *out);
void platform_monitor_rect(HMONITOR monitor, RECT *out);
void platform_monitor_rects(HMONITOR monitor, RECT *mon, RECT *work);
HMONITOR platform_window_monitor(HWND window);
int platform_window_has_caption(HWND window);
int platform_window_is_foreground(HWND window);
void platform_window_place_windowed(HWND hwnd, HMONITOR on, int x, int y, int clientW, int clientH, int centre);
void platform_window_place_frameless(HWND hwnd, HMONITOR on, int w, int h);
void platform_screen_size(int *width, int *height);
void platform_focus_window(HWND window);
void platform_window_frameless(HWND window, const RECT *area);
void platform_window_fullscreen(HWND window);
void platform_ime_cancel(HWND window);
WORD platform_keyboard_language(void);
void platform_ime_alphanumeric(HWND window);

/* Key and pointer state for the window systems that do not keep it (core/user_input.c tells; the key
   and cursor queries above read it back). Windows keeps its own, so there they do nothing. */
void platform_note_key(int vk, int down);
void platform_note_cursor(int x, int y);

#ifndef _WIN32
#define MB_OK 0x0
#define MB_ICONERROR 0x10
#define MB_ICONEXCLAMATION 0x30
#define MB_ICONWARNING 0x30
#define MB_ICONINFORMATION 0x40
#define MB_TASKMODAL 0x2000

void platform_set_main_window(void *window);
#endif

#endif
