#ifndef PLATFORM_WINDOW_H
#define PLATFORM_WINDOW_H

#include "platform/win_types.h"
#include "platform/window_handle.h"
#include <stddef.h>

/* Message boxes and the clipboard take and give the client's UTF-8 text (core/utf8.h); a message
   box shows text that is not UTF-8 as CP949, the encoding of resource names. */
void platform_message_box(PlatformWindow owner, const char *text, const char *caption, unsigned flags);
short platform_async_key_state(int vk);
short platform_key_state(int vk);
int platform_clipboard_set(PlatformWindow owner, const char *text, size_t length);
char *platform_clipboard_get(PlatformWindow owner);
void platform_cursor_client_pos(PlatformWindow window, POINT *out);
void platform_mouse_capture(PlatformWindow window, int capture);
int platform_show_cursor(int show);
void platform_clip_cursor_to_client(PlatformWindow window, int clip);
void platform_client_screen_rect(PlatformWindow window, RECT *out);
void platform_client_rect(PlatformWindow window, RECT *out);
void platform_window_rect(PlatformWindow window, RECT *out);
void platform_monitor_rect(PlatformDisplay display, RECT *out);
void platform_monitor_rects(PlatformDisplay display, RECT *mon, RECT *work);
/* The display the window is on. */
PlatformDisplay platform_window_monitor(PlatformWindow window);
int platform_window_has_caption(PlatformWindow window);
int platform_window_is_foreground(PlatformWindow window);
void platform_window_place_windowed(PlatformWindow window, PlatformDisplay on, int x, int y, int clientW,
                                    int clientH, int centre);
void platform_window_place_frameless(PlatformWindow window, PlatformDisplay on, int w, int h);
/* The primary display's size, for placing a window before there is one. */
void platform_screen_size(int *width, int *height);
void platform_focus_window(PlatformWindow window);
void platform_window_frameless(PlatformWindow window, const RECT *area);
void platform_window_fullscreen(PlatformWindow window);
void platform_ime_cancel(PlatformWindow window);
WORD platform_keyboard_language(void);
void platform_ime_alphanumeric(PlatformWindow window);

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
#endif

#endif
