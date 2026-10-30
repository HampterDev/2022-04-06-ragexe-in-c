#ifndef UI_CHAT_NAVI_TEXT_H
#define UI_CHAT_NAVI_TEXT_H

#include "std/string.h"

int navi_links_base62(struct TextView text);

struct String *navi_links_readable_link(struct String *out,
                                         struct TextView link, int bracketed);

struct String *navi_links_readable_links(struct String *out,
                                          struct TextView text);

struct String *navi_links_to_base62(struct String *out, unsigned value);
int navi_links_has_link(struct TextView text);
struct TextView navi_links_extract_link(struct TextView text);
int navi_links_strip_nr(struct String *out, struct TextView text);

#endif
