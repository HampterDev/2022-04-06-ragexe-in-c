#include <stdbool.h>
#include <stdint.h>
#ifndef TEST_MOUSE_FIXTURE_H
#define TEST_MOUSE_FIXTURE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

/* The clock and SDL's mouse capture, as contents/mouse.c and platform/window_sdl.c call them. */
uint32_t mouse_test_time(void);
bool mouse_test_capture(bool capture);

#define timeGetTime mouse_test_time
#define platform_ticks_ms mouse_test_time
#define SDL_CaptureMouse mouse_test_capture

#endif
