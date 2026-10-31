#ifndef CRASH_DUMP_FIXTURE_H
#define CRASH_DUMP_FIXTURE_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>

DWORD WINAPI test_dump_module_name(HMODULE module, wchar_t *path, DWORD capacity);
void WINAPI test_dump_time(SYSTEMTIME *time);
HANDLE WINAPI test_dump_event(SECURITY_ATTRIBUTES *attributes, BOOL manual, BOOL initial, const wchar_t *name);
HANDLE WINAPI test_dump_thread(SECURITY_ATTRIBUTES *attributes, SIZE_T stack, LPTHREAD_START_ROUTINE start,
                              void *parameter, DWORD flags, DWORD *id);
DWORD WINAPI test_dump_wait(HANDLE object, DWORD timeout);
BOOL WINAPI test_dump_close(HANDLE object);
HMODULE WINAPI test_dump_library(const wchar_t *name, HANDLE file, DWORD flags);
FARPROC WINAPI test_dump_proc(HMODULE module, const char *name);
BOOL WINAPI test_dump_write(HANDLE process, DWORD pid, HANDLE file, MINIDUMP_TYPE type,
                           MINIDUMP_EXCEPTION_INFORMATION *exception,
                           MINIDUMP_USER_STREAM_INFORMATION *streams,
                           MINIDUMP_CALLBACK_INFORMATION *callback);

#ifndef CRASH_DUMP_TEST_DRIVER
#define GetModuleFileNameW test_dump_module_name
#define GetLocalTime test_dump_time
#define CreateEventW test_dump_event
#define CreateThread test_dump_thread
#define WaitForSingleObject test_dump_wait
#define CloseHandle test_dump_close
#define LoadLibraryExW test_dump_library
#define GetProcAddress test_dump_proc
#endif
#endif
