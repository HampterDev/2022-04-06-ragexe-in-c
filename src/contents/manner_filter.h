#ifndef CONTENTS_MANNER_FILTER_H
#define CONTENTS_MANNER_FILTER_H

#include <stddef.h>

struct WordFilter {
    char **words;
    size_t count, capacity;
};

extern struct WordFilter g_nameFilter;
void name_filter_init(void);
void word_filter_clear(struct WordFilter *filter);
int word_filter_add(struct WordFilter *filter, const char *line);
int word_filter_load(struct WordFilter *filter, const char *file_name);
int word_filter_is_clean(const struct WordFilter *filter, const char *text);
int word_filter_contains(const struct WordFilter *filter, const char *text);
void word_filter_censor(const struct WordFilter *filter, char *text);
char *word_filter_lower(char *text);

#endif
