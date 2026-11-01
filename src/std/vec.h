#ifndef STD_VEC_H
#define STD_VEC_H

#include <stddef.h>
#include "std/container_types.h"

static __inline void vec_init(struct Vec *v)
{
    v->first = v->last = v->end = NULL;
}

static __inline size_t vec_count(const struct Vec *v, size_t size)
{
    return (size_t)((const char *)v->last - (const char *)v->first) / size;
}

static __inline size_t vec_capacity(const struct Vec *v, size_t size)
{
    return (size_t)((const char *)v->end - (const char *)v->first) / size;
}

static __inline void vec_clear(struct Vec *v)
{
    v->last = v->first;
}

void vec_reserve(struct Vec *v, size_t count, size_t size);
void *vec_push(struct Vec *v, const void *elem, size_t size);
void *vec_insert(struct Vec *v, void *where, const void *elem, size_t size);
void *vec_insert_range(struct Vec *v, void *where, const void *first, const void *last, size_t size);
void *vec_add(struct Vec *v, size_t count, size_t size);
void vec_erase(struct Vec *v, void *first, void *last);
void vec_resize(struct Vec *v, size_t count, size_t size);
void vec_assign(struct Vec *v, const void *first, const void *last, size_t size);
void vec_copy(struct Vec *dst, const struct Vec *src, size_t size);
void vec_free(struct Vec *v);

#endif
