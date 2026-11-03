/* Windows scene runs: the cursor and the key state come from the scene script, not from the
   desktop, and the scene never captures the real mouse: platform_note_key and platform_note_cursor
   keep them, as on POSIX. Everything else is platform/window.c. */
#define platform_async_key_state platform_async_key_state_os
#define platform_key_state platform_key_state_os
#define platform_cursor_client_pos platform_cursor_client_pos_os
#define platform_mouse_capture platform_mouse_capture_os
#define platform_note_key platform_note_key_os
#define platform_note_cursor platform_note_cursor_os
#include "platform/window.c"
#undef platform_async_key_state
#undef platform_key_state
#undef platform_cursor_client_pos
#undef platform_mouse_capture
#undef platform_note_key
#undef platform_note_cursor

#include "scene.h"

static unsigned char g_keys[256], g_toggles[256];
static POINT g_cursor;

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
    g_cursor.x = x;
    g_cursor.y = y;
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

void platform_cursor_client_pos(PlatformWindow window, POINT *out)
{
    (void)window;
    *out = g_cursor;
}

void platform_mouse_capture(PlatformWindow window, int capture)
{
    (void)window;
    (void)capture;
}
