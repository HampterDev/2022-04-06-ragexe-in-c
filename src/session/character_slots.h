#ifndef SESSION_CHARACTER_SLOTS_H
#define SESSION_CHARACTER_SLOTS_H

#include "base.h"

enum { SESSION_BLOCKED_CHARACTERS = 68 };
struct SessionBlockedChar {
    int gid;
    char name[20];
};
SIZE_ASSERT(SessionBlockedChar, 24);

struct Session;
int session_set_blocked_characters(struct Session *session, const void *packet, size_t size);

#endif
