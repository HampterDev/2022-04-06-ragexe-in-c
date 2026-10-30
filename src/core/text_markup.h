#ifndef CORE_TEXT_MARKUP_H
#define CORE_TEXT_MARKUP_H

#include <stddef.h>

size_t text_unwrap_tag(char *text, size_t length, const char *open, const char *close);

size_t text_strip_info(char *text, size_t length);

int text_has_markup_tags(const char *text, size_t length);

size_t text_unescape(char *text, size_t length);

#endif
