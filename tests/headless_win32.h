#ifndef TEST_HEADLESS_WIN32_H
#define TEST_HEADLESS_WIN32_H
/* Forced into platform/net_win32.c where the tests' client libraries compile it
   (tests/CMakeLists.txt), as headless.h is into platform/window_sdl.c: the two boxes it shows
   itself with MessageBoxA, when ws2_32.dll lacks send or recv, are written to stderr instead.
   The includes are net_win32.c's own, in its order. */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

static __inline int WINAPI headless_message_box_a(HWND owner, LPCSTR text, LPCSTR caption, UINT type)
{
    (void)owner;
    (void)type;
    fprintf(stderr, "message box (not shown in tests): %s: %s\n", caption ? caption : "", text ? text : "");
    return IDOK;
}

#define MessageBoxA headless_message_box_a
#endif
