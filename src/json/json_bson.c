#include <stdlib.h>
#include <string.h>
#include "json/json_tree.h"
#include <stdint.h>

enum {
    BSON_MAX_NESTING_DEPTH = 1024,
};

struct BsonSource {
    const unsigned char *at, *end;
    int depth;
};

static unsigned trailing_bytes(unsigned char c)
{
    if (c < 0xc0)
        return 0;
    if (c < 0xe0)
        return 1;
    if (c < 0xf0)
        return 2;
    if (c < 0xf8)
        return 3;
    if (c < 0xfc)
        return 4;
    return 5;
}

static int legal_utf8(const unsigned char *first, unsigned length)
{
    const unsigned char *src = first + length;
    unsigned char a = 0;

    switch (length) {
    default:
        return 0;
    case 4:
        if (((a = *--src) & 0xc0) != 0x80)
            return 0;
    case 3:
        if (((a = *--src) & 0xc0) != 0x80)
            return 0;
    case 2:
        if (((a = *--src) & 0xc0) != 0x80)
            return 0;
        switch (*first) {
        case 0xe0: if (a < 0xa0) return 0; break;
        case 0xed: if (a > 0x9f) return 0; break;
        case 0xf0: if (a < 0x90) return 0; break;
        case 0xf4: if (a > 0x8f) return 0; break;
        default:   if (a < 0x80) return 0;
        }
    case 1:
        if (*first >= 0x80 && *first < 0xc2)
            return 0;
        break;
    }
    if (*first > 0xf4)
        return 0;
    return 1;
}

static int valid_utf8(const unsigned char *first, size_t n)
{
    const unsigned char *last = first + n;

    while (first != last) {
        unsigned length = trailing_bytes(*first) + 1;

        if (length > (size_t)(last - first))
            return 0;
        if (!legal_utf8(first, length))
            return 0;
        first += length;
    }
    return 1;
}

static int read_bytes(struct BsonSource *src, void *dst, size_t n)
{
    if ((size_t)(src->end - src->at) < n)
        return 0;
    memcpy(dst, src->at, n);
    src->at += n;
    return 1;
}

static int read_byte(struct BsonSource *src, unsigned char *c)
{
    return read_bytes(src, c, 1);
}

static int read_int32(struct BsonSource *src, int *v)
{
    return read_bytes(src, v, 4);
}

static int read_int64(struct BsonSource *src, int64_t *v)
{
    return read_bytes(src, v, 8);
}

static void read_name(struct BsonSource *src, const unsigned char **name, size_t *length)
{
    const unsigned char *first = src->at;

    while (src->at != src->end && *src->at != 0)
        src->at++;
    *name = first;
    *length = (size_t)(src->at - first);
    if (src->at != src->end)
        src->at++;
}

static char *copy_text(const unsigned char *text, size_t length)
{
    char *copy = (char *)malloc(length + 1);

    if (copy) {
        memcpy(copy, text, length);
        copy[length] = 0;
    }
    return copy;
}

static int parse_container(struct BsonSource *src, struct JsonTree *out, int is_array);

static int parse_value(struct BsonSource *src, unsigned char type, struct JsonTree *out)
{
    int length;
    unsigned char c;

    json_tree_init(out);
    switch (type) {
    case 0x01:
        if (!read_bytes(src, &out->u.dbl, 8))
            return 0;
        out->type = JSON_TREE_DOUBLE;
        return 1;
    case 0x02:
        if (!read_int32(src, &length) || length < 1)
            return 0;
        if ((size_t)(src->end - src->at) < (size_t)length - 1)
            return 0;
        {
            const unsigned char *text = src->at;
            size_t count = (size_t)length - 1;

            src->at += count;
            if (!read_byte(src, &c))
                return 0;
            if (!valid_utf8(text, count))
                return 0;
            out->u.text = copy_text(text, count);
            if (!out->u.text)
                return 0;
            out->count = count;
            out->type = JSON_TREE_STRING;
        }
        return 1;
    case 0x03:
        return parse_container(src, out, 0);
    case 0x04:
        return parse_container(src, out, 1);
    case 0x05:
        if (!read_int32(src, &length) || length < 0)
            return 0;
        if (!read_byte(src, &out->subtype))
            return 0;
        if ((size_t)(src->end - src->at) < (size_t)length)
            return 0;
        out->u.bytes = (unsigned char *)malloc(length ? (size_t)length : 1);
        if (!out->u.bytes)
            return 0;
        memcpy(out->u.bytes, src->at, (size_t)length);
        src->at += length;
        out->count = (size_t)length;
        out->type = JSON_TREE_BYTES;
        return 1;
    case 0x08:
        if (!read_byte(src, &c))
            return 0;
        out->boolean = c != 0;
        out->type = JSON_TREE_BOOL;
        return 1;
    case 0x09:
        if (!read_int64(src, &out->u.i64))
            return 0;
        out->type = JSON_TREE_INT64;
        out->tag = JSON_TREE_TAG_EPOCH_MILLI;
        return 1;
    case 0x0a:
        return 1;
    case 0x10:
        if (!read_int32(src, &length))
            return 0;
        out->u.i64 = length;
        out->type = JSON_TREE_INT64;
        return 1;
    case 0x11:
        if (!read_bytes(src, &out->u.u64, 8))
            return 0;
        out->type = JSON_TREE_UINT64;
        return 1;
    case 0x12:
        if (!read_int64(src, &out->u.i64))
            return 0;
        out->type = JSON_TREE_INT64;
        return 1;
    default:
        return 0;
    }
}

static void *grow(void *items, size_t count, size_t *capacity, size_t size)
{
    if (count < *capacity)
        return items;
    *capacity = *capacity ? *capacity * 2 : 8;
    return realloc(items, *capacity * size);
}

static int parse_container(struct BsonSource *src, struct JsonTree *out, int is_array)
{
    size_t capacity = 0;
    int length;

    json_tree_init(out);
    if (++src->depth > BSON_MAX_NESTING_DEPTH)
        return 0;
    if (!read_int32(src, &length))
        return 0;
    out->type = (unsigned char)(is_array ? JSON_TREE_ARRAY : JSON_TREE_OBJECT);
    for (;;) {
        unsigned char type;
        const unsigned char *name;
        size_t name_length;

        if (!read_byte(src, &type))
            return 0;
        if (type == 0)
            break;
        read_name(src, &name, &name_length);
        if (is_array) {
            struct JsonTree *items = (struct JsonTree *)grow(out->u.items, out->count, &capacity, sizeof(*items));

            if (!items)
                return 0;
            out->u.items = items;
            if (!parse_value(src, type, &items[out->count])) {
                out->count++;
                return 0;
            }
            out->count++;
        } else {
            struct JsonTreeMember *members;
            struct JsonTreeMember *m;

            if (!valid_utf8(name, name_length))
                return 0;
            members = (struct JsonTreeMember *)grow(out->u.members, out->count, &capacity, sizeof(*members));
            if (!members)
                return 0;
            out->u.members = members;
            m = &members[out->count];
            m->key = copy_text(name, name_length);
            m->key_length = name_length;
            json_tree_init(&m->value);
            out->count++;
            if (!m->key)
                return 0;
            if (!parse_value(src, type, &m->value))
                return 0;
        }
    }
    src->depth--;
    if (!is_array)
        return json_tree_sort_members(out);
    return 1;
}

int json_tree_from_bson(struct JsonTree *out, const unsigned char *data, size_t size)
{
    struct BsonSource src;

    src.at = data;
    src.end = data + size;
    src.depth = 0;
    if (!parse_container(&src, out, 0)) {
        json_tree_free(out);
        return 0;
    }
    return 1;
}
