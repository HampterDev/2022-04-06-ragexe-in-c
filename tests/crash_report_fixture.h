#ifndef CRASH_REPORT_FIXTURE_H
#define CRASH_REPORT_FIXTURE_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
DWORD WINAPI test_report_module_name(HMODULE module, wchar_t *path, DWORD capacity);
#define GetModuleFileNameW test_report_module_name
#endif
