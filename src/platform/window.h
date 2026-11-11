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
void platform_clip_cursor_to_client(PlatformWindow window, int clip);
/* The client area, from 0, 0, in pixels: the size the game draws at. SDL's window coordinates
   (positions, the frame, the pointer) are pixels on Windows and Android and points on macOS and
   scaled Wayland, where a point is several pixels. */
void platform_client_rect(PlatformWindow window, RECT *out);
/* The window's frame, in window coordinates. */
void platform_window_rect(PlatformWindow window, RECT *out);
/* The display the window is on. */
PlatformDisplay platform_window_monitor(PlatformWindow window);
/* In pixels: the size of the display the window is on, and the largest client area whose window,
   frame included, fits the display's usable area. */
void platform_window_room(PlatformWindow window, int *displayW, int *displayH, int *roomW, int *roomH);
/* Window coordinates for a length in pixels at the window's pixel density, or, without a window,
   at the pixel density of the display's desktop mode: to the nearest, or rounded up so that they
   hold at least that many pixels (display_units, render/display_options.h). */
int platform_window_units(PlatformWindow window, PlatformDisplay display, int pixels, int roundUp);
/* The smallest client area, in pixels, the player can size the window to, at the window's present
   pixel density; set again when that changes. */
void platform_window_minimum(PlatformWindow window, int width, int height);
int platform_window_is_foreground(PlatformWindow window);
int platform_window_is_minimized(PlatformWindow window);
int platform_window_is_maximized(PlatformWindow window);
/* Windowed: out of fullscreen, with its frame, the client area clientW by clientH pixels and the
   frame's corner at x, y in window coordinates, or centred in the work area of the display the
   window is on, and kept in it; then maximised when asked, so that restoring it gives that size
   and place. */
void platform_window_place_windowed(PlatformWindow window, int x, int y, int clientW, int clientH, int centre,
                                    int maximized);
/* Fullscreen: the window covers the display it is on at the display's own size, without changing
   the display's mode (SDL's fullscreen desktop). A hidden window goes fullscreen when shown. */
void platform_window_fullscreen(PlatformWindow window);
int platform_window_is_fullscreen(PlatformWindow window);
/* The primary display's size in window coordinates, for placing a window before there is one. */
void platform_screen_size(int *width, int *height);
void platform_focus_window(PlatformWindow window);
void platform_ime_cancel(PlatformWindow window);
void platform_ime_alphanumeric(PlatformWindow window);

/* Key and pointer state as the client asks for it, kept from the input SDL delivers:
   core/user_input.c tells, the key and cursor queries above read it back. */
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
