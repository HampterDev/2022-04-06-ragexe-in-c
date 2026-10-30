#ifndef TEST_UI_INVALIDATION_FIXTURE_H
#define TEST_UI_INVALIDATION_FIXTURE_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

DWORD ui_fixture_time(void);
#define timeGetTime ui_fixture_time

#endif
