#ifndef SESSION_PARTY_TYPES_H
#define SESSION_PARTY_TYPES_H

#include "base.h"

struct PartyMember {
    DWORD found;
    DWORD aid;
    DWORD char_id;
    struct String name;
    struct String map_name;
    DWORD role;
    DWORD state;
    DWORD color;
    WORD job, level;
    DWORD dead;
};
OFFSET_ASSERT(PartyMember, found, 0x00);
OFFSET_ASSERT(PartyMember, aid, 0x04);
OFFSET_ASSERT(PartyMember, char_id, 0x08);
OFFSET_ASSERT(PartyMember, name, 0x0c);

#endif
