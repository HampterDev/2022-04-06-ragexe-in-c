#ifndef RENDER_TEXT_RUNS_H
#define RENDER_TEXT_RUNS_H

#include <stddef.h>

/* Scripts that may need a fallback font. Everything else, including invalid bytes,
   is drawn in the selected face. */
enum TextScript {
    TEXT_SCRIPT_DEFAULT,
    TEXT_SCRIPT_HANGUL,
    TEXT_SCRIPT_HAN,
    TEXT_SCRIPT_KANA,
    TEXT_SCRIPT_THAI,
    TEXT_SCRIPT_SYMBOL,
    TEXT_SCRIPT_EMOJI,
    TEXT_SCRIPT_COUNT
};

/* Returned by text_script_of for characters that join the run around them: the space,
   combining marks, zero-width joiners and variation selectors. */
#define TEXT_SCRIPT_COMMON (-1)

int text_script_of(unsigned long code_point);

/* Bytes of the UTF-8 run at the start of text whose characters share one script, and
   that script. Common characters join the run; a run of only common characters is
   TEXT_SCRIPT_DEFAULT. Returns 0 when length is 0. */
size_t text_script_run(const char *text, size_t length, enum TextScript *script);

#endif
