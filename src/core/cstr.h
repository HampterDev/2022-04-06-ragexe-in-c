#ifndef CORE_CSTR_H
#define CORE_CSTR_H

#include <stddef.h>

void cstr_copy(char *dst, size_t size, const char *src);
void cstr_copy_n(char *dst, size_t size, const char *src, size_t count);
int cstr_compare_nocase(const char *a, const char *b);
void cstr_lower(char *text);

#endif
