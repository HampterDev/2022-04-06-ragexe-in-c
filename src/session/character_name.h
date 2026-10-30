#ifndef SESSION_CHARACTER_NAME_H
#define SESSION_CHARACTER_NAME_H

#include "core/text_view.h"
struct Session;
const char *session_character_name(const struct Session *session);
void session_set_character_name(struct Session *session, struct TextView name);

#endif
