#include "render/font_match.h"
#ifndef RAGE_FONT_SCAN
#include "core/cstr.h"
#include <fontconfig/fontconfig.h>
#include <stdlib.h>

/* Fontconfig sorts the installed fonts for a request. A font answers to every family name it has,
   in any language, as Fontconfig matches them; a fixed-pitch request asks for monospace spacing and,
   after its families, the generic monospace family. */
struct FontMatches {
    FcFontSet *set;
    int family[];
};

void font_match_init(unsigned display_language)
{
    (void)display_language;
    FcInit();
}

void font_match_shutdown(void)
{
    FcFini();
}

/* Which of families pattern answers to, by any of its family names, or -1. */
static int asked_family(FcPattern *pattern, const char *const *families)
{
    int best = -1, n, i;
    FcChar8 *name;
    for (n = 0; FcPatternGetString(pattern, FC_FAMILY, n, &name) == FcResultMatch; ++n)
        for (i = 0; families && families[i] && (best < 0 || i < best); ++i)
            if (!cstr_compare_nocase((const char *)name, families[i]))
                best = i;
    return best;
}

struct FontMatches *font_match_sort(const struct FontRequest *request)
{
    FcPattern *pattern = FcPatternCreate();
    struct FontMatches *matches;
    FcFontSet *set;
    FcResult result;
    const char *const *name;
    int i;
    if (!pattern)
        return NULL;
    for (name = request->families; name && *name; ++name)
        FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *)*name);
    if (request->fixed_pitch) {
        FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *)"monospace");
        FcPatternAddInteger(pattern, FC_SPACING, FC_MONO);
    }
    if (request->lang)
        FcPatternAddString(pattern, FC_LANG, (const FcChar8 *)request->lang);
    FcPatternAddInteger(pattern, FC_WEIGHT, request->bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
    FcPatternAddInteger(pattern, FC_SLANT, request->italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    set = FcFontSort(NULL, pattern, FcTrue, NULL, &result);
    FcPatternDestroy(pattern);
    if (set && !set->nfont) {
        FcFontSetDestroy(set);
        set = NULL;
    }
    if (!set || !(matches = malloc(sizeof *matches + (size_t)set->nfont * sizeof matches->family[0]))) {
        if (set)
            FcFontSetDestroy(set);
        return NULL;
    }
    matches->set = set;
    for (i = 0; i < set->nfont; ++i)
        matches->family[i] = asked_family(set->fonts[i], request->families);
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

int font_match_family(const struct FontMatches *matches, int i)
{
    return matches->family[i];
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
