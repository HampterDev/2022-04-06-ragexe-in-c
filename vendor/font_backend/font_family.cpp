#include "render/font_family.h"
#include <windows.h>
#include <gdiplus.h>
#include <cwchar>

int font_backend_start(uintptr_t *token)
{
    if (!token)
        return 0;
    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR value = 0;
    if (Gdiplus::GdiplusStartup(&value, &input, nullptr) != Gdiplus::Ok)
        return 0;
    *token = value;
    return 1;
}

void font_backend_stop(uintptr_t token)
{
    if (token)
        Gdiplus::GdiplusShutdown(token);
}

int font_file_read_family(const wchar_t *path, wchar_t *name, size_t capacity)
{
    if (!path || !name || !capacity)
        return 0;
    try {
        Gdiplus::PrivateFontCollection collection;
        if (collection.GetLastStatus() != Gdiplus::Ok || collection.AddFontFile(path) != Gdiplus::Ok)
            return 0;
        Gdiplus::FontFamily family;
        int found = 0;
        if (collection.GetFamilies(1, &family, &found) != Gdiplus::Ok || found != 1)
            return 0;
        wchar_t text[LF_FACESIZE] = {};
        if (family.GetFamilyName(text) != Gdiplus::Ok)
            return 0;
        size_t count = std::wcslen(text) + 1;
        if (count > capacity)
            return 0;
        std::wmemcpy(name, text, count);
        return 1;
    } catch (...) {
        return 0;
    }
}
