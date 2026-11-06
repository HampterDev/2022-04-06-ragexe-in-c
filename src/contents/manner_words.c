#include "contents/manner_filter.h"
#include <stdlib.h>

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
