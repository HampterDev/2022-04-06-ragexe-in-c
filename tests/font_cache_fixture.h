#ifndef TEST_FONT_CACHE_FIXTURE_H
#define TEST_FONT_CACHE_FIXTURE_H
#include <windows.h>
#include <stdlib.h>

HFONT WINAPI font_cache_test_create(int height, int width, int escapement, int orientation, int weight,
                                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                                  DWORD output, DWORD clipping, DWORD quality, DWORD pitch, LPCSTR name);
HFONT WINAPI font_cache_test_create_indirect(const LOGFONTA *font);
int WINAPI font_cache_test_add(LPCSTR path, DWORD flags, PVOID reserved);
BOOL WINAPI font_cache_test_remove(LPCSTR path, DWORD flags, PVOID reserved);
BOOL WINAPI font_cache_test_delete_object(HGDIOBJ object);
int font_cache_test_atexit(void (*callback)(void));
void *font_cache_test_realloc(void *data, size_t size);

#define CreateFontA font_cache_test_create
#define CreateFontIndirectA font_cache_test_create_indirect
#define AddFontResourceExA font_cache_test_add
#define RemoveFontResourceExA font_cache_test_remove
#define DeleteObject font_cache_test_delete_object
#define atexit font_cache_test_atexit
#define realloc font_cache_test_realloc
#endif
