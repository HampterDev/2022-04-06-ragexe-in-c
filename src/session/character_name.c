#include "session/character_name.h"
#include "session/state.h"
#include <string.h>

const char *session_character_name(const struct Session *session)
{
    return session ? session->charName : "";
}

void session_set_character_name(struct Session *session, struct TextView name)
{
    size_t length = name.size;
    const char *end;
    if (!session) return;
    if (length >= sizeof session->charName) length = sizeof session->charName - 1;
    end = length ? memchr(name.data, 0, length) : NULL;
    if (end) length = (size_t)(end - name.data);
    if (length) memmove(session->charName, name.data, length);
    memset(session->charName + length, 0, sizeof session->charName - length);
}
