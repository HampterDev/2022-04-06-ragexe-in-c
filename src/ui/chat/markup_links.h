#ifndef UI_CHAT_MARKUP_LINKS_H
#define UI_CHAT_MARKUP_LINKS_H

#include "ui/chat/navi_text.h"

int chat_filter_is_blocked(struct TextView text);

struct String *navi_links_expand_msg_tags(struct String *out, struct TextView text);

#endif
