#include "render/font_match.h"
#if !defined(_WIN32) && !defined(RAGE_FONT_SCAN)
#include <fontconfig/fontconfig.h>
#include <stdlib.h>

struct FontMatches {
    FcFontSet *set;
};

void font_match_init(void)
{
    FcInit();
}

struct FontMatches *font_match_sort(const char *family, const char *const *fallbacks, const char *lang, int bold,
                                    int italic)
{
    FcPattern *pattern = FcPatternCreate();
    struct FontMatches *matches;
    FcFontSet *set;
    FcResult result;
    const char *const *name;
    if (!pattern)
        return NULL;
    if (family)
        FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *)family);
    for (name = fallbacks; name && *name; ++name)
        FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *)*name);
    FcPatternAddString(pattern, FC_LANG, (const FcChar8 *)lang);
    FcPatternAddInteger(pattern, FC_WEIGHT, bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
    FcPatternAddInteger(pattern, FC_SLANT, italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    set = FcFontSort(NULL, pattern, FcTrue, NULL, &result);
    FcPatternDestroy(pattern);
    if (set && !set->nfont) {
        FcFontSetDestroy(set);
        set = NULL;
    }
    if (!set || !(matches = malloc(sizeof *matches))) {
        if (set)
            FcFontSetDestroy(set);
        return NULL;
    }
    matches->set = set;
    return matches;
}

int font_match_count(const struct FontMatches *matches)
{
    return matches->set->nfont;
}

const char *font_match_file(const struct FontMatches *matches, int i, int *face_index)
{
    FcChar8 *file;
    *face_index = 0;
    if (FcPatternGetString(matches->set->fonts[i], FC_FILE, 0, &file) != FcResultMatch)
        return NULL;
    FcPatternGetInteger(matches->set->fonts[i], FC_INDEX, 0, face_index);
    return (const char *)file;
}

int font_match_covers_language(const struct FontMatches *matches, int i, const char *lang)
{
    FcLangSet *langs;
    return FcPatternGetLangSet(matches->set->fonts[i], FC_LANG, 0, &langs) == FcResultMatch
        && FcLangSetHasLang(langs, (const FcChar8 *)lang) != FcLangDifferentLang;
}

int font_match_has_char(struct FontMatches *matches, int i, unsigned code)
{
    FcCharSet *chars;
    return FcPatternGetCharSet(matches->set->fonts[i], FC_CHARSET, 0, &chars) == FcResultMatch
        && FcCharSetHasChar(chars, code);
}

void font_match_style(const struct FontMatches *matches, int i, int *bold, int *slanted)
{
    int weight = FC_WEIGHT_REGULAR, slant = FC_SLANT_ROMAN;
    FcPatternGetInteger(matches->set->fonts[i], FC_WEIGHT, 0, &weight);
    FcPatternGetInteger(matches->set->fonts[i], FC_SLANT, 0, &slant);
    *bold = weight >= FC_WEIGHT_DEMIBOLD;
    *slanted = slant != FC_SLANT_ROMAN;
}

void font_match_free(struct FontMatches *matches)
{
    if (!matches)
        return;
    FcFontSetDestroy(matches->set);
    free(matches);
}
#endif
