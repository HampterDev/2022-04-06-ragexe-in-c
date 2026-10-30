#ifndef CORE_TEXT_FORMAT_H
#define CORE_TEXT_FORMAT_H

#include "core/owned_text.h"

int text_is_colour_code(const char *text, const char *end);
const char *text_find_colour_code(const char *text, const char *end, unsigned code_page);
const char *text_read_colour_code(const char *text, const char *end, unsigned long *colour);

size_t text_find_charset_tag(const char *text, size_t length, int enabled);

size_t text_strip_colour_codes(char *text, size_t length, unsigned code_page);

int text_copy_without_colours(struct OwnedText *out, struct TextView text,
                              unsigned code_page, int charset_tags);

size_t text_prefix_length(const char *text, size_t length, size_t max_bytes,
                          unsigned code_page);

#endif
