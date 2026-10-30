#ifndef GAMEMODE_PARTY_MEMBER_H
#define GAMEMODE_PARTY_MEMBER_H

#include "base.h"
#include "session/party_types.h"

struct Session;

struct PartyMember *party_member_info_construct(struct PartyMember *m);
static __inline void party_member_info_destroy(struct PartyMember *m)
{
    string_free(&m->map_name);
    string_free(&m->name);
}

#endif
