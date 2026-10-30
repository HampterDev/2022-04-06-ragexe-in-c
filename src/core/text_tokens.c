#include "core/text_util.h"
#include <ctype.h>

static int token_space(unsigned char c)
{
    return c < 128 && isspace(c);
}

static void split_span(struct Vec *tokens, const char *text, size_t size)
{
    size_t at = 0;

    string_vec_clear(tokens);
    while (at < size) {
        size_t start;

        while (at < size && token_space((unsigned char)text[at]))
            ++at;
        start = at;
        while (at < size && !token_space((unsigned char)text[at])) {
            if (text[at] == '/' && at + 1 < size && text[at + 1] == '/')
                return;
            ++at;
        }
        if (at == start)
            break;
        string_vec_push_text(tokens, text + start, at - start);
    }
}

void tokens_split(struct Vec *tokens, const struct String *line)
{
    split_span(tokens, string_cstr(line), line->size);
}
