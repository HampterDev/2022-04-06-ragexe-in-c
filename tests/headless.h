#ifndef TEST_HEADLESS_H
#define TEST_HEADLESS_H
/* Forced into platform/win_utf8.c where the tests' client libraries compile it (tests/CMakeLists.txt):
   the tests run without windows, so a message box that the code under test raises is written to
   stderr instead and answered with its default button, as if the player had pressed Enter.
   platform_message_box (platform/window_win32.c) shows its boxes through win_utf8_message_box. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static __inline int headless_default_button(UINT type)
{
    static const int buttons[][3] = {
        {IDOK, 0, 0}, {IDOK, IDCANCEL, 0}, {IDABORT, IDRETRY, IDIGNORE}, {IDYES, IDNO, IDCANCEL},
        {IDYES, IDNO, 0}, {IDRETRY, IDCANCEL, 0}, {IDCANCEL, IDTRYAGAIN, IDCONTINUE},
    };
    UINT kind = type & MB_TYPEMASK, index = (type & MB_DEFMASK) >> 8;
    if (kind >= sizeof buttons / sizeof buttons[0])
        return IDOK;
    if (index > 2 || !buttons[kind][index])
        index = 0;
    return buttons[kind][index];
}

static __inline int WINAPI headless_message_box_a(HWND owner, LPCSTR text, LPCSTR caption, UINT type)
{
    (void)owner;
    fprintf(stderr, "message box (not shown in tests): %s: %s\n", caption ? caption : "", text ? text : "");
    return headless_default_button(type);
}

static __inline char *headless_utf8(LPCWSTR text)
{
    int size = text ? WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL) : 0;
    char *out = size > 0 ? (char *)malloc((size_t)size) : NULL;
    if (out && !WideCharToMultiByte(CP_UTF8, 0, text, -1, out, size, NULL, NULL))
        out[0] = '\0';
    return out;
}

static __inline int WINAPI headless_message_box_w(HWND owner, LPCWSTR text, LPCWSTR caption, UINT type)
{
    char *narrow_text = headless_utf8(text), *narrow_caption = headless_utf8(caption);
    int result = headless_message_box_a(owner, narrow_text, narrow_caption, type);
    free(narrow_text);
    free(narrow_caption);
    return result;
}

#define MessageBoxA headless_message_box_a
#define MessageBoxW headless_message_box_w
#endif
