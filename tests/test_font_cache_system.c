#include "render/fonts.h"
#include <assert.h>
#include <stdio.h>

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
        LOGFONTA description;
        assert(font && font_cache_get_font(&cache, face, 15, HANGEUL_CHARSET, 0, 0) == font);
        assert(GetObjectA(font, sizeof(description), &description) == sizeof(description));
        previous = SelectObject(dc, font);
        assert(previous && previous != HGDI_ERROR);
        assert(GetTextExtentPoint32W(dc, L"Hello \xd55c\xae00", 8, &extent) && extent.cx > 0 && extent.cy > 0);
        assert(SelectObject(dc, previous) == font);
    }
    font_cache_destruct(&cache);
    font_cache_destruct(&cache);
    assert(DeleteDC(dc));
}

int main(void)
{
    DWORD before, after;
    int i;
    cycle();
    before = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    for (i = 0; i < 32; ++i)
        cycle();
    after = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    assert(after == before);
    puts("Font cache system: real GDI selection, Korean text measurement and 32 balanced lifecycle cycles passed.");
    return 0;
}
