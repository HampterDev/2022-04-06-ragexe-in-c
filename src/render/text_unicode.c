#include "render/text_unicode.h"
#include <stdlib.h>

int text_unicode_init(struct TextUnicode *text, const char *bytes, int length, unsigned code_page)
{
    text->data = NULL;
    text->length = 0;
    if (length < 0 || (!bytes && length))
        return 0;
    if (!length)
        return 1;
    if ((size_t)length > (size_t)-1 / sizeof(WCHAR))
        return 0;
    text->data = length <= (int)(sizeof(text->local) / sizeof(text->local[0]))
        ? text->local : malloc((size_t)length * sizeof(WCHAR));
    if (!text->data)
        return 0;
    text->length = MultiByteToWideChar(code_page, 0, bytes, length, text->data, length);
    if (!text->length) {
        text_unicode_release(text);
        return 0;
    }
    return 1;
}

void text_unicode_release(struct TextUnicode *text)
{
    if (text->data != text->local)
        free(text->data);
    text->data = NULL;
    text->length = 0;
}
