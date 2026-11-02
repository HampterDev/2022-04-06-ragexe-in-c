#include "std/string.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/fatal.h"
#include "core/memory.h"

#define STRING_MAX 0x7fffffffu
#define WSTRING_MAX 0x7ffffffeu
#define STRING_MIN_CAPACITY 15

static size_t grown_capacity(size_t requested, size_t old_capacity, size_t max)
{
    size_t capacity;

    if (old_capacity > max - old_capacity / 2)
        return max;
    capacity = old_capacity + old_capacity / 2;
    if (capacity < requested)
        capacity = requested;
    if (capacity < STRING_MIN_CAPACITY)
        capacity = STRING_MIN_CAPACITY;
    return capacity;
}

static char *string_open(struct String *s, size_t pos, size_t count, size_t len)
{
    size_t size = s->size, tail, new_size;

    if (pos > size)
        fatal_error("invalid string position");
    if (count > size - pos)
        count = size - pos;
    tail = size - pos - count;
    if (STRING_MAX - (size - count) < len)
        fatal_error("string too long");
    new_size = size - count + len;
    if (new_size > s->capacity) {
        size_t capacity = grown_capacity(new_size, s->capacity, STRING_MAX);
        char *block = mem_alloc(capacity + 1);

        if (s->data) {
            memcpy(block, s->data, pos);
            memcpy(block + pos + len, s->data + pos + count, tail);
            mem_free(s->data);
        }
        s->data = block;
        s->capacity = capacity;
    } else if (s->data) {
        memmove(s->data + pos + len, s->data + pos + count, tail);
    } else {
        return NULL;
    }
    s->size = new_size;
    s->data[new_size] = 0;
    return s->data + pos;
}

void string_free(struct String *s)
{
    if (s->data)
        mem_free(s->data);
    string_init(s);
}

void string_reserve(struct String *s, size_t capacity)
{
    char *block;

    if (capacity <= s->capacity)
        return;
    if (capacity > STRING_MAX)
        fatal_error("string too long");
    block = mem_alloc(capacity + 1);
    if (s->data) {
        memcpy(block, s->data, s->size + 1);
        mem_free(s->data);
    } else {
        block[0] = 0;
    }
    s->data = block;
    s->capacity = capacity;
}

void string_replace(struct String *s, size_t pos, size_t count, const char *text, size_t len)
{
    char *hole;

    if (len && s->data && text >= s->data && text <= s->data + s->size) {
        struct String copy;

        string_init(&copy);
        string_assign(&copy, text, len);
        string_replace(s, pos, count, copy.data, len);
        string_free(&copy);
        return;
    }
    hole = string_open(s, pos, count, len);
    if (len)
        memcpy(hole, text, len);
}

void string_copy(struct String *dst, const struct String *src)
{
    string_init(dst);
    string_assign(dst, src->data, src->size);
}

struct String string_from(const char *text, size_t len)
{
    struct String s;

    string_init(&s);
    string_assign(&s, text, len);
    return s;
}

struct String string_from_cstr(const char *text)
{
    return string_from(text, strlen(text));
}

void string_assign(struct String *s, const char *text, size_t len)
{
    string_replace(s, 0, s->size, text, len);
}

void string_assign_cstr(struct String *s, const char *text)
{
    string_assign(s, text, strlen(text));
}

void string_append(struct String *s, const char *text, size_t len)
{
    string_replace(s, s->size, 0, text, len);
}

void string_append_cstr(struct String *s, const char *text)
{
    string_append(s, text, strlen(text));
}

void string_append_char(struct String *s, char c)
{
    string_append(s, &c, 1);
}

void string_insert(struct String *s, size_t pos, const char *text, size_t len)
{
    string_replace(s, pos, 0, text, len);
}

void string_erase(struct String *s, size_t pos, size_t count)
{
    string_open(s, pos, count, 0);
}

void string_resize(struct String *s, size_t size, char fill)
{
    if (size > s->size) {
        size_t count = size - s->size;

        memset(string_open(s, s->size, 0, count), fill, count);
    } else {
        string_truncate(s, size);
    }
}

void string_substring(struct String *dst, const struct String *src, size_t pos, size_t count)
{
    if (src->size < pos)
        fatal_error("invalid string position");
    if (count > src->size - pos)
        count = src->size - pos;
    string_assign(dst, string_cstr(src) + pos, count);
}

static void char_table(unsigned char table[256], const char *chars)
{
    size_t n = strlen(chars), i;

    memset(table, 0, 256);
    for (i = 0; i < n; i++)
        table[(unsigned char)chars[i]] = 1;
}

size_t string_find_first_of(const struct String *s, const char *chars, size_t pos)
{
    unsigned char table[256];
    const char *data = string_cstr(s), *p;

    if (!*chars || pos >= s->size)
        return STRING_NPOS;
    char_table(table, chars);
    for (p = data + pos; p < data + s->size; p++)
        if (table[(unsigned char)*p])
            return (size_t)(p - data);
    return STRING_NPOS;
}

size_t string_find_first_not_of(const struct String *s, const char *chars, size_t pos)
{
    unsigned char table[256];
    const char *data = string_cstr(s), *p;

    if (pos >= s->size)
        return STRING_NPOS;
    char_table(table, chars);
    for (p = data + pos; p < data + s->size; p++)
        if (!table[(unsigned char)*p])
            return (size_t)(p - data);
    return STRING_NPOS;
}

void string_trim_left(struct String *s, const char *chars)
{
    size_t first;

    if (!s->size)
        return;
    first = string_find_first_not_of(s, chars, 0);
    if (first)
        string_erase(s, 0, first);
}

void string_trim_right(struct String *s, const char *chars)
{
    unsigned char table[256];
    const char *data, *p;
    size_t keep;

    if (!s->size)
        return;
    data = string_cstr(s);
    char_table(table, chars);
    p = data + s->size - 1;
    if (table[(unsigned char)*p]) {
        for (;;) {
            if (p == data) {
                p = NULL;
                break;
            }
            if (!table[(unsigned char)*--p])
                break;
        }
    }
    keep = p ? (size_t)(p - data) + 1 : 0;
    if (keep < s->size)
        string_erase(s, keep, s->size - keep);
}

static void formatted(struct String *out, const char *format, va_list args)
{
    va_list measure;
    int size;

    va_copy(measure, args);
    size = vsnprintf(NULL, 0, format, measure);
    va_end(measure);
    if (size < 0)
        fatal_error("invalid format");
    string_init(out);
    if (!size)
        return;
    string_reserve(out, (size_t)size);
    vsnprintf(out->data, (size_t)size + 1, format, args);
    out->size = (size_t)size;
}

void string_format(struct String *s, const char *format, ...)
{
    struct String text;
    va_list args;

    va_start(args, format);
    formatted(&text, format, args);
    va_end(args);
    string_free(s);
    *s = text;
}

void string_append_format(struct String *s, const char *format, ...)
{
    struct String text;
    va_list args;

    va_start(args, format);
    formatted(&text, format, args);
    va_end(args);
    string_append(s, text.data, text.size);
    string_free(&text);
}

size_t string_find(const struct String *s, const char *text, size_t pos)
{
    const char *data = string_cstr(s);
    size_t len = strlen(text);
    const char *end, *hit;

    if (len > s->size || pos > s->size - len)
        return STRING_NPOS;
    if (len == 0)
        return pos;
    end = data + (s->size - len) + 1;
    for (hit = memchr(data + pos, text[0], end - (data + pos)); hit;
         hit = memchr(hit + 1, text[0], end - (hit + 1)))
        if (memcmp(hit, text, len) == 0)
            return (size_t)(hit - data);
    return STRING_NPOS;
}

size_t string_rfind(const struct String *s, const char *text)
{
    size_t len = strlen(text);
    const char *data = string_cstr(s);
    const char *at;

    if (len == 0)
        return s->size;
    if (len > s->size)
        return STRING_NPOS;
    at = data + (s->size - len);
    for (;;) {
        if (*at == *text && memcmp(at, text, len) == 0)
            return (size_t)(at - data);
        if (at == data)
            return STRING_NPOS;
        at--;
    }
}

int string_equal(const struct String *a, const struct String *b)
{
    return a->size == b->size && memcmp(string_cstr(a), string_cstr(b), a->size) == 0;
}

int string_equal_cstr(const struct String *s, const char *text)
{
    size_t len = strlen(text);

    return s->size == len && memcmp(string_cstr(s), text, len) == 0;
}

int string_compare(const struct String *a, const struct String *b)
{
    size_t n = a->size < b->size ? a->size : b->size;
    int cmp = memcmp(string_cstr(a), string_cstr(b), n);

    if (cmp)
        return cmp < 0 ? -1 : 1;
    return a->size < b->size ? -1 : a->size > b->size;
}

void wstring_free(struct WString *s)
{
    if (s->data)
        mem_free(s->data);
    wstring_init(s);
}

void wstring_assign(struct WString *s, const uint16_t *text, size_t len)
{
    size_t capacity;
    uint16_t *block;

    if (s->data && len <= s->capacity) {
        memmove(s->data, text, len * sizeof *text);
        s->data[len] = 0;
        s->size = len;
        return;
    }
    if (!len) {
        s->size = 0;
        return;
    }
    if (len > WSTRING_MAX)
        fatal_error("string too long");
    capacity = grown_capacity(len, s->capacity, WSTRING_MAX);
    block = mem_alloc((capacity + 1) * sizeof *block);
    memcpy(block, text, len * sizeof *text);
    block[len] = 0;
    if (s->data)
        mem_free(s->data);
    s->data = block;
    s->size = len;
    s->capacity = capacity;
}
