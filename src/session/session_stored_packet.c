#include "session/csession.h"
#include "session/session_state.h"
#include "game/skill_list.h"

#include <string.h>

#pragma pack(push, 1)
struct PACKET_ZC_MSG_STATE_CHANGE3 {
    WORD packet_type;
    WORD index;
    DWORD aid;
    BYTE state;
    DWORD total;
    DWORD left;
    DWORD val1;
    DWORD val2;
    DWORD val3;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MSG_STATE_CHANGE3, 0x1d);

static struct Vec *skill_list_packet(struct Vec *out, short op, const struct List *list)
{
    const struct ListNode *node;
    const char *name;
    size_t size, len;
    struct PACKET_ZC_SKILLINFO_LIST *packet;
    struct SkillListEntryV1 *entry;

    size = sizeof *packet + sizeof *entry * list->size;
    vec_init(out);
    packet = vec_add(out, size, 1);
    packet->id = op;
    packet->length = (unsigned short)size;
    entry = packet->skills;
    for (node = list->head->next; node != list->head; node = node->next, entry++) {
        const struct SkillInfo *skill = LIST_VALUE(node, struct SkillInfo);

        entry->id = (unsigned short)skill->id;
        entry->type = skill->type;
        entry->level = (unsigned short)skill->level;
        entry->sp = (unsigned short)skill->sp_cost;
        entry->upgradable = skill->upgradable != 0;
        entry->range = (unsigned short)skill->range;
        name = skill->id_name;
        len = strlen(name) > 0x18 ? 0x18 : strlen(name);
        memcpy(entry->name, name, len);
        entry->name[23] = 0;
    }
    return out;
}

struct Vec *session_get_stored_packet(struct Session *self, struct Vec *out,
                                                     short packetId)
{
    switch (packetId) {
    case 0x10f:
        return skill_list_packet(out, 0x10f, &self->skills.skills);
    case 0x235:
        return skill_list_packet(out, 0x235, &self->homunSkills);
    case 0x29d:
        return skill_list_packet(out, 0x29d, &self->merSkills);
    }
    out->first = NULL;
    out->last = NULL;
    out->end = NULL;
    return out;
}

struct Vec *session_get_stored_packet2(struct Session *self, struct Vec *out)
{
    struct PACKET_ZC_MSG_STATE_CHANGE3 *p;

    vec_init(out);
    p = vec_add(out, 0x1d, 1);
    p->packet_type = 0x983;
    p->index = 0x2a1;
    p->total = 0x270f;
    p->aid = self->ownAid;
    p->state = 1;
    p->left = 0x270f;
    p->val1 = self->cartType;
    return out;
}
