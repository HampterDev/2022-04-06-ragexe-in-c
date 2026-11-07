#ifndef TEST_FONT_CACHE_FIXTURE_H
#define TEST_FONT_CACHE_FIXTURE_H
#include <windows.h>
#include <stdlib.h>

HDC WINAPI font_cache_test_get_dc(HWND window);
int WINAPI font_cache_test_release_dc(HWND window, HDC dc);
int WINAPI font_cache_test_enum(HDC dc, LPLOGFONTA query, FONTENUMPROCA callback, LPARAM context, DWORD flags);
HFONT WINAPI font_cache_test_create(int height, int width, int escapement, int orientation, int weight,
                                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                                  DWORD output, DWORD clipping, DWORD quality, DWORD pitch, LPCSTR name);
HFONT WINAPI font_cache_test_create_indirect(const LOGFONTA *font);
int WINAPI font_cache_test_add(LPCSTR path, DWORD flags, PVOID reserved);
BOOL WINAPI font_cache_test_remove(LPCSTR path, DWORD flags, PVOID reserved);
BOOL WINAPI font_cache_test_delete_object(HGDIOBJ object);
int font_cache_test_atexit(void (*callback)(void));
void *font_cache_test_realloc(void *data, size_t size);

#define GetDC font_cache_test_get_dc
#define ReleaseDC font_cache_test_release_dc
#define EnumFontFamiliesExA font_cache_test_enum
#define CreateFontA font_cache_test_create
#define CreateFontIndirectA font_cache_test_create_indirect
#define AddFontResourceExA font_cache_test_add
#define RemoveFontResourceExA font_cache_test_remove
#define DeleteObject font_cache_test_delete_object
#define atexit font_cache_test_atexit
#define realloc font_cache_test_realloc
#endif
