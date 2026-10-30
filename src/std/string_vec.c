#include "std/string_vec.h"

void string_vec_clear(struct Vec *v)
{
    struct String *s;

    for (s = v->first; s != v->last; s++)
        string_free(s);
    vec_clear(v);
}

void string_vec_free(struct Vec *v)
{
    string_vec_clear(v);
    vec_free(v);
}

struct String *string_vec_push_move(struct Vec *v, struct String *s)
{
    struct String *added = vec_push(v, s, sizeof *s);

    string_init(s);
    return added;
}

struct String *string_vec_insert_copy(struct Vec *v, struct String *where, const struct String *s)
{
    struct String copy;

    string_copy(&copy, s);
    return vec_insert(v, where, &copy, sizeof copy);
}

struct String *string_vec_push_copy(struct Vec *v, const struct String *s)
{
    return string_vec_insert_copy(v, v->last, s);
}

struct String *string_vec_push_text(struct Vec *v, const char *text, size_t len)
{
    struct String s;

    string_init(&s);
    string_assign(&s, text, len);
    return vec_push(v, &s, sizeof s);
}

void string_vec_assign(struct Vec *v, const struct String *first, const struct String *last)
{
    struct Vec copy;
    struct String *s;

    vec_init(&copy);
    s = vec_add(&copy, (size_t)(last - first), sizeof *s);
    for (; first != last; first++, s++)
        string_copy(s, first);
    string_vec_free(v);
    *v = copy;
}

void string_vec_copy(struct Vec *dst, const struct Vec *src)
{
    vec_init(dst);
    string_vec_assign(dst, src->first, src->last);
}

void string_vec_resize(struct Vec *v, size_t count)
{
    struct String *s;

    for (s = (struct String *)v->first + count; s < (struct String *)v->last; s++)
        string_free(s);
    vec_resize(v, count, sizeof(struct String));
}

void string_vec_erase(struct Vec *v, struct String *first, struct String *last)
{
    struct String *s;

    for (s = first; s != last; s++)
        string_free(s);
    vec_erase(v, first, last);
}
