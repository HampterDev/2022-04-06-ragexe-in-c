#ifndef UI_MISC_THAI_LEXICON_H
#define UI_MISC_THAI_LEXICON_H

#include "platform/win_types.h"

struct ThaiLexicon;

struct ThaiBreakIterator {
    struct ThaiLexicon *lexicon;
    DWORD classes[0x100];
};

void thai_break_iterator_delete_lexicon(struct ThaiBreakIterator *self);
BYTE thai_break_iterator_initialize_lexicon(struct ThaiBreakIterator *self);

#endif
