#ifndef TEST_WIN_UTF8_FIXTURE_H
#define TEST_WIN_UTF8_FIXTURE_H
#include <windows.h>
#include <stdlib.h>

int WINAPI win_utf8_test_message_box_w(HWND owner, LPCWSTR text, LPCWSTR caption, UINT type);
int WINAPI win_utf8_test_message_box_a(HWND owner, LPCSTR text, LPCSTR caption, UINT type);
LONG WINAPI win_utf8_test_open(HKEY root, LPCWSTR path, DWORD options, REGSAM access, PHKEY key);
LONG WINAPI win_utf8_test_create(HKEY root, LPCWSTR path, DWORD reserved, LPWSTR class_name, DWORD options,
                                 REGSAM access, const LPSECURITY_ATTRIBUTES security, PHKEY key,
                                 LPDWORD disposition);
LONG WINAPI win_utf8_test_query(HKEY key, LPCWSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data,
                                LPDWORD size);
LONG WINAPI win_utf8_test_set(HKEY key, LPCWSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD size);
void *win_utf8_test_malloc(size_t size);

#define MessageBoxW win_utf8_test_message_box_w
#define MessageBoxA win_utf8_test_message_box_a
#define RegOpenKeyExW win_utf8_test_open
#define RegCreateKeyExW win_utf8_test_create
#define RegQueryValueExW win_utf8_test_query
#define RegSetValueExW win_utf8_test_set
#define malloc win_utf8_test_malloc
#endif
