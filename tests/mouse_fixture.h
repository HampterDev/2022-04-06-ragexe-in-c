#ifndef TEST_MOUSE_FIXTURE_H
#define TEST_MOUSE_FIXTURE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

DWORD mouse_test_time(void);
BOOL mouse_test_cursor(POINT *point);
BOOL mouse_test_client(HWND window, POINT *point);
HWND mouse_test_capture(HWND window);
BOOL mouse_test_release(void);

#define timeGetTime mouse_test_time
#define GetCursorPos mouse_test_cursor
#define ScreenToClient mouse_test_client
#define SetCapture mouse_test_capture
#define ReleaseCapture mouse_test_release

#endif
