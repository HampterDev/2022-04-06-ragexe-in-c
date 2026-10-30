#include "session/snapshot_party.h"
#include "session/state.h"
#include "session/party_types.h"
#include "std/std.h"
#include <string.h>

enum { PARTY_BEGIN = 0xfa0, PARTY_END = 0xfa1, PARTY_END_OLD = 0x1195,
       MEMBER_BEGIN = 0xfb, MEMBER_END = 0xfc, MEMBER_NAME = 0x100, MEMBER_MAP = 0x101 };
struct MemberField { uint16_t tag; size_t offset; };
#define MEMBER(tag, name) {tag, offsetof(struct PartyMember, name)}
static const struct MemberField member_fields[] = {
    MEMBER(0xfd, found), MEMBER(0xfe, aid), MEMBER(0xff, char_id),
    MEMBER(0x102, role), MEMBER(0x103, state), MEMBER(0x104, color)
};
#undef MEMBER

static const struct MemberField *member_field(uint16_t tag)
{
    size_t i;
    for (i = 0; i < sizeof member_fields / sizeof member_fields[0]; ++i)
        if (member_fields[i].tag == tag) return &member_fields[i];
    return NULL;
}
static void write_text(struct SnapshotWriter *writer, uint16_t tag, const struct String *text)
{
    snapshot_write(writer, tag, string_cstr(text), text->size);
}
static void load_text(struct String *text, struct SnapshotSpan value)
{
    const unsigned char *zero = memchr(value.data, 0, value.size);
    size_t size = zero ? (size_t)(zero - value.data) : value.size;
    string_assign(text, (const char *)value.data, size);
}
static void write_list(struct SnapshotWriter *writer, uint16_t tag, const struct List *list)
{
    const struct ListNode *node;
    size_t start = snapshot_begin_record(writer, tag), i;
    if (list->head) for (node = list->head->next; node != list->head; node = node->next) {
        const struct PartyMember *member = LIST_VALUE(node, struct PartyMember);
        snapshot_write(writer, MEMBER_BEGIN, NULL, 0);
        for (i = 0; i < sizeof member_fields / sizeof member_fields[0]; ++i)
            snapshot_write(writer, member_fields[i].tag, (const unsigned char *)member + member_fields[i].offset, sizeof(DWORD));
        write_text(writer, MEMBER_NAME, &member->name);
        write_text(writer, MEMBER_MAP, &member->map_name);
        snapshot_write(writer, MEMBER_END, NULL, 0);
    }
    snapshot_end_record(writer, start);
}

int session_party_save(const struct Session *session, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    if (!session) return 0;
    snapshot_write(&writer, PARTY_BEGIN, NULL, 0);
    snapshot_write(&writer, 0xfaa, &session->partyInviteRefused, sizeof session->partyInviteRefused);
    snapshot_write(&writer, 0xfab, &session->map_rule_flags[4], sizeof session->map_rule_flags[4]);
    snapshot_write(&writer, 0xfac, &session->partyLeader, sizeof session->partyLeader);
    write_text(&writer, 0xfad, &session->newPartyName);
    write_text(&writer, 0xfae, &session->partyName);
    write_list(&writer, 0xfaf, &session->partyMembers);
    write_list(&writer, 0xfb0, &session->friendList);
    snapshot_write(&writer, PARTY_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

static int validate_member(struct SnapshotSpan data)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!snapshot_open(&reader, data.data, data.size, MEMBER_BEGIN)) return -1;
    while (snapshot_next(&reader, &record) == 1) {
        if (record.tag == MEMBER_END) return record.value.size ? -1 : (int)reader.offset;
        if (record.tag == MEMBER_BEGIN || (member_field(record.tag) && record.value.size != sizeof(DWORD)))
            return -1;
    }
    return -1;
}
static int validate_list(struct SnapshotSpan data, size_t *count)
{
    while (data.size) {
        int used = validate_member(data);
        if (used < 0) return 0;
        ++*count;
        data.data += used; data.size -= used;
    }
    return 1;
}
static int validate_party(const void *data, size_t size, size_t counts[2])
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!snapshot_open(&reader, data, size, PARTY_BEGIN)) return -1;
    while (snapshot_next(&reader, &record) == 1) {
        if (record.tag == PARTY_END || record.tag == PARTY_END_OLD)
            return record.value.size ? -1 : (int)reader.offset;
        switch (record.tag) {
        case PARTY_BEGIN: return -1;
        case 0xfaa: if (record.value.size != 1) return -1; break;
        case 0xfab: case 0xfac: if (record.value.size != 4) return -1; break;
        case 0xfaf: case 0xfb0:
            if (!validate_list(record.value, &counts[record.tag - 0xfaf])) return -1;
            break;
        }
    }
    return -1;
}

static void load_list(struct List *list, struct SnapshotSpan data)
{
    while (data.size) {
        struct SnapshotReader reader;
        struct SnapshotRecord record;
        struct PartyMember member;
        memset(&member, 0, sizeof member);
        member.found = 1;
        string_init(&member.name); string_init(&member.map_name);
        snapshot_open(&reader, data.data, data.size, MEMBER_BEGIN);
        while (snapshot_next(&reader, &record) == 1) {
            const struct MemberField *field = member_field(record.tag);
            if (record.tag == MEMBER_END) break;
            if (field) memcpy((unsigned char *)&member + field->offset, record.value.data, sizeof(DWORD));
            else if (record.tag == MEMBER_NAME) load_text(&member.name, record.value);
            else if (record.tag == MEMBER_MAP) load_text(&member.map_name, record.value);
        }
        list_push_back(list, &member, sizeof member);
        data.data += reader.offset; data.size -= reader.offset;
    }
}
int session_party_load(struct Session *session, const void *data, size_t size)
{
    size_t counts[2] = {0}, i;
    struct List *lists[2];
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int used = validate_party(data, size, counts);
    if (!session || used < 0) return -1;
    lists[0] = &session->partyMembers; lists[1] = &session->friendList;
    for (i = 0; i < 2; ++i)
        if (counts[i] && !lists[i]->head)
            return -1;
    snapshot_open(&reader, data, (size_t)used, PARTY_BEGIN);
    while (snapshot_next(&reader, &record) == 1) {
        switch (record.tag) {
        case 0xfaa: memcpy(&session->partyInviteRefused, record.value.data, sizeof session->partyInviteRefused); break;
        case 0xfab: memcpy(&session->map_rule_flags[4], record.value.data, sizeof session->map_rule_flags[4]); break;
        case 0xfac: memcpy(&session->partyLeader, record.value.data, sizeof session->partyLeader); break;
        case 0xfad: load_text(&session->newPartyName, record.value); break;
        case 0xfae: load_text(&session->partyName, record.value); break;
        case 0xfaf: case 0xfb0: load_list(lists[record.tag - 0xfaf], record.value); break;
        }
    }
    return used;
}
