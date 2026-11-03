#ifndef CORE_TEXT_UTIL_H
#define CORE_TEXT_UTIL_H

#include <windows.h>
#include "std/std.h"

struct String *text_utf8_to_codepage(struct String *out, const struct String *utf8, UINT code_page);

#include "core/money_text.h"
DWORD hex_after_prefix_value(const char *text);

void tokens_split(struct Vec *tokens, const struct String *line);

void text_to_lower(struct String *s);

/* Drops the 4-byte extension (.gat) that servers append to a map name. A name shorter
   than 4 bytes cannot carry one and is kept as it is. */
void text_drop_map_extension(struct String *map_name);

char *str_find_last(char *text, char c);

#endif
