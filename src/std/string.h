#ifndef STD_STRING_H
#define STD_STRING_H

#include <stddef.h>
#include <stdint.h>
#include "core/text_view.h"

struct String {
    char *data;
    size_t size;
    size_t capacity;
};

struct WString {
    uint16_t *data;
    size_t size;
    size_t capacity;
};

#define STRING_NPOS ((size_t)-1)
#define STRING_LITERAL(text) { (char *)(text), sizeof(text) - 1, sizeof(text) - 1 }

static __inline void string_init(struct String *s)
{
    s->data = NULL;
    s->size = 0;
    s->capacity = 0;
}

static __inline const char *string_cstr(const struct String *s)
{
    return s->data ? s->data : "";
}

static __inline struct TextView string_view(const struct String *s)
{
    return text_view(string_cstr(s), s->size);
}

static __inline void string_truncate(struct String *s, size_t size)
{
    if (size < s->size) {
        s->size = size;
        s->data[size] = 0;
    }
}

static __inline void string_clear(struct String *s)
{
    string_truncate(s, 0);
}

static __inline void string_move(struct String *dst, struct String *src)
{
    *dst = *src;
    string_init(src);
}

void string_free(struct String *s);
void string_reserve(struct String *s, size_t capacity);
void string_copy(struct String *dst, const struct String *src);
struct String string_from(const char *text, size_t len);
struct String string_from_cstr(const char *text);
void string_assign(struct String *s, const char *text, size_t len);
void string_assign_cstr(struct String *s, const char *text);
void string_append(struct String *s, const char *text, size_t len);
void string_append_cstr(struct String *s, const char *text);
void string_append_char(struct String *s, char c);
void string_insert(struct String *s, size_t pos, const char *text, size_t len);
void string_erase(struct String *s, size_t pos, size_t count);
void string_replace(struct String *s, size_t pos, size_t count, const char *text, size_t len);
void string_resize(struct String *s, size_t size, char fill);
void string_substring(struct String *dst, const struct String *src, size_t pos, size_t count);
void string_trim_left(struct String *s, const char *chars);
void string_trim_right(struct String *s, const char *chars);
void string_format(struct String *s, const char *format, ...);
void string_append_format(struct String *s, const char *format, ...);
size_t string_find(const struct String *s, const char *text, size_t pos);
size_t string_rfind(const struct String *s, const char *text);
size_t string_find_first_of(const struct String *s, const char *chars, size_t pos);
size_t string_find_first_not_of(const struct String *s, const char *chars, size_t pos);
int string_equal(const struct String *a, const struct String *b);
int string_equal_cstr(const struct String *s, const char *text);
int string_compare(const struct String *a, const struct String *b);

static __inline void wstring_init(struct WString *s)
{
    s->data = NULL;
    s->size = 0;
    s->capacity = 0;
}

static __inline const uint16_t *wstring_cstr(const struct WString *s)
{
    static const uint16_t empty = 0;
    return s->data ? s->data : &empty;
}

void wstring_free(struct WString *s);
void wstring_assign(struct WString *s, const uint16_t *text, size_t len);

static __inline size_t utf16_length(const uint16_t *text)
{
    size_t length = 0;
    while (text[length])
        ++length;
    return length;
}

static __inline size_t utf16_length_limit(const uint16_t *text, size_t limit)
{
    size_t length = 0;
    while (length < limit && text[length])
        ++length;
    return length;
}

static __inline int utf16_compare(const uint16_t *a, const uint16_t *b)
{
    while (*a && *a == *b)
        ++a, ++b;
    return (*a > *b) - (*a < *b);
}

#endif
