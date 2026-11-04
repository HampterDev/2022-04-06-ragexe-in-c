#ifndef _WIN32
#include "platform/language.h"
#include "core/utf8.h"
#include <string.h>

/* The IME side of platform/language.c off Windows: core/app_sdl.c hands over SDL's composition
   and committed text, both UTF-8 like the client's own text. */

void language_place_candidates(struct Language *self, int x, int top, int bottom)
{
    struct LanguageCandidatePos *pos = &self->candidate_pos;

    pos->x = x;
    pos->top = top;
    pos->bottom = bottom;
    pos->placed = 1;
}

/* SDL counts the cursor in characters; the editor needs a byte offset. A composition that does not
   fit is cut on a character boundary, as on Windows. */
void language_set_composition(struct Language *self, const char *text, int cursor)
{
    size_t length = utf8_prefix(text, strlen(text), sizeof self->composition - 1);
    const char *at = text, *end = text + length;

    memcpy(self->composition, text, length);
    self->composition[length] = 0;
    self->composition_length = length;
    if (cursor < 0) {
        at = end;
    } else {
        while (cursor-- > 0 && at < end)
            at = utf8_step(at, end);
    }
    self->composition_cursor = (int)(at - text);
    if (!length)
        self->candidate_pos.placed = 0;
}

void language_commit(struct Language *self, const char *text)
{
    self->composition[0] = 0;
    self->composition_length = 0;
    self->pending_unit = 0;
    language_replace_selection(self, text);
}
#endif
