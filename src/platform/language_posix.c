#ifndef _WIN32
#include "platform/language.h"
#include <string.h>

const char *language_candidate_text(struct Language *self, int index)
{
    (void)index;
    self->candidate_text[0] = 0;
    return self->candidate_text;
}

void language_read_candidates(struct Language *self)
{
    self->candidate_count = self->candidate_selection = self->candidate_page = 0;
}

void language_read_composition(struct Language *self)
{
    (void)self;
}

void language_set_composition(struct Language *self, const char *text, int cursor)
{
    size_t length = strlen(text);
    if (length >= sizeof self->composition)
        length = sizeof self->composition - 1;
    memcpy(self->composition, text, length);
    self->composition[length] = 0;
    self->composition_length = length;
    self->composition_cursor = cursor;
}

void language_commit(struct Language *self, const char *text)
{
    self->composition[0] = 0;
    self->composition_length = 0;
    language_replace_selection(self, text);
}
#endif
