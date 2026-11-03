#ifndef TEST_FONT_CACHE_FIXTURE_H
#define TEST_FONT_CACHE_FIXTURE_H
#include <windows.h>
#include <stdlib.h>

HDC WINAPI font_cache_test_get_dc(HWND window);
int WINAPI font_cache_test_release_dc(HWND window, HDC dc);
int WINAPI font_cache_test_enum(HDC dc, LPLOGFONTW query, FONTENUMPROCW callback, LPARAM context, DWORD flags);
HFONT WINAPI font_cache_test_create(int height, int width, int escapement, int orientation, int weight,
                                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                                  DWORD output, DWORD clipping, DWORD quality, DWORD pitch, LPCWSTR name);
HFONT WINAPI font_cache_test_create_indirect(const LOGFONTW *font);
int WINAPI font_cache_test_add(LPCWSTR path, DWORD flags, PVOID reserved);
BOOL WINAPI font_cache_test_remove(LPCWSTR path, DWORD flags, PVOID reserved);
BOOL WINAPI font_cache_test_delete_object(HGDIOBJ object);
HGDIOBJ WINAPI font_cache_test_select(HDC dc, HGDIOBJ object);
DWORD WINAPI font_cache_test_glyphs(HDC dc, LPCWSTR text, int count, LPWORD glyphs, DWORD flags);
BOOL WINAPI font_cache_test_metrics(HDC dc, LPTEXTMETRICW metric);
int font_cache_test_atexit(void (*callback)(void));
void *font_cache_test_realloc(void *data, size_t size);

#define GetDC font_cache_test_get_dc
#define ReleaseDC font_cache_test_release_dc
#define EnumFontFamiliesExW font_cache_test_enum
#define CreateFontW font_cache_test_create
#define CreateFontIndirectW font_cache_test_create_indirect
#define AddFontResourceExW font_cache_test_add
#define RemoveFontResourceExW font_cache_test_remove
#define DeleteObject font_cache_test_delete_object
#define SelectObject font_cache_test_select
#define GetGlyphIndicesW font_cache_test_glyphs
#define GetTextMetricsW font_cache_test_metrics
#define atexit font_cache_test_atexit
#define realloc font_cache_test_realloc
#endif
