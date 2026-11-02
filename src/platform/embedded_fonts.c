#include "platform/embedded_fonts.h"
#include "platform/t2embed_api.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static const char *const font_files[] = {
    "RixSquirrel_10.eot", "RixLoveangel_10.eot", "RixNHCgogo_10.eot",
    "RixDiary_10.eot", "RixMiniHeart_10.eot", "RixFreshman_10.eot",
    "RixKid_10.eot", "RixMagic_10.eot", "RixJJangu_10.eot",
};
#define FONT_COUNT (sizeof font_files / sizeof font_files[0])

static HMODULE font_module;
static EmbeddedFontLoadFn load_font;
static EmbeddedFontDeleteFn delete_font;
static HANDLE fonts[FONT_COUNT];
static int ready;

void embedded_fonts_clear(void)
{
    size_t i = FONT_COUNT;
    while (i--) {
        if (fonts[i]) {
            ULONG status = 0;
            delete_font(fonts[i], 0, &status);
            fonts[i] = NULL;
        }
    }
    ready = 0;
    load_font = NULL;
    delete_font = NULL;
    if (font_module) {
        FreeLibrary(font_module);
        font_module = NULL;
    }
}

static int open_font_service(void)
{
    font_module = LoadLibraryW(L"t2embed.dll");
    if (!font_module)
        return 0;
    load_font = (EmbeddedFontLoadFn)GetProcAddress(font_module, "TTLoadEmbeddedFont");
    delete_font = (EmbeddedFontDeleteFn)GetProcAddress(font_module, "TTDeleteEmbeddedFont");
    if (!load_font || !delete_font) {
        embedded_fonts_clear();
        return 0;
    }
    return 1;
}

static ULONG read_font(void *stream, void *buffer, ULONG size)
{
    return (ULONG)fread(buffer, 1, size, stream);
}

static int load_one_font(size_t index)
{
    char family[LF_FACESIZE];
    wchar_t wide_family[LF_FACESIZE], path[64];
    wchar_t url[] = L"http://www.ragnarok.co.kr";
    TTLOADINFO info = {sizeof info, sizeof url / sizeof url[0], url};
    ULONG privileges = 2, status = 1;
    HANDLE font = NULL;
    FILE *file;
    LONG result;
    int length;

    if (strlen(font_files[index]) >= sizeof family)
        return 1;
    strcpy(family, font_files[index]);
    if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, family, -1,
                            wide_family, sizeof wide_family / sizeof wide_family[0]))
        return 1;
    length = swprintf(path, sizeof path / sizeof path[0], L"System\\Font\\%ls", wide_family);
    if (length < 0 || (size_t)length >= sizeof path / sizeof path[0])
        return 1;
    file = _wfopen(path, L"rb");
    if (!file)
        return 0;
    result = load_font(&font, TTLOAD_PRIVATE, &privileges, LICENSE_EDITABLE,
                       &status, read_font, file, wide_family, family, &info);
    fclose(file);
    if (result == E_NONE && font)
        fonts[index] = font;
    return 1;
}

int embedded_fonts_load(const char **missing_file)
{
    size_t i;
    if (missing_file)
        *missing_file = NULL;
    if (ready)
        return 1;
    if (!open_font_service())
        return 0;
    for (i = 0; i < FONT_COUNT; ++i) {
        if (!load_one_font(i)) {
            if (missing_file)
                *missing_file = font_files[i];
            embedded_fonts_clear();
            return 0;
        }
    }
    ready = 1;
    return 1;
}
