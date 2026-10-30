#ifndef STD_STRING_VEC_H
#define STD_STRING_VEC_H

#include "std/vec.h"
#include "std/string.h"

void string_vec_clear(struct Vec *v);
void string_vec_free(struct Vec *v);
struct String *string_vec_push_move(struct Vec *v, struct String *s);
struct String *string_vec_push_copy(struct Vec *v, const struct String *s);
struct String *string_vec_push_text(struct Vec *v, const char *text, size_t len);
struct String *string_vec_insert_copy(struct Vec *v, struct String *where, const struct String *s);
void string_vec_assign(struct Vec *v, const struct String *first, const struct String *last);
void string_vec_copy(struct Vec *dst, const struct Vec *src);
void string_vec_resize(struct Vec *v, size_t count);
void string_vec_erase(struct Vec *v, struct String *first, struct String *last);

#endif
