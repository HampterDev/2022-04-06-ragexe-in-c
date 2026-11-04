#ifndef CORE_TEXT_FORMAT_H
#define CORE_TEXT_FORMAT_H

#include "core/owned_text.h"
#include "platform/win_types.h"

int text_is_colour_code(const char *text, const char *end);
const char *text_find_colour_code(const char *text, const char *end);
const char *text_read_colour_code(const char *text, const char *end, COLORREF *colour);

size_t text_strip_colour_codes(char *text, size_t length);

int text_copy_without_colours(struct OwnedText *out, struct TextView text);

#endif
