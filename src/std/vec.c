#include "std/vec.h"
#include <string.h>
#include "core/fatal.h"
#include "core/memory.h"

static size_t grown_capacity(const struct Vec *v, size_t needed, size_t size)
{
    size_t capacity = vec_capacity(v, size);
    size_t max = (size_t)-1 / size;

    if (needed > max)
        fatal_error("vector too long");
    if (capacity > max - capacity / 2)
        return needed;
    return capacity + capacity / 2 < needed ? needed : capacity + capacity / 2;
}

static void move_to(struct Vec *v, size_t capacity, size_t size)
{
    size_t bytes = (size_t)((char *)v->last - (char *)v->first);
    char *block = capacity ? (char *)mem_alloc(capacity * size) : NULL;

    if (bytes)
        memcpy(block, v->first, bytes);
    if (v->first)
        mem_free(v->first);
    v->first = block;
    v->last = block + bytes;
    v->end = block + capacity * size;
}

void vec_reserve(struct Vec *v, size_t count, size_t size)
{
    if (count > vec_capacity(v, size)) {
        if (count > (size_t)-1 / size)
            fatal_error("vector too long");
        move_to(v, count, size);
    }
}

static int inside(const struct Vec *v, const void *p)
{
    return (const char *)p >= (const char *)v->first && (const char *)p < (const char *)v->end;
}

void *vec_insert_range(struct Vec *v, void *where, const void *first, const void *last, size_t size)
{
    size_t bytes = (size_t)((const char *)last - (const char *)first);
    size_t at = v->first ? (size_t)((char *)where - (char *)v->first) : 0;
    size_t used = vec_count(v, size);
    char *slot;

    if (!bytes)
        return where;
    if (inside(v, first)) {
        void *copy = mem_alloc(bytes);

        memcpy(copy, first, bytes);
        slot = (char *)vec_insert_range(v, where, copy, (char *)copy + bytes, size);
        mem_free(copy);
        return slot;
    }
    if (bytes > (size_t)((char *)v->end - (char *)v->last))
        move_to(v, grown_capacity(v, used + bytes / size, size), size);
    slot = (char *)v->first + at;
    memmove(slot + bytes, slot, (size_t)((char *)v->last - slot));
    memcpy(slot, first, bytes);
    v->last = (char *)v->last + bytes;
    return slot;
}

void *vec_insert(struct Vec *v, void *where, const void *elem, size_t size)
{
    return vec_insert_range(v, where, elem, (const char *)elem + size, size);
}

void *vec_push(struct Vec *v, const void *elem, size_t size)
{
    return vec_insert_range(v, v->last, elem, (const char *)elem + size, size);
}

void vec_resize(struct Vec *v, size_t count, size_t size)
{
    size_t used = vec_count(v, size);

    if (count > used) {
        if (count > vec_capacity(v, size))
            move_to(v, grown_capacity(v, count, size), size);
        memset(v->last, 0, (count - used) * size);
    }
    v->last = (char *)v->first + count * size;
}

void *vec_add(struct Vec *v, size_t count, size_t size)
{
    size_t used = vec_count(v, size);

    vec_resize(v, used + count, size);
    return (char *)v->first + used * size;
}

void vec_erase(struct Vec *v, void *first, void *last)
{
    if (first == last)
        return;
    memmove(first, last, (size_t)((char *)v->last - (char *)last));
    v->last = (char *)v->last - ((char *)last - (char *)first);
}

void vec_assign(struct Vec *v, const void *first, const void *last, size_t size)
{
    size_t bytes = (size_t)((const char *)last - (const char *)first);

    if (bytes > (size_t)((char *)v->end - (char *)v->first)) {
        size_t capacity = grown_capacity(v, bytes / size, size);

        vec_free(v);
        v->first = v->last = mem_alloc(capacity * size);
        v->end = (char *)v->first + capacity * size;
    }
    if (bytes)
        memmove(v->first, first, bytes);
    v->last = (char *)v->first + bytes;
}

void vec_copy(struct Vec *dst, const struct Vec *src, size_t size)
{
    vec_init(dst);
    vec_assign(dst, src->first, src->last, size);
}

void vec_free(struct Vec *v)
{
    if (v->first)
        mem_free(v->first);
    vec_init(v);
}
