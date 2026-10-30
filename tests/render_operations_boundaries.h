#ifndef TEST_RENDER_OPERATION_BOUNDARIES_H
#define TEST_RENDER_OPERATION_BOUNDARIES_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
DWORD render_test_time(void);
#define timeGetTime render_test_time
#endif
