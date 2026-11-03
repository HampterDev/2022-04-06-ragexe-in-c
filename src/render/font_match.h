#ifndef RENDER_FONT_MATCH_H
#define RENDER_FONT_MATCH_H

/* The installed fonts that can stand in for a Windows font, best first, for fonts_ft.c. Fontconfig
   finds them (font_match_fc.c); on Android, which has no Fontconfig, or with RAGE_FONT_SCAN, a
   scan of the font folders does (font_match_scan.c). */
#if defined(__ANDROID__) && !defined(RAGE_FONT_SCAN)
#define RAGE_FONT_SCAN 1
#endif

struct FontMatches;

void font_match_init(void);

/* The fonts for a family (or NULL) and its fallback families, for text in a language ("ko",
   "zh-cn", ...), in a weight and slant; NULL when there are none. */
struct FontMatches *font_match_sort(const char *family, const char *const *fallbacks, const char *lang, int bold,
                                    int italic);
int font_match_count(const struct FontMatches *matches);
/* The file and the face in it of match i. */
const char *font_match_file(const struct FontMatches *matches, int i, int *face_index);
int font_match_covers_language(const struct FontMatches *matches, int i, const char *lang);
int font_match_has_char(struct FontMatches *matches, int i, unsigned code);
/* Whether match i is itself bold (at least demibold) and itself slanted. */
void font_match_style(const struct FontMatches *matches, int i, int *bold, int *slanted);
void font_match_free(struct FontMatches *matches);

#endif
