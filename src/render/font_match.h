#ifndef RENDER_FONT_MATCH_H
#define RENDER_FONT_MATCH_H

/* The installed fonts, sorted for a request (render/font_faces.h), for render/fonts_ft.c. On Windows
   and Android, which have no Fontconfig, and with RAGE_FONT_SCAN, the finder reads them itself
   (render/font_match_scan.c): on Windows the fonts the registry lists for the machine and for the
   user, the fonts GDI has; elsewhere the faces in the system's font folders; with RAGE_FONT_DIRS
   the faces in those folders instead. Otherwise Fontconfig finds them (render/font_match_fc.c). */
#if (defined(__ANDROID__) || defined(_WIN32)) && !defined(RAGE_FONT_SCAN)
#define RAGE_FONT_SCAN 1
#endif

#include "render/font_faces.h"

struct FontMatches;

/* Reads the installed fonts, once until font_match_shutdown. A face answers to its family name in
   English and in display_language, a Windows language ID; 0 is the system's display language (on
   Windows, the user's interface language; elsewhere none but English). */
void font_match_init(unsigned display_language);
void font_match_shutdown(void);

/* Every installed face, best first for the request (font_faces_sort); NULL when there is none. */
struct FontMatches *font_match_sort(const struct FontRequest *request);
int font_match_count(const struct FontMatches *matches);
/* The file and the face in it of match i. */
const char *font_match_file(const struct FontMatches *matches, int i, int *face_index);
/* Which of the request's families match i answers to (its index in them), or -1. */
int font_match_family(const struct FontMatches *matches, int i);
int font_match_covers_language(const struct FontMatches *matches, int i, const char *lang);
int font_match_has_char(struct FontMatches *matches, int i, unsigned code);
/* Whether match i is itself bold (at least demibold) and itself slanted. */
void font_match_style(const struct FontMatches *matches, int i, int *bold, int *slanted);
void font_match_free(struct FontMatches *matches);

#endif
