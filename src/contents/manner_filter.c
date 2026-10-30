#include "platform/language.h"
#include <windows.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "contents/manner_filter.h"

#define STACK_COPY_MAX 1024

static const char *next_char(const char *p)
{
    if (!*p || !p[1])
        return p + (*p != 0);
    return CharNextExA((WORD)g_codePage, p, 0);
}

char *word_filter_lower(char *text)
{
    char *p = text;

    while (*p) {
        if (*p > 0 && isupper(*p))
            *p += 'a' - 'A';
        p = (char *)next_char(p);
    }
    return text;
}

static const char *skip_item_link(const char *cur)
{
    static const char open[] = "<iteml>", close[] = "</iteml>";
    const char *end;
    if (strncmp(cur, open, sizeof open - 1) != 0)
        return cur;
    end = strstr(cur + sizeof open - 1, close);
    return end ? end + sizeof close - 1 : cur;
}

static const char *word_at(const struct WordFilter *filter, const char *cur)
{
    size_t i;
    for (i = 0; i < filter->count; i++) {
        size_t bytes = strlen(filter->words[i]);
        if (!strncmp(filter->words[i], cur, bytes)) {
            const char *end = cur + bytes, *p = cur;
            while (p < end)
                p = next_char(p);
            if (p == end)
                return filter->words[i];
        }
    }
    return NULL;
}

int word_filter_is_clean(const struct WordFilter *filter, const char *text)
{
    size_t size = strlen(text) + 1;
    char stack_copy[STACK_COPY_MAX];
    char *heap = size > sizeof stack_copy ? malloc(size) : NULL;
    char *copy = size > sizeof stack_copy ? heap : stack_copy;
    const char *cur, *end;
    if (!copy)
        return 0;
    memcpy(copy, text, size);
    cur = word_filter_lower(copy);
    end = cur + strlen(cur);
    for (; cur < end; cur = next_char(cur)) {
        cur = skip_item_link(cur);
        if (word_at(filter, cur)) {
            free(heap);
            return 0;
        }
    }
    free(heap);
    return 1;
}

int word_filter_contains(const struct WordFilter *filter, const char *text)
{
    return !word_filter_is_clean(filter, text);
}

void word_filter_censor(const struct WordFilter *filter, char *text)
{
    size_t size = strlen(text) + 1;
    char stack_copy[STACK_COPY_MAX];
    char *heap = size > sizeof stack_copy ? malloc(size) : NULL;
    char *copy = size > sizeof stack_copy ? heap : stack_copy;
    const char *cur;
    size_t removed = 0;
    if (!copy) {
        text[0] = 0;
        return;
    }
    memcpy(copy, text, size);
    cur = word_filter_lower(copy);
    while (*cur) {
        const char *word;
        cur = skip_item_link(cur);
        word = word_at(filter, cur);
        if (word) {
            const char *p;
            size_t bytes = strlen(word), stars = 0;
            char *match = text + (size_t)(cur - copy) - removed;
            for (p = word; *p; p = next_char(p))
                stars++;
            word_filter_lower(text);
            memmove(match + stars, match + bytes, strlen(match + bytes) + 1);
            memset(match, '*', stars);
            removed += bytes - stars;
            cur += bytes;
        } else {
            cur = next_char(cur);
        }
    }
    free(heap);
}
