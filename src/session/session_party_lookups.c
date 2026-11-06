#include "session/state.h"
#include "game/party_member.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_party.h"

/* The session's party member lookups, by account ID and by name. Apart from
   session/session_party.c, whose friend greeting and party changes reach the actors and
   their effects, so that the predicates asking for a member do not. */

static struct PartyMember *default_member(struct PartyMember *out)
{
    out->found = 0;
    string_init(&out->name);
    string_init(&out->map_name);
    out->dead = 0;
    return out;
}

struct PartyMember *session_get_party_member(struct Session *self,
                                                               struct PartyMember *out, DWORD aid)
{
    struct ListNode *node;

    for (node = self->partyMembers.head->next; node != self->partyMembers.head; node = node->next)
        if (LIST_VALUE(node, struct PartyMember)->aid == aid)
            return party_member_info_copy(out, LIST_VALUE(node, struct PartyMember));
    return default_member(out);
}

struct PartyMember *session_get_party_member_by_name(struct Session *self,
                                                                struct PartyMember *out, const char *name)
{
    struct ListNode *node;

    for (node = self->partyMembers.head->next; node != self->partyMembers.head; node = node->next) {
        const struct String *have = &LIST_VALUE(node, struct PartyMember)->name;
        if (string_equal_cstr(have, name))
            return party_member_info_copy(out, LIST_VALUE(node, struct PartyMember));
    }
    return default_member(out);
}
