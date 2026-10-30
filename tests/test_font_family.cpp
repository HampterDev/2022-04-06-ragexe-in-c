#include "render/font_family.h"
#include <windows.h>
#include <gdiplus.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <new>

#ifdef FONT_FAMILY_MOCK
static int phase, collections, families, arrays, starts, stops;
static int collection_marker, family_marker, clone_marker;
void *operator new[](size_t bytes)
{
    if (phase == 7) throw std::bad_alloc();
    void *p = std::malloc(bytes);
    if (!p) throw std::bad_alloc();
    ++arrays;
    return p;
}
void operator delete[](void *p) noexcept { if (p) { --arrays; std::free(p); } }
void operator delete[](void *p, size_t) noexcept { ::operator delete[](p); }

namespace Gdiplus {
Status WINGDIPAPI GdiplusStartup(ULONG_PTR *token, const GdiplusStartupInput *input,
                               GdiplusStartupOutput *output)
{
    ++starts;
    assert(input->GdiplusVersion == 1 && !input->DebugEventCallback);
    assert(!input->SuppressBackgroundThread && !input->SuppressExternalCodecs && !output);
    if (phase == 8) return OutOfMemory;
    *token = 0x12345;
    return Ok;
}
void WINGDIPAPI GdiplusShutdown(ULONG_PTR token)
{
    assert(token == 0x12345);
    ++stops;
}
namespace DllExports {
GpStatus WINGDIPAPI GdipNewPrivateFontCollection(GpFontCollection **out)
{
    if (phase == 1) return OutOfMemory;
    ++collections; *out = reinterpret_cast<GpFontCollection *>(&collection_marker); return Ok;
}
GpStatus WINGDIPAPI GdipDeletePrivateFontCollection(GpFontCollection **p)
{
    if (*p) { assert(collections == 1); --collections; *p = nullptr; }
    return Ok;
}
GpStatus WINGDIPAPI GdipPrivateAddFontFile(GpFontCollection *, const WCHAR *)
{ return phase == 2 ? FileNotFound : Ok; }
GpStatus WINGDIPAPI GdipGetFontCollectionFamilyList(GpFontCollection *, INT count, GpFontFamily **out, INT *found)
{
    assert(count == 1);
    if (phase == 3) return GenericError;
    *found = phase == 4 ? 0 : 1;
    out[0] = reinterpret_cast<GpFontFamily *>(&family_marker);
    return Ok;
}
GpStatus WINGDIPAPI GdipCloneFontFamily(GpFontFamily *, GpFontFamily **out)
{
    if (phase == 5) return OutOfMemory;
    ++families; *out = reinterpret_cast<GpFontFamily *>(&clone_marker); return Ok;
}
GpStatus WINGDIPAPI GdipDeleteFontFamily(GpFontFamily *p)
{
    if (p) { assert(families == 1); --families; }
    return Ok;
}
GpStatus WINGDIPAPI GdipGetFamilyName(const GpFontFamily *p, WCHAR name[LF_FACESIZE], LANGID)
{
    if (!p || phase == 6) return GenericError;
    std::wcscpy(name, L"Test Font"); return Ok;
}
void *WINGDIPAPI GdipAlloc(size_t bytes) { return std::malloc(bytes); }
void WINGDIPAPI GdipFree(void *p) { std::free(p); }
}}

int main()
{
    uintptr_t token = 42;
    wchar_t out[LF_FACESIZE];
    assert(!font_backend_start(nullptr) && starts == 0);
    phase = 8;
    assert(!font_backend_start(&token) && token == 42 && starts == 1);
    phase = 0;
    assert(font_backend_start(&token) && token == 0x12345 && starts == 2);
    font_backend_stop(0);
    assert(!stops);
    font_backend_stop(token);
    assert(stops == 1);
    for (phase = 0; phase <= 7; ++phase) {
        std::wcscpy(out, L"unchanged");
        int ok = font_file_read_family(L"test.ttf", out, LF_FACESIZE);
        assert(ok == (phase == 0));
        assert(!std::wcscmp(out, phase ? L"unchanged" : L"Test Font"));
        assert(!collections && !families && !arrays);
    }
    phase = 0;
    std::wcscpy(out, L"unchanged");
    assert(!font_file_read_family(L"test.ttf", out, 2));
    assert(!font_file_read_family(nullptr, out, LF_FACESIZE));
    assert(!font_file_read_family(L"test.ttf", nullptr, LF_FACESIZE));
    assert(!font_file_read_family(L"test.ttf", out, 0));
    assert(!std::wcscmp(out, L"unchanged") && !collections && !families && !arrays);
    std::puts("PASS: SDK font startup/shutdown, ownership, API/allocation failure and unchanged outputs");
}
#else
int main()
{
    uintptr_t token = 0;
    wchar_t directory[MAX_PATH], pattern[MAX_PATH], path[MAX_PATH], name[LF_FACESIZE];
    WIN32_FIND_DATAW entry;
    bool found = false;
    assert(font_backend_start(&token) && token);
    UINT count = GetWindowsDirectoryW(directory, MAX_PATH);
    assert(count && count < MAX_PATH);
    swprintf_s(pattern, L"%s\\Fonts\\*.ttf", directory);
    HANDLE search = FindFirstFileW(pattern, &entry);
    assert(search != INVALID_HANDLE_VALUE);
    do {
        swprintf_s(path, L"%s\\Fonts\\%s", directory, entry.cFileName);
        if (font_file_read_family(path, name, LF_FACESIZE)) { found = true; break; }
    } while (FindNextFileW(search, &entry));
    FindClose(search);
    assert(found && name[0]);
    for (int i = 0; i < 20; ++i) {
        wchar_t again[LF_FACESIZE];
        assert(font_file_read_family(path, again, LF_FACESIZE));
        assert(!std::wcscmp(name, again));
    }
    std::wcscpy(name, L"unchanged");
    assert(!font_file_read_family(L"?:\\font-does-not-exist.ttf", name, LF_FACESIZE));
    assert(!std::wcscmp(name, L"unchanged"));
    font_backend_stop(token);
    std::puts("PASS: real Windows font family lookup, repeated ownership and missing-file fallback");
}
#endif
