#include "render/fonts.h"
#include "render/device.h"
#include <assert.h>
#include <stdio.h>
#include <wchar.h>

struct C3dDevice g_3dDevice;

static int korean_face(const wchar_t *name)
{
    return !_wcsicmp(name, L"Gulim") || !wcscmp(name, L"\xad74\xb9bc")
        || !_wcsicmp(name, L"Malgun Gothic") || !wcscmp(name, L"\xb9d1\xc740 \xace0\xb515");
}

static void cycle(void)
{
    struct FontCache cache;
    HDC dc = CreateCompatibleDC(NULL);
    int face;
    assert(dc);
    font_cache_construct(&cache);
    for (face = 0; face <= 29; face += face == 0 ? 20 : 9) {
        HFONT font = font_cache_get_font(&cache, face, 15, HANGEUL_CHARSET, 0, 0);
        HGDIOBJ previous;
        SIZE extent;
        LOGFONTW description;
        assert(font && font_cache_get_font(&cache, face, 15, HANGEUL_CHARSET, 0, 0) == font);
        assert(GetObjectW(font, sizeof(description), &description) == sizeof(description));
        previous = SelectObject(dc, font);
        assert(previous && previous != HGDI_ERROR);
        assert(GetTextExtentPoint32W(dc, L"Hello \xd55c\xae00", 8, &extent) && extent.cx > 0 && extent.cy > 0);
        assert(SelectObject(dc, previous) == font);
    }
    font_cache_destruct(&cache);
    font_cache_destruct(&cache);
    assert(DeleteDC(dc));
}

/* Arial has no Hangul, so Hangul falls back to Gulim or Malgun Gothic, which have it. */
static void fallback(void)
{
    struct FontCache cache;
    HDC dc = CreateCompatibleDC(NULL);
    HFONT face, hangul;
    HGDIOBJ previous;
    wchar_t name[LF_FACESIZE];
    assert(dc);
    font_cache_construct(&cache);
    face = font_cache_get_font(&cache, 0, 15, ANSI_CHARSET, 0, 0);
    assert(face && !font_cache_covers(&cache, dc, face, TEXT_SCRIPT_HANGUL));
    hangul = font_cache_get_script_font(&cache, TEXT_SCRIPT_HANGUL, 15, 0, 0);
    assert(hangul && hangul != face && font_cache_covers(&cache, dc, hangul, TEXT_SCRIPT_HANGUL));
    previous = SelectObject(dc, hangul);
    assert(previous && previous != HGDI_ERROR);
    assert(GetTextFaceW(dc, LF_FACESIZE, name) && korean_face(name));
    assert(SelectObject(dc, previous) == hangul);
    assert(font_cache_ascent(&cache, dc, face) > 0 && font_cache_ascent(&cache, dc, hangul) > 0);
    font_cache_destruct(&cache);
    assert(DeleteDC(dc));
}

int main(void)
{
    DWORD before, after;
    int i;
    g_3dDevice.faceNamesA[0] = L"Arial";
    g_3dDevice.faceNamesB[0] = L"Tahoma";
    cycle();
    fallback();
    before = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    for (i = 0; i < 32; ++i) {
        cycle();
        fallback();
    }
    after = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(after == before);
    puts("Font cache system: real GDI selection, Korean text measurement, Hangul fallback from Arial and 32 balanced lifecycle cycles passed.");
    return 0;
}
