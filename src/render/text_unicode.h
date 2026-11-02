#ifndef RENDER_TEXT_UNICODE_H
#define RENDER_TEXT_UNICODE_H

#include "platform/win_types.h"

struct TextUnicode {
    WCHAR local[512];
    WCHAR *data;
    int length;
};

int text_unicode_init(struct TextUnicode *text, const char *bytes, int length, unsigned code_page);
void text_unicode_release(struct TextUnicode *text);

#endif
