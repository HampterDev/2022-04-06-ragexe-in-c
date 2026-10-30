#ifndef CORE_TEXT_VIEW_H
#define CORE_TEXT_VIEW_H

#include <stddef.h>
#include <string.h>

struct TextView {
    const char *data;
    size_t size;
};

static __inline struct TextView text_view(const char *data, size_t size)
{
    struct TextView view = {data, size};
    return view;
}

static __inline struct TextView text_view_cstr(const char *text)
{
    return text_view(text, strlen(text));
}

static __inline size_t text_view_find(struct TextView text, struct TextView needle, size_t start)
{
    size_t i;

    if (needle.size > text.size || start > text.size - needle.size)
        return (size_t)-1;
    if (!needle.size)
        return start;
    for (i = start; i <= text.size - needle.size; ++i)
        if (text.data[i] == needle.data[0] && !memcmp(text.data + i, needle.data, needle.size))
            return i;
    return (size_t)-1;
}

#endif
