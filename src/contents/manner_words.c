#include "contents/manner_filter.h"
#include "core/table_reader.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct WordFilter g_nameFilter;

void word_filter_clear(struct WordFilter *filter)
{
    size_t i;
    for (i = 0; i < filter->count; i++)
        free(filter->words[i]);
    free(filter->words);
    filter->words = NULL;
    filter->count = filter->capacity = 0;
}

static void name_filter_destroy(void)
{
    word_filter_clear(&g_nameFilter);
}

void name_filter_init(void)
{
    atexit(name_filter_destroy);
}

int word_filter_add(struct WordFilter *filter, const char *line)
{
    const char *start = line;
    char *word;
    size_t length;
    while (*start == ' ' || *start == '\t')
        start++;
    if (!*start)
        return 1;
    length = strlen(line);
    if (length == SIZE_MAX)
        return 0;
    word = malloc(length + 1);
    if (!word)
        return 0;
    memcpy(word, line, length + 1);
    word_filter_lower(word);
    if (filter->count == filter->capacity) {
        size_t capacity;
        char **words;
        if (filter->capacity > SIZE_MAX / 2) {
            free(word);
            return 0;
        }
        capacity = filter->capacity ? filter->capacity * 2 : 8;
        if (capacity > SIZE_MAX / sizeof *words) {
            free(word);
            return 0;
        }
        words = realloc(filter->words, capacity * sizeof *words);
        if (!words) {
            free(word);
            return 0;
        }
        filter->words = words;
        filter->capacity = capacity;
    }
    filter->words[filter->count++] = word;
    return 1;
}

int word_filter_load(struct WordFilter *filter, const char *file_name)
{
    struct TableReader reader = {0};
    struct WordFilter next = {0};
    size_t i;
    int ok = table_reader_read(&reader, file_name, TABLE_FIELDS_AND_LINES);
    if (ok) {
        for (i = 0; i < reader.count; i++) {
            if (!word_filter_add(&next, reader.fields[i])) {
                ok = 0;
                break;
            }
        }
    }
    table_reader_release(&reader);
    if (ok) {
        word_filter_clear(filter);
        *filter = next;
    } else {
        word_filter_clear(&next);
    }
    return ok;
}
