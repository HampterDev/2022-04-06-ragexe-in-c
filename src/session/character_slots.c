#include "session/character_slots.h"
#include "session/state.h"
#include <string.h>

int session_set_blocked_characters(struct Session *session, const void *packet, size_t size)
{
    const unsigned char *bytes = packet;
    unsigned short declared;
    size_t count;

    if (!bytes || size < 4)
        return 0;
    memcpy(&declared, bytes + 2, sizeof declared);
    if (declared != size)
        return 0;
    count = (size - 4) / sizeof *session->blockedChars;
    if (count > SESSION_BLOCKED_CHARACTERS)
        return 0;
    memcpy(session->blockedChars, bytes + 4, count * sizeof *session->blockedChars);
    session->charListCounts[5] = (int)count;
    return 1;
}
