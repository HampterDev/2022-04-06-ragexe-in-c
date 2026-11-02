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
void platform_screen_size(int *width, int *height);
void platform_focus_window(HWND window);
void platform_ime_cancel(HWND window);
void platform_ime_alphanumeric(HWND window);

#ifndef _WIN32
void platform_note_key(int vk, int down);
void platform_note_cursor(int x, int y);
void platform_set_main_window(void *window);
#endif

#endif
