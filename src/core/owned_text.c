#include "core/owned_text.h"
#include <stdlib.h>
#include <string.h>

void owned_text_init(struct OwnedText *text)
{
    text->data = NULL;
    text->size = 0;
}

void owned_text_clear(struct OwnedText *text)
{
    free(text->data);
    owned_text_init(text);
}

int owned_text_assign(struct OwnedText *text, struct TextView source)
{
    char *copy = NULL;
    if (source.size == (size_t)-1)
        return 0;
    if (source.size) {
        copy = malloc(source.size + 1);
        if (!copy)
            return 0;
        memcpy(copy, source.data, source.size);
        copy[source.size] = '\0';
    }
    free(text->data);
    text->data = copy;
    text->size = source.size;
    return 1;
}

int owned_text_concat(struct OwnedText *text, struct TextView first, struct TextView second)
{
    char *copy;
    size_t size;
    if (second.size == (size_t)-1 || first.size > (size_t)-1 - second.size - 1)
        return 0;
    size = first.size + second.size;
    if (!size) {
        owned_text_clear(text);
        return 1;
    }
    copy = malloc(size + 1);
    if (!copy) return 0;
    if (first.size) memcpy(copy, first.data, first.size);
    if (second.size) memcpy(copy + first.size, second.data, second.size);
    copy[size] = '\0';
    free(text->data);
    text->data = copy;
    text->size = size;
    return 1;
}

const char *owned_text_cstr(const struct OwnedText *text)
{
    return text->data ? text->data : "";
}

struct TextView owned_text_view(const struct OwnedText *text)
{
    return text_view(owned_text_cstr(text), text->size);
}
